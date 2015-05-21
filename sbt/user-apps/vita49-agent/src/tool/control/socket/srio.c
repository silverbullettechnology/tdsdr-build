/** Serial RapidIO adapter
 *
 *  \author    Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
 *  \version   v0.0
 *  \copyright Copyright 2015 Silver Bullet Technology
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
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include <lib/log.h>
#include <lib/util.h>
#include <lib/mbuf.h>
#include <lib/mqueue.h>

#include <common/control/srio.h>

#include <tool/control/socket.h>


LOG_MODULE_STATIC("socket_srio", LOG_LEVEL_WARN);


#define CLIENTS_MAX 8
#define NEXT_CHECK  15
#define HEAD_SIZE   (offsetof(struct sd_mesg,      mesg) + \
                     offsetof(struct sd_mesg_mbox, data))


struct socket_srio_priv
{
	struct socket  sock;

	// SRIO device node
	char           *path;
	uint8_t         mbox;
	int             addr;
	int             desc;
	time_t          next;

	// fail-safe buffer
	char            buffer[CONTROL_SRIO_BUFF_SIZE];
};


/** Allocate a socket instance, usually a private which starts with a socket struct
 *  for casting.  The function should zero the memory, and is responsible for allocating
 *  and setting to default values any fields within the private structure.  It should not
 *  initialize any of the fields in the ancestor socket; socket_alloc() does that.
 */
static struct socket *socket_srio_alloc (void)
{
	ENTER("");
	struct socket_srio_priv *priv = malloc(sizeof(struct socket_srio_priv));
	if ( !priv )
		RETURN_VALUE("%p", NULL);

	memset (priv, 0, sizeof(struct socket_srio_priv));
	if ( !(priv->path = strdup(CONTROL_SRIO_NODE_PATH)) )
	{
		LOG_ERROR("Failed to strdup() socket path\n");
		RETURN_VALUE("%p", NULL);
	}
	priv->desc = -1;
	priv->mbox =  0;
	priv->next =  0;
	mqueue_init(&priv->sock.queue, 0);

	RETURN_ERRNO_VALUE(0, "%p", (struct socket *)priv);
}


