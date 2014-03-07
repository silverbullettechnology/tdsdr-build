/** \file      src/lib/ad9361_hal_sim.c
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

#include "api_types.h"
#include "ad9361_hal.h"
#include "ad9361_hal_sim.h"


static void ad9361_hal_sim_gpio_write (unsigned dev, unsigned pin, unsigned val)
{
//	fprintf(stderr, "ad9361_hal_sim_gpio_write(pin %d, val %d)\n", pin, val);
}


static void ad9361_hal_sim_spi_write_byte (unsigned dev, uint16_t addr, uint8_t data)
{
	fprintf(stderr, "SPIWrite\t%03X,%02X\n", addr, data);
}


static void ad9361_hal_sim_spi_read_byte (unsigned dev, uint16_t addr, uint8_t *data)
{
	fprintf(stderr, "SPIRead \t%03X\n", addr);

	*data = 0xFF;
}

static int ad9361_hal_sim_sysfs_read (unsigned dev, const char *root, const char *leaf,
                                      void *dst, int max)
{
	errno = ENOSYS;
	return -1;
}

static int ad9361_hal_sim_sysfs_write (unsigned dev, const char *root, const char *leaf,
                                       const void *src, int len)
{
	errno = ENOSYS;
	return -1;
}


static struct ad9361_hal ad9361_hal_sim = 
{
	.gpio_write     = ad9361_hal_sim_gpio_write,
	.spi_write_byte = ad9361_hal_sim_spi_write_byte,
	.spi_read_byte  = ad9361_hal_sim_spi_read_byte,
	.sysfs_read     = ad9361_hal_sim_sysfs_read,
	.sysfs_write    = ad9361_hal_sim_sysfs_write,
};


int ad9361_hal_sim_attach (void)
{
	return ad9361_hal_attach(&ad9361_hal_sim);
}

