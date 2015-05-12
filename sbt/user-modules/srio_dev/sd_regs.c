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


void sd_regs_srio_reset (struct srio_dev *sd)
{
	REG_RMW(&sd->sys_regs->ctrl, 0, SD_SR_CTRL_SRIO_RESET_M);
	msleep(10);
	REG_RMW(&sd->sys_regs->ctrl, SD_SR_CTRL_SRIO_RESET_M, 0);
	msleep(10);
}

void sd_regs_gt_srio_rxdfelpmreset (struct srio_dev *sd)
{
	REG_RMW(&sd->sys_regs->ctrl, 0, SD_SR_CTRL_GT_SRIO_RXDFELPMRESET_M);
	msleep(10);
	REG_RMW(&sd->sys_regs->ctrl, SD_SR_CTRL_GT_SRIO_RXDFELPMRESET_M, 0);
	msleep(10);
}

void sd_regs_gt_phy_link_reset (struct srio_dev *sd)
{
	REG_RMW(&sd->sys_regs->ctrl, 0, SD_SR_CTRL_GT_PHY_LINK_RESET_M);
	msleep(10);
	REG_RMW(&sd->sys_regs->ctrl, SD_SR_CTRL_GT_PHY_LINK_RESET_M, 0);
	msleep(10);
}

void sd_regs_gt_force_reinit (struct srio_dev *sd)
{
	REG_RMW(&sd->sys_regs->ctrl, 0, SD_SR_CTRL_GT_FORCE_REINIT_M);
	msleep(10);
	REG_RMW(&sd->sys_regs->ctrl, SD_SR_CTRL_GT_FORCE_REINIT_M, 0);
	msleep(10);
}

void sd_regs_gt_phy_mce (struct srio_dev *sd)
{
	REG_RMW(&sd->sys_regs->ctrl, 0, SD_SR_CTRL_GT_PHY_MCE_M);
	msleep(10);
	REG_RMW(&sd->sys_regs->ctrl, SD_SR_CTRL_GT_PHY_MCE_M, 0);
	msleep(10);
}

unsigned sd_regs_srio_status (struct srio_dev *sd)
{
	return REG_READ(&sd->sys_regs->stat);
}


