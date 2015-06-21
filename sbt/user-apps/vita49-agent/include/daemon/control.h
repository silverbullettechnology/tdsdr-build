/** Control class abstraction
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
#ifndef INCLUDE_DAEMON_CONTROL_H
#define INCLUDE_DAEMON_CONTROL_H
#include <sys/select.h>

#include <sbt_common/growlist.h>
#include <sbt_common/descript.h>
#include <sbt_common/mqueue.h>
#include <sbt_common/mbuf.h>
#include <daemon/worker.h>

#include <config/include/config.h>


/** Allocate a control instance, usually a private which starts with a control struct for
 *  casting.  The function should zero the memory, and is responsible for allocating and
 *  setting to default values any fields within the private structure.  It should not
 *  initialize any of the fields in the ancestor control; control_alloc() does that. */
typedef struct control * (* control_alloc_fn) (void);


/** Check status of a control, (re)open/spawn if necessary.  Return <0 on fatal error, if
 *  application should stop.  Otherwise should implement rate limiting internally and try
 *  to maintain/re-establish connections robustly, and return 0 if rest of the application
 *  can continue. */
typedef int (* control_check_fn) (struct control *ctrl);

/** Called each main loop to register the control's receive file descriptors in an fd_set
 *  supplied by the caller, and updates *mfda if necessary.  Note: controls which don't
 *  use select()able file descriptors can use this to poll internally as long as they
 *  don't block. */
typedef void (* control_fd_set_fn) (struct control *ctrl,
                                    fd_set *rfds, fd_set *wfds, int *mfda);

/** Called each main loop after a select(), should return >0 if the read_fn should be
 *  called to process received data, <0 if the application should stop.  Note: controls
 *  which don't use select()able file descriptors can use this to indicate from some other
 *  check that they're ready to be polled. */
typedef int (* control_fd_is_set_fn) (struct control *ctrl, fd_set *rfds, fd_set *wfds);

/** Called to read received data from other side and process it.  Passed rfds pointing to
 *  the read fd_set used.  If a file descriptor in the fd_set is ready, the function
 *  should read from the descriptor.  Return <0 on error, but only if application should
 *  stop. */
typedef int (* control_read_fn) (struct control *ctrl, fd_set *rfds);

/** Called to transmit queued data to other side.  Should return <0 on error, if
 *  application should stop. */
typedef int (* control_write_fn) (struct control *ctrl, fd_set *wfds);


/** Perform internal processing.  For controls which interact with the system through
 *  queued messages, this is where they should dequeue and process incoming messages, and
 *  emit and enqueue outgoing messages.  Return 0 on success. */
typedef void (* control_process_fn) (struct control *control);

/** Called to enqueue a message for controls which want to dispatch enqueued messages to
 *  reduce latency.  If latency is not important, the control can skip this and
 *  control_enqueue() will simply enqueue the mbuf in the control's send.  Return <0 if
 *  the application should stop. */
typedef int (* control_enqueue_fn) (struct control *ctrl, struct mbuf *mbuf);

/** Dequeue a message from the control, for controls which may have more complex queues or
 *  need to reduce latency.  Return an mbuf, or NULL if queue is empty.  */
typedef struct mbuf * (* control_dequeue_fn) (struct control *ctrl);


/** Called to associate a particular control stream with a worker thread.
 *  should read from the descriptor.  Return <0 on error, but only if application should
 *  stop. */
typedef int (* control_connect_fn) (struct control *ctrl, int socket, struct worker *worker);


/** Close files/devices associated with this instance.  Return 0 on success. */
typedef void (* control_close_fn) (struct control *ctrl);

/** Free memory allocated for the instance. */
typedef void (* control_free_fn) (struct control *ctrl);


/** Operation pointers. */
struct control_ops
{
	control_alloc_fn       alloc_fn;
	config_func_t          config_fn;
	control_check_fn       check_fn;
	control_fd_set_fn      fd_set_fn;
	control_fd_is_set_fn   fd_is_set_fn;
	control_read_fn        read_fn;
	control_write_fn       write_fn;
	control_process_fn     process_fn;
	control_enqueue_fn     enqueue_fn;
	control_dequeue_fn     dequeue_fn;
	control_connect_fn     connect_fn;
	control_close_fn       close_fn;
	control_free_fn        free_fn;
};


/** Class registration maps a name to a set of operations.  The list is generated at build
 *  time by control/scan.sh and searchable with control_class_search().  All of this is
 *  internal to control_alloc() and should not be needed by the caller. */
struct control_class
{
	const char         *name;
	struct control_ops *ops;
};


