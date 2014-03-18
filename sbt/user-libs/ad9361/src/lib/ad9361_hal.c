/** \file      src/lib/ad9361_hal.c
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
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

#include "api_types.h"
#include "ad9361_hal.h"
#include "ad9361_hal_linux.h"


const struct ad9361_hal *ad9361_hal = NULL;

static uint16_t ad9361_hal_spi_sniff[2][1024];

void ad9361_hal_spi_reg_clr (void)
{
	memset(ad9361_hal_spi_sniff, 0, sizeof(ad9361_hal_spi_sniff));
}

int ad9361_hal_attach (const struct ad9361_hal *hal)
{
	if ( ad9361_hal )
		return -1;

	ad9361_hal = hal;
	ad9361_hal_spi_reg_clr();
	return 0;
}


int ad9361_hal_detach (void)
{
	if ( !ad9361_hal )
		return -1;

	ad9361_hal = NULL;
	ad9361_hal_spi_reg_clr();
	return 0;
}


void ad9361_hal_spi_reg_set (unsigned dev, uint16_t addr, uint8_t data)
{
	if ( addr >= 1024 || dev >= 2 )
		return;

	ad9361_hal_spi_sniff[dev][addr]  = 0x8000;
	ad9361_hal_spi_sniff[dev][addr] |= data;
}

int ad9361_hal_spi_reg_get (unsigned dev, uint16_t addr)
{
	if ( addr >= 1024 || dev >= 2 )
	{
		errno = EINVAL;
		return -1;
	}

	if ( ! (ad9361_hal_spi_sniff[dev][addr] & 0x8000) )
	{
		errno = ENOENT;
		return -1;
	}

	return ad9361_hal_spi_sniff[dev][addr] & 0xFF;
}

void ad9361_spi_write_byte (unsigned dev, uint16_t addr, uint8_t data)
{
	assert(ad9361_hal);
	assert(ad9361_hal->spi_write_byte);
	ad9361_hal->spi_write_byte(dev, addr, data);
	ad9361_hal_spi_reg_set(dev, addr, data);
}

void ad9361_spi_read_byte (unsigned dev, uint16_t addr, uint8_t *data)
{
	assert(ad9361_hal);
	assert(ad9361_hal->spi_read_byte);
	ad9361_hal->spi_read_byte(dev, addr, data);
	ad9361_hal_spi_reg_set(dev, addr, *data);
}

void ad9361_gpio_write (unsigned dev, unsigned num, unsigned value)
{
	assert(ad9361_hal);
	assert(ad9361_hal->gpio_write);
	ad9361_hal->gpio_write(dev, num, value);
}

int ad9361_sysfs_read (unsigned dev, const char *root, const char *leaf,
                       void *dst, int max)
{
	assert(ad9361_hal);
	assert(ad9361_hal->sysfs_read);
	return ad9361_hal->sysfs_read(dev, root, leaf, dst, max);
}

int ad9361_sysfs_write (unsigned dev, const char *root, const char *leaf,
                        const void *src, int len)
{
	assert(ad9361_hal);
	assert(ad9361_hal->sysfs_write);
	return ad9361_hal->sysfs_write(dev, root, leaf, src, len);
}

int ad9361_sysfs_vscanf (unsigned dev, const char *root, const char *leaf,
                            const char *fmt, va_list ap)
{
	assert(ad9361_hal);
	assert(ad9361_hal->sysfs_vscanf);
	return ad9361_hal->sysfs_vscanf(dev, root, leaf, fmt, ap);
}

int ad9361_sysfs_vprintf (unsigned dev, const char *root, const char *leaf,
                          const char *fmt, va_list ap)
{
	assert(ad9361_hal);
	assert(ad9361_hal->sysfs_vprintf);
	return ad9361_hal->sysfs_vprintf(dev, root, leaf, fmt, ap);
}

int ad9361_sysfs_scanf (unsigned dev, const char *root, const char *leaf,
                        const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = ad9361_sysfs_vscanf(dev, root, leaf, fmt, ap);
	int err = errno;
	va_end(ap);

	errno = err;
	return ret;
}

int ad9361_sysfs_printf (unsigned dev, const char *root, const char *leaf,
                         const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = ad9361_sysfs_vprintf(dev, root, leaf, fmt, ap);
	int err = errno;
	va_end(ap);

	errno = err;
	return ret;
}
