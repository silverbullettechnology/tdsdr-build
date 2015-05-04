/** Local management control
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
#include <fcntl.h>
#include <sys/time.h>
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
#include <daemon/control.h>
#include <daemon/message.h>

#include <common/control/local.h>


LOG_MODULE_STATIC("control_local", LOG_LEVEL_WARN);


#define CONTROL_LOCAL_CLIENTS_MAX 4
#define CONTROL_LOCAL_WORDS_MAX   4

struct control_local_client
{
	int            handle;
	struct worker *worker;
	struct mqueue  queue;
};

struct control_local_priv
{
	struct control       ctrl;

	// UNIX domain socket details
	char                *path;
	int                  desc;
	struct sockaddr_un   addr;
	int                  queue;

	// client connection details
	struct control_local_client  client[CONTROL_LOCAL_CLIENTS_MAX];

	char                 buffer[CONTROL_LOCAL_BUFF_SIZE];
};


/** Allocate a control instance, usually a private which starts with a control struct
 *  for casting.  The function should zero the memory, and is responsible for allocating
 *  and setting to default values any fields within the private structure.  It should not
 *  initialize any of the fields in the ancestor control; control_alloc() does that.
 */
static struct control *control_local_alloc (void)
{
	ENTER("");
	struct control_local_priv *priv = malloc(sizeof(struct control_local_priv));
	if ( !priv )
		RETURN_VALUE("%p", NULL);

	memset (priv, 0, sizeof(struct control_local_priv));
	if ( !(priv->path = strdup(CONTROL_LOCAL_SOCKET_PATH)) )
	{
		LOG_ERROR("Failed to strdup() socket path\n");
		RETURN_VALUE("%p", NULL);
	}
	priv->desc = -1;

	int i;
	for ( i = 0; i < CONTROL_LOCAL_CLIENTS_MAX; i++ )
	{
		priv->client[i].handle = -1;
		priv->client[i].worker = NULL;
		mqueue_init(&priv->client[i].queue, 0);
	}

	RETURN_ERRNO_VALUE(0, "%p", (struct control *)priv);
}


/** Config callback */
static int control_local_config (const char *section, const char *tag, const char *val,
                                 const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);
	struct control_local_priv *priv = (struct control_local_priv *)data;
	
	if ( !tag || !val )
		RETURN_ERRNO_VALUE(0, "%d", 0);

	if ( !strcmp(tag, "socket") )
	{
		free(priv->path);
		if ( !(priv->path = strdup(val)) )
		{
			LOG_ERROR("%s[%d]: %s: failed to strdup() socket path: %s\n", file, line, 
			          priv->ctrl.name, strerror(errno));
			RETURN_VALUE("%d", 0);
		}
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	errno = 0;
	int ret = control_config_common(section, tag, val, file, line, (struct control *)data);
	RETURN_VALUE("%d", ret);
}


/** Open a UNIX domain socket, bind to the configured path, set nonblocking, and
 *  listen() 
 *
 *  \param priv  Private struct
 *
 *  \return 0 on success, <0 on error
 */
static int control_local_server_uds_open (struct control_local_priv *priv)
{
	ENTER("priv %p", priv);

	// open a UNIX domain socket for datagrams
	if ( (priv->desc = socket(PF_UNIX, SOCK_STREAM, 0)) < 0 )
	{
		LOG_ERROR("%s: failed to open socket(): %s\n", priv->ctrl.name, strerror(errno));
		RETURN_VALUE ("%d", -1);
	}

	// set address from filename
	priv->addr.sun_family = AF_UNIX;
	snprintf(priv->addr.sun_path, sizeof(priv->addr.sun_path), "%s", priv->path);
	LOG_DEBUG ("%s: Socket path %s\n", priv->ctrl.name, priv->addr.sun_path);

	// bind socket to address
	if ( bind(priv->desc, (struct sockaddr *)&priv->addr, sizeof(struct sockaddr_un)) < 0 )
	{
		LOG_ERROR("%s: failed to bind(): %s\n", priv->ctrl.name, strerror(errno));
		close (priv->desc);
		RETURN_VALUE("%d", -1);
	}

	// set nonblocking and close-on-exec
	if ( set_fd_bits(priv->desc, 0, O_NONBLOCK, 0, FD_CLOEXEC) < 0 )
	{
		LOG_ERROR("%s: failed to set_fd_bits(): %s\n", priv->ctrl.name, strerror(errno));
		unlink(priv->path);
		close(priv->desc);
		RETURN_VALUE ("%d", -1);
	}

	// make it available for incoming connections
	if ( listen(priv->desc, priv->queue) < 0 )
	{
		LOG_ERROR("%s: failed to listen(): %s\n", priv->ctrl.name, strerror(errno));
		unlink(priv->path);
		close(priv->desc);
		RETURN_VALUE ("%d", -1);
	}
	LOG_INFO("%s: Listening on socket %d, path %s\n", priv->ctrl.name,
	         priv->desc, priv->addr.sun_path);

	RETURN_ERRNO_VALUE (0, "%d", 0);
}


