/** \file      src/app/ad9361/map-adi-iio-sysfs.c
 *  \brief     Legacy compatibility commands via new API
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
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include <ad9361.h>

#include "map.h"
#include "main.h"
#include "util.h"
#include "parse-gen.h"
#include "format-gen.h"


static const struct ad9361_enum_map ad9361_enum_tx_atten[] =
{
	{ "immediate", 1 },
	{ "alert",     0 },
	{ NULL }
};
static int map_TxAtten (int argc, const char **argv)
{
	MAP_ARG(int, tx1, 1, "Attenuation steps for TX1");
	MAP_ARG(int, tx2, 2, "Attenuation steps for TX2");
	MAP_ARG_ENUM(imm, 3, ad9361_enum_tx_atten, "When update takes effect");

	ad9361_set_update_tx_gain_in_alert_enable(ad9361_legacy_dev, !imm);

	ad9361_set_out_voltage0_hardwaregain(ad9361_legacy_dev, tx1 * -250);
	ad9361_get_out_voltage0_hardwaregain(ad9361_legacy_dev, &tx1);
	printf("TX1 attenuation %d.%03d\n", abs(tx1) / 1000, abs(tx1) % 1000);

	ad9361_set_out_voltage1_hardwaregain(ad9361_legacy_dev, tx2 * -250);
	ad9361_get_out_voltage1_hardwaregain(ad9361_legacy_dev, &tx2);
	printf("TX2 attenuation %d.%03d\n", abs(tx2) / 1000, abs(tx2) % 1000);

	printf("Change takes effect %s\n", imm ? "immediately" : "during alert");
	return 0;
}
MAP_CMD(TxAtten, map_TxAtten, 4);


static int map_ADI_get_temperature (int argc, const char **argv)
{
	int temp1, temp2;
	int temp3, temp4;
	int srate_factor [] = {0,0};
	int ensm_mode[] = {-1,-1};
	//int done;
	uint32_t srate[] = {0, 0};

	uint32_t bbpll, dac, t2, t1, tf;
	
	uint8_t b0, b1, e0, e1, done;

	if ( ad9361_get_in_temp0_input(0, &temp1) < 0 ) return -1;
	if ( ad9361_get_in_temp0_input(1, &temp2) < 0 ) return -1;

	//get the current ensm_mode before setting the ADIs to transmit mode to activate the AuxADC
	ad9361_get_ensm_mode(0,&ensm_mode[0]);
	ad9361_get_ensm_mode(1,&ensm_mode[1]);
	
	//note this only works with FDD mode currently.  TODO: check for TDD mode as well.
	if(ensm_mode[0] < 5)
		ad9361_set_ensm_mode(0,5);
	
	if(ensm_mode[1] < 5)
		ad9361_set_ensm_mode(1,5);

	//set manual temperature measurement mode
	ad9361_spi_write_byte(0,0x00C, 0x0F);
	ad9361_spi_write_byte(0,0x00D, 0x00);
	ad9361_spi_write_byte(0,0x00F, 0x00);
	ad9361_spi_write_byte(0,0x01D, 0x01);
	ad9361_spi_write_byte(0,0x00C, 0x00);	

	ad9361_spi_read_byte(0, 0x00C, &done);
	done = done & 0x02;

	while(!done){
		ad9361_spi_read_byte(0, 0x00C, &done);
		printf("waiting %02x\r\n",done);
		done = done & 0x02;
	}

	//set manual temperature measurement mode
	ad9361_spi_write_byte(1,0x00C, 0x0F);
	ad9361_spi_write_byte(1,0x00D, 0x00);
	ad9361_spi_write_byte(1,0x00F, 0x00);
	ad9361_spi_write_byte(1,0x01D, 0x01);
	ad9361_spi_write_byte(1,0x00C, 0x00);	

	ad9361_spi_read_byte(1, 0x00C, &done);
	done = done & 0x02;

	while(!done){
		ad9361_spi_read_byte(1, 0x00C, &done);
		printf("waiting %02x\r\n",done);
		done = done & 0x02;
	}

	//Sample rates seem to have a non-linear affect on the temperature offset required.  Read the sample rates in order to apply the correct offset.
	ad9361_get_tx_path_rates (0, &bbpll, &dac, &t2, &t1, &tf, &srate[0]);
	ad9361_get_tx_path_rates (1, &bbpll, &dac, &t2, &t1, &tf, &srate[1]);

	srate[0] = srate[0]/1000;
	srate[1] = srate[1]/1000;

	srate_factor[0] = (((((int)srate[0]*(-308))-4508580)/1000000)-1)+5;
	srate_factor[1] = ((((int)srate[1]*(-308))-4508580)/1000000)-1;

	ad9361_spi_write_byte(0, 0x01d, 1);
	ad9361_spi_write_byte(1, 0x01d, 1);

	ad9361_spi_read_byte(0, 0x0b, &b0);
	ad9361_spi_read_byte(1, 0x0b, &b1);

	ad9361_spi_read_byte(0, 0x0E, &e0);
	ad9361_spi_read_byte(1, 0x0E, &e1);

	//re-enable the auxdac
	ad9361_spi_write_byte(0, 0x01d, 0);
	ad9361_spi_write_byte(1, 0x01d, 0);
	//re-enable periodic measurements
	ad9361_spi_write_byte(0,0x00D, 0x05);
	ad9361_spi_write_byte(1,0x00D, 0x05);

	ad9361_set_ensm_mode(0,ensm_mode[0]);
	ad9361_set_ensm_mode(1,ensm_mode[1]);

	temp3 = (1000*((e0+srate_factor[0])-64))/1148;
	temp4 = (1000*((e1+srate_factor[1])-64))/1148;

	printf("\nAD1 Sample Rate: %d \n", srate[0]);
	printf("AD2 Sample Rate: %d \n\n", srate[1]);

	printf("AD1 Rate Offset: %d \n", srate_factor[0]);
	printf("AD2 Rate Offset: %d \n\n", srate_factor[1]);

	printf("Offset Register AD1: %d\n", b0);
	printf("Offset Register AD2: %d\n\n", b1);
	printf("Uncorrected Temperature AD1: %d\n", e0);
	printf("Uncorrected Temperature AD2: %d\n\n", e1);
	printf("Corrected Temperature (app note 2.4) AD1: %d\n",   temp3);
	printf("Corrected Temperature (app note 2.4) AD2: %d\n\n", temp4);
	//printf("Corrected Temperature (IIO drv) AD1: %d\n",   temp1);
	//printf("Corrected Temperature (IIO drv) AD2: %d\n\n", temp2);

	return 0;
}
MAP_CMD(ADI_get_temperature, map_ADI_get_temperature, 1);

