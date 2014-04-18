/** \file      src/app/ad9361/map-debug.c
 *  \brief     Extensions, debugging, and hacking
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

#include <ad9361.h>

#include "map.h"
#include "main.h"


static int map_ext_reg_dump (int argc, const char **argv)
{
	uint16_t  r;
	uint8_t   v;

	for ( r = 0; r < 0x400; r++ )
	{
		ad9361_spi_read_byte(ad9361_legacy_dev, r, &v);
		printf("reg_0x%03x=\"0x%02x\"\n", r, v);
	}

	return 0;
}
MAP_CMD(RegDump, map_ext_reg_dump, 1);

static int map_ext_reg_grid (int argc, const char **argv)
{
	int      x,y;
	uint8_t  v;

	printf("     ");
	for ( x = 0; x < 0x10; x++ )
		printf("  x%x", x);

	for ( y = 0; y < 0x400; y += 0x10 )
	{
		printf("\n0x%02xx", y >> 4);
		for ( x = 0; x < 0x10; x++ )
		{
			ad9361_spi_read_byte(ad9361_legacy_dev, y | x, &v);
			printf("  %02x", v);
		}
	}

	printf("\n");
	return 0;
}
MAP_CMD(RegGrid, map_ext_reg_grid, 1);


