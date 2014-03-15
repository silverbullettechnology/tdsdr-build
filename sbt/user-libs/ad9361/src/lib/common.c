/** \file      src/lib/common.c
 *  \brief     Common layer source
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
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "api_types.h"
#include "lib.h"
#include "common.h"

#include "ad9361_hal.h"


unsigned ad9361_legacy_dev = 0;


void CMB_init (void)
{
	errno = ENOSYS;
}


/*********************SPI*********************/


void CMB_SPIWriteField (UINT16 addr, UINT8 field_val, UINT8 mask, UINT8 start_bit)
{
	uint8_t Val;
	ad9361_spi_read_byte(ad9361_legacy_dev, addr, &Val);
	Val = (Val & ~mask) | ((field_val << start_bit) & mask);
	ad9361_spi_write_byte(ad9361_legacy_dev, addr, Val);
}


void CMB_SPIWriteByte (UINT16 addr, UINT16 data)
{
	ad9361_spi_write_byte(ad9361_legacy_dev, addr, data & 0xFF);
}


void CMB_SPIReadByte (UINT16 addr, UINT8 *readdata)
{
	ad9361_spi_read_byte(ad9361_legacy_dev, addr, readdata);
}


/********************UART********************/


void CMB_uartSendString (const char *dataString)
{
	errno = ENOSYS;
}


void CMB_hostTxPacket (UINT8 *dataString, int maxSize)
{
	errno = ENOSYS;
}


UINT8 CMB_hostRxPacket (UINT8 *RxString, int maxSize)
{
	errno = ENOSYS;
	return 0;
}


/******** Timer ********/


void CMB_DelayU (int usec)
{
	usleep(usec);
}


/******** GPIO ********/


void CMB_gpioWrite (int gpio_num, int gpio_value)
{
	ad9361_gpio_write(ad9361_legacy_dev, gpio_num, gpio_value);
}


void CMB_gpioPulse (int gpio_num, int pulseTime) //0-2.4us, 1-3.1us, 2-3.8us, 3-4.5us, 4-5.2us...
{
	if ( gpio_num == GPIO_Resetn_pin )
	{
		ad9361_gpio_write(ad9361_legacy_dev, gpio_num, 0);
		usleep(pulseTime);
		ad9361_gpio_write(ad9361_legacy_dev, gpio_num, 1);
	}
	else
	{
		ad9361_gpio_write(ad9361_legacy_dev, gpio_num, 1);
		usleep(pulseTime);
		ad9361_gpio_write(ad9361_legacy_dev, gpio_num, 0);
	}
}


const char *ad9361_err_desc (ADI_ERR err)
{
	static char desc[32];

	switch ( err )
	{
		case ADIERR_OK:            return "OK";
		case ADIERR_FALSE:         return "FALSE";
		case ADIERR_TRUE:          return "TRUE";
		case ADIERR_INV_PARM:      return "INV_PARM";
		case ADIERR_NOT_AVAILABLE: return "NOT_AVAILABLE";
		case ADIERR_FAILED:        return "FAILED";
	}

	snprintf(desc, sizeof(desc), "Unknown(%d)", err);
	return desc;
}


