/** \file      include/lib/hal.h
 *  \brief     HAL structure 
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
#ifndef _INCLUDE_LIB_HAL_H_
#define _INCLUDE_LIB_HAL_H_
#include <stdint.h>
#include <assert.h>

#include "api_types.h"


typedef void  (* spi_write_byte_fn)  (unsigned dev, uint16_t addr, uint8_t  data);
typedef void  (* spi_read_byte_fn)   (unsigned dev, uint16_t addr, uint8_t *data);
typedef void  (* gpio_write_fn)      (unsigned dev, unsigned num,  unsigned value);
typedef int   (* sysfs_read_fn)      (unsigned dev, const char *root, const char *leaf,
                                      void *dst, int max);
typedef int   (* sysfs_write_fn)     (unsigned dev, const char *root, const char *leaf,
                                      const void *src, int len);


struct ad9361_hal
{
	spi_write_byte_fn        spi_write_byte;
	spi_read_byte_fn         spi_read_byte;
	gpio_write_fn            gpio_write;
	sysfs_read_fn            sysfs_read;
	sysfs_write_fn           sysfs_write;
};


void ad9361_spi_write_byte (unsigned dev, uint16_t addr, uint8_t data);
void ad9361_spi_read_byte  (unsigned dev, uint16_t addr, uint8_t *data);
void ad9361_gpio_write     (unsigned dev, unsigned num, unsigned value);
int  ad9361_sysfs_read     (unsigned dev, const char *root, const char *leaf,
                            void *dst, int max);
int  ad9361_sysfs_write    (unsigned dev, const char *root, const char *leaf,
                            const void *src, int len);


int ad9361_hal_attach (const struct ad9361_hal *hal);
int ad9361_hal_detach (void);

int  ad9361_hal_spi_reg_get (unsigned dev, uint16_t addr);
void ad9361_hal_spi_reg_clr (void);


#endif /* _INCLUDE_LIB_HAL_H_ */
