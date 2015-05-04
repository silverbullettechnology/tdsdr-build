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
#include <lib/log.h>
#include <lib/mbuf.h>
#include <lib/mqueue.h>

#include <common/vita49/types.h>
#include <common/vita49/common.h>
#include <common/vita49/command.h>

#include <worker/command.h>


LOG_MODULE_STATIC("worker_command", LOG_LEVEL_DEBUG);


/** Receive a command packet from a client
 *
 *  \param  mbuf  Message buffer containing VITA-49 context packet
 *
 *  \return 0 on success, <0 on fatal error
 */
int worker_command_recv (struct mbuf *mbuf)
{
	ENTER("mbuf %p", mbuf);

	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);
}