/* Macro for linker-generated list */
#define CONTROL_CLASS(name, ops) \
	static struct control_class _control_class_ ## name \
	     __attribute__((unused,used,section("control_class_map"),aligned(sizeof(void *)))) \
	     = { #name, &ops };


/** Instance of a particular control.  The control may define a larger private struct
 *  which includes this as the first member, so the control pointer may be cast to the
 *  private pointer inside the control's code */
struct control
{
	const char                  *name;
	const struct control_class  *class;
	struct mqueue                recv;
	struct mqueue                send;
};


/** List of all controls allocated with control_alloc */
extern struct growlist control_list;


/** Allocate a new, idle instance of the class
 *
 *  \return Allocated control pointer, or NULL on failure
 */
struct control *control_alloc (const char *name);


/** Configure an instance
 *
 *  This will call config_parse() on the passed path to configure it, using the instance
 *  name as the section name and the class config_fn for matching items.  It is expected
 *  the class config_fn will call control_config_common() for items it's not handling
 *  specifically.
 *
 *  \param path Config file path
 *
 *  \return 0 on success, <0 on failure
 */
int control_config_inst (struct control *ctrl, const char *path);


/** Common configuration callback
 *
 *  This is a convenience function for individual controls' config handlers.  The intent
 *  is for controls' handlers to handle control-specific config items and return early.
 *  Unrecognized items are passed through to this common handler.
 *
 *  \return 0 on success, <0 on failure
 */
int control_config_common (const char *section, const char *tag, const char *val,
                           const char *file, int line, struct control *ctrl);


/** Check the instance post-configuration
 *
 *  \param ctrl Instance to be checked
 *
 *  \return 0 on success, <0 on failure
 */
static inline int control_check (struct control *ctrl)
{
	if ( !ctrl )
		return -1;

	if ( !ctrl->class->ops->check_fn )
		return 0;

	return ctrl->class->ops->check_fn(ctrl);
}


/** Add the instance's file handle(s) to an fd_set for a select() call
 *
 *  Calls FD_SET() for the instance's file handle(s), and updates *mfda if the instance's
 *  file handle(s) are higher than the current value.
 *
 *  \param ctrl Instance to add to the fd_set
 *  \param rfds An fd_set being setup for a select() call
 *  \param mfda Pointer to an int to be passed as arg 1 to select()
 */
static inline void control_fd_set (struct control *ctrl,
                                   fd_set *rfds, fd_set *wfds, int *mfda)
{
	if ( !ctrl || !ctrl->class->ops->fd_set_fn )
		return;

	ctrl->class->ops->fd_set_fn(ctrl, rfds, wfds, mfda);
}


/** Test the instance's file handle(s) against an fd_set after a select() call
 *
 *  \param ctrl Instance to test against the fd_set
 *  \param ret  Pointer to an int returned by the select()
 *  \param rfds A read fd_set previously passed to a select() call
 *  \param wfds A write fd_set previously passed to a select() call
 *
 *  \return Number of instance's file handle(s) set in the fd_set, or <0 on failure
 */
static inline int control_fd_is_set (struct control *ctrl, fd_set *rfds, fd_set *wfds)
{
	if ( !ctrl )
		return -1;

	if ( !ctrl->class->ops->fd_is_set_fn )
		return 0;

	return ctrl->class->ops->fd_is_set_fn(ctrl, rfds, wfds);
}


/** Poll a control indicated ready to read
 *
 *  \param ctrl Instance to be polled
 *  \param rfds A read fd_set previously passed to a select() call
 *
 *  \return 0 on success, <0 on failure
 */
static inline int control_read (struct control *ctrl, fd_set *rfds)
{
	if ( !ctrl )
		return -1;

	if ( !ctrl->class->ops->read_fn )
		return 0;

	return ctrl->class->ops->read_fn(ctrl, rfds);
}


/** Transmit queued messages from a control indicated ready to send
 *
 *  \param ctrl Instance to send message to
 *  \param ret  Pointer to an int returned by the select()
 *  \param wfds A write fd_set previously passed to a select() call
 *
 *  \return 0 on success, <0 on failure
 */
static inline int control_write (struct control *ctrl, fd_set *wfds)
{
	if ( !ctrl )
		return -1;

	if ( !ctrl->class->ops->write_fn )
		return 0;

	return ctrl->class->ops->write_fn(ctrl, wfds);
}


/** Internal processing
 *
 *  \param control  Instance to process
 */
static inline void control_process (struct control *ctrl)
{
	if ( !ctrl || !ctrl->class || !ctrl->class->ops || !ctrl->class->ops->process_fn )
		return;

	return ctrl->class->ops->process_fn(ctrl);
}


/** Connect/disconnect a socket to a worker
 *
 *  \param control  Control to connect
 *  \param socket   Socket within control
 *  \param worker   Worker to connect to, or NULL to disconnect
 */
static inline int control_connect (struct control *ctrl, int socket, struct worker *worker)
{
	if ( !ctrl || !ctrl->class || !ctrl->class->ops )
		return -1;

	if ( !ctrl->class->ops->connect_fn )
		return 0;

	return ctrl->class->ops->connect_fn(ctrl, socket, worker);
}


/** enqueue a message for transmit
 *
 *  \note Since the message may be submitted to multiple controls, individual controls may
 *        not modify the passed message; if the control needs to modify the message it
 *        should use mbuf_clone() to make a local copy.
 *
 *  \param ctrl Instance to send message via
 *  \param mbuf Message buffer to send
 *
 *  \return 0 on success, <0 on failure
 */
int control_enqueue (struct control *ctrl, struct mbuf *mbuf);


/** dequeue a message emitted.  Pass mbuf as a pointer to an mbuf pointer, the
 *  
 *  \param ctrl Instance to dequeue message from
 *
 *  \return Pointer to mbuf pointer, value will be NULL if the queue is empty.
 */
struct mbuf *control_dequeue (struct control *ctrl);


/** Close the instance's file handles, yielding the source device
 *
 *  After this call, the control's structures and buffers remain in memory, and the
 *  control can be reopened with a call to control_check().  
 *
 *  \param ctrl  Instance to close
 */
static inline void control_close (struct control *ctrl)
{
	if ( !ctrl || !ctrl->class->ops->close_fn )
		return;

	ctrl->class->ops->close_fn(ctrl);
}


/** Free the instance's memory structures
 *
 *  \param ctrl  Instance to free
 */
void control_free (struct control *ctrl);


static inline const char *control_name (struct control *ctrl)
{
	if ( !ctrl || !ctrl->name )
		return "(null)";
	else
		return ctrl->name;
}


#endif /* INCLUDE_DAEMON_CONTROL_H */
/* Ends */