/** Check status of a control, (re)open/spawn if necessary.  Return <0 on fatal error, if
 *  application should stop.  Otherwise should implement rate limiting internally and try
 *  to maintain/re-establish connections robustly, and return 0 if rest of the application
 *  can continue. */
static int control_local_check (struct control *ctrl)
{
	ENTER("ctrl %p", ctrl);
	if ( !ctrl )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	struct control_local_priv *priv = (struct control_local_priv *)ctrl;

	if ( priv->desc < 0 && control_local_server_uds_open(priv) )
		RETURN_VALUE("%d", -1);

	int             ret, idx;
	struct sockaddr sock_addr;
	socklen_t       addr_size = sizeof(sock_addr);

	// try to accept on our listening socket; loop through possible backlog
	while ( (ret = accept(priv->desc, &sock_addr, &addr_size)) > -1 )
	{
		// set nonblocking and close-on-exec
		if ( set_fd_bits(ret, 0, 0, O_NONBLOCK, FD_CLOEXEC) < 0 )
		{
			LOG_ERROR("%s: failed to set_fd_bits(): %s\n", priv->ctrl.name, strerror(errno));
			close(ret);
			RETURN_VALUE ("%d", -1);
		}

		for ( idx = 0; idx < CONTROL_LOCAL_CLIENTS_MAX; idx++ )
			if ( priv->client[idx].handle < 0 )
			{
				priv->client[idx].handle = ret;
				break;
			}

		// too many connections: close, should send a "busy" message
		if ( idx >= CONTROL_LOCAL_CLIENTS_MAX )
		{
			LOG_ERROR ("%s: Client limit reached\n", priv->ctrl.name);
			close (ret);
			continue;
		}

		LOG_DEBUG("%s: Client slot %d accepted\n", priv->ctrl.name, idx);
	}

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Called each main loop to register the control's receive file descriptors in an fd_set
 *  supplied by the caller, and updates *mfda if necessary.  Note: controls which don't
 *  use select()able file descriptors can use this to poll internally as long as they
 *  don't block. */
static void control_local_fd_set (struct control *ctrl,
                                  fd_set *rfds, fd_set *wfds, int *mfda)
{
	ENTER("ctrl %p, rfds %p, wfds %p, mfda %d", ctrl, rfds, wfds, *mfda);
	if ( !ctrl )
		RETURN_ERRNO(EFAULT);
	struct control_local_priv *priv = (struct control_local_priv *)ctrl;

	if ( !rfds && !wfds )
		RETURN_ERRNO(0);

	if ( rfds )
	{
		FD_SET(priv->desc, rfds);
		if ( priv->desc > *mfda )
			*mfda = priv->desc;
	}

	int idx;
	for ( idx = 0; idx < CONTROL_LOCAL_CLIENTS_MAX; idx++ )
		if ( priv->client[idx].handle >= 0 )
		{
			if ( rfds )
			{
				FD_SET(priv->client[idx].handle, rfds);
				if ( priv->client[idx].handle > *mfda )
					*mfda = priv->client[idx].handle;
			}

			// for writes, only fd_set if the TX queue is not empty
			if ( wfds && mqueue_used(&priv->client[idx].queue) )
			{
				FD_SET(priv->client[idx].handle, wfds);
				if ( priv->client[idx].handle > *mfda )
					*mfda = priv->client[idx].handle;
			}
		}

	RETURN_ERRNO(0);
}


/** Called each main loop after a select(), should return >0 if the recv_fn should be
 *  called to process received data, <0 if the application should stop.  Note: controls
 *  which don't use select()able file descriptors can use this to indicate from some other
 *  check that they're ready to be polled. */
static int control_local_fd_is_set (struct control *ctrl, fd_set *rfds, fd_set *wfds)
{
	ENTER("ctrl %p, rfds %p, wfds %p", ctrl, rfds, wfds);
	if ( !ctrl )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	
	if ( !rfds && !wfds )
		RETURN_ERRNO_VALUE(0, "%d", 0);

	struct control_local_priv *priv = (struct control_local_priv *)ctrl;

	int ret = 0;
	if ( rfds && FD_ISSET(priv->desc, rfds) )
		ret++;

	int idx;
	for ( idx = 0; idx < CONTROL_LOCAL_CLIENTS_MAX; idx++ )
		if ( priv->client[idx].handle >= 0 )
		{
			if ( rfds && FD_ISSET(priv->client[idx].handle, rfds) )
				ret++;
			else if ( wfds && FD_ISSET(priv->client[idx].handle, wfds) )
				ret++;
		}

	RETURN_VALUE("%d", ret);
}


/** Called to read received data from other side and process it.  Passed sel as a pointer
 *  to the int holding the return value of a select() call, and rfds pointing to the read
 *  fd_set used.  If sel is passed, >0, and a file descriptor in the fd_set is ready, the
 *  function should read from the descriptor and decrement *sel.  Return <0 on error, but
 *  only if application should stop. */
static int control_local_read (struct control *ctrl, fd_set *rfds)
{
	ENTER("ctrl %p, rfds %p", ctrl, rfds);
	if ( !ctrl )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	struct control_local_priv *priv = (struct control_local_priv *)ctrl;

	int ret;
	if ( FD_ISSET(priv->desc, rfds) && (ret = control_local_check(ctrl)) < 0 )
	{
		LOG_ERROR("%s: control_local_check(): %d: %s\n", ctrl->name, ret, strerror(errno));
		RETURN_VALUE("%d", ret);
	}

	int idx;
	for ( idx = 0; idx < CONTROL_LOCAL_CLIENTS_MAX; idx++ )
		if ( priv->client[idx].handle >= 0 && FD_ISSET(priv->client[idx].handle, rfds) )
		{
			struct mbuf *mbuf = mbuf_alloc(CONTROL_LOCAL_BUFF_SIZE, sizeof(struct message));
			int          len;

			// Receive into failsafe buffer if mbuf_alloc failed
			if ( !mbuf )
			{
				memset(priv->buffer, 0, sizeof(priv->buffer));
				LOG_WARN("%s: mbuf_alloc() failed, message dropped\n", control_name(ctrl));

				errno = 0;
				len = read(priv->client[idx].handle, priv->buffer, sizeof(priv->buffer));
			}
			else
			{
				struct message *user = mbuf_user(mbuf);
				user->control = ctrl;
				user->socket  = idx;
				user->worker  = priv->client[idx].worker;

				len = mbuf_read(mbuf, priv->client[idx].handle, CONTROL_LOCAL_BUFF_SIZE);
			}

			// error handling
			if ( len < 0 )
			{
				// treat interrupted as idle
				if ( errno == EINTR )
					continue;
				
				// fall-through for disconnect
				if ( errno != ECONNRESET )
				{
					LOG_ERROR("%s: read() failed: %s\n", control_name(ctrl), strerror(errno));
					RETURN_VALUE("%d", len);
				}
			}

			// closed socket
			if ( len == 0 && errno == 0 )
			{
				LOG_DEBUG("%s: Client slot %d closed\n", control_name(ctrl), idx);

				// close socket
				close(priv->client[idx].handle);
				priv->client[idx].handle = -1;
				priv->client[idx].worker = NULL;
				mqueue_flush(&priv->client[idx].queue);

				// discard unused mbuf
				if ( mbuf )
					mbuf_deref(mbuf);
				continue;
			}

			// message to handle
			LOG_DEBUG("%s: Client slot %d sent %d bytes:\n", control_name(ctrl), idx, len);
			if ( mbuf )
			{
				mbuf_dump(mbuf);
				if ( mqueue_enqueue(&ctrl->recv, mbuf) < 0 )
				{
					LOG_ERROR("%s: enqueue failed: %s\n", control_name(ctrl), strerror(errno));
					RETURN_VALUE("%d", -1);
				}
			}
			else
				LOG_HEXDUMP(priv->buffer, len);
		}

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Called to transmit queued data to other side.  Should return <0 on error, if
 *  application should stop. */
static int control_local_write (struct control *ctrl, fd_set *wfds)
{
	ENTER("ctrl %p, wfds %p", ctrl, wfds);
	if ( !ctrl )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	struct control_local_priv *priv = (struct control_local_priv *)ctrl;
	struct mbuf               *mbuf;
	int                        idx;

	for ( idx = 0; idx < CONTROL_LOCAL_CLIENTS_MAX; idx++ )
		if ( priv->client[idx].handle >= 0 && FD_ISSET(priv->client[idx].handle, wfds) )
		{
			if ( !(mbuf = mqueue_dequeue(&priv->client[idx].queue)) )
			{
				LOG_WARN("%s: socket %d wfds set, but empty queue?\n", control_name(ctrl), idx);
				continue;
			}

			mbuf_cur_set_beg(mbuf);
			if ( mbuf_write(mbuf, priv->client[idx].handle, CONTROL_LOCAL_BUFF_SIZE) < 0 )
				LOG_ERROR("%s: socket %d write failed: %s: dropped\n", control_name(ctrl),
						  idx, strerror(errno));

			mbuf_deref(mbuf);
		}

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Enqueue a message for the control */
static int control_local_enqueue (struct control *ctrl, struct mbuf *mbuf)
{
	ENTER("ctrl %p", ctrl);
	if ( !ctrl | !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	struct control_local_priv *priv = (struct control_local_priv *)ctrl;
	struct message            *user;
	int                        done;
	int                        idx;

	user = mbuf_user(mbuf);
	done = 0;
	LOG_DEBUG("%s: mbuf %p: worker %s, socket %d\n", control_name(ctrl),
	          mbuf, worker_name(user->worker), user->socket);
	for ( idx = 0; idx < CONTROL_LOCAL_CLIENTS_MAX; idx++ )
	{
		// skip if closed, or wrong socket for source worker
		if ( priv->client[idx].handle < 0 )
			continue;

		LOG_DEBUG("compare socket %d: worker %s\n", idx,
		          worker_name(priv->client[idx].worker));
		if ( user->socket != idx && priv->client[idx].worker != user->worker )
		{
			LOG_DEBUG("drop\n");
			continue;
		}

		// subsequent sockets: clone and enqueue
		if ( done )
		{
			struct mbuf *clone = mbuf_clone(mbuf);
			if ( clone )
				mqueue_enqueue(&priv->client[idx].queue, clone);
			else
				LOG_WARN("%s: mbuf_clone() failed\n", control_name(ctrl));
			continue;
		}

		// first time through: enqueue directly
		LOG_DEBUG("%s: enqueue for TX to %d\n", control_name(ctrl), idx);
		mqueue_enqueue(&priv->client[idx].queue, mbuf);
		done = 1;
	}

	// not dispatched: log and drop
	if ( !done )
	{
		LOG_DEBUG("%s: packet from %p not matched: dropped\n", control_name(ctrl),
		          user->worker);
		mbuf_deref(mbuf);
	}

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Called to associate a particular control stream with a worker thread.
 *  should read from the descriptor.  Return <0 on error, but only if application should
 *  stop. */
static int control_local_connect (struct control *ctrl, int socket, struct worker *worker)
{
	ENTER("ctrl %p, socket %d, worker %p", ctrl, socket, worker);
	if ( !ctrl )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( socket < 0 || socket >= CONTROL_LOCAL_CLIENTS_MAX )
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);

	struct control_local_priv *priv = (struct control_local_priv *)ctrl;

	LOG_DEBUG("%s: change socket %d worker %s -> %s\n", control_name(ctrl),
	          socket, worker_name(priv->client[socket].worker), worker_name(worker));
	priv->client[socket].worker = worker;

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Close files/devices associated with this instance.  Return 0 on success. */
static void control_local_close (struct control *ctrl)
{
	ENTER("ctrl %p", ctrl);
	if ( !ctrl )
		RETURN_ERRNO(EFAULT);
	struct control_local_priv *priv = (struct control_local_priv *)ctrl;

	LOG_INFO("%s: close socket %d and unlink %s\n", ctrl->name,
	         priv->desc, priv->addr.sun_path);
	unlink (priv->addr.sun_path);
	close  (priv->desc);
	priv->desc = -1;

	RETURN_ERRNO(0);
}


/** Free memory allocated for the instance. */
static void control_local_free (struct control *ctrl)
{
	ENTER("ctrl %p", ctrl);
	struct control_local_priv *priv = (struct control_local_priv *)ctrl;

	free(priv->path);
	free(priv);

	RETURN_ERRNO(0);
}


/** Operation pointers. */
static struct control_ops control_local_ops =
{
	alloc_fn:      control_local_alloc,
	config_fn:     control_local_config,
	check_fn:      control_local_check,
	fd_set_fn:     control_local_fd_set,
	fd_is_set_fn:  control_local_fd_is_set,
	read_fn:       control_local_read,
	write_fn:      control_local_write,
	enqueue_fn:    control_local_enqueue,
	connect_fn:    control_local_connect,
	close_fn:      control_local_close,
	free_fn:       control_local_free,
};
CONTROL_CLASS(local, control_local_ops);


/* Ends    : src/daemon/control/local.c */
