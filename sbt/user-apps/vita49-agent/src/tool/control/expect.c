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
#include <stdio.h>

#include <lib/log.h>

#include <common/control/local.h>

#include "control.h"
#include "expect.h"


LOG_MODULE_STATIC("control_expect", LOG_LEVEL_DEBUG);


static struct mqueue  send_queue = MQUEUE_INIT(0);


/** Enqueue an mbuf to send to the daemon
 *
 *  \param  mbuf  Message buffer to send
 *
 *  \return >= 0 on success, <0 on error
 */
int expect_send (struct mbuf *mbuf)
{
	return mqueue_enqueue(&send_queue, mbuf);
}


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
int expect_mbuf (struct mbuf *mbuf, struct expect_map *map)
{
	int ret;

	ENTER("mbuf %p, map %p\n", mbuf, map);
	if ( !mbuf || !map )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	while ( map->func )
	{
		mbuf_cur_set_beg(mbuf);

		errno = 0;
		if ( (ret = map->func(mbuf, map->data)) < 0 )
		{
			LOG_ERROR("Expect failed: %d: %s\n", ret, strerror(errno));
			RETURN_VALUE("%d", ret);
		}
		else if ( ret )
		{
			LOG_DEBUG("Expect matched: %d\n", ret);
			RETURN_VALUE("%d", ret);
		}
		LOG_DEBUG("Expect not matched, continue\n");

		map++;
	}

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Transmit any enqueued mbufs to the daemon, the run an expect loop on received messages
 *  until one matches or a fatal error or timeout occurs
 *
 *  \param  map      Array of expect_map entries to test the message against
 *  \param  timeout  Timeout in clocks
 */
int expect_loop (struct expect_map *map, clocks_t timeout)
{
	/* Standard set of vars to support select */
	struct timeval  tv_cur;
	fd_set          rfds;
	fd_set          wfds;
	int             sel;
	int             ret;

	ENTER("map %p, timeout %llu\n", map, (unsigned long long)timeout);

	// convert timeout to deadline
	timeout += get_clocks();
	do
	{
		LOG_DEBUG("top of loop\n");
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		FD_SET(sock_desc, &rfds);
		if ( mqueue_used(&send_queue) )
			FD_SET(sock_desc, &wfds);

		tv_cur.tv_sec  = 0;
		tv_cur.tv_usec = 333000;

		LOG_DEBUG("top of loop, select:\n");
		if ( (sel = select(sock_desc + 1, &rfds, &wfds, NULL, &tv_cur)) < 0 )
		{
			LOG_ERROR("select: %s: stop\n", strerror(errno));
			RETURN_VALUE("%d", sel);
		}
		else if ( !sel )
		{
			LOG_DEBUG("select: idle\n");
			continue;
		}
		LOG_DEBUG("select: %d\n", sel);

		// received data from server - parse
		if ( FD_ISSET(sock_desc, &rfds) )
		{
			struct mbuf *rxbuf = mbuf_alloc(CONTROL_LOCAL_BUFF_SIZE, 0);

			LOG_DEBUG("sock is in rfds\n");
			errno = 0;
			if ( (ret = mbuf_read(rxbuf, sock_desc, CONTROL_LOCAL_BUFF_SIZE)) < 1 )
			{
				LOG_INFO("server closed connection: %d: %s\n", ret, strerror(errno));
				mbuf_deref(rxbuf);
				RETURN_VALUE("%d", ret);
			}

			errno = 0;
			if ( (ret = expect_mbuf(rxbuf, map)) < 0 )
			{
				LOG_ERROR("Expect failed: %d: %s\n", ret, strerror(errno));
				mbuf_deref(rxbuf);
				RETURN_VALUE("%d", ret);
			}
			else if ( ret )
			{
				LOG_DEBUG("Expect matched: %d\n", ret);
				mbuf_deref(rxbuf);
				RETURN_VALUE("%d", ret);
			}
			LOG_DEBUG("Expect not matched, continue\n");
			mbuf_deref(rxbuf);
		}
		else
			LOG_DEBUG("sock NOT in rfds\n");

		// data to send to server - send
		if ( mqueue_used(&send_queue) && FD_ISSET(sock_desc, &wfds) )
		{
			struct mbuf *txbuf = mqueue_dequeue(&send_queue);

			mbuf_cur_set_beg(txbuf);
			mbuf_dump(txbuf);

			errno = 0;
			LOG_DEBUG("sock is in wfds, %d bytes to send\n", mbuf_get_avail(txbuf));
			if ( (ret = mbuf_write(txbuf, sock_desc, CONTROL_LOCAL_BUFF_SIZE)) < 0 )
			{
				LOG_ERROR("Failed to mbuf_write(): %s", strerror(errno));
				mbuf_deref(txbuf);
				RETURN_VALUE("%d", ret);
			}
			LOG_DEBUG("wrote %d bytes\n", ret);
			
			mbuf_deref(txbuf);
		}
		else
			LOG_DEBUG("sock NOT in wfds or send_queue empty\n");
	}
	while ( timeout > get_clocks() );

	RETURN_ERRNO_VALUE(0, "%d", 0);
}




