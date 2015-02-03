/** \file      src/app/cal_ctl/map-device.c
 *  \brief     Commands for switching between two ADIs on the fly
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
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>

#include <cal_ctl.h>

#include "map.h"
#include "main.h"
#include "parse.h"
#include "util.h"


static int map_device (int argc, const char **argv)
{
	const char *arg;
	unsigned    num;

#ifdef SIM_HAL
	return 0;
#endif

	if ( argc < 2 || !argv[1] )
	{
		errno = EINVAL;
		return -1;
	}
	arg = argv[1];

	if ( tolower(arg[0]) == 'a' && tolower(arg[1]) == 'd' )
		num = strtoul(arg + 2, NULL, 10) - 1;
	else
		num = strtoul(arg, NULL, 10);

	if ( dev_reopen(num, 1) )
	{
		fprintf(stderr, "dev_reopen(%d): %s\n", num, strerror(errno));
		return -1;
	}
	
	errno = 0;
	return 0;
}
MAP_CMD(device, map_device, 2);
MAP_CMD(dev,    map_device, 2);


static int set_gpio (int pin, int val)
{
	char path[PATH_MAX];
	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", pin);

	// stat /sys tree, if not present try to export by pin number
	struct stat sb;
	if ( stat(path, &sb) && errno == ENOENT )
	{
		proc_printf("/sys/class/gpio/export", "%d\n", pin);
		if ( stat(path, &sb) )
			return -1;
	}

	// check direction, if already correct then move on
	char buff[16];
	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
	if ( proc_scanf(path, "%15s\n", buff) != 1 )
		return -1;
	if ( strcmp(buff, "out") )
		proc_printf(path, "out\n");

	// open file descriptor for value before checking direction
	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
	if ( proc_printf(path, "%d\n", val) < 0 )
		return -1;

	return 0;
}


static int map_set_emio (int argc, const char **argv)
{
	MAP_ARG(UINT8, pin, 1, "EMIO pin number");
	MAP_ARG(UINT8, val, 2, "Value to set");

	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(setEMIO, map_set_emio, 3);

static int map_pulse_emio (int argc, const char **argv)
{
	MAP_ARG(UINT8, pin, 1, "EMIO pin number");
	MAP_ARG(UINT8, val, 2, "Active value (0 for active low), opposite will be written to de-assert");
	MAP_ARG(int,   dly, 3, "Time to pulse in milliseconds");

	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	if ( set_gpio(pin, val ? 1 : 0) < 0 )
		return -1;
	
	CMB_DelayU(dly * 1000);

	return set_gpio(pin, val ? 0 : 1);
}
MAP_CMD(pulseEMIO, map_pulse_emio, 3);


