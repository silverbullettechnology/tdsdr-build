/** \file      src/lib/ad9361_enum.c
 *  \brief     Enum lookup code
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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

#include "lib.h"
#include "ad9361_hal.h"
#include "ad9361_hal_linux.h"


const char *ad9361_enum_get_string (const struct ad9361_enum_map *map, int value)
{
	for ( ; map->string; map++ )
		if ( map->value == value )
			return map->string;

	errno = ENOENT;
	return NULL;
}


int ad9361_enum_get_value (const struct ad9361_enum_map *map, const char *string)
{
	for ( ; map->string; map++ )
		if ( !strcasecmp(map->string, string) )
			return map->value;

	errno = ENOENT;
	return -1;
}


