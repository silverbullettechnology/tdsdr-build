/** socket tool UNIX socket adapter
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
#include <lib/descript.h>

#include <common/control/local.h>
#include <tool/control/socket.h>


LOG_MODULE_STATIC("socket_unix", LOG_LEVEL_DEBUG);


struct socket_unix_priv
{
	struct socket  sock;

	// UNIX domain socket details
	char          *path;
	int            desc;

	char           buffer[CONTROL_LOCAL_BUFF_SIZE];
};


/** Allocate a socket instance, usually a private which starts with a socket struct
 *  for casting.  The function should zero the memory, and is responsible for allocating
 *  and setting to default values any fields within the private structure.  It should not
 *  initialize any of the fields in the ancestor socket; socket_alloc() does that.
 */
static struct socket *socket_unix_alloc (void)
{
	ENTER("");
	struct socket_unix_priv *priv = malloc(sizeof(struct socket_unix_priv));
	if ( !priv )
		RETURN_VALUE("%p", NULL);

	memset (priv, 0, sizeof(struct socket_unix_priv));
	if ( !(priv->path = strdup(CONTROL_LOCAL_SOCKET_PATH)) )
	{
		LOG_ERROR("Failed to strdup() socket path\n");
		RETURN_VALUE("%p", NULL);
	}
	priv->desc = -1;

	RETURN_ERRNO_VALUE(0, "%p", (struct socket *)priv);
}


/** Config callback */
static int socket_unix_config (const char *section, const char *tag, const char *val,
                                 const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);
	struct socket_unix_priv *priv = (struct socket_unix_priv *)data;
	
	if ( !tag || !val )
		RETURN_ERRNO_VALUE(0, "%d", 0);

	if ( !strcmp(tag, "socket") )
	{
		free(priv->path);
		if ( !(priv->path = strdup(val)) )
		{
			LOG_ERROR("%s[%d]: %s: failed to strdup() socket path: %s\n", file, line, 
			          priv->sock.name, strerror(errno));
			RETURN_VALUE("%d", 0);
		}
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	errno = 0;
	int ret = socket_config_common(section, tag, val, file, line, (struct socket *)data);
	RETURN_VALUE("%d", ret);
}


/** Check status of a socket, (re)open/spawn if necessary.  Return <0 on fatal error, if
 *  application should stop.  Otherwise should implement rate limiting internally and try
 *  to maintain/re-establish connections robustly, and return 0 if rest of the application
 *  can continue. */
