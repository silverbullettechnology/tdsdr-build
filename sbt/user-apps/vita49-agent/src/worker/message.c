/** Worker message routing
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
#include <lib/timer.h>
#include <lib/growlist.h>

#include <common/vita49/types.h>
#include <common/vita49/common.h>
#include <common/vita49/command.h>
#include <common/vita49/context.h>
#include <common/vita49/control.h>

#include <worker/command.h>
#include <worker/context.h>
//#include <worker/control.h>


LOG_MODULE_STATIC("worker_message", LOG_LEVEL_WARN);


struct mqueue message_queue = MQUEUE_INIT(0);


void worker_southbound (struct mbuf *mbuf)
{
	struct v49_common  v49;
	int                ret;

	ENTER("mbuf %p", mbuf);

	LOG_DEBUG("Received southbound from manager:\n");
	mbuf_dump(mbuf);
	mbuf_cur_set_beg(mbuf);

	// parse header first
	v49_reset(&v49);
	ret = v49_common_parse(&v49, mbuf);
	if ( V49_RET_ERROR(ret) )
	{
		LOG_ERROR("Dropped southbound: %s\n", v49_return_desc(ret));
		mbuf_deref(mbuf);
		RETURN_ERRNO(0);
	}

	// early-out: local control messages go to server
	if ( ret == V49_OK_CONTROL )
	{
#if 0
		struct v49_control *ctrl;
		if ( !(ctrl = v49_control_parse(mbuf)) )
		{
			LOG_ERROR("Dropped: malformed control\n");
			mbuf_deref(mbuf);
			RETURN_ERRNO(0);
		}
		worker_control_recv(ctrl);
#endif

		LOG_DEBUG("Ignored a control message\n");
		mbuf_deref(mbuf);
		RETURN_ERRNO(0);
	}

	// parse body of the message, drop if malformed
	switch ( v49.type )
	{
		case V49_TYPE_COMMAND: ret = v49_command_parse(&v49, mbuf); break;
		case V49_TYPE_CONTEXT: ret = v49_context_parse(&v49, mbuf); break;
		default:               ret = V49_ERR_HEADER_TYPE;           break;
	}
	mbuf_deref(mbuf);

	if ( V49_RET_ERROR(ret) )
	{
		LOG_ERROR("Dropped %s southbound: %s\n", v49_type(v49.type), v49_return_desc(ret));
		RETURN_ERRNO(0);
	}

	// parse body of the message, drop if malformed
	switch ( v49.type )
	{
		case V49_TYPE_COMMAND:
			worker_command_recv(&v49);
			RETURN_ERRNO(0);

		case V49_TYPE_CONTEXT:
			worker_context_recv(&v49);
			RETURN_ERRNO(0);

		default:
			RETURN_ERRNO(0);
	}
}

