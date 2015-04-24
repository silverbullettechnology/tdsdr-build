/** \file      src/lib/ad9361_hal_linux_dev.c
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
#include <glob.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include "lib.h"
#include "util.h"
#include "api_types.h"
#include "ad9361_hal.h"
#include "ad9361_hal_linux.h"
#include "ad9361_hal_linux_private.h"

struct gpio
{
	int  desc;
	int  gpio;
	int  init;
};

struct inst
{
	struct gpio  gpio[3];
	int          spi_fd;
	int          spi_spd;
	char        *sysfs;
};

static struct inst inst[2] =
{
	[0] =
	{
		gpio: 
		{
			[GPIO_TXNRX_pin]  = { desc: -1, init: 0 },
			[GPIO_Enable_pin] = { desc: -1, init: 0 },
			[GPIO_Resetn_pin] = { desc: -1, init: 1 },
		},
		spi_fd:  -1,
		spi_spd: 1000000,
		sysfs:   NULL,
	},
	[1] =
	{
		gpio: 
		{
			[GPIO_TXNRX_pin]  = { desc: -1, init: 0 },
			[GPIO_Enable_pin] = { desc: -1, init: 0 },
			[GPIO_Resetn_pin] = { desc: -1, init: 1 },
		},
		spi_fd:  -1,
		spi_spd: 1000000,
		sysfs:   NULL,
	},
};


/******** GPIO access through /sys/class/gpio ********/


static void ad9361_hal_linux_dev_gpio_write (unsigned dev, unsigned pin, unsigned val)
{
	char clr[2] = { '0', '\n' };
	char set[2] = { '1', '\n' };

	assert(dev < 2);

	assert(pin >= 0);
	assert(pin <= 2);
	if ( inst[dev].gpio[pin].desc < 0 )
		return;

	lseek(inst[dev].gpio[pin].desc, 0, SEEK_SET);
	write(inst[dev].gpio[pin].desc, val ? set : clr, 2);
}


int ad9361_hal_linux_dev_gpio_init (unsigned dev, const int pins[3])
{
	int pin;

	assert(dev < 2);

	for ( pin = 0; pin < 3; pin++ )
	{
		inst[dev].gpio[pin].gpio = pins[pin];
		if ( inst[dev].gpio[pin].gpio < 0 )
			continue;

		char path[PATH_MAX];
		snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", inst[dev].gpio[pin].gpio);

		// stat /sys tree, if not present try to export by pin number
		struct stat sb;
		if ( stat(path, &sb) && errno == ENOENT )
		{
			proc_printf("/sys/class/gpio/export", "%d\n", inst[dev].gpio[pin].gpio);
			if ( stat(path, &sb) )
				return -1;
		}

		// open file descriptor for value before checking direction
		snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value",
		         inst[dev].gpio[pin].gpio);
		inst[dev].gpio[pin].desc = open(path, O_RDWR);
		if ( inst[dev].gpio[pin].desc < 0 )
			return -1;

		ad9361_hal_linux_atexit_register();

		// check direction, if already correct then move on
		char buff[16];
		snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction",
		         inst[dev].gpio[pin].gpio);
		if ( proc_scanf(path, "%15s\n", buff) != 1 )
			return -1;
		if ( !strcmp(buff, "out") )
			continue;

		// set output value first using already-opened descriptor, then set direction
		ad9361_hal_linux_dev_gpio_write(dev, pin, inst[dev].gpio[pin].init);
		proc_printf(path, "out\n");
	}

	errno = 0;
	return 0;
}


void ad9361_hal_linux_dev_gpio_done (void)
{
	int dev, pin;

	for ( dev = 0; dev < 2; dev++ )
		for ( pin = 0; pin < 3; pin++ )
			if ( inst[dev].gpio[pin].desc > -1 )
			{
				close(inst[dev].gpio[pin].desc);
				inst[dev].gpio[pin].desc = -1;
			}
}


/******** SPI emulated through spidev ********/



