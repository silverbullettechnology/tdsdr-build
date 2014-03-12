/** \file      src/app/ad9361/format.c
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
#include <stdlib.h>
#include <errno.h>

#include <ad9361.h>

#include "struct-gen.h"
#include "format-gen.h"


/******** Scalars ********/


void format_int (const int *val, const char *name, int num)
{
	printf("%s=\"%d", name, *val++);

	for ( num--; num > 0; num-- )
		printf(",%d", *val++);

	printf("\"\n");
}


void format_BOOL (const BOOL *val, const char *name, int num)
{
	printf("%s=\"%d", name, *val++ ? 1 : 0);

	for ( num--; num > 0; num-- )
		printf(",%d", *val++ ? 1 : 0);

	printf("\"\n");
}


void format_uint8_t (const uint8_t *val, const char *name, int num)
{
	printf("%s=\"%u", name, *val++);

	for ( num--; num > 0; num-- )
		printf(",%u", *val++);

	printf("\"\n");
}


void format_uint16_t (const uint16_t *val, const char *name, int num)
{
	printf("%s=\"%u", name, *val++);

	for ( num--; num > 0; num-- )
		printf(",%u", *val++);

	printf("\"\n");
}


void format_int32_t (const int32_t *val, const char *name, int num)
{
	printf("%s=\"%ld", name, (long)*val++);

	for ( num--; num > 0; num-- )
		printf(",%ld", (long)*val++);

	printf("\"\n");
}


void format_uint32_t (const uint32_t *val, const char *name, int num)
{
	printf("%s=\"%lu", name, (unsigned long)*val++);

	for ( num--; num > 0; num-- )
		printf(",%lu", (unsigned long)*val++);

	printf("\"\n");
}


void format_uint64_t (const uint64_t *val, const char *name, int num)
{
	printf("%s=\"%llu", name, (unsigned long long)*val++);

	for ( num--; num > 0; num-- )
		printf(",%llu", (unsigned long long)*val++);

	printf("\"\n");
}


void format_enum (const int *val, const char *name, const struct ad9361_enum_map *map)
{
	printf("%s=\"%d\"\n", name, *val);

	const char *word = ad9361_enum_get_string(map, *val);
	printf("%s_enum=\"%s\"\n", name, word ? word : "???");
}


/******** Structures ********/


void format_struct (const struct struct_map *map, const void *val, const char *name, int num)
{
	const char *dat;
	char        buf[256];
	char       *end = buf + sizeof(buf);
	char       *ins = buf + snprintf(buf, sizeof(buf), "%s_", name);

	while ( map->type < ST_MAX )
	{
		dat = (const char *)val + map->offs;
		snprintf(ins, end - ins, "%s", map->name);

		switch ( map->type )
		{
			case ST_INT:
				format_int((const int *)dat, buf, 1);
				break;

			case ST_BOOL:
				format_BOOL((const BOOL *)dat, buf, 1);
				break;

			case ST_UINT8:
				format_uint8_t((const uint8_t *)dat, buf, 1);
				break;

			case ST_UINT16:
				format_uint16_t((const uint16_t *)dat, buf, 1);
				break;

			case ST_UINT32:
				format_uint32_t((const uint32_t *)dat, buf, 1);
				break;

			case ST_UINT64:
				format_uint64_t((const uint64_t *)dat, buf, 1);
				break;

			default:
				errno = EINVAL;
				return;
		}

		map++;
	}
}


