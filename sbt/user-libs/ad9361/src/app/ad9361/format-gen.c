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


void format_BOOL (const BOOL *val, const char *name, int num)
{
	printf("%s=\"%d", name, *val++ ? 1 : 0);

	for ( num--; num > 0; num-- )
		printf(",%d", *val++ ? 1 : 0);

	printf("\"\n");
}


void format_UINT8 (const UINT8 *val, const char *name, int num)
{
	printf("%s=\"0x%02x", name, *val++);

	for ( num--; num > 0; num-- )
		printf(",0x%02x", *val++);

	printf("\"\n");
}


void format_UINT16 (const UINT16 *val, const char *name, int num)
{
	printf("%s=\"0x%04x", name, *val++);

	for ( num--; num > 0; num-- )
		printf(",0x%04x", *val++);

	printf("\"\n");
}


void format_UINT32 (const UINT32 *val, const char *name, int num)
{
	printf("%s=\"0x%08lx", name, *val++);

	for ( num--; num > 0; num-- )
		printf(",0x%08lx", *val++);

	printf("\"\n");
}


void format_FLOAT (const FLOAT *val, const char *name, int num)
{
	printf("%s=\"%6G", name, *val++);

	for ( num--; num > 0; num-- )
		printf(",0x%6G", *val++);

	printf("\"\n");
}


void format_DOUBLE (const DOUBLE *val, const char *name, int num)
{
	printf("%s=\"%6G", name, *val++);

	for ( num--; num > 0; num-- )
		printf(",%6G", *val++);

	printf("\"\n");
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
			case ST_BOOL:
				format_BOOL((const BOOL *)dat, buf, 1);
				break;

			case ST_UINT8:
				format_UINT8((const UINT8 *)dat, buf, 1);
				break;

			case ST_UINT16:
				format_UINT16((const UINT16 *)dat, buf, 1);
				break;

			case ST_UINT32:
				format_UINT32((const UINT32 *)dat, buf, 1);
				break;

			case ST_DOUBLE:
				format_DOUBLE((const DOUBLE *)dat, buf, 1);
				break;

			default:
				errno = EINVAL;
				return;
		}

		map++;
	}
}


