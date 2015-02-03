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
#include <stdio.h>

#include "api_types.h"
#include "lib.h"
#include "common.h"

#include "cal_ctl_hal.h"
#include "cal_ctl_hal_linux.h"


void CMB_init (void)
{
	if ( !cal_ctl_hal )
	{
		printf("CMB_init() called without setting up the HAL first, stop.\n");
		exit(1);
	}
}


/*********************SPI*********************/


void CMB_SPIWriteByte (UINT16 data)
{
//fprintf(stderr, "W %03x:%02x\n", addr, data);
	HAL_SPIWriteByteArray(&data, 1);
}

void CMB_SPIWriteByteArray (UINT16 *data, UINT16 size)
{
//fprintf(stderr, "W %03x:%02x\n", addr, data);
	HAL_SPIWriteByteArray(data, size);
}


void CMB_SPIReadByte (UINT16 addr, UINT8 *readdata)
{
	UINT16 data;
	HAL_SPIReadByte(addr, &data);
	*readdata = data & 0xFF;
//fprintf(stderr, "R %03x:%02x\n", addr, data);
}


/********************UART********************/


void CMB_uartSendString (const char *dataString)
{
	while ( *dataString )
		HAL_uartSendByte(*dataString++);

	HAL_uartSendByte('\r');
	HAL_uartSendByte('\n');
}


void CMB_hostTxPacket (UINT8 *dataString, int maxSize)
{
	while ( maxSize-- > 0 )
		HAL_uartSendByte(*dataString++);
}


UINT8 CMB_hostRxPacket (UINT8 *RxString, int maxSize)
{
	int   loopIndex = 0;
	BOOL  rxflag;
	UINT8 rxbyte;

	while (1)
	{
		rxflag = HAL_uartReceiveByte(&rxbyte);

		// return 0 if idle on the first byte
		if ((!rxflag) && (!loopIndex))
			return 0;

		// have a byte, store 
		else if (rxflag)
		{
			RxString[loopIndex] = rxbyte;
			loopIndex++;
			if ( loopIndex == maxSize )
				break;
		}
	}

	return loopIndex;
}


/******** Timer ********/


void CMB_DelayU (int usec)
{
	Timer_Wait(usec);
}


/******** GPIO ********/


void CMB_gpioWrite (int gpio_num, int gpio_value)
{
	HAL_gpioWrite(gpio_num, gpio_value);
}


void CMB_gpioPulse (int gpio_num, int pulseTime) //0-2.4us, 1-3.1us, 2-3.8us, 3-4.5us, 4-5.2us...
{
	if ( gpio_num == GPIO_Resetn_pin )
	{
		HAL_gpioWrite(gpio_num, 0);
		Timer_Wait(pulseTime);
		HAL_gpioWrite(gpio_num, 1);
	}
	else
	{
		HAL_gpioWrite(gpio_num, 1);
		Timer_Wait(pulseTime);
		HAL_gpioWrite(gpio_num, 0);
	}
}


const char *cal_ctl_err_desc (ADI_ERR err)
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


