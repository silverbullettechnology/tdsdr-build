/** \file      dsxx/regs.c
 *  \brief     Register access to data source/sink and pause
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
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/ioport.h>

#include "srio_dev.h"
#include "dsxx/regs.h"


struct sd_dsrc_regs __iomem  *sd_dsrc_regs  = NULL;
struct sd_dsnk_regs __iomem  *sd_dsnk_regs  = NULL;
struct sd_pause_regs __iomem *sd_dsrc_pause = NULL;
struct sd_pause_regs __iomem *sd_dsnk_pause = NULL;


/******* DSRC regs *******/


void sd_dsrc_regs_ctrl (uint32_t val)
{
	REG_WRITE(&sd_dsrc_regs->ctrl, val);
}

uint32_t sd_dsrc_regs_stat (void)
{
	return REG_READ(&sd_dsrc_regs->stat);
}

void sd_dsrc_regs_bytes (uint32_t val)
{
	REG_WRITE(&sd_dsrc_regs->bytes, val);
}

void sd_dsrc_regs_reps (uint32_t val)
{
	REG_WRITE(&sd_dsrc_regs->reps, val);
}

void sd_dsrc_regs_type (uint32_t val)
{
	REG_WRITE(&sd_dsrc_regs->type, val);
}

void sd_dsrc_regs_dump (void)
{
	pr_debug("DSRC regs:\n");
	pr_debug("  ctrl  = %08x\n", REG_READ(&sd_dsrc_regs->ctrl));
	pr_debug("  stat  = %08x\n", REG_READ(&sd_dsrc_regs->stat));
	pr_debug("  bytes = %08x\n", REG_READ(&sd_dsrc_regs->bytes));
	pr_debug("  bsent = %08x\n", REG_READ(&sd_dsrc_regs->bsent));
	pr_debug("  type  = %08x\n", REG_READ(&sd_dsrc_regs->type));
	pr_debug("  reps  = %08x\n", REG_READ(&sd_dsrc_regs->reps));
	pr_debug("  rsent = %08x\n", REG_READ(&sd_dsrc_regs->rsent));
}


/******* DSRC Pause regs *******/


void sd_dsrc_pause_ctrl (uint32_t val)
{
	REG_WRITE(&sd_dsrc_pause->ctrl, val);
}

uint32_t sd_dsrc_pause_stat (void)
{
	return REG_READ(&sd_dsrc_pause->stat);
}

void sd_dsrc_pause_on (uint32_t val)
{
	REG_WRITE(&sd_dsrc_pause->on, val);
}

void sd_dsrc_pause_off (uint32_t val)
{
	REG_WRITE(&sd_dsrc_pause->off, val);
}


/******* DSNK regs *******/


void sd_dsnk_regs_ctrl (uint32_t val)
{
	REG_WRITE(&sd_dsnk_regs->ctrl, val);
}

uint32_t sd_dsnk_regs_stat (void)
{
	return REG_READ(&sd_dsnk_regs->stat);
}

uint64_t sd_dsnk_regs_sum64 (void)
{
	uint32_t sum_h = REG_READ(&sd_dsnk_regs->sum_h);
	uint32_t sum_l = REG_READ(&sd_dsnk_regs->sum_l);
	uint64_t sum64 = sum_h;
	sum64 <<= 32;
	sum64  |= sum_l;
	return sum64;
}

void sd_dsnk_regs_dump (void)
{
	pr_debug("DSNK regs:\n");
	pr_debug("  ctrl  = %08x\n", REG_READ(&sd_dsnk_regs->ctrl));
	pr_debug("  stat  = %08x\n", REG_READ(&sd_dsnk_regs->stat));
	pr_debug("  bytes = %08x\n", REG_READ(&sd_dsnk_regs->bytes));
	pr_debug("  sum_h = %08x\n", REG_READ(&sd_dsnk_regs->sum_h));
	pr_debug("  sum_l = %08x\n", REG_READ(&sd_dsnk_regs->sum_l));
}


/******* DSNK Pause regs *******/


void sd_dsnk_pause_ctrl (uint32_t val)
{
	REG_WRITE(&sd_dsnk_pause->ctrl, val);
}

uint32_t sd_dsnk_pause_stat (void)
{
	return REG_READ(&sd_dsnk_pause->stat);
}

void sd_dsnk_pause_on (uint32_t val)
{
	REG_WRITE(&sd_dsnk_pause->on, val);
}

void sd_dsnk_pause_off (uint32_t val)
{
	REG_WRITE(&sd_dsnk_pause->off, val);
}


