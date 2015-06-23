/** Worker abtraction code
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
#include <sys/select.h>
#include <errno.h>

#include <sbt_common/log.h>
#include <sbt_common/growlist.h>
#include <daemon/worker.h>


LOG_MODULE_STATIC("worker", LOG_LEVEL_WARN);


char *worker_exec_path;


/** List of all workers allocated with worker_alloc */
struct growlist worker_list = GROWLIST_INIT;


/** Linker-generated symbols for the map */
extern struct worker_class __start_worker_class_map;
extern struct worker_class  __stop_worker_class_map;

/** Find a registered static class by name
 *
 *  \param name Name to match class on
 *
 *  \return Static class pointer, or NULL on failure
 */
const struct worker_class *worker_class_search (const char *name)
{
	const struct worker_class *map;

	errno = 0;
	for ( map = &__start_worker_class_map; map != &__stop_worker_class_map; map++ )
		if ( !strcasecmp(map->name, name) )
			return map;

	errno = ENOENT;
	return NULL;
}


/** Allocate a new, idle instance of the class
 *
 *  \return Allocated worker pointer, or NULL on failure
 */
struct worker *worker_alloc (const char *name, unsigned sid)
{
	ENTER("name %s", name);

	const struct worker_class *dc = worker_class_search(name);
	if ( !dc )
		RETURN_VALUE("%p", NULL);

	if ( !dc->ops || !dc->ops->alloc_fn )
		RETURN_ERRNO_VALUE(ENOSYS, "%p", NULL);

	errno = 0;
	struct worker *di = dc->ops->alloc_fn(sid);
	if ( !di )
		RETURN_VALUE("%p", NULL);

	di->class = dc;
	if ( growlist_append(&worker_list, di) < 0 )
	{
		int err = errno;
		if ( dc->ops->free_fn )
			dc->ops->free_fn(di);
		else
			free(di);
		RETURN_ERRNO_VALUE(err, "%p", NULL);
	}

	mqueue_init(&di->send, 0);
	mqueue_init(&di->recv, 0);
	di->state = WS_CONFIG;
	di->flags = WF_AUTO_START | WF_RESTART_CLEAN | WF_RESTART_ERROR;

	RETURN_ERRNO_VALUE(0, "%p", di);
}


/** Configure an allocated instance from a configuration file
 *
 *  \param inst  Instance to be configured
 *  \param path  Config-file path for config_parse();
 *
 *  \return 0 on success, <0 on failure
 */
int worker_config_inst (struct worker *inst, const char *path)
{
	ENTER("inst %p, path %p", inst, path);
	if ( !inst )                RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	if ( !inst->class )         RETURN_ERRNO_VALUE(ENOSYS, "%d", -1);
	if ( !inst->class->ops ) RETURN_ERRNO_VALUE(ENOSYS, "%d", -1);

	/* If the class has a libconfig callback, use that */
	if ( inst->class->ops->config_fn )
	{
		struct config_section_map cm[] =
		{
			{ (char *)inst->name, inst->class->ops->config_fn, inst },
			{ NULL }
		};
		struct config_context cc =
		{
			.default_section   = NULL,
			.default_function  = NULL,
			.catchall_function = NULL,
			.section_map       = cm,
			.data              = inst
		};

		errno = 0;
		LOG_DEBUG("Parse config %s for section %s\n", path, inst->name);
		int ret = config_parse (&cc, path);
		RETURN_VALUE("%d", ret);
	}

	/* If neither are supplied, return a not-implemented errno, but don't fail */
	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);
}



/** Common configuration callback
 *
 *  This is a convenience function for individual workers' config handlers.  The intent
 *  is for workers' handlers to handle worker-specific config items and return early.
 *  Unrecognized items are passed through to this common handler.
 *
 *  \return 0 on success, <0 on failure
 */
int worker_config_common (const char *section, const char *tag, const char *val,
                       const char *file, int line, struct worker *inst)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, inst %p",
	      section, tag, val, file, line, inst);

	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);

	RETURN_ERRNO_VALUE(EINVAL, "%d", -1);
}


/** enqueue a message for transmit
 *
 *  \note Since the message may be submitted to multiple workers, individual workers may
 *        not modify the passed message; if the worker needs to modify the message it
 *        should use mbuf_clone() to make a local copy.
 *
 *  \param worker Instance to send message via
 *  \param mbuf Message buffer to send
 *
 *  \return 0 on success, <0 on failure
 */
