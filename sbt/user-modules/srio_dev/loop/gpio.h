/** \file      loop/gpio.h
 *  \brief     GPIO control/status for Loopback test
 *
 *  \copyright Copyright 2013,2014 Silver Bullet Technologies
 *
 *             This program is free software; you can redistribute it and/or modify it
 *             under the terms of the GNU General Public License as published by the Free
 *             Software Foundation; either version 2 of the License, or (at your option)
 *             any later version.
 *
 *             This program is distributed in the hope that it will be useful, but WITHOUT
 *             ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *             FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *             more details.
 *
 * vim:ts=4:noexpandtab
 */
#ifndef _INCLUDE_LOOP_GPIO_H_
#define _INCLUDE_LOOP_GPIO_H_
#include <linux/kernel.h>


int  sd_loop_gpio_init (void);
void sd_loop_gpio_exit (void);

void sd_loop_gpio_srio_reset (void);

size_t sd_loop_gpio_print_srio    (char *dst, size_t max);
size_t sd_loop_gpio_print_gt_prbs (char *dst, size_t max);
size_t sd_loop_gpio_print_counts  (char *dst, size_t max);

void sd_loop_gpio_set_pcie_srio_sel (int en);
int  sd_loop_gpio_get_pcie_srio_sel (void);

void sd_loop_gpio_set_gt_txdiffctrl   (int val);
int  sd_loop_gpio_get_gt_txdiffctrl   (void);
void sd_loop_gpio_set_gt_txprecursor  (int val);
int  sd_loop_gpio_get_gt_txprecursor  (void);
void sd_loop_gpio_set_gt_txpostcursor (int val);
int  sd_loop_gpio_get_gt_txpostcursor (void);


#endif /* _INCLUDE_LOOP_GPIO_H_ */