/** Config callback */
static int socket_srio_config (const char *section, const char *tag, const char *val,
                                const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);
	struct socket_srio_priv *priv = (struct socket_srio_priv *)data;

	if ( !tag || !val )
		RETURN_ERRNO_VALUE(0, "%d", 0);

	if ( !strcmp(tag, "dev") )
	{
		free(priv->path);
		if ( !(priv->path = strdup(val)) )
		{
			LOG_ERROR("%s[%d]: %s: failed to strdup() dev node path: %s\n", file, line, 
			          priv->sock.name, strerror(errno));
			RETURN_VALUE("%d", -1);
		}
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	if ( !strcmp(tag, "mbox") )
	{
		if ( (priv->mbox = strtoul(val, NULL, 0)) > 3 )
		{
			LOG_ERROR("%s[%d]: %s: mbox number must be 0..3\n", file, line, 
			          priv->sock.name);
			RETURN_ERRNO_VALUE(EINVAL, "%d", -1);
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
static int socket_srio_check (struct socket *sock)
{
	ENTER("sock %p", sock);
	if ( !sock )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	struct socket_srio_priv *priv = (struct socket_srio_priv *)sock;

	// (re)open device node as needed and subscribe to mailbox
	if ( priv->desc < 0 )
	{
		time_t now = time(NULL);
		if ( priv->next > now )
		{
			LOG_DEBUG("%s: Waiting to (re)open\n", priv->sock.name);
			RETURN_VALUE("%d", 0);
		}

		if ( (priv->desc = open(priv->path, O_RDWR|O_NONBLOCK)) < 0 )
		{
			LOG_ERROR("%s: open(%s) failed: %s\n", priv->sock.name,
			          priv->path, strerror(errno));
			priv->next = time(NULL) + NEXT_CHECK;
			RETURN_VALUE ("%d", 0);
		}

		// set nonblocking and close-on-exec
		if ( set_fd_bits(priv->desc, 0, O_NONBLOCK, 0, FD_CLOEXEC) < 0 )
		{
			LOG_ERROR("%s: failed to set_fd_bits(): %s\n", priv->path, strerror(errno));
			close(priv->desc);
			priv->desc = -1;
			priv->next = time(NULL) + NEXT_CHECK;
			RETURN_VALUE ("%d", -1);
		}

		unsigned mbox_sub = 1 << priv->mbox;
		if ( ioctl(priv->desc, SD_USER_IOCS_MBOX_SUB, mbox_sub) )
		{
			LOG_ERROR("%s: ioctl(SD_USER_IOCS_MBOX_SUB) failed: %s\n", priv->sock.name,
			          strerror(errno));
			close(priv->desc);
			priv->desc = -1;
			priv->next = time(NULL) + NEXT_CHECK;
			RETURN_VALUE ("%d", 0);
		}
	}

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Called each main loop to register the socket's receive file descriptors in an fd_set
 *  supplied by the caller, and updates *mfda if necessary.  Note: sockets which don't
 *  use select()able file descriptors can use this to poll internally as long as they
 *  don't block. */
static void socket_srio_fd_set (struct socket *sock, fd_set *rfds, fd_set *wfds, int *mfda)
{
	ENTER("sock %p, rfds %p, wfds %p, mfda %d", sock, rfds, wfds, *mfda);
	if ( !sock )
		RETURN_ERRNO(EFAULT);
	struct socket_srio_priv *priv = (struct socket_srio_priv *)sock;

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
static int socket_srio_fd_is_set (struct socket *sock, fd_set *rfds, fd_set *wfds)
{
	ENTER("sock %p, rfds %p, wfds %p", sock, rfds, wfds);
	if ( !sock )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( !rfds && !wfds )
		RETURN_ERRNO_VALUE(0, "%d", 0);

	struct socket_srio_priv *priv = (struct socket_srio_priv *)sock;

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
static struct mbuf *socket_srio_read (struct socket *sock)
{
	ENTER("sock %p", sock);
	if ( !sock )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);
	struct socket_srio_priv *priv = (struct socket_srio_priv *)sock;

	struct mbuf *mbuf = mbuf_alloc(CONTROL_SRIO_BUFF_SIZE, 0);
	int          len;

	// Receive into failsafe buffer if mbuf_alloc failed
	if ( !mbuf )
	{
		memset(priv->buffer, 0, sizeof(priv->buffer));
		LOG_WARN("%s: mbuf_alloc() failed, message dropped\n", socket_name(sock));

		errno = 0;
		len = read(priv->desc, priv->buffer, sizeof(priv->buffer));
		RETURN_ERRNO_VALUE(0, "%p", NULL);
	}

	// read and error handling
	if ( (len = mbuf_read(mbuf, priv->desc, CONTROL_SRIO_BUFF_SIZE)) < 0 )
	{
		// treat interrupted as idle
		if ( errno == EINTR )
			RETURN_ERRNO_VALUE(0, "%p", NULL);

		// anything else: close node and prep for reopen
		LOG_ERROR("%s: read() failed: %s\n", socket_name(sock), strerror(errno));
		RETURN_ERRNO_VALUE(0, "%p", NULL);
		close(priv->desc);
		priv->desc = -1;
		priv->next = time(NULL) + 1;
	}

	// message to handle - get payload pointer and verify length
	struct sd_mesg *mesg = mbuf_beg_ptr(mbuf);
	if ( !mesg )
	{
		LOG_ERROR("%s: mbuf_beg_ptr() should never fail here: %s\n",
		          socket_name(sock), strerror(errno));
		mbuf_deref(mbuf);
		RETURN_ERRNO_VALUE(0, "%p", NULL);
	}
	mbuf_cur_set_beg(mbuf);
	errno = 0;
	if ( (int)mesg->size != mbuf_get_avail(mbuf) )
	{
		LOG_ERROR("%s: mesg->size %zu but mbuf_get_avail() %d, must match: %s\n",
		          socket_name(sock), mesg->size, mbuf_get_avail(mbuf),
		          strerror(errno));
		_mbuf_dump(LOG_LEVEL_ERROR, mbuf, __FILE__, __LINE__, __func__);
		mbuf_deref(mbuf);
		RETURN_ERRNO_VALUE(0, "%p", NULL);
	}
	LOG_DEBUG("%s: mesg->size %zu matches mbuf_get_avail() %d\n", socket_name(sock),
	          mesg->size, mbuf_get_avail(mbuf));

	// verify type and mbox number
	if ( mesg->type != 11 || mesg->mesg.mbox.mbox != priv->mbox )
	{
		LOG_ERROR("%s: expect mesg->type 11 and mbox %u, dropped\n",
		          socket_name(sock), priv->mbox);
		_mbuf_dump(LOG_LEVEL_ERROR, mbuf, __FILE__, __LINE__, __func__);
		mbuf_deref(mbuf);
		RETURN_ERRNO_VALUE(0, "%p", NULL);
	}
	mbuf_dump(mbuf);

	// done header validation, move beg up to pass into daemon
	mbuf_beg_adj(mbuf, HEAD_SIZE);
	LOG_DEBUG("%s: Letter %d from 0x%04x, %d bytes payload:\n", socket_name(sock),
	          mesg->mesg.mbox.letter, mesg->src_addr, mbuf_get_avail(mbuf));

	RETURN_ERRNO_VALUE(0, "%p", mbuf);
}


/** Called to transmit queued data to other side.  Should return <0 on error, if
 *  application should stop. */
static int socket_srio_write (struct socket *sock)
{
	ENTER("sock %p", sock);
	if ( !sock )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	struct socket_srio_priv *priv = (struct socket_srio_priv *)sock;

	if ( priv->desc < 0 )
	{
		LOG_WARN("%s: wfds set, but desc closed?\n", socket_name(sock));
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	struct mbuf *mbuf;
	if ( !(mbuf = mqueue_dequeue(&priv->sock.queue)) )
	{
		LOG_WARN("%s: wfds set, but empty queue?\n", socket_name(sock));
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	// mbufs must be allocated with enough headroom to construct header
	struct sd_mesg *mesg;
	if ( mbuf_beg_adj(mbuf, 0 - HEAD_SIZE) < 0 || !(mesg = mbuf_beg_ptr(mbuf)) )
	{
		LOG_WARN("%s: mbuf doesn't have %u bytes headroom, drop\n",
		         socket_name(sock), HEAD_SIZE);
		mbuf_deref(mbuf);
		RETURN_ERRNO_VALUE(ENOMEM, "%d", 0);
	}

	// Construct the header: type 11, dest from socket 
	mesg->type = 11;
	mesg->dst_addr = priv->addr;
	mesg->mesg.mbox.mbox = priv->mbox;
	mbuf_cur_set_beg(mbuf);
	mesg->size = mbuf_get_avail(mbuf);

	// write in one shot 
	if ( mbuf_write(mbuf, priv->desc, mesg->size) < mesg->size )
		LOG_ERROR("%s: write %zu bytes failed: %s: dropped\n", socket_name(sock),
				  mesg->size, strerror(errno));

	mbuf_deref(mbuf);
	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Close files/devices associated with this instance.  Return 0 on success. */
static void socket_srio_close (struct socket *sock)
{
	ENTER("sock %p", sock);
	if ( !sock )
		RETURN_ERRNO(EFAULT);
	struct socket_srio_priv *priv = (struct socket_srio_priv *)sock;
	struct mbuf              *mbuf;

	while ( (mbuf = mqueue_dequeue(&priv->sock.queue)) )
		mbuf_deref(mbuf);

	LOG_INFO("%s: close device\n", sock->name);
	close(priv->desc);
	priv->desc = -1;

	RETURN_ERRNO(0);
}


/** Free memory allocated for the instance. */
static void socket_srio_free (struct socket *sock)
{
	ENTER("sock %p", sock);
	struct socket_srio_priv *priv = (struct socket_srio_priv *)sock;
	struct mbuf              *mbuf;

	while ( (mbuf = mqueue_dequeue(&priv->sock.queue)) )
		mbuf_deref(mbuf);

	free(priv->path);

	RETURN_ERRNO(0);
}


/** Operation pointers. */
static struct socket_ops socket_srio_ops =
{
	alloc_fn:      socket_srio_alloc,
	config_fn:     socket_srio_config,
	check_fn:      socket_srio_check,
	fd_set_fn:     socket_srio_fd_set,
	fd_is_set_fn:  socket_srio_fd_is_set,
	read_fn:       socket_srio_read,
	write_fn:      socket_srio_write,
	close_fn:      socket_srio_close,
	free_fn:       socket_srio_free,
};
SOCKET_CLASS(srio, socket_srio_ops);


/* Ends    : src/daemon/socket/srio.c */