int worker_enqueue (struct worker *worker, struct mbuf *mbuf)
{
	ENTER("worker %p, mbuf %p", worker, mbuf);
	if ( !worker || !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	
	int ret;
	if ( worker->class->ops->enqueue_fn )
	{
		ret = worker->class->ops->enqueue_fn(worker, mbuf);
		RETURN_VALUE("%d", ret);
	}

	if ( (ret = mqueue_enqueue(&worker->send, mbuf)) < 0 )
		RETURN_VALUE("%d", ret);

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** dequeue a message emitted.  Pass mbuf as a pointer to an mbuf pointer, the
 *  
 *  \param worker Instance to dequeue message from
 *
 *  \return Pointer to mbuf pointer, value will be NULL if the queue is empty.
 */
struct mbuf *worker_dequeue (struct worker *worker)
{
	ENTER("worker %p", worker);
	if ( !worker )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);
	
	struct mbuf *mbuf;
	if ( worker->class->ops->dequeue_fn )
	{
		mbuf = worker->class->ops->dequeue_fn(worker);
		RETURN_VALUE("%p", mbuf);
	}

	mbuf = mqueue_dequeue(&worker->recv);

	RETURN_ERRNO_VALUE(0, "%p", mbuf);
}


/** Free the instance's memory structures
 *
 *  \param inst  Instance to free
 */
void worker_free (struct worker *inst)
{
	ENTER("inst %p", inst);
	if ( !inst )
		RETURN_ERRNO(EFAULT);

	growlist_remove(&worker_list, inst);

	// free_fn is optional for simple modules, use free() if undefined
	if ( inst->class->ops->free_fn )
		inst->class->ops->free_fn(inst);
	else
		free(inst);

	RETURN;
}


/** Return a safe string for a state
 *
 *  \param state worker_state_t value
 *
 *  \return ASCII string for log messages, or "???" for invalid state values
 */
const char *worker_state_desc (worker_state_t state)
{
	switch ( state )
	{
		case WS_CONFIG: return("CONFIG");
		case WS_READY:  return("READY");
		case WS_START:  return("START");
		case WS_NORMAL: return("NORMAL");
		case WS_STOP:   return("STOP");
		case WS_LIMIT:  return("LIMIT");
		default:        return("???");
	}
}


/** Rate limiting for _START transitions
 *
 *  \param worker worker instance
 *  \param period time in seconds
 *  \param count  max number of _START attempts
 *
 *  \return 0 if worker can start, !0 if worker is rate-limited
 */
int worker_limit (struct worker *worker, int period, int count)
{
	ENTER("worker %p -> %s", worker, worker_name(worker));

	// never been set: not limited yet
	if ( ! worker->clocks )
	{
		worker->clocks = get_clocks();
		worker->starts = 1;
		LOG_DEBUG("%s: limit init, clocks %llu, starts 1\n", 
		          worker_name(worker), worker->clocks);
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	// otherwise get 
	clocks_t now = get_clocks();
	clocks_t dly = now;
	dly -= worker->clocks;

	// already limited: check for timeout, reset if long enough
	if ( worker->state == WS_LIMIT )
	{
		if ( clocks_to_s(dly) < period )
		{
			LOG_DEBUG("%s: limited, dly %llu, wait %u sec\n",
			          worker_name(worker), dly, clocks_to_s(dly));
			RETURN_ERRNO_VALUE(0, "%d", 1);
		}

		worker->clocks = now;
		worker->starts = 1;
		LOG_DEBUG("%s: limit expired, clear\n", worker_name(worker));
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	// not yet limited: if counter under threshold allow start
	if ( worker->starts < count )
	{
		worker->starts++;
		LOG_DEBUG("%s: starts < count, starts++ now %d\n",
		          worker_name(worker), worker->starts);
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	// over count but period elapsed: reset and allow start
	if ( clocks_to_s(dly) >= period )
	{
		worker->clocks = now;
		worker->starts = 1;
		LOG_DEBUG("%s: delay expired, reset and clear\n", worker_name(worker));
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	// over count and period not yet elapsed: deny start
	LOG_DEBUG("%s: still limited\n", worker_name(worker));
	RETURN_ERRNO_VALUE(0, "%d", 1);
}


/* Ends    : src/daemon/worker.c */
