/** \file      dsm_xparameters.c
 *  \brief     Kernelspace code to import xparameters.h from Xilinx-generated code, and
 *             insulate the main module from it.
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

#include "dma_streamer_mod.h"
#include "dsm_xparameters.h"

#include <xparameters.h>


// These seem to change names during work being done in the PL side, so map them to
// shorter names used in this code.
#if defined(XPAR_AXIS_DSRC_0_BASEADDR)
#  define DSM_DSRC0_BASE XPAR_AXIS_DSRC_0_BASEADDR
#else
#  undef DSM_DSRC0_BASE
#endif
#if defined(XPAR_AXIS_DSRC_1_BASEADDR)
#  define DSM_DSRC1_BASE XPAR_AXIS_DSRC_1_BASEADDR
#else
#  undef DSM_DSRC1_BASE
#endif

#if defined(XPAR_AXIS_DSNK_0_BASEADDR)
#  define DSM_DSNK0_BASE XPAR_AXIS_DSNK_0_BASEADDR
#else
#  undef DSM_DSNK0_BASE
#endif
#if defined(XPAR_AXIS_DSNK_1_BASEADDR)
#  define DSM_DSNK1_BASE XPAR_AXIS_DSNK_1_BASEADDR
#else
#  undef DSM_DSNK1_BASE
#endif

#if defined(XPAR_ADI_DPFD_1_BASEADDR)
#  define DSM_ADI1_OLD_BASE XPAR_ADI_DPFD_1_BASEADDR
#  define DSM_ADI1_OLD_SIZE sizeof(struct dsm_lvds_regs)
#elif defined(XPAR_ADI_LVDS_CTRL_1_BASEADDR)
#  define DSM_ADI1_OLD_BASE XPAR_ADI_LVDS_CTRL_1_BASEADDR
#  define DSM_ADI1_OLD_SIZE sizeof(struct dsm_lvds_regs)
#else
#  undef DSM_ADI1_OLD_BASE
#endif

#if defined(XPAR_ADI_DPFD_2_BASEADDR)
#  define DSM_ADI2_OLD_BASE XPAR_ADI_DPFD_2_BASEADDR
#  define DSM_ADI2_OLD_SIZE sizeof(struct dsm_lvds_regs)
#elif defined(XPAR_ADI_LVDS_CTRL_2_BASEADDR)
#  define DSM_ADI2_OLD_BASE XPAR_ADI_LVDS_CTRL_2_BASEADDR
#  define DSM_ADI2_OLD_SIZE sizeof(struct dsm_lvds_regs)
#else
#  undef DSM_ADI2_OLD_BASE
#endif


#if defined(XPAR_AXI_AD9361_0_BASEADDR)
#  define DSM_ADI1_NEW_BASE XPAR_AXI_AD9361_0_BASEADDR
#  define DSM_ADI1_NEW_SIZE 0x8000
#endif

#if defined(XPAR_AXI_AD9361_1_BASEADDR)
#  define DSM_ADI2_NEW_BASE XPAR_AXI_AD9361_1_BASEADDR
#  define DSM_ADI2_NEW_SIZE 0x8000
#endif


struct dsm_dsrc_regs __iomem *dsm_dsrc0_regs = NULL;
struct dsm_dsnk_regs __iomem *dsm_dsnk0_regs = NULL;
struct dsm_dsrc_regs __iomem *dsm_dsrc1_regs = NULL;
struct dsm_dsnk_regs __iomem *dsm_dsnk1_regs = NULL;
struct dsm_lvds_regs __iomem *dsm_adi1_old_regs = NULL;
struct dsm_lvds_regs __iomem *dsm_adi2_old_regs = NULL;

u32 __iomem *dsm_adi1_new_regs = NULL;
u32 __iomem *dsm_adi2_new_regs = NULL;

u32 __iomem *dsm_rx_fifo1_cnt = NULL;
u32 __iomem *dsm_rx_fifo2_cnt = NULL;
u32 __iomem *dsm_tx_fifo1_cnt = NULL;
u32 __iomem *dsm_tx_fifo2_cnt = NULL;


static void __attribute__((unused)) dsm_unmap (void __iomem *mapped, unsigned long base,
                                               size_t size)
{
	iounmap(mapped);
	release_mem_region(base, size);
}

void dsm_xparameters_exit (void)
{
#ifdef DSM_DSNK0_BASE
	if ( dsm_dsnk0_regs )
		dsm_unmap(dsm_dsnk0_regs, DSM_DSNK0_BASE, sizeof(struct dsm_dsnk_regs));
#endif

#ifdef DSM_DSNK1_BASE
	if ( dsm_dsnk1_regs )
		dsm_unmap(dsm_dsnk1_regs, DSM_DSNK1_BASE, sizeof(struct dsm_dsnk_regs));
#endif

#ifdef DSM_DSRC0_BASE
	if ( dsm_dsrc0_regs )
		dsm_unmap(dsm_dsrc0_regs, DSM_DSRC0_BASE, sizeof(struct dsm_dsrc_regs));
#endif

#ifdef DSM_DSRC1_BASE
	if ( dsm_dsrc1_regs )
		dsm_unmap(dsm_dsrc1_regs, DSM_DSRC1_BASE, sizeof(struct dsm_dsrc_regs));
#endif


#ifdef DSM_ADI1_OLD_BASE
	if ( dsm_adi1_old_regs )
		dsm_unmap(dsm_adi1_old_regs, DSM_ADI1_OLD_BASE, DSM_ADI1_OLD_SIZE);
#endif

#ifdef DSM_ADI2_OLD_BASE
	if ( dsm_adi2_old_regs )
		dsm_unmap(dsm_adi2_old_regs, DSM_ADI2_OLD_BASE, DSM_ADI2_OLD_SIZE);
#endif


#ifdef XPAR_RX_FIFO1_CNT_BASEADDR
	if ( dsm_rx_fifo1_cnt )
		dsm_unmap(dsm_rx_fifo1_cnt, XPAR_RX_FIFO1_CNT_BASEADDR, 8);
#endif

#ifdef XPAR_RX_FIFO2_CNT_BASEADDR
	if ( dsm_rx_fifo2_cnt )
		dsm_unmap(dsm_rx_fifo2_cnt, XPAR_RX_FIFO2_CNT_BASEADDR, 8);
#endif

#ifdef XPAR_TX_FIFO1_CNT_BASEADDR
	if ( dsm_tx_fifo1_cnt )
		dsm_unmap(dsm_tx_fifo1_cnt, XPAR_TX_FIFO1_CNT_BASEADDR, 8);
#endif

#ifdef XPAR_TX_FIFO2_CNT_BASEADDR
	if ( dsm_tx_fifo2_cnt )
		dsm_unmap(dsm_tx_fifo2_cnt, XPAR_TX_FIFO2_CNT_BASEADDR, 8);
#endif


#ifdef DSM_ADI1_NEW_BASE
	if ( dsm_adi1_new_regs )
		dsm_unmap(dsm_adi1_new_regs, DSM_ADI1_NEW_BASE, DSM_ADI1_NEW_SIZE);
#endif

#ifdef DSM_ADI2_NEW_BASE
	if ( dsm_adi2_new_regs )
		dsm_unmap(dsm_adi2_new_regs, DSM_ADI2_NEW_BASE, DSM_ADI2_NEW_SIZE);
#endif
}


static void __attribute__((unused)) __iomem *dsm_iomap (unsigned long base, size_t size)
{
	void __iomem *ret;

	if ( !request_mem_region(base, size, DSM_DRIVER_NODE) )
	{
		pr_err("request_mem_region(%08lx, %08zu) denied\n", base, size);
		return NULL;
	}

	if ( !(ret = ioremap(base, size)) )
	{
		pr_err("ioremap(%08lx, %08zu) failed\n", base, size);
		release_mem_region(base, size);
		return NULL;
	}

	return ret;
}

int dsm_xparameters_init (void)
{
#ifdef DSM_DSNK0_BASE
	if ( !(dsm_dsnk0_regs = dsm_iomap(DSM_DSNK0_BASE, sizeof(struct dsm_dsnk_regs))) )
		goto error;
	pr_debug("DSNK0 regs %08x -> %p\n", DSM_DSNK0_BASE, dsm_dsnk0_regs);
#endif

#ifdef DSM_DSNK1_BASE
	if ( !(dsm_dsnk1_regs = dsm_iomap(DSM_DSNK1_BASE, sizeof(struct dsm_dsnk_regs))) )
		goto error;
	pr_debug("DSNK1 regs %08x -> %p\n", DSM_DSNK1_BASE, dsm_dsnk1_regs);
#endif

#ifdef DSM_DSRC0_BASE
	if ( !(dsm_dsrc0_regs = dsm_iomap(DSM_DSRC0_BASE, sizeof(struct dsm_dsrc_regs))) )
		goto error;
	pr_debug("DSRC0 regs %08x -> %p\n", DSM_DSNK0_BASE, dsm_dsnk0_regs);
#endif

#ifdef DSM_DSRC1_BASE
	if ( !(dsm_dsrc1_regs = dsm_iomap(DSM_DSRC1_BASE, sizeof(struct dsm_dsrc_regs))) )
		goto error;
	pr_debug("DSRC1 regs %08x -> %p\n", DSM_DSNK1_BASE, dsm_dsnk1_regs);
#endif


#ifdef DSM_ADI1_OLD_BASE
	if ( !(dsm_adi1_old_regs = dsm_iomap(DSM_ADI1_OLD_BASE, DSM_ADI1_OLD_SIZE)) )
		goto error;
#endif

#ifdef DSM_ADI2_OLD_BASE
	if ( !(dsm_adi2_old_regs = dsm_iomap(DSM_ADI2_OLD_BASE, DSM_ADI2_OLD_SIZE)) )
		goto error;
#endif


#ifdef XPAR_RX_FIFO1_CNT_BASEADDR
	if ( !(dsm_rx_fifo1_cnt = dsm_iomap(XPAR_RX_FIFO1_CNT_BASEADDR, 8)) )
		goto error;
#endif

#ifdef XPAR_RX_FIFO2_CNT_BASEADDR
	if ( !(dsm_rx_fifo2_cnt = dsm_iomap(XPAR_RX_FIFO2_CNT_BASEADDR, 8)) )
		goto error;
#endif

#ifdef XPAR_TX_FIFO1_CNT_BASEADDR
	if ( !(dsm_tx_fifo1_cnt = dsm_iomap(XPAR_TX_FIFO1_CNT_BASEADDR, 8)) )
		goto error;
#endif

#ifdef XPAR_TX_FIFO2_CNT_BASEADDR
	if ( !(dsm_tx_fifo2_cnt = dsm_iomap(XPAR_TX_FIFO2_CNT_BASEADDR, 8)) )
		goto error;
#endif


#ifdef DSM_ADI1_NEW_BASE
	if ( !(dsm_adi1_new_regs = dsm_iomap(DSM_ADI1_NEW_BASE, DSM_ADI1_NEW_SIZE)) )
		goto error;
	pr_debug("ADI1 new IF @%08x:\n", DSM_ADI1_NEW_BASE);
	pr_debug("RX VER %08x ID %08x\n", 
	         REG_READ(ADI_NEW_RX_ADDR(dsm_adi1_new_regs, 0x0000)),
	         REG_READ(ADI_NEW_RX_ADDR(dsm_adi1_new_regs, 0x0004)));
	pr_debug("TX VER %08x ID %08x\n", 
	         REG_READ(ADI_NEW_TX_ADDR(dsm_adi1_new_regs, 0x0000)),
	         REG_READ(ADI_NEW_TX_ADDR(dsm_adi1_new_regs, 0x0004)));
#endif

#ifdef DSM_ADI2_NEW_BASE
	if ( !(dsm_adi2_new_regs = dsm_iomap(DSM_ADI2_NEW_BASE, DSM_ADI2_NEW_SIZE)) )
		goto error;
	pr_debug("ADI2 new IF @%08x:\n", DSM_ADI2_NEW_BASE);
	pr_debug("RX VER %08x ID %08x\n", 
	         REG_READ(ADI_NEW_RX_ADDR(dsm_adi2_new_regs, 0x0000)),
	         REG_READ(ADI_NEW_RX_ADDR(dsm_adi2_new_regs, 0x0004)));
	pr_debug("TX VER %08x ID %08x\n", 
	         REG_READ(ADI_NEW_TX_ADDR(dsm_adi2_new_regs, 0x0000)),
	         REG_READ(ADI_NEW_TX_ADDR(dsm_adi2_new_regs, 0x0004)));
#endif


	return 0;

	goto error; // quash compiler warning if none of the #ifdef's above use it
error:
	dsm_xparameters_exit();
	return -EIO;
}