static void ad9361_hal_linux_dev_spi_write_byte (unsigned dev, uint16_t addr, uint8_t data)
{
	uint8_t  txb[3];
	int      ret;

	assert(dev < 2);

	assert(inst[dev].spi_fd > -1);
	txb[0] = addr >> 8;
	txb[1] = addr & 0xFF;
	txb[2] = data;

	txb[0] |= 0x80;

	ret = ad9361_hal_linux_spidev_txn(inst[dev].spi_fd, 3, txb, NULL, inst[dev].spi_spd);
	assert(ret == 3);
}


static void ad9361_hal_linux_dev_spi_read_byte (unsigned dev, uint16_t addr, uint8_t *data)
{
	uint8_t  txb[3];
	uint8_t  rxb[3];
	int      ret;

	assert(dev < 2);

	assert(inst[dev].spi_fd > -1);
	txb[0] = addr >> 8;
	txb[1] = addr & 0xFF;
	txb[2] = 0;

	ret = ad9361_hal_linux_spidev_txn(inst[dev].spi_fd, 3, txb, rxb, inst[dev].spi_spd);
	assert(ret == 3);

	*data = rxb[2];
}


int ad9361_hal_linux_dev_spi_init (unsigned dev, const char *path)
{
	assert(dev < 2);

	inst[dev].spi_fd = open(path, O_RDWR);
	if ( inst[dev].spi_fd < 0 )
		return inst[dev].spi_fd;

	// setup bus here with ioctl, if it fails abort.  Mode 0 (CPOL 0, CPHA 0), MSB first,
	// 8-bit words, 250kHz max speed
	if ( ad9361_hal_linux_spidev_init(inst[dev].spi_fd) )
		goto fail;

	ad9361_hal_linux_atexit_register();
	return 0;

fail:
	close(inst[dev].spi_fd);
	inst[dev].spi_fd = -1;
	return 1;
}


void ad9361_hal_linux_dev_spi_done (void)
{
	int dev;

	for ( dev = 0; dev < 2; dev++ )
	{
		if ( inst[dev].spi_fd > -1 )
			close(inst[dev].spi_fd);
		inst[dev].spi_fd = -1;
	}
}


/******** sysfs/debugfs interface ********/


static int ad9361_hal_linux_dev_sysfs_read (unsigned dev, const char *root,
                                            const char *leaf, void *dst, int max)
{
	char  path[PATH_MAX];

	assert(dev < 2);
	if ( !inst[dev].sysfs )
		return -1;

	snprintf(path, sizeof(path), "%s/%s/%s", root, inst[dev].sysfs, leaf);
	int ret = proc_read(path, dst, max);
	if ( ret < 0 )
	{
		perror(path);
		return ret;
	}

	return ret;
}


static int ad9361_hal_linux_dev_sysfs_write (unsigned dev, const char *root,
                                             const char *leaf, const void *src, int len)
{
	char  path[PATH_MAX];

	assert(dev < 2);
	if ( !inst[dev].sysfs )
		return -1;

	snprintf(path, sizeof(path), "%s/%s/%s", root, inst[dev].sysfs, leaf);
	int ret = proc_write(path, src, len);
	if ( ret < 0 )
	{
		perror(path);
		return ret;
	}

	return ret;
}


static int ad9361_hal_linux_dev_sysfs_vscanf (unsigned dev,
                                              const char *root, const char *leaf,
                                              const char *fmt, va_list ap)
{
	assert(dev < 2);
	if ( !inst[dev].sysfs )
		return -1;

	char path[PATH_MAX];
	snprintf(path, sizeof(path), "%s/%s/%s", root, inst[dev].sysfs, leaf);
	int ret = proc_vscanf(path, fmt, ap);
	if ( ret < 0 )
	{
		perror(path);
		return ret;
	}

	return ret;
}