size_t sd_regs_print_srio (struct srio_dev *sd, char *dst, size_t max)
{
	uint32_t  v = REG_READ(&sd->sys_regs->ctrl);
	char     *p = dst;
	char     *e = dst + max;

	p += scnprintf(p, e - p, "ctrl:\n");
	p += scnprintf(p, e - p, "  srio_reset     : %u\n", (v & SD_SR_CTRL_SRIO_RESET_M     ) >> SD_SR_CTRL_SRIO_RESET_S     );
	p += scnprintf(p, e - p, "  swrite_bypass  : %u\n", (v & SD_SR_CTRL_SWRITE_BYPASS_M  ) >> SD_SR_CTRL_SWRITE_BYPASS_S  );
	p += scnprintf(p, e - p, "  srio_loopback  : %u\n", (v & SD_SR_CTRL_SRIO_LOOPBACK_M  ) >> SD_SR_CTRL_SRIO_LOOPBACK_S  );
	p += scnprintf(p, e - p, "  gt_diffctrl    : %u\n", (v & SD_SR_CTRL_GT_DIFFCTRL_M    ) >> SD_SR_CTRL_GT_DIFFCTRL_S    );
	p += scnprintf(p, e - p, "  gt_txprecursor : %u\n", (v & SD_SR_CTRL_GT_TXPRECURSOR_M ) >> SD_SR_CTRL_GT_TXPRECURSOR_S );
	p += scnprintf(p, e - p, "  gt_txpostcursor: %u\n", (v & SD_SR_CTRL_GT_TXPOSTCURSOR_M) >> SD_SR_CTRL_GT_TXPOSTCURSOR_S);
	p += scnprintf(p, e - p, "  gt_rxlpmen     : %u\n", (v & SD_SR_CTRL_GT_RXLPMEN_M     ) >> SD_SR_CTRL_GT_RXLPMEN_S     );
	p += scnprintf(p, e - p, "  gt_srio_rxdfelpmreset: %u\n", (v & SD_SR_CTRL_GT_SRIO_RXDFELPMRESET_M) >> SD_SR_CTRL_GT_SRIO_RXDFELPMRESET_S);
	p += scnprintf(p, e - p, "  gt_phy_link_reset    : %u\n", (v & SD_SR_CTRL_GT_PHY_LINK_RESET_M    ) >> SD_SR_CTRL_GT_PHY_LINK_RESET_S    );
	p += scnprintf(p, e - p, "  gt_force_reinit      : %u\n", (v & SD_SR_CTRL_GT_FORCE_REINIT_M      ) >> SD_SR_CTRL_GT_FORCE_REINIT_S      );
	p += scnprintf(p, e - p, "  gt_phy_mce           : %u\n", (v & SD_SR_CTRL_GT_PHY_MCE_M           ) >> SD_SR_CTRL_GT_PHY_MCE_S           );

	v = sd_regs_srio_status(sd);
	p += scnprintf(p, e - p, "\nstatus:\n");
	p += scnprintf(p, e - p, "  srio_link_initialized: %u\n", (v & SD_SR_STAT_SRIO_LINK_INITIALIZED_M) >> SD_SR_STAT_SRIO_LINK_INITIALIZED_S);
	p += scnprintf(p, e - p, "  srio_port_initialized: %u\n", (v & SD_SR_STAT_SRIO_PORT_INITIALIZED_M) >> SD_SR_STAT_SRIO_PORT_INITIALIZED_S);
	p += scnprintf(p, e - p, "  srio_clock_out_lock  : %u\n", (v & SD_SR_STAT_SRIO_CLOCK_OUT_LOCK_M  ) >> SD_SR_STAT_SRIO_CLOCK_OUT_LOCK_S  );
	p += scnprintf(p, e - p, "  srio_mode_1x         : %u\n", (v & SD_SR_STAT_SRIO_MODE_1X_M         ) >> SD_SR_STAT_SRIO_MODE_1X_S         );
	p += scnprintf(p, e - p, "  port_error           : %u\n", (v & SD_SR_STAT_PORT_ERROR_M           ) >> SD_SR_STAT_PORT_ERROR_S           );
	p += scnprintf(p, e - p, "  gtrx_notintable_or   : %u\n", (v & SD_SR_STAT_GTRX_NOTINTABLE_OR_M   ) >> SD_SR_STAT_GTRX_NOTINTABLE_OR_S   );
	p += scnprintf(p, e - p, "  gtrx_disperr_or      : %u\n", (v & SD_SR_STAT_GTRX_DISPERR_OR_M      ) >> SD_SR_STAT_GTRX_DISPERR_OR_S      );
	p += scnprintf(p, e - p, "  phy_rcvd_mce         : %u\n", (v & SD_SR_STAT_PHY_RCVD_MCE_M         ) >> SD_SR_STAT_PHY_RCVD_MCE_S         );
	p += scnprintf(p, e - p, "  phy_rcvd_link_reset  : %u\n", (v & SD_SR_STAT_PHY_RCVD_LINK_RESET_M  ) >> SD_SR_STAT_PHY_RCVD_LINK_RESET_S  );
	p += scnprintf(p, e - p, "  device_id            : %u\n", (v & SD_SR_STAT_DEVICE_ID_M            ) >> SD_SR_STAT_DEVICE_ID_S            );
	p += scnprintf(p, e - p, "\n");

	return p - dst;
}


void sd_regs_set_swrite_bypass (struct srio_dev *sd, unsigned val)
{
	pr_debug("Set swrite_bypass to %u\n", val);
	REG_RMW(&sd->sys_regs->ctrl, SD_SR_CTRL_SWRITE_BYPASS_M, (val << SD_SR_CTRL_SWRITE_BYPASS_S) & SD_SR_CTRL_SWRITE_BYPASS_M);
}

void sd_regs_set_gt_loopback (struct srio_dev *sd, unsigned mode)
{
	pr_debug("Set gt_loopback to %u\n", mode);
	REG_RMW(&sd->sys_regs->ctrl, SD_SR_CTRL_SRIO_LOOPBACK_M, (mode << SD_SR_CTRL_SRIO_LOOPBACK_S) & SD_SR_CTRL_SRIO_LOOPBACK_M);
}

