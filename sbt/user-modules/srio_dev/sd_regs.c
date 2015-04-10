/** \file      sd_regs.c
 *  \brief     SYS_REG control
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
#include <linux/delay.h>

#include "srio_dev.h"
#include "sd_regs.h"


#define CTRL_SRIO_RESET       0x00000001
#define CTRL_SRIO_LOOPBACK    0x0000001C
#define CTRL_GT_DIFFCTRL      0x000001E0
#define CTRL_GT_TXPRECURSOR   0x00003E00
#define CTRL_GT_TXPOSTCURSOR  0x0007C000
#define CTRL_GT_RXLPMEN       0x00080000

#define STAT_SRIO_LINK_INITIALIZED  0x00000001
#define STAT_SRIO_PORT_INITIALIZED  0x00000002
#define STAT_SRIO_CLOCK_OUT_LOCK    0x00000004
#define STAT_SRIO_MODE_1X           0x00000008
#define STAT_PORT_ERROR             0x00000010
#define STAT_GTRX_NOTINTABLE_OR     0x00000020
#define STAT_GTRX_DISPERR_OR        0x00000040
#define STAT_DEVICE_ID              0xFF000000


void sd_regs_srio_reset (struct srio_dev *sd)
{
	REG_RMW(&sd->sys_regs->ctrl, 0, CTRL_SRIO_RESET);
	msleep(10);
	REG_RMW(&sd->sys_regs->ctrl, CTRL_SRIO_RESET, 0);
	msleep(10);
}


size_t sd_regs_print_srio (struct srio_dev *sd, char *dst, size_t max)
{
	uint32_t  v = REG_READ(&sd->sys_regs->ctrl);
	char     *p = dst;
	char     *e = dst + max;

	p += scnprintf(p, e - p, "ctrl:\n");
	p += scnprintf(p, e - p, "  srio_reset     : %u\n", (v & CTRL_SRIO_RESET     )      );
	p += scnprintf(p, e - p, "  srio_loopback  : %u\n", (v & CTRL_SRIO_LOOPBACK  ) >>  2);
	p += scnprintf(p, e - p, "  gt_diffctrl    : %u\n", (v & CTRL_GT_DIFFCTRL    ) >>  5);
	p += scnprintf(p, e - p, "  gt_txprecursor : %u\n", (v & CTRL_GT_TXPRECURSOR ) >>  9);
	p += scnprintf(p, e - p, "  gt_txpostcursor: %u\n", (v & CTRL_GT_TXPOSTCURSOR) >> 14);
	p += scnprintf(p, e - p, "  gt_rxlpmen     : %u\n", (v & CTRL_GT_RXLPMEN     ) >> 19);

	v = REG_READ(&sd->sys_regs->stat);
	p += scnprintf(p, e - p, "\nstatus:\n");
	p += scnprintf(p, e - p, "  srio_link_initialized: %u\n", (v & STAT_SRIO_LINK_INITIALIZED)      );
	p += scnprintf(p, e - p, "  srio_port_initialized: %u\n", (v & STAT_SRIO_PORT_INITIALIZED) >>  1);
	p += scnprintf(p, e - p, "  srio_clock_out_lock  : %u\n", (v & STAT_SRIO_CLOCK_OUT_LOCK  ) >>  2);
	p += scnprintf(p, e - p, "  srio_mode_1x         : %u\n", (v & STAT_SRIO_MODE_1X         ) >>  3);
	p += scnprintf(p, e - p, "  port_error           : %u\n", (v & STAT_PORT_ERROR           ) >>  4);
	p += scnprintf(p, e - p, "  gtrx_notintable_or   : %u\n", (v & STAT_GTRX_NOTINTABLE_OR   ) >>  5);
	p += scnprintf(p, e - p, "  gtrx_disperr_or      : %u\n", (v & STAT_GTRX_DISPERR_OR      ) >>  6);
	p += scnprintf(p, e - p, "  device_id            : %u\n", (v & STAT_DEVICE_ID            ) >> 24);
	p += scnprintf(p, e - p, "\n");

	return p - dst;
}


void sd_regs_set_gt_loopback (struct srio_dev *sd, unsigned mode)
{
	pr_debug("Set gt_loopback to %u\n", mode);
	REG_RMW(&sd->sys_regs->ctrl, CTRL_SRIO_LOOPBACK, (mode << 2) & CTRL_SRIO_LOOPBACK);
}

void sd_regs_set_gt_diffctrl (struct srio_dev *sd, unsigned val)
{
	pr_debug("Set gt_diffctrl to %u\n", val);
	REG_RMW(&sd->sys_regs->ctrl, CTRL_GT_DIFFCTRL, (val << 5) & CTRL_GT_DIFFCTRL);
}

void sd_regs_set_gt_txprecursor (struct srio_dev *sd, unsigned val)
{
	pr_debug("Set gt_txprecursor to %u\n", val);
	REG_RMW(&sd->sys_regs->ctrl, CTRL_GT_TXPRECURSOR, (val << 9) & CTRL_GT_TXPRECURSOR);
}

void sd_regs_set_gt_txpostcursor (struct srio_dev *sd, unsigned val)
{
	pr_debug("Set gt_txpostcursor to %u\n", val);
	REG_RMW(&sd->sys_regs->ctrl, CTRL_GT_TXPOSTCURSOR, (val << 14) & CTRL_GT_TXPOSTCURSOR);
}

void sd_regs_set_gt_rxlpmen (struct srio_dev *sd, unsigned val)
{
	pr_debug("Set gt_rxlpmen to %u\n", val);
	REG_RMW(&sd->sys_regs->ctrl, CTRL_GT_RXLPMEN, (val << 19) & CTRL_GT_RXLPMEN);
}


unsigned sd_regs_get_gt_loopback (struct srio_dev *sd)
{
	return (REG_READ(&sd->sys_regs->ctrl) & CTRL_SRIO_LOOPBACK) >> 2;
}

unsigned sd_regs_get_gt_diffctrl (struct srio_dev *sd)
{
	return (REG_READ(&sd->sys_regs->ctrl) & CTRL_GT_DIFFCTRL) >> 5;
}

unsigned sd_regs_get_gt_txprecursor (struct srio_dev *sd)
{
	return (REG_READ(&sd->sys_regs->ctrl) & CTRL_GT_TXPRECURSOR) >> 9;
}

unsigned sd_regs_get_gt_txpostcursor (struct srio_dev *sd)
{
	return (REG_READ(&sd->sys_regs->ctrl) & CTRL_GT_TXPOSTCURSOR) >> 14;
}

unsigned sd_regs_get_gt_rxlpmen (struct srio_dev *sd)
{
	return (REG_READ(&sd->sys_regs->ctrl) & CTRL_GT_RXLPMEN) >> 19;
}
