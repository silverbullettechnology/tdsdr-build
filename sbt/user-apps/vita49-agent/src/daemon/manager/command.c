/** Daemon manager message handler
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
#include <lib/log.h>
#include <lib/mbuf.h>
#include <lib/mqueue.h>

#include <common/control/local.h>
#include <common/vita49/types.h>
#include <common/vita49/common.h>
#include <common/vita49/command.h>

#include <daemon/resource.h>
#include <daemon/worker.h>
#include <daemon/control.h>
#include <daemon/manager.h>
#include <daemon/message.h>


LOG_MODULE_IMPORT(daemon_manager_module_level);
#define module_level daemon_manager_module_level


void daemon_manager_command_recv (struct v49_common *req_v49, struct message *req_user,
                                  struct worker *worker)
{
	struct v49_common  rsp_v49;
	struct mbuf       *mbuf;
	int                ret;

	ENTER("req_v49 %p, worker %p", req_v49, worker);

	LOG_DEBUG("%s: got message for worker %s:\n", __func__, worker_name(worker));
	v49_dump(LOG_LEVEL_DEBUG, req_v49);

	if ( !(mbuf = mbuf_alloc(CONTROL_LOCAL_BUFF_SIZE, sizeof(struct message))) )
	{
		LOG_ERROR("mbuf_alloc() failed: %s\n", strerror(errno));
		RETURN;
	}
	memcpy(mbuf_user(mbuf), req_user, sizeof(*req_user));

	v49_reset(&rsp_v49);
	rsp_v49.type = V49_TYPE_COMMAND;
	rsp_v49.command.request = req_v49->command.request;

	if ( req_v49->command.indicator & (1 << V49_CMD_IND_BIT_CID) )
	{
		LOG_DEBUG("Proxy CID from req to rsp: %s\n", uuid_to_str(&req_v49->command.cid));
		rsp_v49.command.indicator |= 1 << V49_CMD_IND_BIT_CID;
		memcpy(&rsp_v49.command.cid, &req_v49->command.cid, sizeof(uuid_t));
		LOG_DEBUG("rsp: %s\n", uuid_to_str(&rsp_v49.command.cid));
	}

	switch ( req_v49->command.request )
	{
		case V49_CMD_REQ_DISCO:
			rsp_v49.command.role     = V49_CMD_ROLE_RESULT;
			rsp_v49.command.result   = V49_CMD_RES_SUCCESS;
			rsp_v49.command.rid_list = &resource_list;
			rsp_v49.command.indicator |= (1 << V49_CMD_IND_BIT_RID_LIST);
			break;

		default:
			LOG_ERROR("Command not (yet) handled: %s\n",
			          v49_command_request(req_v49->command.request));
			mbuf_deref(mbuf);
			RETURN_ERRNO(EBADMSG);
	}

	LOG_DEBUG("%s: format reply:\n", __func__);
	v49_dump(LOG_LEVEL_DEBUG, &rsp_v49);

	if ( (ret = v49_format(&rsp_v49, mbuf, NULL)) != V49_OK_COMPLETE )
	{
		LOG_ERROR("v49_format: %s\n", v49_return_desc(ret));
		mbuf_deref(mbuf);
		RETURN_ERRNO(0);
	}

	daemon_northbound(mbuf);
	RETURN_ERRNO(0);
}

/* Ends    : src/daemon/worker/manager/command.c */
