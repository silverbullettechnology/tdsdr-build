/** \file      include/lib/hal.h
 *  \brief     HAL structure 
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
#ifndef _INCLUDE_LIB_HAL_H_
#define _INCLUDE_LIB_HAL_H_
#include <assert.h>

#include "api_types.h"


typedef void  (* HAL_SPIWriteByte_fn)     (UINT16 addr, UINT16 data);
typedef void  (* HAL_SPIReadByte_fn)      (UINT16 addr, UINT16 *readdata);
typedef void  (* HAL_uartSendByte_fn)     (UINT8 sendByte);
typedef BOOL  (* HAL_uartReceiveByte_fn)  (UINT8 *RxByte);
typedef void  (* Timer_Wait_fn)           (int WaitTime);
typedef void  (* HAL_gpioWrite_fn)        (int gpio_num, int gpio_value);


struct ad9361_hal
{
	HAL_SPIWriteByte_fn        HAL_SPIWriteByte;
	HAL_SPIReadByte_fn         HAL_SPIReadByte;
	HAL_uartSendByte_fn        HAL_uartSendByte;
	HAL_uartReceiveByte_fn     HAL_uartReceiveByte;
	Timer_Wait_fn              Timer_Wait;
	HAL_gpioWrite_fn           HAL_gpioWrite;
};
extern const struct ad9361_hal *ad9361_hal;

void ad9361_hal_spi_reg_set (UINT16 addr, UINT8 data);

static inline void HAL_SPIWriteByte (UINT16 addr, UINT16 data)
{
	assert(ad9361_hal);
	assert(ad9361_hal->HAL_SPIWriteByte);
	ad9361_hal->HAL_SPIWriteByte(addr, data);
	ad9361_hal_spi_reg_set(addr, data);
}

static inline void HAL_SPIReadByte (UINT16 addr, UINT16 *readdata)
{
	assert(ad9361_hal);
	assert(ad9361_hal->HAL_SPIReadByte);
	ad9361_hal->HAL_SPIReadByte(addr, readdata);
	ad9361_hal_spi_reg_set(addr, *readdata);
}

static inline void HAL_uartSendByte (UINT8 sendByte)
{
	assert(ad9361_hal);
	assert(ad9361_hal->HAL_uartSendByte);
	ad9361_hal->HAL_uartSendByte(sendByte);
}

static inline BOOL HAL_uartReceiveByte (UINT8 *RxByte)
{
	assert(ad9361_hal);
	assert(ad9361_hal->HAL_uartReceiveByte);
	return ad9361_hal->HAL_uartReceiveByte(RxByte);
}

static inline void Timer_Wait (int WaitTime)
{
	assert(ad9361_hal);
	assert(ad9361_hal->Timer_Wait);
	ad9361_hal->Timer_Wait(WaitTime);
}

static inline void HAL_gpioWrite (int gpio_num, int gpio_value)
{
	assert(ad9361_hal);
	assert(ad9361_hal->HAL_gpioWrite);
	ad9361_hal->HAL_gpioWrite(gpio_num, gpio_value);
}


int ad9361_hal_attach (const struct ad9361_hal *hal);
int ad9361_hal_detach (void);

int  ad9361_hal_spi_reg_get (UINT16 addr);
void ad9361_hal_spi_reg_clr (void);


#endif /* _INCLUDE_LIB_HAL_H_ */
