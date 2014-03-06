/** \file      src/lib/ad9361_hal_linux.c
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


static int ad9361_hal_linux_atexit_armed = 0;
static void ad9361_hal_linux_atexit_register (void)
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
	ad9361_hal_linux_gpio_done();
	ad9361_hal_linux_spi_done();
	ad9361_hal_linux_uart_done();
}


/******** Delay timer via usleep(), may add librt support in future ********/


static void ad9361_hal_linux_timer_wait (int usec)
{
	usleep(usec);
}


/******** GPIO access through /sys/class/gpio, future ********/

static struct gpio
{
	int  desc;
	int  gpio;
	int  init;
}
gpio[3] =
{
	[GPIO_TXNRX_pin]  = { desc: -1, init: 0 },
	[GPIO_Enable_pin] = { desc: -1, init: 0 },
	[GPIO_Resetn_pin] = { desc: -1, init: 1 },
};


static void ad9361_hal_linux_gpio_write (int pin, int val)
{
	char clr[2] = { '0', '\n' };
	char set[2] = { '1', '\n' };

	assert(pin >= 0);
	assert(pin <= 2);
	if ( gpio[pin].desc < 0 )
		return;

	lseek(gpio[pin].desc, 0, SEEK_SET);
	write(gpio[pin].desc, val ? set : clr, 2);
}


int ad9361_hal_linux_gpio_init (const int pins[3])
{
	int pin;
	for ( pin = 0; pin < 3; pin++ )
	{
		gpio[pin].gpio = pins[pin];
		if ( gpio[pin].gpio < 0 )
			continue;

		char path[PATH_MAX];
		snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", gpio[pin].gpio);

		// stat /sys tree, if not present try to export by pin number
		struct stat sb;
		if ( stat(path, &sb) && errno == ENOENT )
		{
			proc_printf("/sys/class/gpio/export", "%d\n", gpio[pin].gpio);
			if ( stat(path, &sb) )
				return -1;
		}

		// open file descriptor for value before checking direction
		snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio[pin].gpio);
		gpio[pin].desc = open(path, O_RDWR);
		if ( gpio[pin].desc < 0 )
			return -1;

		ad9361_hal_linux_atexit_register();

		// check direction, if already correct then move on
		char buff[16];
		snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio[pin].gpio);
		if ( proc_scanf(path, "%15s\n", buff) != 1 )
			return -1;
		if ( !strcmp(buff, "out") )
			continue;

		// set output value first using already-opened descriptor, then set direction
		ad9361_hal_linux_gpio_write(pin, gpio[pin].init);
		proc_printf(path, "out\n");
	}

	errno = 0;
	return 0;
}


void ad9361_hal_linux_gpio_done (void)
{
	int pin;
	for ( pin = 0; pin < 3; pin++ )
		if ( gpio[pin].desc > -1 )
		{
			close(gpio[pin].desc);
			gpio[pin].desc = -1;
		}
}


/******** UART emulated through stdin/stdout by default ********/


static int ad9361_hal_linux_spi_fd = -1;
static int ad9361_hal_linux_spi_spd = 100000;


static void ad9361_hal_linux_spi_write_byte (UINT16 addr, UINT16 data)
{
	UINT8  txb[3];
	int    ret;

	assert(ad9361_hal_linux_spi_fd > -1);
	txb[0] = addr >> 8;
	txb[1] = addr & 0xFF;
	txb[2] = data & 0xFF;

	txb[0] |= 0x80;

	ret = ad9361_hal_linux_spidev_txn(ad9361_hal_linux_spi_fd, 3, txb, NULL,
	                                  ad9361_hal_linux_spi_spd);
	assert(ret == 3);
}


static void ad9361_hal_linux_spi_read_byte (UINT16 addr, UINT16 *data)
{
	UINT8  txb[3];
	UINT8  rxb[3];
	int    ret;

	assert(ad9361_hal_linux_spi_fd > -1);
	txb[0] = addr >> 8;
	txb[1] = addr & 0xFF;
	txb[2] = 0;

	ret = ad9361_hal_linux_spidev_txn(ad9361_hal_linux_spi_fd, 3, txb, rxb,
	                                  ad9361_hal_linux_spi_spd);
	assert(ret == 3);

	*data = rxb[2];
}


int ad9361_hal_linux_spi_init (const char *dev)
{
	ad9361_hal_linux_spi_fd = open(dev, O_RDWR);
	if ( ad9361_hal_linux_spi_fd < 0 )
		return ad9361_hal_linux_spi_fd;

	// setup bus here with ioctl, if it fails abort.  Mode 0 (CPOL 0, CPHA 0), MSB first,
	// 8-bit words, 250kHz max speed
	if ( ad9361_hal_linux_spidev_init(ad9361_hal_linux_spi_fd) )
		goto fail;

	ad9361_hal_linux_atexit_register();
	return 0;

fail:
	close(ad9361_hal_linux_spi_fd);
	ad9361_hal_linux_spi_fd = -1;
	return 1;
}


void ad9361_hal_linux_spi_done (void)
{
	if ( ad9361_hal_linux_spi_fd > -1 )
		close(ad9361_hal_linux_spi_fd);
	ad9361_hal_linux_spi_fd = -1;
}


/******** UART emulated through stdin/stdout by default ********/


static int  ad9361_hal_linux_uart_fd_tx = 0;
static int  ad9361_hal_linux_uart_fd_rx = 1;


static void ad9361_hal_linux_uart_send_byte (UINT8 val)
{
	int ret = write(ad9361_hal_linux_uart_fd_tx, &val, sizeof(UINT8));
	assert(ret >= 0);
}


static BOOL ad9361_hal_linux_uart_receive_byte (UINT8 *val)
{
	int ret = read(ad9361_hal_linux_uart_fd_rx, val, sizeof(UINT8));
	assert(ret >= 0);
	return 1;
}


int ad9361_hal_linux_uart_init (const char *dev, const char *speed)
{
//	ad9361_hal_linux_atexit_register();
	errno = ENOSYS;
	return -1;
}


void ad9361_hal_linux_uart_done (void)
{
}


static struct ad9361_hal ad9361_hal_linux = 
{
	.HAL_gpioWrite          = ad9361_hal_linux_gpio_write,
	.HAL_SPIWriteByte       = ad9361_hal_linux_spi_write_byte,
	.HAL_SPIReadByte        = ad9361_hal_linux_spi_read_byte,
	.Timer_Wait             = ad9361_hal_linux_timer_wait,
	.HAL_uartSendByte       = ad9361_hal_linux_uart_send_byte,
	.HAL_uartReceiveByte    = ad9361_hal_linux_uart_receive_byte,
};


int ad9361_hal_linux_attach (void)
{
	return ad9361_hal_attach(&ad9361_hal_linux);
}

