/** Control abstraction code
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

#include <sbt_common/log.h>
#include <sbt_common/growlist.h>
#include <daemon/control.h>


LOG_MODULE_STATIC("control", LOG_LEVEL_WARN);


/** List of all controls allocated with control_alloc */
struct growlist control_list = GROWLIST_INIT;


/** Linker-generated symbols for the map */
extern struct control_class __start_control_class_map;
extern struct control_class  __stop_control_class_map;

/** Find a registered static class by name
 *
 *  \note Implemented in build-generated src/control/control-list.c
 *
 *  \param name Name to match class on
 *
 *  \return Static class pointer, or NULL on failure
 */
const struct control_class *control_class_search (const char *name)
{
	const struct control_class *map;

	errno = 0;
	for ( map = &__start_control_class_map; map != &__stop_control_class_map; map++ )
		if ( !strcasecmp(map->name, name) )
			return map;

	errno = ENOENT;
	return NULL;
}


/** Allocate a new, idle instance of the class
 *
 *  \return Allocated control pointer, or NULL on failure
 */
struct control *control_alloc (const char *name)
{
	ENTER("name %s", name);

	const struct control_class *ac = control_class_search(name);
	if ( !ac )
		RETURN_VALUE("%p", NULL);

	if ( !ac->ops )
		RETURN_ERRNO_VALUE(ENOSYS, "%p", NULL);

	// alloc_fn is optional for simple modules, use calloc() if undefined
	struct control *ai;
	errno = 0;
	if ( ac->ops->alloc_fn )
		ai = ac->ops->alloc_fn();
	else
		ai = calloc(1, sizeof(struct control));
	if ( !ai )
		RETURN_VALUE("%p", NULL);

	ai->class = ac;
	if ( growlist_append(&control_list, ai) < 0 )
	{
		int err = errno;
		if ( ac->ops->free_fn )
			ac->ops->free_fn(ai);
		else
			free(ai);
		RETURN_ERRNO_VALUE(err, "%p", NULL);
	}

	mqueue_init(&ai->send, 0);
	mqueue_init(&ai->recv, 0);

	RETURN_ERRNO_VALUE(0, "%p", ai);
}


/** Configure an allocated instance from a configuration file
 *
 *  \param ctrl  Instance to be configured
 *  \param path  Config-file path for config_parse();
 *
 *  \return 0 on success, <0 on failure
 */
int control_config_inst (struct control *ctrl, const char *path)
{
	ENTER("ctrl %p, path %p", ctrl, path);
	if ( !ctrl )             RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	if ( !ctrl->class )      RETURN_ERRNO_VALUE(ENOSYS, "%d", -1);
	if ( !ctrl->class->ops ) RETURN_ERRNO_VALUE(ENOSYS, "%d", -1);

	/* If the class has a libconfig callback, use that */
	if ( ctrl->class->ops->config_fn )
	{
		struct config_section_map cm[] =
		{
			{ (char *)ctrl->name, ctrl->class->ops->config_fn, ctrl },
			{ NULL }
		};
		struct config_context cc =
		{
			.default_section   = NULL,
			.default_function  = NULL,
			.catchall_function = NULL,
			.section_map       = cm,
			.data              = ctrl
		};

		errno = 0;
		LOG_DEBUG("Parse config %s for section %s\n", path, ctrl->name);
		int ret = config_parse (&cc, path);
		RETURN_VALUE("%d", ret);
	}

	/* If neither are supplied, return a not-implemented errno, but don't fail */
	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);
}


/** Common configuration callback
 *
 *  This is a convenience function for individual controls' config handlers.  The intent
 *  is for controls' handlers to handle control-specific config items and return early.
 *  Unrecognized items are passed through to this common handler.
 *
 *  \return 0 on success, <0 on failure
 */
int control_config_common (const char *section, const char *tag, const char *val,
                         const char *file, int line, struct control *ctrl)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, ctrl %p",
	      section, tag, val, file, line, ctrl);

	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);

	RETURN_ERRNO_VALUE(EINVAL, "%d", -1);
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
int control_enqueue (struct control *ctrl, struct mbuf *mbuf)
{
	ENTER("ctrl %p, mbuf %p", ctrl, mbuf);
	if ( !ctrl || !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	int ret;
	if ( ctrl->class->ops->enqueue_fn )
	{
		ret = ctrl->class->ops->enqueue_fn(ctrl, mbuf);
		RETURN_VALUE("%d", ret);
	}

	if ( (ret = mqueue_enqueue(&ctrl->send, mbuf)) < 0 )
		RETURN_VALUE("%d", ret);

	RETURN_ERRNO_VALUE(0, "%d", ret);
}


/** dequeue a message emitted.  Pass mbuf as a pointer to an mbuf pointer, the
 *  
 *  \param ctrl Instance to dequeue message from
 *
 *  \return Pointer to mbuf pointer, value will be NULL if the queue is empty.
 */
struct mbuf *control_dequeue (struct control *ctrl)
{
	ENTER("ctrl %p", ctrl);
	if ( !ctrl )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	struct mbuf *mbuf;
	if ( ctrl->class->ops->dequeue_fn )
	{
		mbuf = ctrl->class->ops->dequeue_fn(ctrl);
		RETURN_VALUE("%p", mbuf);
	}

	mbuf = mqueue_dequeue(&ctrl->recv);

	RETURN_ERRNO_VALUE(0, "%p", mbuf);
}


/** Free the instance's memory structures
 *
 *  \param ctrl  Instance to free
 */
void control_free (struct control *ctrl)
{
	ENTER("ctrl %p", ctrl);
	if ( !ctrl )
		RETURN_ERRNO(EFAULT);

	growlist_remove(&control_list, ctrl);

	// free_fn is optional for simple modules, use free() if undefined
	if ( ctrl->class->ops->free_fn )
		ctrl->class->ops->free_fn(ctrl);
	else
		free(ctrl);

	RETURN;
}



/* Ends    : src/control/control.c */
