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
#include <sbt_common/log.h>
#include <sbt_common/mbuf.h>
#include <sbt_common/mqueue.h>
#include <daemon/worker.h>
#include <daemon/control.h>
#include <daemon/manager.h>
#include <daemon/message.h>


LOG_MODULE_IMPORT(daemon_manager_module_level);
#define module_level daemon_manager_module_level


void daemon_manager_control_recv (struct v49_control *ctrl)
{
	ENTER("ctrl %p", ctrl);
#if 0
	struct message *user = mbuf_user(mbuf);
	char           *cmd = mbuf_beg_ptr(mbuf);

	// request for list of workers from the user - reuse mbuf
	if ( !strcmp(cmd, "list") )
	{
		mbuf_cur_set_beg(mbuf);
		mbuf_end_set_beg(mbuf);
		mbuf_printf(mbuf, 4096, "Process list:\n");

		struct worker *worker;
		growlist_reset(&worker_list);
		while ( (worker = growlist_next(&worker_list) ) )
			mbuf_printf(mbuf, 4096, "  %s (%p):\n", worker_name(worker), worker);
		mbuf_printf(mbuf, 4096, "(ends)");

		LOG_DEBUG("Returning process list to %s.%d\n", user->control->name, user->socket);
		mbuf_dump(mbuf);

		// reflect the mbuf
		control_enqueue(user->control, mbuf);
		RETURN_ERRNO(0);
	}

	// search workers next: if we get a worker name match, just connect the socket to
	// resume interactive mode
	struct worker *worker;
	growlist_reset(&worker_list);
	while ( (worker = growlist_next(&worker_list)) )
		if ( !strcmp(mbuf_beg_ptr(mbuf), worker_name(worker)) )
		{
			control_connect(user->control, user->socket, worker);
			LOG_INFO("Reconnect %s to %s.%d\n", worker_name(worker), user->control->name, user->socket);
			RETURN_ERRNO(0);
		}



	LOG_WARN("Message from %s.%d unmatched, dropped\n", user->control->name, user->socket);
	mbuf_dump(mbuf);
	mbuf_deref(mbuf);
#endif
	RETURN_ERRNO(0);
}

/* Ends    : src/daemon/worker/manager/control.c */
