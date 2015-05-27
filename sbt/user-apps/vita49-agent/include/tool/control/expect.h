/** Expect framework
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
#ifndef INCLUDE_TOOL_CONTROL_EXPECT_H
#define INCLUDE_TOOL_CONTROL_EXPECT_H
#include <lib/mbuf.h>
#include <lib/clocks.h>

#include <common/vita49/common.h>
#include <common/vita49/control.h>


/* Message test typedefs for the entries in expect_map.  A function should return 0 if
 * the message does not match (ie move the protocol on), >0 if it does, and <0 on fatal
 * error */
typedef int (* expect_mbuf_fn) (struct mbuf *mbuf, void *data);

struct expect_map
{
	expect_mbuf_fn  func;
	void           *data;
};


/** Enqueue an mbuf to send to the daemon
 *
 *  \param  mbuf  Message buffer to send
 *
 *  \return >= 0 on success, <0 on error
 */
int expect_send (struct mbuf *mbuf);


/** Common parsing used in various sequences' expect functions
 *
 *  \param  rsp   Pointer to v49 response buffer, to parse into
 *  \param  mbuf  Message buffer received from the daemon
 *  \param  req   Pointer to v49 request we sent (optional)
 *  \param  data  Context pointer
 *
 *  \return  >0 for a match (return value of the matching function), <0 if a function
 *           indicated a fatal error, 0 if no function indicated a match.
 */
int expect_common (struct v49_common *rsp,       struct mbuf *mbuf, 
                   const struct v49_common *req, void *data);


/** Test a received mbuf against an expect map
 *
 *  \note This is used internally by expect_loop() and exposed for unusual cases.
 *
 *  \param  mbuf  Message buffer received from the daemon
 *  \param  map   Array of expect_map entries to test the message against
 *
 *  \return  >0 for a match (return value of the matching function), <0 if a function
 *           indicated a fatal error, 0 if no function indicated a match.
 */
int expect_mbuf (struct mbuf *mbuf, struct expect_map *map);


/** Transmit any enqueued mbufs to the daemon, the run an expect loop on received messages
 *  until one matches or a fatal error or timeout occurs
 *
 *  \param  map      Array of expect_map entries to test the message against
 *  \param  timeout  Timeout in clocks
 */
int expect_loop (struct expect_map *map, clocks_t timeout);


#endif // INCLUDE_TOOL_CONTROL_EXPECT_H
