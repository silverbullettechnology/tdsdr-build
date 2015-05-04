/** Debug control: never sources message, dumps received messages to the log
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
#include <daemon/control.h>


LOG_MODULE_STATIC("control_debug", LOG_LEVEL_DEBUG);


static int control_debug_check (struct control *ctrl)
{
	ENTER("ctrl %p", ctrl);

	struct mbuf *mbuf;

	while ( (mbuf = mqueue_dequeue(ctrl->send)) )
	{
		mbuf_dump(mbuf);
		mbuf_deref(mbuf);
	}

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Operation pointers. */
struct control_ops control_debug_ops =
{
	check_fn: control_debug_check,
};
CONTROL_CLASS(debug, control_debug_ops);


/* Ends    : src/control/debug.c */
