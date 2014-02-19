/** \file      dsm_xparameters.c
 *  \brief     Definitions used in main module and dsm_xparameters.c
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
#ifndef _DSM_XPARAMETERS_H_
#define _DSM_XPARAMETERS_H_
#include <linux/kernel.h>


struct dsm_dsrc_regs
{
	uint32_t  ctrl;  // DSRC_CTRL W
	uint32_t  stat;  // DSRC_STAT R
	uint32_t  bytes; // DSRC_BYTES RW
	uint32_t  sent;  // DSRC_BYTES_SENT R
	uint32_t  type;  // DSRC_DATA_TYPE RW
};

struct dsm_dsnk_regs
{
	uint32_t  ctrl;  // DSNK_CTRL W
	uint32_t  stat;  // DSNK_STAT R
	uint32_t  bytes; // DSNK_BYTES R
	uint32_t  sum_h; // DSNK_CHKSUM_H R
	uint32_t  sum_l; // DSNK_CHKSUM_L R
};

struct dsm_lvds_regs
{
	uint32_t  ctrl;    // ADI_LVDS_CTRL RW
	uint32_t  tx_cnt;  // ADI_LVDS_TX_CNT RW
	uint32_t  rx_cnt;  // ADI_LVDS_RX_CNT RW
	uint32_t  tx_low;  // LAST_TX_LOW R
	uint32_t  tx_hi;   // LAST_TX_HI R
	uint32_t  cs_rst;  // CHKSUM_RST W (DSM_IOCG_CLK_CNT R)
	uint32_t  cs_low;  // CHKSUM_LOW R
	uint32_t  cs_hi;   // CHKSUM_HI R
};


extern struct dsm_dsrc_regs __iomem *dsm_dsrc_regs;
extern struct dsm_dsnk_regs __iomem *dsm_dsnk_regs;
extern struct dsm_lvds_regs __iomem *dsm_adi1_old_regs;
extern struct dsm_lvds_regs __iomem *dsm_adi2_old_regs;

extern u32 __iomem *dsm_adi1_new_regs;
extern u32 __iomem *dsm_adi2_new_regs;

extern u32 __iomem *dsm_rx_fifo1_cnt;
extern u32 __iomem *dsm_rx_fifo2_cnt;
extern u32 __iomem *dsm_tx_fifo1_cnt;
extern u32 __iomem *dsm_tx_fifo2_cnt;

int dsm_xparameters_init (void);
void dsm_xparameters_exit (void);


// calculating the offset of a register in the new ADI controls - constrain offs to
// within the 0x4000-byte address space for each dir, and enforce 32-bit alignment
#define ADI_NEW_RT_ADDR(base, offs, tx) \
	((void __iomem *)(base) + ((tx) ? 0x4000 : 0) + ((offs) & 0x3FFC))

#define ADI_NEW_RX_ADDR(base, offs)       ADI_NEW_RT_ADDR((base), (offs), 0)
#define ADI_NEW_TX_ADDR(base, offs)       ADI_NEW_RT_ADDR((base), (offs), 1)

#define REG_WRITE(addr, val)  do{ iowrite32(val, addr); }while(0)
#define REG_READ(addr)        (ioread32(addr))


#endif // _DSM_XPARAMETERS_H_
