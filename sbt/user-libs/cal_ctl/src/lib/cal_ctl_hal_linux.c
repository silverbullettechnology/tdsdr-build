/** \file      src/lib/cal_ctl_hal_linux.c
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
#include "cal_ctl_hal.h"
#include "cal_ctl_hal_linux.h"


static int cal_ctl_hal_linux_atexit_armed = 0;
static void cal_ctl_hal_linux_atexit_register (void)
{
	if ( cal_ctl_hal_linux_atexit_armed )
		return;

	cal_ctl_hal_linux_atexit_armed = 1;
	atexit(cal_ctl_hal_linux_cleanup);
}

void cal_ctl_hal_linux_cleanup (void)
{
	if ( !cal_ctl_hal_linux_atexit_armed )
		return;

	cal_ctl_hal_linux_atexit_armed = 0;
	cal_ctl_hal_linux_gpio_done();
	cal_ctl_hal_linux_spi_done();
	cal_ctl_hal_linux_uart_done();
}


/******** Delay timer via usleep(), may add librt support in future ********/


static void cal_ctl_hal_linux_timer_wait (int usec)
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


static void cal_ctl_hal_linux_gpio_write (int pin, int val)
{
	char clr[2] = { '0', '\n' };
	char set[2] = { '1', '\n' };

	assert(pin >= 0);
	assert(pin <= 2);
	assert(gpio[pin].desc >= 0);

	lseek(gpio[pin].desc, 0, SEEK_SET);
	write(gpio[pin].desc, val ? set : clr, 2);
}


int cal_ctl_hal_linux_gpio_init (const int pins[3])
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

		cal_ctl_hal_linux_atexit_register();

		// check direction, if already correct then move on
		char buff[16];
		snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio[pin].gpio);
		if ( proc_scanf(path, "%15s\n", buff) != 1 )
			return -1;
		if ( !strcmp(buff, "out") )
			continue;

		// set output value first using already-opened descriptor, then set direction
		cal_ctl_hal_linux_gpio_write(pin, gpio[pin].init);
		proc_printf(path, "out\n");
	}

	errno = 0;
	return 0;
}

int cal_ctl_hal_linux_set_gpio (int pin, int val)
{
	char path[PATH_MAX];
	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", pin);

	// stat /sys tree, if not present try to export by pin number
	struct stat sb;
	if ( stat(path, &sb) && errno == ENOENT )
	{
		proc_printf("/sys/class/gpio/export", "%d\n", pin);
		if ( stat(path, &sb) )
			return -1;
	}

	// check direction, if already correct then move on
	char buff[16];
	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
	if ( proc_scanf(path, "%15s\n", buff) != 1 )
		return -1;
	if ( strcmp(buff, "out") )
		proc_printf(path, "out\n");

	// open file descriptor for value before checking direction
	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
	if ( proc_printf(path, "%d\n", val) < 0 )
		return -1;

	return 0;
}

void cal_ctl_hal_linux_gpio_done (void)
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


static int cal_ctl_hal_linux_spi_fd = -1;
static int cal_ctl_hal_linux_spi_spd = 100000;


static void cal_ctl_hal_linux_spi_write_byte_array (UINT16 *data, UINT16 size)
{
	UINT8  txb[256];
	int    ret;
	int    i;

	assert(cal_ctl_hal_linux_spi_fd > -1);

	for(i = 0; i < size; i++){
		txb[i] = data[i];
	}		

	ret = cal_ctl_hal_linux_spidev_txn(cal_ctl_hal_linux_spi_fd, size, txb, NULL,
	                                    cal_ctl_hal_linux_spi_spd);
	assert(ret == size);
}

