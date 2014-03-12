/** \file      src/lib/ad9361_hal_linux_private.h
 *  \brief     HAL internals
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
#ifndef _SRC_LIB_AD9361_HAL_LINUX_PRIVATE_H_
#define _SRC_LIB_AD9361_HAL_LINUX_PRIVATE_H_


void ad9361_hal_linux_atexit_register (void);
int  ad9361_hal_linux_dev_attach      (void);

int  ad9361_hal_linux_dev_gpio_init  (unsigned dev, const int pins[3]);
void ad9361_hal_linux_dev_gpio_done  (void);
int  ad9361_hal_linux_dev_spi_init   (unsigned dev, const char *path);
void ad9361_hal_linux_dev_spi_done   (void);
int  ad9361_hal_linux_dev_sysfs_init (void);
void ad9361_hal_linux_dev_sysfs_done (void);


#endif /* _SRC_LIB_AD9361_HAL_LINUX_PRIVATE_H_ */

