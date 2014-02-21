/** \file      src/lib/asfe_ctl_hal_linux.c
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

// quash a warning about using kernel headers in userspace
#define __EXPORTED_HEADERS__
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "lib.h"
#include "util.h"
#include "api_types.h"
#include "asfe_ctl_hal.h"
#include "asfe_ctl_hal_linux.h"


static int asfe_ctl_hal_linux_atexit_armed = 0;
static void asfe_ctl_hal_linux_atexit_register (void)
{
	if ( asfe_ctl_hal_linux_atexit_armed )
		return;

	asfe_ctl_hal_linux_atexit_armed = 1;
	atexit(asfe_ctl_hal_linux_cleanup);
}

void asfe_ctl_hal_linux_cleanup (void)
{
	if ( !asfe_ctl_hal_linux_atexit_armed )
		return;

	asfe_ctl_hal_linux_atexit_armed = 0;
	asfe_ctl_hal_linux_gpio_done();
	asfe_ctl_hal_linux_spi_done();
	asfe_ctl_hal_linux_uart_done();
}


/******** Delay timer via usleep(), may add librt support in future ********/


static void asfe_ctl_hal_linux_timer_wait (int usec)
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


static void asfe_ctl_hal_linux_gpio_write (int pin, int val)
{
	char clr[2] = { '0', '\n' };
	char set[2] = { '1', '\n' };

	assert(pin >= 0);
	assert(pin <= 2);
	assert(gpio[pin].desc >= 0);

	lseek(gpio[pin].desc, 0, SEEK_SET);
	write(gpio[pin].desc, val ? set : clr, 2);
}


int asfe_ctl_hal_linux_gpio_init (const int pins[3])
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

		asfe_ctl_hal_linux_atexit_register();

		// check direction, if already correct then move on
		char buff[16];
		snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio[pin].gpio);
		if ( proc_scanf(path, "%15s\n", buff) != 1 )
			return -1;
		if ( !strcmp(buff, "out") )
			continue;

		// set output value first using already-opened descriptor, then set direction
		asfe_ctl_hal_linux_gpio_write(pin, gpio[pin].init);
		proc_printf(path, "out\n");
	}

	errno = 0;
	return 0;
}


void asfe_ctl_hal_linux_gpio_done (void)
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


static int asfe_ctl_hal_linux_spi_fd = -1;
static int asfe_ctl_hal_linux_spi_spd = 100000;


static void asfe_ctl_hal_linux_spi_write_byte_array (UINT16 *data, UINT16 size)
{
	struct spi_ioc_transfer xfr;
	UINT8                   txb[3];
	int                     ret;
	int			i;

	assert(asfe_ctl_hal_linux_spi_fd > -1);
	/*txb[0] = addr >> 8;
	txb[1] = addr & 0xFF;
	txb[2] = data & 0xFF;

	txb[0] |= 0x80;*/

	for(i = 0; i < size; i++){
		txb[i] = data[i];
	}		

	memset(&xfr, 0, sizeof(xfr));
	xfr.tx_buf = (unsigned long)txb;
    xfr.len    = size;

	xfr.speed_hz      = asfe_ctl_hal_linux_spi_spd;
	xfr.bits_per_word = 8;
	xfr.cs_change     = 0;
	xfr.delay_usecs   = 0;

	ret = ioctl(asfe_ctl_hal_linux_spi_fd, SPI_IOC_MESSAGE(1), &xfr);
	assert(ret == size);
}