static void cal_ctl_hal_linux_spi_exchange_byte_array (UINT16 *wr_data, UINT16 *rd_data, UINT16 size)
{
	UINT8  txb[256];
	UINT8  rxb[256];
	int    ret;
	int    i;

	assert(cal_ctl_hal_linux_spi_fd > -1);

	for(i = 0; i < size; i++){
		txb[i] = wr_data[i];
	}		

	ret = cal_ctl_hal_linux_spidev_txn(cal_ctl_hal_linux_spi_fd, size, txb, rxb,
	                                    cal_ctl_hal_linux_spi_spd);
	assert(ret == size);

	for(i = 0; i < size; i++){
		rxb[i] = rd_data[i];
	}

}


static void cal_ctl_hal_linux_spi_read_byte (UINT16 addr, UINT16 *data)
{
	UINT8  txb[3];
	UINT8  rxb[3];
	int    ret;

	assert(cal_ctl_hal_linux_spi_fd > -1);
	txb[0] = addr >> 8;
	txb[1] = addr & 0xFF;
	txb[2] = 0;

	ret = cal_ctl_hal_linux_spidev_txn(cal_ctl_hal_linux_spi_fd, 3, txb, rxb,
	                                    cal_ctl_hal_linux_spi_spd);
	assert(ret == 3);

	*data = rxb[2];
}


int cal_ctl_hal_linux_spi_init (const char *dev)
{
	cal_ctl_hal_linux_spi_fd = open(dev, O_RDWR);
	if ( cal_ctl_hal_linux_spi_fd < 0 )
		return cal_ctl_hal_linux_spi_fd;
	
	// setup bus here with ioctl, if it fails abort.  Mode 0 (CPOL 0, CPHA 0), MSB first,
	// 8-bit words, 250kHz max speed
	if ( cal_ctl_hal_linux_spidev_init(cal_ctl_hal_linux_spi_fd) )
		goto fail;

	cal_ctl_hal_linux_atexit_register();
	return 0;

fail:
	close(cal_ctl_hal_linux_spi_fd);
	cal_ctl_hal_linux_spi_fd = -1;
	return 1;
}


void cal_ctl_hal_linux_spi_done (void)
{
	if ( cal_ctl_hal_linux_spi_fd > -1 )
		close(cal_ctl_hal_linux_spi_fd);
	cal_ctl_hal_linux_spi_fd = -1;
}


/******** UART emulated through stdin/stdout by default ********/


static int  cal_ctl_hal_linux_uart_fd_tx = 0;
static int  cal_ctl_hal_linux_uart_fd_rx = 1;


static void cal_ctl_hal_linux_uart_send_byte (UINT8 val)
{
	int ret = write(cal_ctl_hal_linux_uart_fd_tx, &val, sizeof(UINT8));
	assert(ret >= 0);
}


static BOOL cal_ctl_hal_linux_uart_receive_byte (UINT8 *val)
{
	int ret = read(cal_ctl_hal_linux_uart_fd_rx, val, sizeof(UINT8));
	assert(ret >= 0);
	return 1;
}


int cal_ctl_hal_linux_uart_init (const char *dev, const char *speed)
{
//	cal_ctl_hal_linux_atexit_register();
	errno = ENOSYS;
	return -1;
}


void cal_ctl_hal_linux_uart_done (void)
{
}


static struct cal_ctl_hal cal_ctl_hal_linux = 
{
	.HAL_gpioWrite          = cal_ctl_hal_linux_gpio_write,
	.HAL_SPIWriteByteArray  = cal_ctl_hal_linux_spi_write_byte_array,
	.HAL_SPIExchangeByteArray  = cal_ctl_hal_linux_spi_exchange_byte_array,
	.HAL_SPIReadByte        = cal_ctl_hal_linux_spi_read_byte,
	.Timer_Wait             = cal_ctl_hal_linux_timer_wait,
	.HAL_uartSendByte       = cal_ctl_hal_linux_uart_send_byte,
	.HAL_uartReceiveByte    = cal_ctl_hal_linux_uart_receive_byte,
};


int cal_ctl_hal_linux_attach (void)
{
	return cal_ctl_hal_attach(&cal_ctl_hal_linux);
}

