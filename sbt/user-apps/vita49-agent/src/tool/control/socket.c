/** Socket abstraction code
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
#include <tool/control/socket.h>


LOG_MODULE_STATIC("socket", LOG_LEVEL_INFO);


struct socket *sock = NULL;


/** Linker-generated symbols for the map */
extern struct socket_class __start_socket_class_map;
extern struct socket_class  __stop_socket_class_map;

/** Find a registered static class by name
 *
 *  \note Implemented in build-generated src/socket/socket-list.c
 *
 *  \param name Name to match class on
 *
 *  \return Static class pointer, or NULL on failure
 */
const struct socket_class *socket_class_search (const char *name)
{
	const struct socket_class *map;

	errno = 0;
	for ( map = &__start_socket_class_map; map != &__stop_socket_class_map; map++ )
		if ( !strcasecmp(map->name, name) )
			return map;

	errno = ENOENT;
	return NULL;
}


/** Allocate a new, idle instance of the class
 *
 *  \return Allocated socket pointer, or NULL on failure
 */
struct socket *socket_alloc (const char *name)
{
	ENTER("name %s", name);

	const struct socket_class *ac = socket_class_search(name);
	if ( !ac )
		RETURN_VALUE("%p", NULL);

	if ( !ac->ops )
		RETURN_ERRNO_VALUE(ENOSYS, "%p", NULL);

	// alloc_fn is optional for simple modules, use calloc() if undefined
	struct socket *ai;
	errno = 0;
	if ( ac->ops->alloc_fn )
		ai = ac->ops->alloc_fn();
	else
		ai = calloc(1, sizeof(struct socket));
	if ( !ai )
		RETURN_VALUE("%p", NULL);

	ai->class = ac;
	ai->name = strdup(name);

	mqueue_init(&ai->queue, 0);

	RETURN_ERRNO_VALUE(0, "%p", ai);
}


/** Configure an allocated instance from a configuration file
 *
 *  \param sock  Instance to be configured
 *  \param path  Config-file path for config_parse();
 *
 *  \return 0 on success, <0 on failure
 */
int socket_config_inst (struct socket *sock, const char *path)
{
	ENTER("sock %p, path %p", sock, path);
	if ( !sock )             RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	if ( !sock->class )      RETURN_ERRNO_VALUE(ENOSYS, "%d", -1);
	if ( !sock->class->ops ) RETURN_ERRNO_VALUE(ENOSYS, "%d", -1);

	/* If the class has a libconfig callback, use that */
	if ( sock->class->ops->config_fn )
	{
		struct config_context cc =
		{
			.default_section   = NULL,
			.default_function  = NULL,
			.catchall_function = sock->class->ops->config_fn,
			.section_map       = NULL,
			.data              = sock
		};

		errno = 0;
		LOG_DEBUG("Parse config %s for section %s\n", path, sock->name);
		int ret = config_parse (&cc, path);
		RETURN_VALUE("%d", ret);
	}

	/* If neither are supplied, return a not-implemented errno, but don't fail */
	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);
}


/** Common configuration callback
 *
 *  This is a convenience function for individual sockets' config handlers.  The intent
 *  is for sockets' handlers to handle socket-specific config items and return early.
 *  Unrecognized items are passed through to this common handler.
 *
 *  \return 0 on success, <0 on failure
 */
int socket_config_common (const char *section, const char *tag, const char *val,
                         const char *file, int line, struct socket *sock)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, sock %p",
	      section, tag, val, file, line, sock);

	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);

	RETURN_ERRNO_VALUE(EINVAL, "%d", -1);
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
int socket_enqueue (struct socket *sock, struct mbuf *mbuf)
{
	ENTER("sock %p, mbuf %p", sock, mbuf);
	if ( !sock || !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	int ret;
	if ( sock->class->ops->enqueue_fn )
	{
		ret = sock->class->ops->enqueue_fn(sock, mbuf);
		RETURN_VALUE("%d", ret);
	}

	if ( (ret = mqueue_enqueue(&sock->queue, mbuf)) < 0 )
		RETURN_VALUE("%d", ret);

	RETURN_ERRNO_VALUE(0, "%d", ret);
}


/** Free the instance's memory structures
 *
 *  \param sock  Instance to free
 */
void socket_free (struct socket *sock)
{
	ENTER("sock %p", sock);
	if ( !sock )
		RETURN_ERRNO(EFAULT);
	
	free((char *)sock->name);

	// free_fn is optional for simple modules, use free() if undefined
	if ( sock->class->ops->free_fn )
		sock->class->ops->free_fn(sock);
	else
		free(sock);

	RETURN;
}