void sd_regs_set_gt_diffctrl (struct srio_dev *sd, unsigned val)
{
	pr_debug("Set gt_diffctrl to %u\n", val);
	REG_RMW(&sd->sys_regs->ctrl, SD_SR_CTRL_GT_DIFFCTRL_M, (val << SD_SR_CTRL_GT_DIFFCTRL_S) & SD_SR_CTRL_GT_DIFFCTRL_M);
}

void sd_regs_set_gt_txprecursor (struct srio_dev *sd, unsigned val)
{
	pr_debug("Set gt_txprecursor to %u\n", val);
	REG_RMW(&sd->sys_regs->ctrl, SD_SR_CTRL_GT_TXPRECURSOR_M, (val << SD_SR_CTRL_GT_TXPRECURSOR_S) & SD_SR_CTRL_GT_TXPRECURSOR_M);
}

void sd_regs_set_gt_txpostcursor (struct srio_dev *sd, unsigned val)
{
	pr_debug("Set gt_txpostcursor to %u\n", val);
	REG_RMW(&sd->sys_regs->ctrl, SD_SR_CTRL_GT_TXPOSTCURSOR_M, (val << SD_SR_CTRL_GT_TXPOSTCURSOR_S) & SD_SR_CTRL_GT_TXPOSTCURSOR_M);
}

void sd_regs_set_gt_rxlpmen (struct srio_dev *sd, unsigned val)
{
	pr_debug("Set gt_rxlpmen to %u\n", val);
	REG_RMW(&sd->sys_regs->ctrl, SD_SR_CTRL_GT_RXLPMEN_M, (val << SD_SR_CTRL_GT_RXLPMEN_S) & SD_SR_CTRL_GT_RXLPMEN_M);
}


unsigned sd_regs_get_swrite_bypass (struct srio_dev *sd)
{
	return (REG_READ(&sd->sys_regs->ctrl) & SD_SR_CTRL_SWRITE_BYPASS_M) >> SD_SR_CTRL_SWRITE_BYPASS_S;
}

unsigned sd_regs_get_gt_loopback (struct srio_dev *sd)
{
	return (REG_READ(&sd->sys_regs->ctrl) & SD_SR_CTRL_SRIO_LOOPBACK_M) >> SD_SR_CTRL_SRIO_LOOPBACK_S;
}

unsigned sd_regs_get_gt_diffctrl (struct srio_dev *sd)
{
	return (REG_READ(&sd->sys_regs->ctrl) & SD_SR_CTRL_GT_DIFFCTRL_M) >> SD_SR_CTRL_GT_DIFFCTRL_S;
}

unsigned sd_regs_get_gt_txprecursor (struct srio_dev *sd)
{
	return (REG_READ(&sd->sys_regs->ctrl) & SD_SR_CTRL_GT_TXPRECURSOR_M) >> SD_SR_CTRL_GT_TXPRECURSOR_S;
}

unsigned sd_regs_get_gt_txpostcursor (struct srio_dev *sd)
{
	return (REG_READ(&sd->sys_regs->ctrl) & SD_SR_CTRL_GT_TXPOSTCURSOR_M) >> SD_SR_CTRL_GT_TXPOSTCURSOR_S;
}

unsigned sd_regs_get_gt_rxlpmen (struct srio_dev *sd)
{
	return (REG_READ(&sd->sys_regs->ctrl) & SD_SR_CTRL_GT_RXLPMEN_M) >> SD_SR_CTRL_GT_RXLPMEN_S;
}


uint16_t sd_regs_get_devid (struct srio_dev *sd)
{
	uint32_t csr = REG_READ(sd->maint + RIO_DID_CSR);

	if ( sd->pef & (1 << 4) )
	{
		sd->devid = csr & 0x0000FFFF;
		return sd->devid;
	}

	csr >>= 16;
	sd->devid = csr & 0x000000FF;
	return sd->devid;
}

void sd_regs_set_devid (struct srio_dev *sd, uint16_t id)
{
	uint32_t csr = id;

	sd->devid = id;
	if ( sd->pef & (1 << 4) )
		csr &= 0x0000FFFF;
	else
	{
		sd->devid &= 0x000000FF;
		csr       &= 0x000000FF;
		csr      <<= 16;
	}
	REG_WRITE(sd->maint + RIO_DID_CSR, csr);
}


