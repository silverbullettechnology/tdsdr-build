/** \file      src/app/asfe_ctl/map-asfe-common.c
 *  \brief     Primary ASFE commands
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
#include "parse.h"
#include "format.h"
#include "common.h"


static int map_CMB_SPIWriteByte (int argc, const char **argv)
{
	MAP_ARG(UINT16, data, 1, "");
	//MAP_ARG(UINT16, data, 2, "");

	CMB_SPIWriteByte(data);

	return 0;
}
MAP_CMD(SPIWriteByte, map_CMB_SPIWriteByte, 2);
MAP_CMD(W,            map_CMB_SPIWriteByte, 2);

static int map_CMB_SPIWriteByteArray (int argc, const char **argv)
{
	
	MAP_ARG(UINT16, size, 1, "");
	MAP_ARR(UINT16, data, 2, size, "");	

	CMB_SPIWriteByteArray(data,size);

	return 0;
}
MAP_CMD(SPIWriteByteArray, map_CMB_SPIWriteByteArray, 3);
MAP_CMD(WA,            	   map_CMB_SPIWriteByteArray, 3);


/*static int map_CMB_SPIReadByte (int argc, const char **argv)
{
	MAP_ARG(UINT16, addr, 1, "");

	UINT8  readdata;

	CMB_SPIReadByte(addr, &readdata);

	MAP_RESULT(UINT8, readdata, 1);
	return 0;
}
MAP_CMD(SPIReadByte, map_CMB_SPIReadByte, 2);
MAP_CMD(R,           map_CMB_SPIReadByte, 2);
*/

/*static int map_CMB_DelayU (int argc, const char **argv)
{
	MAP_ARG(int, usec, 1, "");

	CMB_DelayU(usec);

	return 0;
}
MAP_CMD(DelayU, map_CMB_DelayU, 1);
*/

static int map_CMB_gpioWrite (int argc, const char **argv)
{
	MAP_ARG(int, gpio_num,   1, "");
	MAP_ARG(int, gpio_value, 2, "");

	CMB_gpioWrite(gpio_num, gpio_value);

	return 0;
}
MAP_CMD(gpioWrite, map_CMB_gpioWrite, 3);


static int map_CMB_gpioPulse (int argc, const char **argv)
{
	MAP_ARG(int, gpio_num,  1, "");
	MAP_ARG(int, pulseTime, 1, "");

	CMB_gpioPulse(gpio_num, pulseTime);

	return 0;
}
MAP_CMD(gpioPulse, map_CMB_gpioPulse, 3);


