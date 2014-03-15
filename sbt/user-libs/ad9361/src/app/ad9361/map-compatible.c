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
	
	uint8_t b0, b1, e0, e1;

	if ( ad9361_get_in_temp0_input(0, &temp1) < 0 ) return -1;
	if ( ad9361_get_in_temp0_input(1, &temp2) < 0 ) return -1;

	ad9361_spi_read_byte(0, 0x0b, &b0);
	ad9361_spi_read_byte(1, 0x0b, &b1);

	ad9361_spi_read_byte(0, 0x0E, &e0);
	ad9361_spi_read_byte(1, 0x0E, &e1);

	temp3 = (1000*(e0-64))/1148;
	temp4 = (1000*(e1-64))/1148;

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

