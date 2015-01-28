/** \file      dsxx/regs.c
 *  \brief     Definitions for data source/sink registers
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
#ifndef _INCLUDE_DSXX_REGS_H_
#define _INCLUDE_DSXX_REGS_H_
#include <linux/kernel.h>


#define SD_DSXX_CTRL_ENABLE         0x01
#define SD_DSXX_CTRL_RESET          0x02
#define SD_DSXX_CTRL_DISABLE        0x03
#define SD_DSXX_STAT_ENABLED        (1 << 0)
#define SD_DSXX_STAT_DONE           (1 << 1)
#define SD_DSXX_STAT_ERROR          (1 << 2)
#define SD_DSRC_TYPE_INCREMENT      0x00
#define SD_DSRC_TYPE_DECREMENT      0x01

#define SD_DSXX_PAUSE_CTRL_ENABLE   0x01
#define SD_DSXX_PAUSE_CTRL_RESET    0x02
#define SD_DSXX_PAUSE_CTRL_DISABLE  0x03
#define SD_DSXX_PAUSE_STAT_ENABLED  (1 << 0)
#define SD_DSXX_PAUSE_STAT_ACTIVE   (1 << 1)


/* Data source / sink registers - structs, pointers, accessors */
struct sd_dsrc_regs
{
	uint32_t  ctrl;  // DSRC_CTRL W
	uint32_t  stat;  // DSRC_STAT R
	uint32_t  bytes; // DSRC_BYTES RW
	uint32_t  bsent; // DSRC_BYTES_SENT R
	uint32_t  type;  // DSRC_DATA_TYPE RW
	uint32_t  reps;  // DSRC_REPS RW
	uint32_t  rsent; // DSRC_REPS_SENT R
};

struct sd_dsnk_regs
{
	uint32_t  ctrl;  // DSNK_CTRL W
	uint32_t  stat;  // DSNK_STAT R
	uint32_t  bytes; // DSNK_BYTES R
	uint32_t  sum_h; // DSNK_CHKSUM_H R
	uint32_t  sum_l; // DSNK_CHKSUM_L R
};

struct sd_pause_regs
{
	uint32_t  ctrl; // DPAUSE_CTRL W
	uint32_t  stat; // DPAUSE_STAT R
	uint32_t  on;   // DPAUSE_ONCYCLE RW
	uint32_t  off;  // DPAUSE_OFFCYCLE R
};

extern struct sd_dsrc_regs __iomem  *sd_dsrc_regs;
extern struct sd_dsnk_regs __iomem  *sd_dsnk_regs;
extern struct sd_pause_regs __iomem *sd_dsrc_pause;
extern struct sd_pause_regs __iomem *sd_dsnk_pause;

void     sd_dsrc_regs_ctrl  (uint32_t val);
uint32_t sd_dsrc_regs_stat  (void);
void     sd_dsrc_regs_bytes (uint32_t val);
void     sd_dsrc_regs_reps  (uint32_t val);
void     sd_dsrc_regs_type  (uint32_t val);
void     sd_dsrc_regs_dump  (void);

void     sd_dsrc_pause_ctrl (uint32_t val);
uint32_t sd_dsrc_pause_stat (void);
void     sd_dsrc_pause_on   (uint32_t val);
void     sd_dsrc_pause_off  (uint32_t val);

void     sd_dsnk_regs_ctrl  (uint32_t val);
uint32_t sd_dsnk_regs_stat  (void);
uint64_t sd_dsnk_regs_sum64 (void);
void     sd_dsrc_regs_dump  (void);

void     sd_dsnk_pause_ctrl (uint32_t val);
uint32_t sd_dsnk_pause_stat (void);
void     sd_dsnk_pause_on   (uint32_t val);
void     sd_dsnk_pause_off  (uint32_t val);


#endif // _INCLUDE_DSXX_REGS_H_
