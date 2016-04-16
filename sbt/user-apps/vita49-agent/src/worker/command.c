/** Worker command message handling
 *
 *  \author    Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
 *  \version   v0.0
 *  \copyright Copyright 2014,2015 Silver Bullet Technology
 *
 *             Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *             use this file except in compliance with the License.  You may obtain a copy
 *             of the License at: 
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *             Unless required by applicable law or agreed to in writing, software
 *             distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *             WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 *             License for the specific language governing permissions and limitations
 *             under the License.
 *
 *  vim:ts=4:noexpandtab
 */
#include <sbt_common/log.h>
#include <sbt_common/mbuf.h>
#include <sbt_common/mqueue.h>

#include <common/default.h>
#include <v49_message/types.h>
#include <v49_message/common.h>
#include <v49_message/command.h>

#include <worker/worker.h>
#include <worker/message.h>
#include <worker/command.h>


LOG_MODULE_STATIC("worker_command", LOG_LEVEL_DEBUG);


/** Receive a command packet from a client
 *
 *  \param  v49  Parsed command structure
 */
void worker_command_recv (struct v49_common *req_v49)
{
	struct v49_common     rsp_v49;
	struct mbuf          *mbuf;
	int                   ret;

	ENTER("req_v49 %p", req_v49);

	v49_dump(LOG_LEVEL_DEBUG, req_v49);

	if ( !(mbuf = mbuf_alloc(DEFAULT_MBUF_SIZE, 0)) )
	{
		LOG_ERROR("mbuf_alloc() failed: %s\n", strerror(errno));
		RETURN;
	}

	v49_reset(&rsp_v49);
	rsp_v49.type = V49_TYPE_COMMAND;
	rsp_v49.command.request = req_v49->command.request;

	if ( req_v49->command.indicator & (1 << V49_CMD_IND_BIT_CID) )
	{
		LOG_DEBUG("Proxy CID from req to rsp: %s\n", uuid_to_str(&req_v49->command.cid));
		rsp_v49.command.indicator |= 1 << V49_CMD_IND_BIT_CID;
		memcpy(&rsp_v49.command.cid, &req_v49->command.cid, sizeof(uuid_t));
	}

	rsp_v49.command.role   = V49_CMD_ROLE_RESULT;
	rsp_v49.command.result = V49_CMD_RES_UNSPEC;
	switch ( req_v49->command.request )
	{
		case V49_CMD_REQ_OPEN:
			LOG_DEBUG("%s: got an OPEN request:\n", __func__);
			worker_command_open(req_v49, &rsp_v49);
			break;

		case V49_CMD_REQ_START:
			LOG_DEBUG("%s: got a START request:\n", __func__);
			worker_command_start(req_v49, &rsp_v49);
			break;

		case V49_CMD_REQ_STOP:
			LOG_DEBUG("%s: got a STOP request:\n", __func__);
			worker_command_stop(req_v49, &rsp_v49);
			break;

		case V49_CMD_REQ_CLOSE:
			LOG_DEBUG("%s: got a CLOSE request:\n", __func__);
			worker_command_close(req_v49, &rsp_v49);
			break;

		case V49_CMD_REQ_RELEASE:
			LOG_DEBUG("Release request for SID %u\n", req_v49->sid);
			rsp_v49.command.result = V49_CMD_RES_SUCCESS;
			worker_shutdown();
			break;

		default:
			LOG_ERROR("Command not (yet) handled: %s\n",
			          v49_command_request(req_v49->command.request));
			break;
	}

	LOG_DEBUG("%s: dump reply:\n", __func__);
	v49_dump(LOG_LEVEL_DEBUG, &rsp_v49);

	LOG_DEBUG("%s: format reply\n", __func__);
	if ( (ret = v49_format(&rsp_v49, mbuf, NULL)) != V49_OK_COMPLETE )
	{
		LOG_ERROR("v49_format: %s\n", v49_return_desc(ret));
		mbuf_deref(mbuf);
		RETURN_ERRNO(0);
	}

	LOG_DEBUG("%s: enqueue reply\n", __func__);
	worker_northbound(mbuf);
	RETURN_ERRNO(0);
}

