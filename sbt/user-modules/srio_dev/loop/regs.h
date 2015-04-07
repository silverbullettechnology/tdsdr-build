/** \file      loop/regs.h
 *  \brief     SYS_REG type and bits
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
#ifndef _INCLUDE_LOOP_REGS_H_
#define _INCLUDE_LOOP_REGS_H_
#include <linux/kernel.h>


int  sd_loop_regs_init (void);
void sd_loop_regs_exit (void);

void sd_loop_regs_srio_reset          (void);

void sd_loop_regs_set_gt_loopback     (unsigned mode);
void sd_loop_regs_set_gt_diffctrl     (unsigned val);
void sd_loop_regs_set_gt_txprecursor  (unsigned val);
void sd_loop_regs_set_gt_txpostcursor (unsigned val);
void sd_loop_regs_set_gt_rxlpmen      (unsigned val);

unsigned sd_loop_regs_get_gt_loopback     (void);
unsigned sd_loop_regs_get_gt_diffctrl     (void);
unsigned sd_loop_regs_get_gt_txprecursor  (void);
unsigned sd_loop_regs_get_gt_txpostcursor (void);
unsigned sd_loop_regs_get_gt_rxlpmen      (void);

size_t sd_loop_regs_print_srio (char *dst, size_t max);


#endif /* _INCLUDE_LOOP_REGS_H_ */
