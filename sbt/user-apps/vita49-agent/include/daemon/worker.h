/** Worker class abstraction
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
#ifndef INCLUDE_DAEMON_WORKER_H
#define INCLUDE_DAEMON_WORKER_H
#include <sys/select.h>

#include <lib/growlist.h>
#include <lib/descript.h>
#include <lib/clocks.h>
#include <lib/mqueue.h>
#include <lib/mbuf.h>
#include <lib/uuid.h>

#include <config/include/config.h>


extern char *worker_exec_path;


#define  WF_AUTO_START     (1 << 0)
#define  WF_RESTART_CLEAN  (1 << 1)
#define  WF_RESTART_ERROR  (1 << 2)


typedef enum
{
	WS_CONFIG,  /** Worker being configured, goes to _READY                          */
	WS_READY,   /** Worker ready to (re)start (see WF_AUTO_START), may go to _START  */
	WS_START,   /** Worker is starting (can _set manually), goes to _RUN or _READY   */
	WS_NORMAL,  /** Worker is running, goes to _STOP or _ERROR                       */
	WS_STOP,    /** Worker is stopping (can _set manually), goes to _READY           */
	WS_LIMIT,   /** Worker is waiting for rate-limited timer, goes to _READY         */
	WS_ZOMBIE,  /** Worker has exited, waiting for de-allocation                     */
	WS_MAX      /** For sizing arrays, never returned                                */
}
worker_state_t;


/** Allocate a worker instance, usually a private which starts with a worker struct for
 *  casting.  The function should zero the memory, and is responsible for allocating and
 *  setting to default values any fields within the private structure.  It should not
 *  initialize any of the fields in the ancestor worker; worker_alloc() does that. */
typedef struct worker * (* worker_alloc_fn) (unsigned sid);

/** Command-line configuration of a worker, usually instead of a config-file parse.
 *  Return nonzero if the passed argument is invalid. */
typedef int (* worker_cmdline_fn) (struct worker *worker,
                                   const char *tag, const char *val);


/** Get state of a worker.  May be used for logging messages if the auto (re)start flags
 *  are set, or a management thread.  Return <0 on fatal error, if application should
 *  stop.  Otherwise should implement rate limiting and return current state. */
typedef worker_state_t (* worker_state_get_fn) (struct worker *worker);

/** Set status of a worker.  Should return the new state, or <0 if the application should
 * stop. */
typedef worker_state_t (* worker_state_set_fn) (struct worker *worker,
                                                worker_state_t state);


/** Called each main loop to register the worker's read/write file descriptors in a pair
 *  of fd_sets supplied by the caller, and updates *nfds if necessary.  Note: workers
 *  which don't use select()able file descriptors can use this to poll internally as long
 *  as they don't block. */
typedef void (* worker_fd_set_fn) (struct worker *worker,
                                   fd_set *rfds, fd_set *wfds, int *nfds);

/** Called each main loop after a select(); if rfds is passed check the read descriptors;
 *  if wfds is passed check the write descriptors.  Return the number of descriptors which
 *  need to be handled, 0 if none are needed, <0 if the application should stop.
 *  Note: workers which don't use select()able file descriptors can use this to indicate
 *  from some other check that they're ready to be polled. */
typedef int (* worker_fd_is_set_fn) (struct worker *worker, fd_set *rfds, fd_set *wfds);

/** Read available data from worker, process, validate, and update internal state.  Return
 * <0 if application should stop */
typedef int (* worker_read_fn) (struct worker *worker, fd_set *rfds);

/** Send queued data to worker.  Return <0 if application should stop */
typedef int (* worker_write_fn) (struct worker *worker, fd_set *wfds);


/** Perform internal processing.  For workers which interact with the system through
 *  queued messages, this is where they should dequeue and process incoming messages, and
 *  emit and enqueue outgoing messages.  Return 0 on success. */
typedef void (* worker_process_fn) (struct worker *worker);

/** Called to enqueue a message for workers which want to dispatch enqueued messages to
 *  reduce latency.  If latency is not important, the worker can skip this and
 *  worker_enqueue() will simply enqueue the mbuf in the worker's send.  Return <0 if
 *  the application should stop. */
typedef int (* worker_enqueue_fn) (struct worker *worker, struct mbuf *mbuf);

/** Dequeue a message from the worker, for workers which may have more complex queues or
 *  need to reduce latency.  Return an mbuf, or NULL if queue is empty.  */
typedef struct mbuf * (* worker_dequeue_fn) (struct worker *worker);


/** Close files/worker associated with this instance.  Return 0 on success. */
typedef int (* worker_close_fn) (struct worker *worker);