static int ad9361_hal_linux_dev_sysfs_vprintf (unsigned dev,
                                               const char *root, const char *leaf,
                                               const char *fmt, va_list ap)
{
	assert(dev < 2);
	if ( !inst[dev].sysfs )
		return -1;

	char path[PATH_MAX];
	snprintf(path, sizeof(path), "%s/%s/%s", root, inst[dev].sysfs, leaf);
	int ret = proc_vprintf(path, fmt, ap);
	if ( ret < 0 )
	{
		perror(path);
		return ret;
	}

	return ret;
}


int ad9361_hal_linux_dev_sysfs_init (void)
{
	char    path[PATH_MAX];
	char    buff[256];
	char   *p;
	glob_t  gl;
	int     i;
	int     c = 0;

	memset(&gl, 0, sizeof(gl));
	if ( glob("/sys/bus/iio/devices/iio:device*", 0, NULL, &gl) )
	{
		int err = errno;
		globfree(&gl);
		if ( err )
			perror("glob(/sys/bus/iio/devices/iio:device*)");
		errno = err;
		return -1;
	}

	for ( i = 0; c < 2 && i < gl.gl_pathc; i++ )
	{
		snprintf(path, sizeof(path), "%s/name", gl.gl_pathv[i]);
		if ( proc_read(path, buff, sizeof(buff)) < 0 )
			return -1;

		if ( strstr(buff, "ad9361-phy") != buff )
			continue;

		p = strstr(gl.gl_pathv[i], "iio:device");

		free(inst[c].sysfs);
		inst[c].sysfs = strdup(p);
		c++;
	}

	ad9361_hal_linux_atexit_register();
	globfree(&gl);
	return 2 - c;
}


void ad9361_hal_linux_dev_sysfs_done (void)
{
	int dev;

	for ( dev = 0; dev < 2; dev++ )
	{
		free(inst[dev].sysfs);
		inst[dev].sysfs = NULL;
	}
}


/******** sysfs/debugfs interface ********/


void ad9361_hal_linux_done (void)
{
	ad9361_hal_linux_dev_gpio_done();
	ad9361_hal_linux_dev_spi_done();
	ad9361_hal_linux_dev_sysfs_done();
}

static struct
{
	const char *node;
	int         gpio[3];
}
defaults[2] = 
{
	{ "/dev/spidev3.1", { 37 + 54,  36 + 54,  4 + 54 } },
	{ "/dev/spidev4.1", { 44 + 54,  43 + 54,  5 + 54 } },
};

void ad9361_hal_linux_defaults_set_spidev (unsigned dev, const char *path)
{
	assert(dev < 2);

	defaults[dev].node = path;
}

void ad9361_hal_linux_defaults_set_gpio (unsigned dev, const int   pins[3])
{
	int i;

	assert(dev < 2);

	for ( i = 0; i < 3; i++ )
		defaults[dev].gpio[i] = pins[i];
}

int ad9361_hal_linux_init (void)
{
	int dev;

	if ( ad9361_hal_linux_dev_sysfs_init() < 0 )
		return -1;

	for ( dev = 0; dev < 2; dev++ )
	{
		ad9361_hal_linux_dev_gpio_init(dev, defaults[dev].gpio);
		if ( ad9361_hal_linux_dev_spi_init(dev, defaults[dev].node) < 0 )
			return -1;
	}

	return 0;
}


/******** HAL glue and attach ********/


static struct ad9361_hal ad9361_hal_linux_dev = 
{
	.gpio_write       = ad9361_hal_linux_dev_gpio_write,
	.spi_write_byte   = ad9361_hal_linux_dev_spi_write_byte,
	.spi_read_byte    = ad9361_hal_linux_dev_spi_read_byte,
	.sysfs_read       = ad9361_hal_linux_dev_sysfs_read,
	.sysfs_write      = ad9361_hal_linux_dev_sysfs_write,
	.sysfs_vscanf     = ad9361_hal_linux_dev_sysfs_vscanf,
	.sysfs_vprintf    = ad9361_hal_linux_dev_sysfs_vprintf,
};

int ad9361_hal_linux_attach (void)
{
	return ad9361_hal_attach(&ad9361_hal_linux_dev);
}