static void asfe_ctl_hal_linux_spi_read_byte (UINT16 addr, UINT16 *data)
{
	struct spi_ioc_transfer xfr;
	UINT8                   txb[3];
	UINT8                   rxb[3];
	int                     ret;

	assert(asfe_ctl_hal_linux_spi_fd > -1);
	txb[0] = addr >> 8;
	txb[1] = addr & 0xFF;
	txb[2] = 0;

	memset(&xfr, 0, sizeof(xfr));
	xfr.tx_buf = (unsigned long)txb;
	xfr.rx_buf = (unsigned long)rxb;
    xfr.len    = 3;

	xfr.speed_hz      = asfe_ctl_hal_linux_spi_spd;
	xfr.bits_per_word = 8;
	xfr.cs_change     = 0;
	xfr.delay_usecs   = 0;

	ret = ioctl(asfe_ctl_hal_linux_spi_fd, SPI_IOC_MESSAGE(1), &xfr);
	assert(ret == 3);

	*data = rxb[2];
}


int asfe_ctl_hal_linux_spi_init (const char *dev)
{
	UINT8   u8;
	UINT32  u32;

	asfe_ctl_hal_linux_spi_fd = open(dev, O_RDWR);
	if ( asfe_ctl_hal_linux_spi_fd < 0 )
		return asfe_ctl_hal_linux_spi_fd;
	
	// setup bus here with ioctl, if it fails abort.  Mode 0 (CPOL 0, CPHA 0), MSB first,
	// 8-bit words, 250kHz max speed
	u8 = 0;
	if ( ioctl(asfe_ctl_hal_linux_spi_fd, SPI_IOC_WR_MODE, &u8) )
		goto fail;

	u8 = 0;
	if ( ioctl(asfe_ctl_hal_linux_spi_fd, SPI_IOC_WR_LSB_FIRST, &u8) )
		goto fail;

	u8 = 8;
	if ( ioctl(asfe_ctl_hal_linux_spi_fd, SPI_IOC_WR_BITS_PER_WORD, &u8) )
		goto fail;

	u32 = 250000;
	if ( ioctl(asfe_ctl_hal_linux_spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &u32) )
		goto fail;

	asfe_ctl_hal_linux_atexit_register();
	return 0;

fail:
	close(asfe_ctl_hal_linux_spi_fd);
	asfe_ctl_hal_linux_spi_fd = -1;
	return 1;
}


void asfe_ctl_hal_linux_spi_done (void)
{
	if ( asfe_ctl_hal_linux_spi_fd > -1 )
		close(asfe_ctl_hal_linux_spi_fd);
	asfe_ctl_hal_linux_spi_fd = -1;
}


/******** UART emulated through stdin/stdout by default ********/


static int  asfe_ctl_hal_linux_uart_fd_tx = 0;
static int  asfe_ctl_hal_linux_uart_fd_rx = 1;


static void asfe_ctl_hal_linux_uart_send_byte (UINT8 val)
{
	int ret = write(asfe_ctl_hal_linux_uart_fd_tx, &val, sizeof(UINT8));
	assert(ret >= 0);
}


static BOOL asfe_ctl_hal_linux_uart_receive_byte (UINT8 *val)
{
	int ret = read(asfe_ctl_hal_linux_uart_fd_rx, val, sizeof(UINT8));
	assert(ret >= 0);
	return 1;
}


int asfe_ctl_hal_linux_uart_init (const char *dev, const char *speed)
{
//	asfe_ctl_hal_linux_atexit_register();
	errno = ENOSYS;
	return -1;
}


void asfe_ctl_hal_linux_uart_done (void)
{
}


static struct asfe_ctl_hal asfe_ctl_hal_linux = 
{
	.HAL_gpioWrite          = asfe_ctl_hal_linux_gpio_write,
	.HAL_SPIWriteByteArray  = asfe_ctl_hal_linux_spi_write_byte_array,
	.HAL_SPIReadByte        = asfe_ctl_hal_linux_spi_read_byte,
	.Timer_Wait             = asfe_ctl_hal_linux_timer_wait,
	.HAL_uartSendByte       = asfe_ctl_hal_linux_uart_send_byte,
	.HAL_uartReceiveByte    = asfe_ctl_hal_linux_uart_receive_byte,
};


int asfe_ctl_hal_linux_attach (void)
{
	return asfe_ctl_hal_attach(&asfe_ctl_hal_linux);
}

