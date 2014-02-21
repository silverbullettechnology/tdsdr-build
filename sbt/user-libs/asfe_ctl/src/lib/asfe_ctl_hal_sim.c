/** \file      src/lib/asfe_ctl_hal_sim.c
 *  \brief     Simulated HAL for host-side unit tests
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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

// quash a warning about using kernel headers in userspace
#define __EXPORTED_HEADERS__
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "api_types.h"
#include "asfe_ctl_hal.h"
#include "asfe_ctl_hal_sim.h"


static void asfe_ctl_hal_sim_timer_wait (int usec)
{
//	fprintf(stderr, "asfe_ctl_hal_sim_timer_wait(%d)\n", usec);
	usleep(usec);
}


static void asfe_ctl_hal_sim_gpio_write (int pin, int val)
{
//	fprintf(stderr, "asfe_ctl_hal_sim_gpio_write(pin %d, val %d)\n", pin, val);
}


static void asfe_ctl_hal_sim_spi_write_byte (UINT16 *data, UINT16 size)
{
	fprintf(stderr, "SPIWrite\t%03X,%02X\n", *data, size);
}


static void asfe_ctl_hal_sim_spi_read_byte (UINT16 addr, UINT16 *data)
{
	fprintf(stderr, "SPIRead \t%03X\n", addr);

	*data = 0xFF;
}


/******** UART emulated through stdin/stdout by default ********/


static void asfe_ctl_hal_sim_uart_send_byte (UINT8 val)
{
	int ret = write(1, &val, sizeof(UINT8));
	assert(ret >= 0);
}


static BOOL asfe_ctl_hal_sim_uart_receive_byte (UINT8 *val)
{
	int ret = read(0, val, sizeof(UINT8));
	assert(ret >= 0);
	return 1;
}


static struct asfe_ctl_hal asfe_ctl_hal_sim = 
{
	.HAL_gpioWrite          = asfe_ctl_hal_sim_gpio_write,
	.HAL_SPIWriteByteArray       = asfe_ctl_hal_sim_spi_write_byte,
	.HAL_SPIReadByte        = asfe_ctl_hal_sim_spi_read_byte,
	.Timer_Wait             = asfe_ctl_hal_sim_timer_wait,
	.HAL_uartSendByte       = asfe_ctl_hal_sim_uart_send_byte,
	.HAL_uartReceiveByte    = asfe_ctl_hal_sim_uart_receive_byte,
};


int asfe_ctl_hal_sim_attach (void)
{
	return asfe_ctl_hal_attach(&asfe_ctl_hal_sim);
}

