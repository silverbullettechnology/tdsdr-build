/** Control tool socket handling
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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <lib/log.h>
#include <lib/util.h>

#include "control.h"


LOG_MODULE_STATIC("control_socket", LOG_LEVEL_DEBUG);


int  sock_desc = -1;


int socket_open (void)
{
	struct sockaddr_un  addr_unix;
	struct sockaddr    *addr_ptr;
	socklen_t           addr_len;
	int                 sock;

	ENTER();

	LOG_DEBUG("open sock...\n");
	if ( (sock = socket(opt_sock_type, SOCK_STREAM, 0)) < 0 )
	{
		LOG_ERROR("Failed to open socket: %s\n", strerror(errno));
		RETURN_VALUE("%d", -1);
	}

	// set nonblocking and close-on-exec
	if ( set_fd_bits(sock, 0, O_NONBLOCK, 0, FD_CLOEXEC) < 0 )
	{
		LOG_ERROR("Failed to set_fd_bits(): %s\n", strerror(errno));
		close(sock);
		RETURN_VALUE ("%d", -1);
	}

	// set destination per type 
	LOG_DEBUG("sock %d opened, set addr...\n", sock);
	switch ( opt_sock_type )
	{
		case AF_UNIX:
			LOG_DEBUG("setup UNIX addr...\n");
			addr_unix.sun_family = AF_UNIX;
			snprintf(addr_unix.sun_path, sizeof(addr_unix.sun_path), "%s", opt_sock_path);
			addr_ptr = (struct sockaddr *)&addr_unix;
			addr_len = sizeof(addr_unix);
			break;

		default:
			close(sock);
			LOG_ERROR("Unsupported address type %d\n", opt_sock_type);
			RETURN_ERRNO_VALUE(EPFNOSUPPORT, "%d", -1);
	}

	// connect to server
	LOG_DEBUG("connect sock...\n");
	if ( connect(sock, addr_ptr, addr_len) ) 
	{
		int err = errno;
		close(sock);
		LOG_ERROR("Failed to connect to %s: %s", opt_sock_path, strerror(err));
		RETURN_ERRNO_VALUE(err, "%d", -1);
	}

	LOG_DEBUG("sock connected...\n");
	sock_desc = sock;
	RETURN_ERRNO_VALUE(0, "%d", sock);
}


int socket_close (void)
{
	ENTER();

	int ret = close(sock_desc);
	sock_desc = -1;

	RETURN_VALUE("%d", ret);
}

