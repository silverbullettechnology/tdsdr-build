/** \file      src/lib/ad9361_hal_linux_legacy.c
 *  \brief     Linux/POSIX HAL implementation
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include "lib.h"
#include "util.h"
#include "api_types.h"
#include "ad9361_hal.h"
#include "ad9361_hal_linux.h"
#include "ad9361_hal_linux_private.h"


static int ad9361_hal_linux_atexit_armed = 0;
void ad9361_hal_linux_atexit_register (void)
{
	if ( ad9361_hal_linux_atexit_armed )
		return;

	ad9361_hal_linux_atexit_armed = 1;
	atexit(ad9361_hal_linux_cleanup);
}

void ad9361_hal_linux_cleanup (void)
{
	if ( !ad9361_hal_linux_atexit_armed )
		return;

	ad9361_hal_linux_atexit_armed = 0;
	ad9361_hal_linux_done();
}


/******** GPIO access through /sys/class/gpio, future ********/


int ad9361_hal_linux_gpio_init (const int pins[3])
{
	return ad9361_hal_linux_dev_gpio_init(ad9361_legacy_dev, pins);
}


void ad9361_hal_linux_gpio_done (void)
{
	ad9361_hal_linux_dev_gpio_done();
}


/******** UART emulated through stdin/stdout by default ********/


int ad9361_hal_linux_spi_init (const char *dev)
{
	return ad9361_hal_linux_dev_spi_init(ad9361_legacy_dev, dev);
}


void ad9361_hal_linux_spi_done (void)
{
	ad9361_hal_linux_dev_spi_done();
}


/******** UART emulated through stdin/stdout by default ********/


int ad9361_hal_linux_uart_init (const char *dev, const char *speed)
{
	errno = ENOSYS;
	return -1;
}


void ad9361_hal_linux_uart_done (void)
{
}


