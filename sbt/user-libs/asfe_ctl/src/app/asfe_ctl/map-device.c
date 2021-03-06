/** \file      src/app/asfe_ctl/map-device.c
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

#include <asfe_ctl.h>

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

/* GPIO PIN command mappings */

static int map_ASFE_Spare_1 (int argc, const char **argv)
{
	MAP_ARG(UINT8, val, 1, "value to set");
	//MAP_ARG(UINT8, val, 2, "Value to set");
	
	int pin = 6;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(ASFE_Spare_1, map_ASFE_Spare_1, 2);


static int map_ASFE_Spare_2 (int argc, const char **argv)
{
	MAP_ARG(UINT8, val, 1, "value to set");
	//MAP_ARG(UINT8, val, 2, "Value to set");
	
	int pin = 7;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(ASFE_Spare_2, map_ASFE_Spare_2, 2);


static int map_ASFE_RSTN (int argc, const char **argv)
{
	MAP_ARG(UINT8, val, 1, "value to set");
	//MAP_ARG(UINT8, val, 2, "Value to set");
	
	int pin = 8;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(ASFE_RSTN, map_ASFE_RSTN, 2);


static int map_ADI1_TX_EN (int argc, const char **argv)
{
	MAP_ARG(UINT8, val, 1, "value to set");
	//MAP_ARG(UINT8, val, 2, "Value to set");
	
	int pin = 9;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(ADI1_TX_EN, map_ADI1_TX_EN, 2);


static int map_ASFE_Reserve1 (int argc, const char **argv)
{
	MAP_ARG(UINT8, val, 1, "value to set");
	//MAP_ARG(UINT8, val, 2, "Value to set");
	
	int pin = 10;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(ASFE_Reserve1, map_ASFE_Reserve1, 2);


static int map_ASFE_Reserve2 (int argc, const char **argv)
{
	MAP_ARG(UINT8, val, 1, "value to set");
	//MAP_ARG(UINT8, val, 2, "Value to set");
	
	int pin = 11;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(ASFE_Reserve2, map_ASFE_Reserve2, 2);


static int map_ASFE_Reserve4 (int argc, const char **argv)
{
	MAP_ARG(UINT8, val, 1, "value to set");
	//MAP_ARG(UINT8, val, 2, "Value to set");
	
	int pin = 12;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(ASFE_Reserve4, map_ASFE_Reserve4, 2);


static int map_ASFE_Reserve3 (int argc, const char **argv)
{
	MAP_ARG(UINT8, val, 1, "value to set");
	//MAP_ARG(UINT8, val, 2, "Value to set");
	
	int pin = 13;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(ASFE_Reserve3, map_ASFE_Reserve3, 2);


static int map_ADI2_TX_EN (int argc, const char **argv)
{
	MAP_ARG(UINT8, val, 1, "value to set");
	//MAP_ARG(UINT8, val, 2, "Value to set");
	
	int pin = 14;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(ADI2_TX_EN, map_ADI2_TX_EN, 2);


static int map_ASFE_Spare_3 (int argc, const char **argv)
{
	MAP_ARG(UINT8, val, 1, "value to set");
	//MAP_ARG(UINT8, val, 2, "Value to set");
	
	int pin = 15;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(ASFE_Spare_3, map_ASFE_Spare_3, 2);


static int map_ASFE_Spare_4 (int argc, const char **argv)
{
	MAP_ARG(UINT8, val, 1, "value to set");
	//MAP_ARG(UINT8, val, 2, "Value to set");
	
	int pin = 16;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(ASFE_Spare_4, map_ASFE_Spare_4, 2);

/*static void Set_ASFE_Mode (UINT8 ADx, UINT8 TX_EN, UINT8 RX1_LNAB, UINT8 RX2_LNAB, UINT8 TR_N, UINT8 ASFE_RESET)
{
	UINT8 lna_bypass;
		
	if(ADx == 1 || ADx == 3){			
		//TX_EN setup		
		set_gpio(63, TX_EN);
		
		//LNA Bypass Setup
		lna_bypass = 0;		
		lna_bypass = (TR_N << 7) + (RX2_LNAB << 5) + (RX1_LNAB << 4);		
		dev_reopen(0, 1);		
		CMB_SPIWriteByte (0x020, 0x00);
		CMB_SPIWriteByte (0x026, 0x90);
		CMB_SPIWriteByte (0x027, lna_bypass);
	} else{
		//LNA Bypass Setup
		dev_reopen(0, 1);		
		CMB_SPIWriteByte (0x020, 0x00);
		CMB_SPIWriteByte (0x026, 0x90);
		CMB_SPIWriteByte (0x027, 0x00);	
		
		//TX_EN setup
		set_gpio(63,0);
	}

	if(ADx == 2 || ADx == 3){
		//TX_EN Setup
		set_gpio(68, TX_EN);
		//LNA Bypass Setup
		lna_bypass = 0;		
		lna_bypass = (TR_N << 7) + (RX2_LNAB << 5) + (RX1_LNAB << 4);
		dev_reopen(1, 1);		
		CMB_SPIWriteByte (0x020, 0x00);
		CMB_SPIWriteByte (0x026, 0x90);
		CMB_SPIWriteByte (0x027, lna_bypass);

	} else{
		//TX_En Setup
		set_gpio(68,0);

		//LNA Bypass Setup
		dev_reopen(1, 1);		
		CMB_SPIWriteByte (0x020, 0x00);
		CMB_SPIWriteByte (0x026, 0x90);
		CMB_SPIWriteByte (0x027, 0x00);	
	}
		
	//ASFE Reset setup
	set_gpio(62,ASFE_RESET);

	printf("ASFE Mode Setup Complete\r\n");
		
}

static int map_Set_ASFE_Mode (int argc, const char **argv)
{
	MAP_ARG(UINT8, ADx,      1, "Select ADI Chip, 1, 2, or 3 for both");
	MAP_ARG(UINT8, TX_EN,    2, "Set TX_EN active (1) or inactive (0)");
	MAP_ARG(UINT8, RX1_LNAB, 3, "Set RX1 LNA Bypass active (1) or inactive (0)");
	MAP_ARG(UINT8, RX2_LNAB, 4, "Set RX2 LNA Bypass active (1) or inactive (0)");
	MAP_ARG(UINT8, TR_N,     5, "Set ASFE_ADx_TR_N active (1) or inactive (0)");
	MAP_ARG(UINT8, ASFE_RESET, 6, "Set ASFE Reset high (1) or low (0)");

	Set_ASFE_Mode (ADx, TX_EN, RX1_LNAB, RX2_LNAB, TR_N, ASFE_RESET);

	return 0;
}
MAP_CMD(Set_ASFE_Mode, map_Set_ASFE_Mode, 7);
*/
