/** Daemon message routing
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
#include <sbt_common/timer.h>
#include <sbt_common/growlist.h>

#include <vita49/types.h>
#include <vita49/common.h>
#include <vita49/command.h>
#include <vita49/context.h>
#include <vita49/control.h>

#include <daemon/worker.h>
#include <daemon/control.h>
#include <daemon/daemon.h>
#include <daemon/config.h>
#include <daemon/message.h>
#include <daemon/manager.h>


LOG_MODULE_STATIC("daemon_recv", LOG_LEVEL_WARN);


// TODO: move to manager control handling, add to common control union
//static void daemon_log_shim (struct mbuf *mbuf)
//{
//	ENTER("mbuf %p", mbuf);
//
//	log_config("log", mesg->strings[0], mesg->strings[1], "command", 0, NULL);
//	RETURN_ERRNO(0);
//}


/** Dispatch a southbound message: if it's for the daemon layer, consume it, otherwise
 *  route it to the appropriate worker
 *
 *  \param mbuf Message to be processed
 *
 *  \return 0 on success, <0 on error.
 */
void daemon_southbound (struct mbuf *mbuf)
{
	struct v49_common   v49;
	struct v49_control *ctrl;
	struct message     *user = mbuf_user(mbuf);
	int                 ret;

	ENTER("mbuf %p", mbuf);

	LOG_DEBUG("Received southbound from %s.%d:\n", 
	          user->control ? user->control->name : "???", user->socket);
	mbuf_dump(mbuf);
	mbuf_cur_set_beg(mbuf);

	// parse header first
	v49_reset(&v49);
	ret = v49_common_parse(&v49, mbuf);
	if ( V49_RET_ERROR(ret) )
	{
		LOG_ERROR("Dropped southbound: %s\n", v49_return_desc(ret));
		RETURN_ERRNO(0);
	}

	// early-out: local control messages go to server
	if ( ret == V49_OK_CONTROL )
	{
		if ( (ctrl = v49_control_parse(mbuf)) )
			daemon_manager_control_recv(ctrl);
		else
			LOG_ERROR("Dropped: malformed control\n");

		RETURN_ERRNO(0);
	}

	// parse body of the message, drop if malformed
	switch ( v49.type )
	{
		case V49_TYPE_COMMAND: ret = v49_command_parse(&v49, mbuf); break;
		case V49_TYPE_CONTEXT: ret = v49_context_parse(&v49, mbuf); break;
		default:               ret = V49_ERR_HEADER_TYPE;           break;
	}
	if ( V49_RET_ERROR(ret) )
	{
		LOG_ERROR("Dropped %s southbound: %s\n", v49_type(v49.type), v49_return_desc(ret));
		RETURN_ERRNO(0);
	}

	// search workers next by SID, for context and command
	struct worker *worker = NULL;
	if ( v49.sid != V49_CMD_RSVD_SID )
	{
		growlist_reset(&worker_list);
		while ( (worker = growlist_next(&worker_list)) )
			if ( v49.sid == worker->sid )
			{
				LOG_DEBUG("%s: Message matched by SID\n", worker_name(worker));
				break;
			}
	}

	// command request messages handled in daemon layer; pass the worker for the manager
	// to manipulate, or NULL if not matched
	if ( v49.type == V49_TYPE_COMMAND && v49.command.role == V49_CMD_ROLE_REQUEST )
		switch ( v49.command.request )
		{
			case V49_CMD_REQ_DISCO:   // Discovery / Advertisement
			case V49_CMD_REQ_ENUM:    // Enumeration
			case V49_CMD_REQ_ACCESS:  // Access
				daemon_manager_command_recv(&v49, mbuf_user(mbuf), worker);
				RETURN_ERRNO(0);

			case V49_CMD_REQ_RELEASE:  // Release falls through to worker deliver
				daemon_manager_command_recv(&v49, mbuf_user(mbuf), worker);
				break;

			default:
				break;
		}

	// not a manager-layer message: pass to worker
	if ( worker )
	{
		LOG_DEBUG("%s: Dispatch southbound\n", worker_name(worker));
		mbuf_cur_set_beg(mbuf);
		mbuf_dump(mbuf);
		worker_enqueue(worker, mbuf);
		RETURN_ERRNO(0);
	}

	// unmatched southbound: ignore for now
	LOG_ERROR("Droped: no match against worker SID\n");
	RETURN_ERRNO(0);
}


/** Dispatch a northbound message: if it's for the daemon layer, consume it, otherwise
 *  route it to the appropriate control
 *
 *  \param mbuf Message to be processed
 *
 *  \return 0 on success, <0 on error.
 */
void daemon_northbound (struct mbuf *mbuf)
{
	ENTER("mbuf %p", mbuf);
	struct message *user = mbuf_user(mbuf);

	LOG_DEBUG("Received northbound from %s:\n", worker_name(user->worker));
	mbuf_dump(mbuf);

	// early-out: if it has an explicit control destination trust that
	if ( user->control )
	{
		LOG_DEBUG("%s: unicast %p (socket %d)\n", control_name(user->control), mbuf,
		          user->socket);
		control_enqueue(user->control, mbuf);
		RETURN_ERRNO(0);
	}

	// broadcast northbound
	struct control *ctrl;
	int             done = 0;
	growlist_reset(&control_list);
	while ( (ctrl = growlist_next(&control_list) ) )
	{
		if ( done++ )
		{
			struct mbuf *clone = mbuf_clone(mbuf);
			LOG_DEBUG("%s: cloned bcast %p->%p\n", control_name(user->control), mbuf, clone);
			control_enqueue(ctrl, mbuf);
		}
		else
		{
			LOG_DEBUG("%s: direct bcast %p\n", control_name(user->control), mbuf);
			control_enqueue(ctrl, mbuf);
		}
	}

	RETURN_ERRNO(0);
}


/* Ends    : src/daemon/message.c */
