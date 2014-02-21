/** \file      include/lib/common.h
 *  \brief     Common layer header
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
#ifndef _INCLUDE_LIB_COMMON_H_
#define _INCLUDE_LIB_COMMON_H_

#include "api_types.h"

void CMB_init (void);

void CMB_SPIWriteByte (UINT16 data);
void CMB_SPIWriteByteArray (UINT16 *data, UINT16 size);
void CMB_SPIReadByte (UINT16 addr, UINT8 *readdata);

void CMB_uartSendHex (UINT32 hexValue);
void CMB_uartSendString (const char *dataString);
UINT8 CMB_uartReceiveString (UINT8 *RxString, int maxSize);
void CMB_hostTxPacket (UINT8 *dataString, int maxSize);
UINT8 CMB_hostRxPacket (UINT8 *RxString, int maxSize);

void CMB_DelayU (int usec);
void CMB_gpioInit (int gpio_num);
void CMB_gpioWrite (int gpio_num, int gpio_value);
void CMB_gpioPulse (int gpio_num, int pulseTime);


const char *asfe_ctl_err_desc (ADI_ERR err);


#endif /* _INCLUDE_LIB_COMMON_H_ */
