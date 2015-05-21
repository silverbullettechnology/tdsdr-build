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

#include <tool/control/socket.h>
#include <tool/control/control.h>
#include <tool/control/expect.h>


LOG_MODULE_STATIC("control_expect", LOG_LEVEL_DEBUG);


/** Enqueue an mbuf to send to the daemon
 *
 *  \param  mbuf  Message buffer to send
 *
 *  \return >= 0 on success, <0 on error
 */
int expect_send (struct mbuf *mbuf)
{
	return socket_enqueue(sock, mbuf);
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
	struct timeval  tv_cur;
	fd_set          rfds;
	fd_set          wfds;
	int             mfda = -1;
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

		socket_check(sock);

		socket_fd_set(sock, &rfds, &wfds, &mfda);
		tv_cur.tv_sec  = 0;
		tv_cur.tv_usec = 333000;

		LOG_DEBUG("top of loop, select:\n");
		if ( (sel = select(mfda + 1, &rfds, &wfds, NULL, &tv_cur)) < 0 )
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
		if ( socket_fd_is_set(sock, &rfds, NULL) )
		{
			struct mbuf *mbuf;

			LOG_DEBUG("sock is in rfds\n");
			errno = 0;
			if ( (mbuf = socket_read(sock)) )
			{
				errno = 0;
				if ( (ret = expect_mbuf(mbuf, map)) < 0 )
				{
					LOG_ERROR("Expect failed: %d: %s\n", ret, strerror(errno));
					mbuf_deref(mbuf);
					RETURN_VALUE("%d", ret);
				}
				else if ( ret )
				{
					LOG_DEBUG("Expect matched: %d\n", ret);
					mbuf_deref(mbuf);
					RETURN_VALUE("%d", ret);
				}
				LOG_DEBUG("Expect not matched, continue\n");
			}
			else
			{
				LOG_WARN("No read after sock in rfds?\n");
				continue;
			}
			mbuf_deref(mbuf);
		}
		else
			LOG_DEBUG("sock NOT in rfds\n");

		// data to send to server - send
		if ( socket_fd_is_set(sock, NULL, &wfds) )
		{
			errno = 0;
			LOG_DEBUG("sock is in wfds\n");
			if ( socket_write(sock) < 0 )
				LOG_ERROR("Failed socket write(): %s", strerror(errno));
		}
		else
			LOG_DEBUG("sock NOT in wfds or send_queue empty\n");
	}
	while ( timeout > get_clocks() );

	RETURN_ERRNO_VALUE(0, "%d", 0);
}




