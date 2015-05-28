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
#ifndef INCLUDE_WORKER_MESSAGE_H
#define INCLUDE_WORKER_MESSAGE_H
#include <lib/mbuf.h>
#include <lib/mqueue.h>



extern struct mqueue message_queue;


static inline void worker_northbound (struct mbuf *mbuf)
{
	mqueue_enqueue(&message_queue, mbuf);
}


void worker_southbound (struct mbuf *mbuf);


#endif // INCLUDE_WORKER_MESSAGE_H