/** Free memory allocated for the instance. */
typedef void (* worker_free_fn) (struct worker *worker);


/* Operation pointers */
struct worker_ops
{
	worker_alloc_fn      alloc_fn;
	worker_cmdline_fn    cmdline_fn;
	config_func_t        config_fn;
	worker_state_get_fn  state_get_fn;
	worker_state_set_fn  state_set_fn;
	worker_fd_set_fn     fd_set_fn;
	worker_fd_is_set_fn  fd_is_set_fn;
	worker_read_fn       read_fn;
	worker_write_fn      write_fn;
	worker_process_fn    process_fn;
	worker_enqueue_fn    enqueue_fn;
	worker_dequeue_fn    dequeue_fn;
	worker_close_fn      close_fn;
	worker_free_fn       free_fn;
};


/** Class registration maps a name to a set of operations.  */
struct worker_class
{
	const char        *name;
	struct worker_ops *ops;
};

#define WORKER_CLASS(name, ops) \
	static const struct worker_class _worker_class_ ## name \
	     __attribute__((unused,used,section("worker_class_map"),aligned(sizeof(void *)))) \
	     = { #name, &ops }


/** Instance of a particular worker.  The worker may define a larger private struct which
 *  includes this as the first member, so the worker pointer may be cast to the private
 *  pointer inside the worker's code */
struct worker
{
	const char                 *name;
	const struct worker_class  *class;
	struct control             *control;
	int                         socket;
	unsigned                    sid;
	unsigned long               flags;
	worker_state_t              state;
	int                         starts;
	struct mqueue               recv;
	struct mqueue               send;
	clocks_t                    clocks;
	uuid_t                      cid;
	uuid_t                      rid;
	struct resource_info       *res;
};


/** List of all workers allocated with worker_alloc */
extern struct growlist worker_list;


/** Allocate a new, idle instance of the class
 *
 *  \return Allocated worker pointer, or NULL on failure
 */
struct worker *worker_alloc (const char *name, unsigned sid);


/** Configure an instance
 *
 *  This will call config_parse() on the passed path to configure it, using the instance
 *  name as the section name and the class config_fn for matching items.  It is expected
 *  the class config_fn will call worker_config_common() for items it's not handling
 *  specifically.
 *
 *  \param worker Instance to configure
 *  \param path Config file path
 *
 *  \return 0 on success, <0 on failure
 */
int worker_config_inst (struct worker *worker, const char *path);


/** Common configuration callback
 *
 *  This is a convenience function for individual workers' config handlers.  The intent
 *  is for workers' handlers to handle worker-specific config items and return early.
 *  Unrecognized items are passed through to this common handler.
 *
 *  \return 0 on success, <0 on failure
 */
int worker_config_common (const char *section, const char *tag, const char *val,
                          const char *file, int line, struct worker *worker);


/** Command-line config of a worker
 *
 *  \param worker Instance to be configured
 *  \param tag  Argument name
 *  \param val  Argument value
 *
 *  \return 0 on success, <0 on failure
 */
static inline int worker_cmdline (struct worker *worker, const char *tag, const char *val)
{
	if ( !worker )
		return -1;

	if ( !worker->class->ops->cmdline_fn )
		return 0;

	return worker->class->ops->cmdline_fn(worker, tag, val);
}


/** Get worker state
 *
 *  \param worker Instance to check
 *
 *  \return 0 on success, <0 on failure
 */
static inline worker_state_t worker_state_get (struct worker *worker)
{
	if ( !worker || !worker->class || !worker->class->ops )
		return -1;

	if ( !worker->class->ops->state_get_fn )
		return worker->state;

	return worker->class->ops->state_get_fn(worker);
}


/** Set worker state
 *
 *  \param worker Instance to change
 *  \param state  New state to move to
 *
 *  \return New state on success, <0 on failure
 */
static inline worker_state_t worker_state_set (struct worker *worker,
                                               worker_state_t state)
{
	if ( !worker || !worker->class || !worker->class->ops )
		return -1;

	if ( !worker->class->ops->state_set_fn )
	{
		worker->state = state;
		return worker_state_get(worker);
	}

	return worker->class->ops->state_set_fn(worker, state);
}


/** Add the instance's file handle(s) to the fd_set(s) for a select() call
 *
 *  Calls FD_SET() for the instance's file handle(s), and updates *nfds if the instance's
 *  file handle(s) are higher than the current value.
 *
 *  \param worker Instance to add to the fd_sets
 *  \param rfds An fd_set being setup for reads with a select() call
 *  \param wfds An fd_set being setup for writes with a select() call
 *  \param nfds Pointer to an int to be passed as arg 1 to select()
 */
static inline void worker_fd_set (struct worker *worker,
                                  fd_set *rfds, fd_set *wfds, int *nfds)
{
	if ( !worker || !worker->class || !worker->class->ops || !worker->class->ops->fd_set_fn )
		return;

	worker->class->ops->fd_set_fn(worker, rfds, wfds, nfds);
}


/** Test the instance's file handle(s) against a read fd_set after a select() call
 *
 *  \param worker Instance to test against the fd_set
 *  \param rfds A read fd_set previously passed to a select() call
 *  \param rfds A write fd_set previously passed to a select() call
 *
 *  \return Number of instance's file handle(s) set in the fd_set, or <0 on failure
 */
static inline int worker_fd_is_set (struct worker *worker, fd_set *rfds, fd_set *wfds)
{
	if ( !worker || !worker->class || !worker->class->ops )
		return -1;

	if ( !worker->class->ops->fd_is_set_fn )
		return 0;

	return worker->class->ops->fd_is_set_fn(worker, rfds, wfds);
}


/** Read data from a worker's read file descriptor
 *
 *  \param worker  Instance to read
 */
static inline int worker_read (struct worker *worker, fd_set *rfds)
{
	if ( !worker || !worker->class || !worker->class->ops )
		return -1;

	if ( !worker->class->ops->read_fn )
		return 0;

	return worker->class->ops->read_fn(worker, rfds);
}


/** Write queued data to a worker
 *
 *  \param worker  Instance to write
 */
static inline int worker_write (struct worker *worker, fd_set *wfds)
{
	if ( !worker || !worker->class || !worker->class->ops )
		return -1;

	if ( !worker->class->ops->write_fn )
		return 0;

	return worker->class->ops->write_fn(worker, wfds);
}


/** Internal processing
 *
 *  \param worker  Instance to process
 */
static inline void worker_process (struct worker *worker)
{
	if ( !worker || !worker->class || !worker->class->ops || !worker->class->ops->process_fn )
		return;

	return worker->class->ops->process_fn(worker);
}


/** enqueue a message for transmit
 *
 *  \note Since the message may be submitted to multiple workers, individual workers may
 *        not modify the passed message; if the worker needs to modify the message it
 *        should use mbuf_clone() to make a local copy.
 *
 *  \param ctrl Instance to send message via
 *  \param mbuf Message buffer to send
 *
 *  \return 0 on success, <0 on failure
 */
int worker_enqueue (struct worker *ctrl, struct mbuf *mbuf);


/** dequeue a message emitted.  Pass mbuf as a pointer to an mbuf pointer, the
 *  
 *  \param ctrl Instance to dequeue message from
 *
 *  \return Pointer to mbuf pointer, value will be NULL if the queue is empty.
 */
struct mbuf *worker_dequeue (struct worker *ctrl);


/** Close the instance's file handles, yielding the source worker
 *
 *  \param worker  Instance to close
 */
static inline int worker_close (struct worker *worker)
{
	if ( !worker || !worker->class || !worker->class->ops )
		return -1;

	if ( !worker->class->ops->close_fn )
		return 0;

	return worker->class->ops->close_fn(worker);
}


/** free the instance's memory structures
 *
 *  \param worker  instance to free
 */
void worker_free (struct worker *worker);



/** Search functions for growlist_search(), for finding worker instances by name or class
 */
static inline int worker_cmp_name (const void *a, const void *b)
{
	return strcmp ( ((const struct worker*)a)->name, (const char *)b );
}
static inline int worker_cmp_class (const void *a, const void *b)
{
	return strcmp ( ((const struct worker*)a)->class->name, (const char *)b );
}


/** Return a safe string for worker
 *
 *  \param worker worker instance
 *
 *  \return ASCII string for log messages, or "(null)" 
 */
static inline const char *worker_name (struct worker *worker)
{
	if ( !worker || !worker->name )
		return "(null)";
	else
		return worker->name;
}


/** Return a safe string for a state
 *
 *  \param state worker_state_t value
 *
 *  \return ASCII string for log messages, or "???" for invalid state values
 */
const char *worker_state_desc (worker_state_t state);


/** Rate limiting for _START transitions
 *
 *  \param worker worker instance
 *  \param period time in seconds
 *  \param count  max number of _START attempts
 *
 *  \return 0 if worker can start, !0 if worker is rate-limited
 */
int worker_limit (struct worker *worker, int period, int count);


#endif /* INCLUDE_DAEMON_WORKER_H */
/* Ends */
