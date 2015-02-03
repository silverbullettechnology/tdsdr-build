/** \file      src/lib/cal_ctl_hal.c
 *  \brief     HAL registration
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
#include <string.h>
#include <errno.h>

#include "api_types.h"
#include "cal_ctl_hal.h"
#include "cal_ctl_hal_linux.h"


const struct cal_ctl_hal *cal_ctl_hal = NULL;

static UINT16 cal_ctl_hal_spi_sniff[1024];

void cal_ctl_hal_spi_reg_clr (void)
{
	memset(cal_ctl_hal_spi_sniff, 0, sizeof(cal_ctl_hal_spi_sniff));
}

int cal_ctl_hal_attach (const struct cal_ctl_hal *hal)
{
	if ( cal_ctl_hal )
		return -1;

	cal_ctl_hal = hal;
	cal_ctl_hal_spi_reg_clr();
	return 0;
}


int cal_ctl_hal_detach (void)
{
	if ( !cal_ctl_hal )
		return -1;

	cal_ctl_hal = NULL;
	cal_ctl_hal_spi_reg_clr();
	return 0;
}

void cal_ctl_hal_spi_reg_set (UINT16 addr, UINT8 data)
{
	if ( addr >= 1024 )
		return;

	cal_ctl_hal_spi_sniff[addr]  = 0x8000;
	cal_ctl_hal_spi_sniff[addr] |= data;
}

int cal_ctl_hal_spi_reg_get (UINT16 addr)
{
	if ( addr >= 1024 )
	{
		errno = EINVAL;
		return -1;
	}

	if ( ! (cal_ctl_hal_spi_sniff[addr] & 0x8000) )
	{
		errno = ENOENT;
		return -1;
	}

	return cal_ctl_hal_spi_sniff[addr] & 0xFF;
}

