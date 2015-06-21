/** Socket abstraction
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
#ifndef INCLUDE_TOOL_CONTROL_SOCKET_H
#define INCLUDE_TOOL_CONTROL_SOCKET_H
#include <sys/select.h>

#include <sbt_common/growlist.h>
#include <sbt_common/descript.h>
#include <sbt_common/mqueue.h>
#include <sbt_common/mbuf.h>

#include <config/config.h>


/** Allocate a socket instance, usually a private which starts with a socket struct for
 *  casting.  The function should zero the memory, and is responsible for allocating and
 *  setting to default values any fields within the private structure.  It should not
 *  initialize any of the fields in the ancestor socket; socket_alloc() does that. */
typedef struct socket * (* socket_alloc_fn) (void);

/** Command-line configuration of a socket, usually instead of a config-file parse,
 *  usually the address of the remote peer.   Return nonzero if the passed argument is
 *  invalid. */
typedef int (* socket_cmdline_fn) (struct socket *sock, const char *tag, const char *val);

/** Check status of a socket, (re)open/spawn if necessary.  Return <0 on fatal error, if
 *  application should stop.  Otherwise should implement rate limiting internally and try
 *  to maintain/re-establish connections robustly, and return 0 if rest of the application
 *  can continue. */
typedef int (* socket_check_fn) (struct socket *sock);

/** Called each main loop to register the socket's receive file descriptors in an fd_set
 *  supplied by the caller, and updates *mfda if necessary.  Note: sockets which don't
 *  use select()able file descriptors can use this to poll internally as long as they
 *  don't block. */
typedef void (* socket_fd_set_fn) (struct socket *sock,
                                   fd_set *rfds, fd_set *wfds, int *mfda);

/** Called each main loop after a select(), should return >0 if the read_fn should be
 *  called to process received data, <0 if the application should stop.  Note: sockets
 *  which don't use select()able file descriptors can use this to indicate from some other
 *  check that they're ready to be polled. */
typedef int (* socket_fd_is_set_fn) (struct socket *sock, fd_set *rfds, fd_set *wfds);

/** Called to read received data from other side and process it.  the function should read
 * from the descriptor.  Return NULL on error, but only if application should stop. */
typedef struct mbuf * (* socket_read_fn) (struct socket *sock);

/** Called to transmit queued data to other side.  Should return <0 on error, if
 *  application should stop. */
typedef int (* socket_write_fn) (struct socket *sock);


/** Perform internal processing.  For sockets which interact with the system through
 *  queued messages, this is where they should dequeue and process incoming messages, and
 *  emit and enqueue outgoing messages.  Return 0 on success. */
typedef void (* socket_process_fn) (struct socket *socket);

/** Called to enqueue a message for sockets which want to dispatch enqueued messages to
 *  reduce latency.  If latency is not important, the socket can skip this and
 *  socket_enqueue() will simply enqueue the mbuf in the socket's send.  Return <0 if
 *  the application should stop. */
typedef int (* socket_enqueue_fn) (struct socket *sock, struct mbuf *mbuf);


/** Close files/devices associated with this instance.  Return 0 on success. */
typedef void (* socket_close_fn) (struct socket *sock);

/** Free memory allocated for the instance. */
typedef void (* socket_free_fn) (struct socket *sock);


/** Operation pointers. */
struct socket_ops
{
	socket_alloc_fn       alloc_fn;
	config_func_t         config_fn;
	socket_cmdline_fn     cmdline_fn;
	socket_check_fn       check_fn;
	socket_fd_set_fn      fd_set_fn;
	socket_fd_is_set_fn   fd_is_set_fn;
	socket_read_fn        read_fn;
	socket_write_fn       write_fn;
	socket_process_fn     process_fn;
	socket_enqueue_fn     enqueue_fn;
	socket_close_fn       close_fn;
	socket_free_fn        free_fn;
};


/** Class registration maps a name to a set of operations.  The list is generated at build
 *  time by socket/scan.sh and searchable with socket_class_search().  All of this is
 *  internal to socket_alloc() and should not be needed by the caller. */
struct socket_class
{
	const char         *name;
	struct socket_ops *ops;
};


