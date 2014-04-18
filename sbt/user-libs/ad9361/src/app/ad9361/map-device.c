/** \file      src/app/ad9361/map-device.c
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
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>

#include <ad9361.h>

#include "map.h"
#include "main.h"
#include "parse-gen.h"
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

	ad9361_legacy_dev = num;

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
	MAP_ARG(uint8_t, pin, 1, "EMIO pin number");
	MAP_ARG(uint8_t, val, 2, "Value to set");

	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return set_gpio(pin, val ? 1 : 0);
}
MAP_CMD(setEMIO, map_set_emio, 3);

static int map_pulse_emio (int argc, const char **argv)
{
	MAP_ARG(uint8_t, pin, 1, "EMIO pin number");
	MAP_ARG(uint8_t, val, 2, "Active value (0 for active low), opposite will be written to de-assert");
	MAP_ARG(int,     dly, 3, "Time to pulse in milliseconds");

	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	if ( set_gpio(pin, val ? 1 : 0) < 0 )
		return -1;
	
	usleep(dly * 1000);

	return set_gpio(pin, val ? 0 : 1);
}
MAP_CMD(pulseEMIO, map_pulse_emio, 3);

static int map_Set_ASFE_Mode (int argc, const char **argv)
{
	MAP_ARG(uint8_t, ADx,      1, "Select ADI Chip, 1, 2, or 3 for both");
	MAP_ARG(uint8_t, RX1_LNAB, 2, "Set RX1 LNA Bypass active (1) or inactive (0)");
	MAP_ARG(uint8_t, RX2_LNAB, 3, "Set RX2 LNA Bypass active (1) or inactive (0)");
	MAP_ARG(uint8_t, TR_N,     4, "Set ASFE_ADx_TR_N active (1) or inactive (0)");
	uint8_t lna_bypass;
		
	if(ADx == 1 || ADx == 3){			
		
		//LNA Bypass Setup
		lna_bypass = 0;		
		lna_bypass = (TR_N << 7) + (RX2_LNAB << 5) + (RX1_LNAB << 4);		
		ad9361_spi_write_byte(0, 0x020, 0x00);
		ad9361_spi_write_byte(0, 0x026, 0x90);
		ad9361_spi_write_byte(0, 0x027, lna_bypass);
	} else{
		//LNA Bypass Setup
		ad9361_spi_write_byte(0, 0x020, 0x00);
		ad9361_spi_write_byte(0, 0x026, 0x90);
		ad9361_spi_write_byte(0, 0x027, 0x00);	
	}

	if(ADx == 2 || ADx == 3){
		//LNA Bypass Setup
		lna_bypass = 0;		
		lna_bypass = (TR_N << 7) + (RX2_LNAB << 5) + (RX1_LNAB << 4);
		ad9361_spi_write_byte(1, 0x020, 0x00);
		ad9361_spi_write_byte(1, 0x026, 0x90);
		ad9361_spi_write_byte(1, 0x027, lna_bypass);

	} else{
		//LNA Bypass Setup
		ad9361_spi_write_byte(1, 0x020, 0x00);
		ad9361_spi_write_byte(1, 0x026, 0x90);
		ad9361_spi_write_byte(1, 0x027, 0x00);	
	}

	printf("ADI ASFE Control Setup Complete\r\n");

	return 0;
}
MAP_CMD(Set_ASFE_Mode, map_Set_ASFE_Mode, 5);


static void ADI_Get_Voltage(void){

	uint8_t temp1;
	uint8_t temp2;
	int adc1;
	int adc2;

	ad9361_spi_write_byte(0, 0x00C, 0x00);
	ad9361_spi_write_byte(0, 0x00D, 0x00);
	ad9361_spi_write_byte(0, 0x00F, 0x04);
	ad9361_spi_write_byte(0, 0x01C, 0x63);
	ad9361_spi_write_byte(0, 0x01D, 0x00);
	ad9361_spi_write_byte(0, 0x035, 0x1E);
	ad9361_spi_write_byte(0, 0x036, 0xFF);

	usleep(1000);

	ad9361_spi_read_byte(0, 0x01E, &temp1);
	ad9361_spi_read_byte(0, 0x01F, &temp2);

	//printf("temp1: 0x%2X temp2: 0x%2X\r\n",temp1,temp2);

	adc1 = (temp1<<4)+temp2;
	adc1 = ((adc1*147)/500)+26;

	ad9361_spi_write_byte(1, 0x00C, 0x00);
	ad9361_spi_write_byte(1, 0x00D, 0x00);
	ad9361_spi_write_byte(1, 0x00F, 0x04);
	ad9361_spi_write_byte(1, 0x01C, 0x63);
	ad9361_spi_write_byte(1, 0x01D, 0x00);
	ad9361_spi_write_byte(1, 0x035, 0x1E);
	ad9361_spi_write_byte(1, 0x036, 0xFF);

	usleep(1000);

	ad9361_spi_read_byte(1, 0x01E, &temp1);
	ad9361_spi_read_byte(1, 0x01F, &temp2);

	//printf("temp1: 0x%2X temp2: 0x%2X\r\n",temp1,temp2);

	adc2 = (temp1<<4)+temp2;
	adc2 = ((adc2*147)/500)+26;

	printf("\r\nADC AD1_PWR_DET: %d mV\r\n", adc1);

	printf("ADC AD2_PWR_DET: %d mV\r\n", adc2);
}

static int map_ADI_Get_Voltage (int argc, const char **argv)
{

	ADI_Get_Voltage ();

	return 0;
}
MAP_CMD(ADI_Get_AuxADCVoltage, map_ADI_Get_Voltage, 1);
