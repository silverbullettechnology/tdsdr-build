/** Sequence code
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
#include <string.h>

#include <sbt_common/log.h>

#include <v49_client/sequence.h>


LOG_MODULE_STATIC("control_sequence", LOG_LEVEL_INFO);


/** Linker-generated symbols for the map - inside library */
extern struct sequence_map __start_sequence_map;
extern struct sequence_map  __stop_sequence_map;

struct sequence_map *sequence_find_lib (struct sequence_map *start_map_app,
                                        struct sequence_map *stop_map_app,
                                        const char *name)
{
	struct sequence_map *map;

	for ( map = start_map_app; map != stop_map_app; map++ )
		if ( !strcmp(map->name, name) )
			return map;

	for ( map = &__start_sequence_map; map != &__stop_sequence_map; map++ )
		if ( !strcmp(map->name, name) )
			return map;

	return NULL;
}


void sequence_list_lib (struct sequence_map *start_map_app,
                        struct sequence_map *stop_map_app,
                        int level)
{
	struct sequence_map *map;
	int                  len = 0;

	for ( map = start_map_app; map != stop_map_app; map++ )
		if ( strlen(map->name) > len )
			len = strlen(map->name);

	for ( map = &__start_sequence_map; map != &__stop_sequence_map; map++ )
		if ( strlen(map->name) > len )
			len = strlen(map->name);

	for ( map = start_map_app; map != stop_map_app; map++ )
		LOG_MESSAGE(level, "  %*s  %s\n", 0 - len, map->name, map->desc);

	for ( map = &__start_sequence_map; map != &__stop_sequence_map; map++ )
		LOG_MESSAGE(level, "  %*s  %s\n", 0 - len, map->name, map->desc);
}


/** Temporary to ensure list is not empty */
static int _sequence_list (struct socket *sock, int argc, char **argv)
{
	ENTER("sock %p, argc %d, argv [%s,...]", sock, argc, argv[0]);

	sequence_list(LOG_LEVEL_INFO);

	RETURN_ERRNO_VALUE(0, "%d", 0);
}
SEQUENCE_MAP("list", _sequence_list, "List commands available");




