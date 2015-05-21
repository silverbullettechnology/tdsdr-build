/** Agent message structure & prototypes
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
#ifndef INCLUDE_DAEMON_MESSAGE_H
#define INCLUDE_DAEMON_MESSAGE_H

#include <daemon/control.h>
#include <daemon/worker.h>

/* Currently sized for a maximum-length SRIO type 11 (message) packet with sd_user header
 * size reserved */
#define DAEMON_MBUF_SIZE  5120
#define DAEMON_MBUF_HEAD   256


struct message
{
	struct control *control;
	int             socket;
	struct worker  *worker;
};


typedef void (* daemon_recv_fn) (struct mbuf *mbuf);
struct daemon_recv_map
{
	char           *channel;
	daemon_recv_fn  recv_fn;
};


/** Dispatch a southbound message: if it's for the daemon layer, consume it, otherwise
 *  route it to the appropriate worker
 *
 *  \param mbuf Message to be processed
 */
void daemon_southbound (struct mbuf *mbuf);


/** Dispatch a northbound message: if it's for the daemon layer, consume it, otherwise
 *  route it to the appropriate control
 *
 *  \param mbuf Message to be processed
 */
void daemon_northbound (struct mbuf *mbuf);


#endif /* INCLUDE_DAEMON_MESSAGE_H */
/* Ends */
