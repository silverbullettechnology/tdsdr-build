/** \file      src/app/asfe_ctl/map.c
 *  \brief     Generic command mapping code
 *  \copyright Copyright 2013,2014 Silver Bullet Technology
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
 * vim:ts=4:noexpandtab
 */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <asfe_ctl.h>

#include "map.h"
#include "main.h"
#include "util.h"
#include "parse.h"
#include "format.h"


#ifdef ASFE_CTL_USE_READLINE
#include <readline/readline.h>
#endif


extern struct asfe_ctl_map_cmd_t __start_asfe_ctl_map_cmd;
extern struct asfe_ctl_map_cmd_t  __stop_asfe_ctl_map_cmd;

struct asfe_ctl_map_cmd_t *asfe_ctl_map_find (const char *arg0)
{
	struct asfe_ctl_map_cmd_t *map;

	for ( map = &__start_asfe_ctl_map_cmd; map != &__stop_asfe_ctl_map_cmd; map++ )
		if ( !strcasecmp(arg0, map->arg0) )
			return map;

	errno = ENOENT;
	return NULL;
}

int asfe_ctl_map_call (struct asfe_ctl_map_cmd_t *map, int argc, const char **argv)
{
	if ( argc < map->argc )
	{
		fprintf(stderr, "%s requires %d args, %d given\n",
		        map->arg0, map->argc - 1, argc - 1);
		errno = EINVAL;
		return -1;
	}

	return map->func(argc, argv);
}


extern struct asfe_ctl_map_arg_t __start_asfe_ctl_map_arg;
extern struct asfe_ctl_map_arg_t  __stop_asfe_ctl_map_arg;

static int asfe_ctl_map_help_cmp (const void *a, const void *b)
{
	int ret = strcmp(((struct asfe_ctl_map_arg_t *)a)->func,
	                 ((struct asfe_ctl_map_arg_t *)b)->func);

	if ( ret )
		return ret;

	return ((struct asfe_ctl_map_arg_t *)a)->idx - ((struct asfe_ctl_map_arg_t *)b)->idx;
}

void asfe_ctl_map_help (const char *name)
{
	static int asfe_ctl_map_help_sorted = 0;
	if ( !asfe_ctl_map_help_sorted )
	{
		asfe_ctl_map_help_sorted = 1;
		qsort(&__start_asfe_ctl_map_arg, &__stop_asfe_ctl_map_arg - &__start_asfe_ctl_map_arg,
		      sizeof(struct asfe_ctl_map_arg_t), asfe_ctl_map_help_cmp);
	}

	struct asfe_ctl_map_arg_t *arg;
	struct asfe_ctl_map_arg_t *beg = NULL;
	for ( arg = &__start_asfe_ctl_map_arg; arg != &__stop_asfe_ctl_map_arg; arg++ )
		if ( !strcmp(arg->func, name) )
		{
			beg = arg;
			break;
		}

	if ( !beg )
	{
		fprintf(stderr, "No help available\n");
		return;
	}

	int  len;
	int  type_len = -1;
	int  name_len = -1;
	for ( arg = beg; arg != &__stop_asfe_ctl_map_arg; arg++ )
		if ( !strcmp(arg->func, name) )
		{
			if ( (len = strlen(arg->type)) > type_len )  type_len = len;
			if ( (len = strlen(arg->name)) > name_len )  name_len = len;
		}
		else
			break;

	for ( arg = beg; arg != &__stop_asfe_ctl_map_arg; arg++ )
		if ( !strcmp(arg->func, name) )
			fprintf(stderr, "%3d: %-*s  %-*s  %s\n", arg->idx, type_len,
			       arg->type, name_len, arg->name, arg->desc);
		else
			break;
}


#ifdef ASFE_CTL_USE_READLINE
static char *asfe_ctl_map_generate (const char *text, int state)
{
	static struct asfe_ctl_map_cmd_t *map;
	static int                      len;
	const char                     *name;

	if ( !state )
	{
		map = &__start_asfe_ctl_map_cmd;
		len = strlen(text);
	}

	name = map->arg0;
	while ( map != &__stop_asfe_ctl_map_cmd )
	{
		map++;

		if ( !strncasecmp(name, text, len) )
			return strdup(name);

		name = map->arg0;
	}

	return NULL;
}

char **asfe_ctl_map_completion (const char *text, int start, int end)
{
	if ( start )
		return NULL;

	return rl_completion_matches(text, asfe_ctl_map_generate);
}
#endif


