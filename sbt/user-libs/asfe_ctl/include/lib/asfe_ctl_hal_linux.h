/** \file      include/lib/asfe_ctl_hal_linux.h
 *  \brief     HAL implementation for Linux
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
#ifndef _INCLUDE_LIB_ASFE_CTL_HAL_LINUX_H_
#define _INCLUDE_LIB_ASFE_CTL_HAL_LINUX_H_


int  asfe_ctl_hal_linux_attach   (void);
void asfe_ctl_hal_linux_cleanup  (void);


int  asfe_ctl_hal_linux_gpio_init (const int pins[3]);
int  asfe_ctl_hal_linux_set_gpio (int pin, int val);
void asfe_ctl_hal_linux_gpio_done (void);
int  asfe_ctl_hal_linux_spi_init  (const char *dev);
void asfe_ctl_hal_linux_spi_done  (void);
int  asfe_ctl_hal_linux_uart_init (const char *dev, const char *speed);
void asfe_ctl_hal_linux_uart_done (void);

int  asfe_ctl_hal_linux_spidev_txn  (int fd, int len, void *txb, void *rxb, int spd);
int  asfe_ctl_hal_linux_spidev_init (int fd);


#endif /* _INCLUDE_LIB_ASFE_CTL_HAL_H_ */