/* Macro for linker-generated list */
#define SOCKET_CLASS(name, ops) \
	static struct socket_class _socket_class_ ## name \
	     __attribute__((unused,used,section("socket_class_map"),aligned(sizeof(void *)))) \
	     = { #name, &ops };


/** Instance of a particular socket.  The socket may define a larger private struct
 *  which includes this as the first member, so the socket pointer may be cast to the
 *  private pointer inside the socket's code */
struct socket
{
	const char                 *name;
	const struct socket_class  *class;
	struct mqueue               queue;
};


/** List of all sockets allocated with socket_alloc */
extern struct socket *sock;


/** Allocate a new, idle instance of the class
 *
 *  \return Allocated socket pointer, or NULL on failure
 */
struct socket *socket_alloc (const char *name);


/** Configure an instance
 *
 *  This will call config_parse() on the passed path to configure it, using the instance
 *  name as the section name and the class config_fn for matching items.  It is expected
 *  the class config_fn will call socket_config_common() for items it's not handling
 *  specifically.
 *
 *  \param path Config file path
 *
 *  \return 0 on success, <0 on failure
 */
int socket_config_inst (struct socket *sock, const char *path);


/** Common configuration callback
 *
 *  This is a convenience function for individual sockets' config handlers.  The intent
 *  is for sockets' handlers to handle socket-specific config items and return early.
 *  Unrecognized items are passed through to this common handler.
 *
 *  \return 0 on success, <0 on failure
 */
int socket_config_common (const char *section, const char *tag, const char *val,
                          const char *file, int line, struct socket *sock);


/** Command-line config of a socket
 *
 *  \param sock Instance to be configured
 *  \param tag  Argument name
 *  \param val  Argument value
 *
 *  \return 0 on success, <0 on failure
 */
static inline int socket_cmdline (struct socket *sock, const char *tag, const char *val)
{
	if ( !sock )
		return -1;

	if ( !sock->class->ops->cmdline_fn )
		return 0;

	return sock->class->ops->cmdline_fn(sock, tag, val);
}


/** Check the instance post-configuration
 *
 *  \param sock Instance to be checked
 *
 *  \return 0 on success, <0 on failure
 */
static inline int socket_check (struct socket *sock)
{
	if ( !sock )
		return -1;

	if ( !sock->class->ops->check_fn )
		return 0;

	return sock->class->ops->check_fn(sock);
}


/** Add the instance's file handle(s) to an fd_set for a select() call
 *
 *  Calls FD_SET() for the instance's file handle(s), and updates *mfda if the instance's
 *  file handle(s) are higher than the current value.
 *
 *  \param sock Instance to add to the fd_set
 *  \param rfds An fd_set being setup for a select() call
 *  \param mfda Pointer to an int to be passed as arg 1 to select()
 */
static inline void socket_fd_set (struct socket *sock,
                                  fd_set *rfds, fd_set *wfds, int *mfda)
{
	if ( !sock || !sock->class->ops->fd_set_fn )
		return;

	sock->class->ops->fd_set_fn(sock, rfds, wfds, mfda);
}


/** Test the instance's file handle(s) against an fd_set after a select() call
 *
 *  \param sock Instance to test against the fd_set
 *  \param ret  Pointer to an int returned by the select()
 *  \param rfds A read fd_set previously passed to a select() call
 *  \param wfds A write fd_set previously passed to a select() call
 *
 *  \return Number of instance's file handle(s) set in the fd_set, or <0 on failure
 */
static inline int socket_fd_is_set (struct socket *sock, fd_set *rfds, fd_set *wfds)
{
	if ( !sock )
		return -1;

	if ( !sock->class->ops->fd_is_set_fn )
		return 0;

	return sock->class->ops->fd_is_set_fn(sock, rfds, wfds);
}


/** Poll a socket indicated ready to read
 *
 *  \param sock Instance to be polled
 *
 *  \return 0 on success, <0 on failure
 */
static inline struct mbuf *socket_read (struct socket *sock)
{
	if ( !sock || !sock->class->ops->read_fn )
		return NULL;

	return sock->class->ops->read_fn(sock);
}


/** Transmit queued messages from a socket indicated ready to send
 *
 *  \param sock Instance to send message to
 *  \param ret  Pointer to an int returned by the select()
 *
 *  \return 0 on success, <0 on failure
 */
static inline int socket_write (struct socket *sock)
{
	if ( !sock )
		return -1;

	if ( !sock->class->ops->write_fn )
		return 0;

	return sock->class->ops->write_fn(sock);
}


/** Internal processing
 *
 *  \param socket  Instance to process
 */
static inline void socket_process (struct socket *sock)
{
	if ( !sock || !sock->class || !sock->class->ops || !sock->class->ops->process_fn )
		return;

	return sock->class->ops->process_fn(sock);
}


/** enqueue a message for transmit
 *
 *  \note Since the message may be submitted to multiple sockets, individual sockets may
 *        not modify the passed message; if the socket needs to modify the message it
 *        should use mbuf_clone() to make a local copy.
 *
 *  \param sock Instance to send message via
 *  \param mbuf Message buffer to send
 *
 *  \return 0 on success, <0 on failure
 */
int socket_enqueue (struct socket *sock, struct mbuf *mbuf);


/** dequeue a message emitted.  Pass mbuf as a pointer to an mbuf pointer, the
 *  
 *  \param sock Instance to dequeue message from
 *
 *  \return Pointer to mbuf pointer, value will be NULL if the queue is empty.
 */
struct mbuf *socket_dequeue (struct socket *sock);


/** Close the instance's file handles, yielding the source device
 *
 *  After this call, the socket's structures and buffers remain in memory, and the
 *  socket can be reopened with a call to socket_check().  
 *
 *  \param sock  Instance to close
 */
static inline void socket_close (struct socket *sock)
{
	if ( !sock || !sock->class->ops->close_fn )
		return;

	sock->class->ops->close_fn(sock);
}


/** Free the instance's memory structures
 *
 *  \param sock  Instance to free
 */
void socket_free (struct socket *sock);


static inline const char *socket_name (struct socket *sock)
{
	if ( !sock || !sock->name )
		return "(null)";
	else
		return sock->name;
}


#endif /* INCLUDE_TOOL_CONTROL_SOCKET_H */
/* Ends */
