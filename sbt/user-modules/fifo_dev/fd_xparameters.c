/** \file      fd_xparameters.c
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

#include "fd_main.h"
#include "fd_xparameters.h"

#include <xparameters.h>


// These seem to change names during work being done in the PL side, so map them to
// shorter names used in this code.
#if defined(XPAR_AXIS_DSRC_0_BASEADDR)
#  define FD_DSRC0_BASE XPAR_AXIS_DSRC_0_BASEADDR
#elif defined(XPAR_DSRC_0_BASEADDR)
#  define FD_DSRC0_BASE XPAR_DSRC_0_BASEADDR
#else
#  undef FD_DSRC0_BASE
#  error need a FD_DSRC0_BASE
#endif
#if defined(XPAR_AXIS_DSRC_1_BASEADDR)
#  define FD_DSRC1_BASE XPAR_AXIS_DSRC_1_BASEADDR
#else
#  undef FD_DSRC1_BASE
#endif

#if defined(XPAR_AXIS_DSNK_0_BASEADDR)
#  define FD_DSNK0_BASE XPAR_AXIS_DSNK_0_BASEADDR
#elif defined(XPAR_DSNK_0_BASEADDR)
#  define FD_DSNK0_BASE XPAR_DSNK_0_BASEADDR
#else
#  undef FD_DSNK0_BASE
#  error need a FD_DSNK0_BASE
#endif
#if defined(XPAR_AXIS_DSNK_1_BASEADDR)
#  define FD_DSNK1_BASE XPAR_AXIS_DSNK_1_BASEADDR
#else
#  undef FD_DSNK1_BASE
#endif

#if defined(XPAR_ADI_DPFD_1_BASEADDR)
#  define FD_ADI1_OLD_BASE XPAR_ADI_DPFD_1_BASEADDR
#  define FD_ADI1_OLD_SIZE sizeof(struct fd_lvds_regs)
#elif defined(XPAR_ADI_LVDS_CTRL_1_BASEADDR)
#  define FD_ADI1_OLD_BASE XPAR_ADI_LVDS_CTRL_1_BASEADDR
#  define FD_ADI1_OLD_SIZE sizeof(struct fd_lvds_regs)
#else
#  undef FD_ADI1_OLD_BASE
#endif

#if defined(XPAR_ADI_DPFD_2_BASEADDR)
#  define FD_ADI2_OLD_BASE XPAR_ADI_DPFD_2_BASEADDR
#  define FD_ADI2_OLD_SIZE sizeof(struct fd_lvds_regs)
#elif defined(XPAR_ADI_LVDS_CTRL_2_BASEADDR)
#  define FD_ADI2_OLD_BASE XPAR_ADI_LVDS_CTRL_2_BASEADDR
#  define FD_ADI2_OLD_SIZE sizeof(struct fd_lvds_regs)
#else
#  undef FD_ADI2_OLD_BASE
#endif


#if defined(XPAR_AXI_AD9361_0_BASEADDR)
#  define FD_ADI1_NEW_BASE XPAR_AXI_AD9361_0_BASEADDR
#  define FD_ADI1_NEW_SIZE 0x8000
#endif

#if defined(XPAR_AXI_AD9361_1_BASEADDR)
#  define FD_ADI2_NEW_BASE XPAR_AXI_AD9361_1_BASEADDR
#  define FD_ADI2_NEW_SIZE 0x8000
#endif


#ifdef XPAR_AXI_PERF_MON_0_BASEADDR
#define PMON_BASE XPAR_AXI_PERF_MON_0_BASEADDR
#define PMON_SIZE (1 + XPAR_AXI_PERF_MON_0_HIGHADDR - XPAR_AXI_PERF_MON_0_BASEADDR)
#endif


struct fd_dsrc_regs __iomem *fd_dsrc0_regs = NULL;
struct fd_dsnk_regs __iomem *fd_dsnk0_regs = NULL;
struct fd_dsrc_regs __iomem *fd_dsrc1_regs = NULL;
struct fd_dsnk_regs __iomem *fd_dsnk1_regs = NULL;
struct fd_lvds_regs __iomem *fd_adi1_old_regs = NULL;
struct fd_lvds_regs __iomem *fd_adi2_old_regs = NULL;

u32 __iomem *fd_adi1_new_regs = NULL;
u32 __iomem *fd_adi2_new_regs = NULL;

u32 __iomem *fd_rx_fifo1_cnt = NULL;
u32 __iomem *fd_rx_fifo2_cnt = NULL;
u32 __iomem *fd_tx_fifo1_cnt = NULL;
u32 __iomem *fd_tx_fifo2_cnt = NULL;

void __iomem *fd_pmon_regs = NULL;


static void __attribute__((unused)) fd_unmap (void __iomem *mapped, unsigned long base,
                                               size_t size)
{
	iounmap(mapped);
	release_mem_region(base, size);
}

void fd_xparameters_exit (void)
{
#ifdef FD_DSNK0_BASE
	if ( fd_dsnk0_regs )
		fd_unmap(fd_dsnk0_regs, FD_DSNK0_BASE, sizeof(struct fd_dsnk_regs));
#endif

#ifdef FD_DSNK1_BASE
	if ( fd_dsnk1_regs )
		fd_unmap(fd_dsnk1_regs, FD_DSNK1_BASE, sizeof(struct fd_dsnk_regs));
#endif

#ifdef FD_DSRC0_BASE
	if ( fd_dsrc0_regs )
		fd_unmap(fd_dsrc0_regs, FD_DSRC0_BASE, sizeof(struct fd_dsrc_regs));
#endif

#ifdef FD_DSRC1_BASE
	if ( fd_dsrc1_regs )
		fd_unmap(fd_dsrc1_regs, FD_DSRC1_BASE, sizeof(struct fd_dsrc_regs));
#endif


#ifdef FD_ADI1_OLD_BASE
	if ( fd_adi1_old_regs )
		fd_unmap(fd_adi1_old_regs, FD_ADI1_OLD_BASE, FD_ADI1_OLD_SIZE);
#endif

#ifdef FD_ADI2_OLD_BASE
	if ( fd_adi2_old_regs )
		fd_unmap(fd_adi2_old_regs, FD_ADI2_OLD_BASE, FD_ADI2_OLD_SIZE);
#endif


#ifdef XPAR_RX_FIFO1_CNT_BASEADDR
	if ( fd_rx_fifo1_cnt )
		fd_unmap(fd_rx_fifo1_cnt, XPAR_RX_FIFO1_CNT_BASEADDR, 8);
#endif

#ifdef XPAR_RX_FIFO2_CNT_BASEADDR
	if ( fd_rx_fifo2_cnt )
		fd_unmap(fd_rx_fifo2_cnt, XPAR_RX_FIFO2_CNT_BASEADDR, 8);
#endif

#ifdef XPAR_TX_FIFO1_CNT_BASEADDR
	if ( fd_tx_fifo1_cnt )
		fd_unmap(fd_tx_fifo1_cnt, XPAR_TX_FIFO1_CNT_BASEADDR, 8);
#endif

#ifdef XPAR_TX_FIFO2_CNT_BASEADDR
	if ( fd_tx_fifo2_cnt )
		fd_unmap(fd_tx_fifo2_cnt, XPAR_TX_FIFO2_CNT_BASEADDR, 8);
#endif


#ifdef FD_ADI1_NEW_BASE
	if ( fd_adi1_new_regs )
		fd_unmap(fd_adi1_new_regs, FD_ADI1_NEW_BASE, FD_ADI1_NEW_SIZE);
#endif

#ifdef FD_ADI2_NEW_BASE
	if ( fd_adi2_new_regs )
		fd_unmap(fd_adi2_new_regs, FD_ADI2_NEW_BASE, FD_ADI2_NEW_SIZE);
#endif

#ifdef PMON_BASE
	if ( fd_pmon_regs )
		fd_unmap(fd_pmon_regs, PMON_BASE, PMON_SIZE);
#endif
}


static void __attribute__((unused)) __iomem *fd_iomap (unsigned long base, size_t size)
{
	void __iomem *ret;

	if ( !request_mem_region(base, size, FD_DRIVER_NODE) )
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

int fd_xparameters_init (void)
{
#ifdef FD_DSNK0_BASE
	if ( !(fd_dsnk0_regs = fd_iomap(FD_DSNK0_BASE, sizeof(struct fd_dsnk_regs))) )
		goto error;
	pr_debug("DSNK0 regs %08x -> %p\n", FD_DSNK0_BASE, fd_dsnk0_regs);
#else
	pr_debug("DSNK0 regs unset, no FD_DSNK0_BASE\n");
#endif

#ifdef FD_DSNK1_BASE
	if ( !(fd_dsnk1_regs = fd_iomap(FD_DSNK1_BASE, sizeof(struct fd_dsnk_regs))) )
		goto error;
	pr_debug("DSNK1 regs %08x -> %p\n", FD_DSNK1_BASE, fd_dsnk1_regs);
#else
	pr_debug("DSNK1 regs unset, no FD_DSNK1_BASE\n");
#endif

#ifdef FD_DSRC0_BASE
	if ( !(fd_dsrc0_regs = fd_iomap(FD_DSRC0_BASE, sizeof(struct fd_dsrc_regs))) )
		goto error;
	pr_debug("DSRC0 regs %08x -> %p\n", FD_DSRC0_BASE, fd_dsrc0_regs);
#else
	pr_debug("DSRC0 regs unset, no FD_DSRC0_BASE\n");
#endif

#ifdef FD_DSRC1_BASE
	if ( !(fd_dsrc1_regs = fd_iomap(FD_DSRC1_BASE, sizeof(struct fd_dsrc_regs))) )
		goto error;
	pr_debug("DSRC1 regs %08x -> %p\n", FD_DSRC1_BASE, fd_dsrc1_regs);
#else
	pr_debug("DSRC1 regs unset, no FD_DSRC1_BASE\n");
#endif


#ifdef FD_ADI1_OLD_BASE
	if ( !(fd_adi1_old_regs = fd_iomap(FD_ADI1_OLD_BASE, FD_ADI1_OLD_SIZE)) )
		goto error;
#endif

#ifdef FD_ADI2_OLD_BASE
	if ( !(fd_adi2_old_regs = fd_iomap(FD_ADI2_OLD_BASE, FD_ADI2_OLD_SIZE)) )
		goto error;
#endif


#ifdef XPAR_RX_FIFO1_CNT_BASEADDR
	if ( !(fd_rx_fifo1_cnt = fd_iomap(XPAR_RX_FIFO1_CNT_BASEADDR, 8)) )
		goto error;
#endif

#ifdef XPAR_RX_FIFO2_CNT_BASEADDR
	if ( !(fd_rx_fifo2_cnt = fd_iomap(XPAR_RX_FIFO2_CNT_BASEADDR, 8)) )
		goto error;
#endif

#ifdef XPAR_TX_FIFO1_CNT_BASEADDR
	if ( !(fd_tx_fifo1_cnt = fd_iomap(XPAR_TX_FIFO1_CNT_BASEADDR, 8)) )
		goto error;
#endif

#ifdef XPAR_TX_FIFO2_CNT_BASEADDR
	if ( !(fd_tx_fifo2_cnt = fd_iomap(XPAR_TX_FIFO2_CNT_BASEADDR, 8)) )
		goto error;
#endif


#ifdef FD_ADI1_NEW_BASE
	if ( !(fd_adi1_new_regs = fd_iomap(FD_ADI1_NEW_BASE, FD_ADI1_NEW_SIZE)) )
		goto error;
	pr_debug("ADI1 new IF @%08x:\n", FD_ADI1_NEW_BASE);
	pr_debug("RX VER %08x ID %08x\n", 
	         REG_READ(ADI_NEW_RX_ADDR(fd_adi1_new_regs, 0x0000)),
	         REG_READ(ADI_NEW_RX_ADDR(fd_adi1_new_regs, 0x0004)));
	pr_debug("TX VER %08x ID %08x\n", 
	         REG_READ(ADI_NEW_TX_ADDR(fd_adi1_new_regs, 0x0000)),
	         REG_READ(ADI_NEW_TX_ADDR(fd_adi1_new_regs, 0x0004)));
#endif

#ifdef FD_ADI2_NEW_BASE
	if ( !(fd_adi2_new_regs = fd_iomap(FD_ADI2_NEW_BASE, FD_ADI2_NEW_SIZE)) )
		goto error;
	pr_debug("ADI2 new IF @%08x:\n", FD_ADI2_NEW_BASE);
	pr_debug("RX VER %08x ID %08x\n", 
	         REG_READ(ADI_NEW_RX_ADDR(fd_adi2_new_regs, 0x0000)),
	         REG_READ(ADI_NEW_RX_ADDR(fd_adi2_new_regs, 0x0004)));
	pr_debug("TX VER %08x ID %08x\n", 
	         REG_READ(ADI_NEW_TX_ADDR(fd_adi2_new_regs, 0x0000)),
	         REG_READ(ADI_NEW_TX_ADDR(fd_adi2_new_regs, 0x0004)));
#endif


#ifdef PMON_BASE
	if ( !(fd_pmon_regs = fd_iomap(PMON_BASE, PMON_SIZE)) )
		goto error;
	printk("PMON regs %08x -> %p\n", PMON_BASE, fd_pmon_regs);
#else
	printk("PMON regs unset, no PMON_BASE\n");
#endif

	return 0;

	goto error; // quash compiler warning if none of the #ifdef's above use it
error:
	fd_xparameters_exit();
	return -EIO;
}


