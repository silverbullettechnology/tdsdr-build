/** \file      src/app/ad9361/map.c
 *  \brief     
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
#include <stdlib.h>
#include <errno.h>

#include <ad9361.h>

#include "map.h"
#include "main.h"
//#include "util.h"
//#include "trx_app.h"
//#include "parse.h"
//#include "format.h"





#ifdef AD9361_USE_READLINE 
#include <readline/readline.h>
#endif


extern struct ad9361_map_cmd_t __start_ad9361_map_cmd;
extern struct ad9361_map_cmd_t  __stop_ad9361_map_cmd;

struct ad9361_map_cmd_t *ad9361_map_find (const char *arg0)
{
	struct ad9361_map_cmd_t *map;

	for ( map = &__start_ad9361_map_cmd; map != &__stop_ad9361_map_cmd; map++ )
		if ( !strcasecmp(arg0, map->arg0) )
			return map;

	errno = ENOENT;
	return NULL;
}

int ad9361_map_call (struct ad9361_map_cmd_t *map, int argc, const char **argv)
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


extern struct ad9361_map_arg_t __start_ad9361_map_arg;
extern struct ad9361_map_arg_t  __stop_ad9361_map_arg;

static int ad9361_map_help_cmp (const void *a, const void *b)
{
	int ret = strcmp(((struct ad9361_map_arg_t *)a)->func,
	                 ((struct ad9361_map_arg_t *)b)->func);

	if ( ret )
		return ret;

	return ((struct ad9361_map_arg_t *)a)->idx - ((struct ad9361_map_arg_t *)b)->idx;
}

void ad9361_map_help (const char *name)
{
	static int ad9361_map_help_sorted = 0;
	if ( !ad9361_map_help_sorted )
	{
		ad9361_map_help_sorted = 1;
		qsort(&__start_ad9361_map_arg, &__stop_ad9361_map_arg - &__start_ad9361_map_arg,
		      sizeof(struct ad9361_map_arg_t), ad9361_map_help_cmp);
	}

	struct ad9361_map_arg_t *arg;
	struct ad9361_map_arg_t *beg = NULL;
	for ( arg = &__start_ad9361_map_arg; arg != &__stop_ad9361_map_arg; arg++ )
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
	for ( arg = beg; arg != &__stop_ad9361_map_arg; arg++ )
		if ( !strcmp(arg->func, name) )
		{
			if ( (len = strlen(arg->type)) > type_len )  type_len = len;
			if ( (len = strlen(arg->name)) > name_len )  name_len = len;
		}
		else
			break;

	for ( arg = beg; arg != &__stop_ad9361_map_arg; arg++ )
		if ( !strcmp(arg->func, name) )
		{
			fprintf(stderr, "%3d: %-*s  %-*s  %s\n", arg->idx, type_len,
			       arg->type, name_len, arg->name, arg->desc);
			if ( arg->help )
				arg->help(arg->arg);
		}
		else
			break;
}

void ad9361_map_help_enum (void *arg)
{
	if ( !arg )
		return;

	struct ad9361_enum_map *map = (struct ad9361_enum_map *)arg;
	fprintf(stderr, "     Values: %d:%s", map->value, map->string);
	for ( map++; map->string; map++ )
		fprintf(stderr, ", %d:%s", map->value, map->string);
	fprintf(stderr, "\n");
}

#ifdef AD9361_USE_READLINE
static char *ad9361_map_generate (const char *text, int state)
{
	static struct ad9361_map_cmd_t *map;
	static int                      len;
	const char                     *name;

	if ( !state )
	{
		map = &__start_ad9361_map_cmd;
		len = strlen(text);
	}

	name = map->arg0;
	while ( map != &__stop_ad9361_map_cmd )
	{
		map++;

		if ( !strncasecmp(name, text, len) )
			return strdup(name);

		name = map->arg0;
	}

	return NULL;
}

char **ad9361_map_completion (const char *text, int start, int end)
{
	if ( start )
		return NULL;

	return rl_completion_matches(text, ad9361_map_generate);
}
#endif