static int socket_unix_check (struct socket *sock)
{
	ENTER("sock %p", sock);
	if ( !sock )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	struct socket_unix_priv *priv = (struct socket_unix_priv *)sock;

	if ( priv->desc < 0 )
	{
		struct sockaddr_un  addr_unix;
		struct sockaddr    *addr_ptr;
		socklen_t           addr_len;

		LOG_DEBUG("open sock...\n");
		if ( (priv->desc = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 )
		{
			LOG_ERROR("Failed to open socket: %s\n", strerror(errno));
			RETURN_VALUE("%d", -1);
		}

		// set nonblocking and close-on-exec
		if ( set_fd_bits(priv->desc, 0, O_NONBLOCK, 0, FD_CLOEXEC) < 0 )
		{
			LOG_ERROR("Failed to set_fd_bits(): %s\n", strerror(errno));
			close(priv->desc);
			priv->desc = -1;
			RETURN_VALUE ("%d", -1);
		}

		// set destination per type 
		LOG_DEBUG("sock %d opened, set UNIX addr...\n", priv->desc);
		addr_unix.sun_family = AF_UNIX;
		snprintf(addr_unix.sun_path, sizeof(addr_unix.sun_path), "%s", priv->path);
		addr_ptr = (struct sockaddr *)&addr_unix;
		addr_len = sizeof(addr_unix);

		// connect to server
		LOG_DEBUG("connect sock...\n");
		if ( connect(priv->desc, addr_ptr, addr_len) ) 
		{
			int err = errno;
			close(priv->desc);
			priv->desc = -1;
			LOG_ERROR("Failed to connect to %s: %s", priv->path, strerror(err));
			RETURN_ERRNO_VALUE(err, "%d", -1);
		}

		LOG_DEBUG("sock connected...\n");
	}

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Called each main loop to register the socket's receive file descriptors in an fd_set
 *  supplied by the caller, and updates *mfda if necessary.  Note: sockets which don't
 *  use select()able file descriptors can use this to poll internally as long as they
 *  don't block. */
static void socket_unix_fd_set (struct socket *sock,
                                  fd_set *rfds, fd_set *wfds, int *mfda)
{
	ENTER("sock %p, rfds %p, wfds %p, mfda %d", sock, rfds, wfds, *mfda);
	if ( !sock )
		RETURN_ERRNO(EFAULT);
	struct socket_unix_priv *priv = (struct socket_unix_priv *)sock;

	if ( priv->desc < 0 ) RETURN_ERRNO(0);
	if ( !rfds && !wfds ) RETURN_ERRNO(0);

	if ( rfds )
	{
		FD_SET(priv->desc, rfds);
		if ( priv->desc > *mfda )
			*mfda = priv->desc;
	}

	// for writes, only fd_set if the TX queue is not empty
	if ( wfds && mqueue_used(&priv->sock.queue) )
	{
		FD_SET(priv->desc, wfds);
		if ( priv->desc > *mfda )
			*mfda = priv->desc;
	}

	RETURN_ERRNO(0);
}


/** Called each main loop after a select(), should return >0 if the recv_fn should be
 *  called to process received data, <0 if the application should stop.  Note: sockets
 *  which don't use select()able file descriptors can use this to indicate from some other
 *  check that they're ready to be polled. */
static int socket_unix_fd_is_set (struct socket *sock, fd_set *rfds, fd_set *wfds)
{
	ENTER("sock %p, rfds %p, wfds %p", sock, rfds, wfds);
	if ( !sock )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	
	if ( !rfds && !wfds )
		RETURN_ERRNO_VALUE(0, "%d", 0);

	struct socket_unix_priv *priv = (struct socket_unix_priv *)sock;

	int ret = 0;
	if ( rfds && FD_ISSET(priv->desc, rfds) )
		ret++;
	else if ( wfds && FD_ISSET(priv->desc, wfds) )
				ret++;

	RETURN_VALUE("%d", ret);
}


/** Called to read received data from other side and process it.  Passed sel as a pointer
 *  to the int holding the return value of a select() call, and rfds pointing to the read
 *  fd_set used.  If sel is passed, >0, and a file descriptor in the fd_set is ready, the
 *  function should read from the descriptor and decrement *sel.  Return <0 on error, but
 *  only if application should stop. */
static struct mbuf *socket_unix_read (struct socket *sock)
{
	ENTER("sock %p", sock);
	if ( !sock )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);
	struct socket_unix_priv *priv = (struct socket_unix_priv *)sock;
	struct mbuf             *mbuf = mbuf_alloc(CONTROL_LOCAL_BUFF_SIZE, 0);
	int                      ret;

	LOG_DEBUG("sock is in rfds\n");
	errno = 0;
	if ( (ret = mbuf_read(mbuf, priv->desc, CONTROL_LOCAL_BUFF_SIZE)) < 1 )
	{
		LOG_INFO("server closed connection: %d: %s\n", ret, strerror(errno));
		mbuf_deref(mbuf);
		RETURN_VALUE("%p", NULL);
	}

	RETURN_ERRNO_VALUE(0, "%p", mbuf);
}


/** Called to transmit queued data to other side.  Should return <0 on error, if
 *  application should stop. */
static int socket_unix_write (struct socket *sock)
{
	ENTER("sock %p", sock);
	if ( !sock )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	struct socket_unix_priv *priv = (struct socket_unix_priv *)sock;
	struct mbuf             *mbuf = mqueue_dequeue(&priv->sock.queue);
	int                      ret;

	if ( !mbuf )
	{
		LOG_WARN("%s with nothing to write?\n", socket_name(sock));
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	mbuf_cur_set_beg(mbuf);
	mbuf_dump(mbuf);

	errno = 0;
	LOG_DEBUG("sock is in wfds, %d bytes to send\n", mbuf_get_avail(mbuf));
	if ( (ret = mbuf_write(mbuf, priv->desc, CONTROL_LOCAL_BUFF_SIZE)) < 0 )
	{
		LOG_ERROR("Failed to mbuf_write(): %s", strerror(errno));
		mbuf_deref(mbuf);
		RETURN_VALUE("%d", 0);
	}
	LOG_DEBUG("wrote %d bytes\n", ret);
			
	mbuf_deref(mbuf);

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Close files/devices associated with this instance.  Return 0 on success. */
static void socket_unix_close (struct socket *sock)
{
	ENTER("sock %p", sock);
	if ( !sock )
		RETURN_ERRNO(EFAULT);
	struct socket_unix_priv *priv = (struct socket_unix_priv *)sock;

	close(priv->desc);
	priv->desc = -1;

	RETURN_ERRNO(0);
}


/** Free memory allocated for the instance. */
static void socket_unix_free (struct socket *sock)
{
	ENTER("sock %p", sock);
	struct socket_unix_priv *priv = (struct socket_unix_priv *)sock;

	free(priv);

	RETURN_ERRNO(0);
}


/** Operation pointers. */
static struct socket_ops socket_unix_ops =
{
	alloc_fn:      socket_unix_alloc,
	config_fn:     socket_unix_config,
	check_fn:      socket_unix_check,
	fd_set_fn:     socket_unix_fd_set,
	fd_is_set_fn:  socket_unix_fd_is_set,
	read_fn:       socket_unix_read,
	write_fn:      socket_unix_write,
	close_fn:      socket_unix_close,
	free_fn:       socket_unix_free,
};
SOCKET_CLASS(unix, socket_unix_ops);


