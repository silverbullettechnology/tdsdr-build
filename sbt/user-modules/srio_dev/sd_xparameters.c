/** \file      sd_xparameters.c
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

#include "srio_dev.h"
#include "sd_xparameters.h"
#include "dsxx/test.h"
#include "dsxx/regs.h"
#include "sd_regs.h"
#include "loop/test.h"

#include <xparameters.h>


#ifdef CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX
#if defined(XPAR_DSNK_0_BASEADDR)
#define SD_DSNK_REGS XPAR_DSNK_0_BASEADDR
#elif defined(XPAR_DSNK_1_BASEADDR)
#define SD_DSNK_REGS XPAR_DSNK_1_BASEADDR
#elif defined(XPAR_AXIS_DSNK32_0_BASEADDR)
#define SD_DSNK_REGS XPAR_AXIS_DSNK32_0_BASEADDR
#else
#error NO SD_DSNK_REGS
#endif

#if defined(XPAR_DSRC_0_BASEADDR)
#define SD_DSRC_REGS XPAR_DSRC_0_BASEADDR
#elif defined(XPAR_DSRC_1_BASEADDR)
#define SD_DSRC_REGS XPAR_DSRC_1_BASEADDR
#elif defined(XPAR_AXIS_DSRC32_0_BASEADDR)
#define SD_DSRC_REGS XPAR_AXIS_DSRC32_0_BASEADDR
#elif defined(XPAR_DSRC_REP_1_BASEADDR)
#define SD_DSRC_REGS XPAR_DSRC_REP_1_BASEADDR
#else
#error NO SD_DSRC_REGS
#endif

#if defined(XPAR_AXI_FIFO_0_BASEADDR)
#define SD_FIFO_REGS XPAR_AXI_FIFO_0_BASEADDR
#else
#error NO SD_FIFO_REGS
#endif

#if defined(XPAR_AXI_FIFO_0_AXI4_BASEADDR)
#define SD_FIFO_DATA XPAR_AXI_FIFO_0_AXI4_BASEADDR
#elif defined(XPAR_AXI_FIFO_0_BASEADDR)
#define SD_FIFO_DATA XPAR_AXI_FIFO_0_BASEADDR
#else
#error NO SD_FIFO_DATA
#endif

#if defined(XPAR_FABRIC_AXI_FIFO_MM_S_0_INTERRUPT_INTR)
#define SD_FIFO_IRQ  XPAR_FABRIC_AXI_FIFO_MM_S_0_INTERRUPT_INTR
#else
#warning NO SD_FIFO_IRQ
#endif

#if defined(XPAR_AXIS_PAUSE_0_BASEADDR)
#define SD_DSRC_PAUSE XPAR_AXIS_PAUSE_0_BASEADDR
#else
#warning NO SD_DSRC_PAUSE
#endif

#if defined(XPAR_AXIS_PAUSE_1_BASEADDR)
#define SD_DSNK_PAUSE XPAR_AXIS_PAUSE_1_BASEADDR
#else
#warning NO SD_DSNK_PAUSE
#endif
#endif // CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX


#ifdef CONFIG_USER_MODULES_SRIO_DEV_TEST_LOOP
#if defined(XPAR_AXI_SRIO_INITIATOR_FIFO_BASEADDR)
#define SD_INIT_FIFO_REGS XPAR_AXI_SRIO_INITIATOR_FIFO_BASEADDR
#else
#error NO SD_INIT_FIFO_REGS
#endif

#if defined(XPAR_AXI_SRIO_INITIATOR_FIFO_AXI4_BASEADDR)
#define SD_INIT_FIFO_DATA XPAR_AXI_SRIO_INITIATOR_FIFO_AXI4_BASEADDR
#else
#error NO SD_INIT_FIFO_DATA
#endif

#if defined(XPAR_FABRIC_AXI_SRIO_INITIATOR_FIFO_INTERRUPT_INTR)
#define SD_INIT_FIFO_IRQ XPAR_FABRIC_AXI_SRIO_INITIATOR_FIFO_INTERRUPT_INTR
#else
#warning NO SD_INIT_FIFO_IRQ
#endif


#if defined(XPAR_AXI_SRIO_TARGET_FIFO_BASEADDR)
#define SD_TARG_FIFO_REGS XPAR_AXI_SRIO_TARGET_FIFO_BASEADDR
#else
#error NO SD_TARG_FIFO_REGS
#endif

#if defined(XPAR_AXI_SRIO_TARGET_FIFO_AXI4_BASEADDR)
#define SD_TARG_FIFO_DATA XPAR_AXI_SRIO_TARGET_FIFO_AXI4_BASEADDR
#else
#error NO SD_TARG_FIFO_DATA
#endif

#if defined(XPAR_FABRIC_AXI_SRIO_TARGET_FIFO_INTERRUPT_INTR)
#define SD_TARG_FIFO_IRQ XPAR_FABRIC_AXI_SRIO_TARGET_FIFO_INTERRUPT_INTR
#else
#warning NO SD_TARG_FIFO_IRQ
#endif


#if defined(XPAR_AXI_SRIO_USERDEF_FIFO_BASEADDR)
#define SD_USER_FIFO_REGS XPAR_AXI_SRIO_USERDEF_FIFO_BASEADDR
#else
#error NO SD_USER_FIFO_REGS
#endif

#if defined(XPAR_AXI_SRIO_USERDEF_FIFO_AXI4_BASEADDR)
#define SD_USER_FIFO_DATA XPAR_AXI_SRIO_USERDEF_FIFO_AXI4_BASEADDR
#else
#error NO SD_USER_FIFO_DATA
#endif

#if defined(XPAR_FABRIC_AXI_SRIO_USERDEF_FIFO_INTERRUPT_INTR)
#define SD_USER_FIFO_IRQ XPAR_FABRIC_AXI_SRIO_USERDEF_FIFO_INTERRUPT_INTR
#else
#warning NO SD_USER_FIFO_IRQ
#endif


#if defined(XPAR_SRIO_GEN2_0_BASEADDR)
#define SD_SRIO_CORE_REGS XPAR_SRIO_GEN2_0_BASEADDR
#else
#error NO SD_SRIO_CORE_REGS
#endif

#if defined(XPAR_SRIO_SRC_DEST_REG_0_BASEADDR)
#define SD_SRIO_ADDR_REGS XPAR_SRIO_SRC_DEST_REG_0_BASEADDR
#else
#error NO SD_SRIO_ADDR_REGS
#endif


#endif // CONFIG_USER_MODULES_SRIO_DEV_TEST_LOOP


void sd_unmap (void __iomem *mapped, unsigned long base, size_t size)
{
	iounmap(mapped);
	release_mem_region(base, size);
}

void sd_xparameters_exit ()
{
#ifdef CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX
#ifdef SD_DSNK_REGS
	if ( sd_dsnk_regs )
		sd_unmap(sd_dsnk_regs, SD_DSNK_REGS, sizeof(struct sd_dsnk_regs));
#endif
#ifdef SD_DSRC_REGS
	if ( sd_dsrc_regs )
		sd_unmap(sd_dsrc_regs, SD_DSRC_REGS, sizeof(struct sd_dsrc_regs));
#endif
#ifdef SD_DSRC_PAUSE
	if ( sd_dsrc_pause )
		sd_unmap(sd_dsrc_pause, SD_DSRC_PAUSE, sizeof(struct sd_pause_regs));
#endif
#ifdef SD_DSNK_PAUSE
	if ( sd_dsnk_pause )
		sd_unmap(sd_dsnk_pause, SD_DSNK_PAUSE, sizeof(struct sd_pause_regs));
#endif
#endif // CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX
}


void __iomem *sd_iomap (unsigned long base, size_t size)
{
	void __iomem *ret;

	if ( !request_mem_region(base, size, SD_DRIVER_NODE) )
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

int sd_xparameters_init (void)
{
#ifdef CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX
#ifdef SD_DSNK_REGS
	if ( !(sd_dsnk_regs = sd_iomap(SD_DSNK_REGS, sizeof(struct sd_dsnk_regs))) )
		goto error;
#endif
#ifdef SD_DSRC_REGS
	if ( !(sd_dsrc_regs = sd_iomap(SD_DSRC_REGS, sizeof(struct sd_dsrc_regs))) )
		goto error;
#endif

	memset(&sd_dsxx_fifo_cfg, 0, sizeof(struct sd_fifo_config));
#ifdef SD_FIFO_REGS
	sd_dsxx_fifo_cfg.regs = SD_FIFO_REGS;
#endif
#if defined(SD_FIFO_DATA) && (SD_FIFO_DATA != SD_FIFO_REGS)
	sd_dsxx_fifo_cfg.data = SD_FIFO_DATA;
#else
	sd_dsxx_fifo_cfg.data = sd_dsxx_fifo_cfg.regs;
#endif
#ifdef SD_FIFO_IRQ
	sd_dsxx_fifo_cfg.irq = SD_FIFO_IRQ;
#else
	sd_dsxx_fifo_cfg.irq = -1;
#endif

#ifdef SD_DSRC_PAUSE
	if ( !(sd_dsrc_pause = sd_iomap(SD_DSRC_PAUSE, sizeof(struct sd_pause_regs))) )
		goto error;
	printk("DSRC Pause regs %08x -> %p\n", SD_DSRC_PAUSE, sd_dsrc_pause);
#endif
#ifdef SD_DSNK_PAUSE
	if ( !(sd_dsnk_pause = sd_iomap(SD_DSNK_PAUSE, sizeof(struct sd_pause_regs))) )
		goto error;
	printk("DSNK Pause regs %08x -> %p\n", SD_DSNK_PAUSE, sd_dsnk_pause);
#endif
#else
	printk("No CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX, skipped DSXX setup\n");
#endif // CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX


#ifdef CONFIG_USER_MODULES_SRIO_DEV_TEST_LOOP
	memset(&sd_loop_init_fifo_cfg, 0, sizeof(struct sd_fifo_config));
#ifdef SD_INIT_FIFO_REGS
	sd_loop_init_fifo_cfg.regs = SD_INIT_FIFO_REGS;
#endif
#if defined(SD_INIT_FIFO_DATA) && (SD_INIT_FIFO_DATA != SD_INIT_FIFO_REGS)
	sd_loop_init_fifo_cfg.data = SD_INIT_FIFO_DATA;
#else
	sd_loop_init_fifo_cfg.data = sd_loop_init_fifo_cfg.regs;
#endif
#ifdef SD_INIT_FIFO_IRQ
	sd_loop_init_fifo_cfg.irq = SD_INIT_FIFO_IRQ;
#else
	sd_loop_init_fifo_cfg.irq = -1;
#endif

	memset(&sd_loop_targ_fifo_cfg, 0, sizeof(struct sd_fifo_config));
#ifdef SD_TARG_FIFO_REGS
	sd_loop_targ_fifo_cfg.regs = SD_TARG_FIFO_REGS;
#endif
#if defined(SD_TARG_FIFO_DATA) && (SD_TARG_FIFO_DATA != SD_TARG_FIFO_REGS)
	sd_loop_targ_fifo_cfg.data = SD_TARG_FIFO_DATA;
#else
	sd_loop_targ_fifo_cfg.data = sd_loop_targ_fifo_cfg.regs;
#endif
#ifdef SD_TARG_FIFO_IRQ
	sd_loop_targ_fifo_cfg.irq = SD_TARG_FIFO_IRQ;
#else
	sd_loop_targ_fifo_cfg.irq = -1;
#endif

	memset(&sd_loop_user_fifo_cfg, 0, sizeof(struct sd_fifo_config));
#ifdef SD_USER_FIFO_REGS
	sd_loop_user_fifo_cfg.regs = SD_USER_FIFO_REGS;
#endif
#if defined(SD_USER_FIFO_DATA) && (SD_USER_FIFO_DATA != SD_USER_FIFO_REGS)
	sd_loop_user_fifo_cfg.data = SD_USER_FIFO_DATA;
#else
	sd_loop_user_fifo_cfg.data = sd_loop_user_fifo_cfg.regs;
#endif
#ifdef SD_USER_FIFO_IRQ
	sd_loop_user_fifo_cfg.irq = SD_USER_FIFO_IRQ;
#else
	sd_loop_user_fifo_cfg.irq = -1;
#endif

#ifdef SD_SRIO_CORE_REGS
	sd_srio_cfg.regs = SD_SRIO_CORE_REGS;
#else
	sd_srio_cfg.regs = 0;
#endif

#ifdef SD_SRIO_ADDR_REGS
	sd_srio_cfg.addr = SD_SRIO_ADDR_REGS;
#else
	sd_srio_cfg.addr = 0;
#endif

	printk("CONFIG_USER_MODULES_SRIO_DEV_TEST_LOOP setup:\n");
	printk("  Init FIFO: regs %08x data %08x irq %d\n", sd_loop_init_fifo_cfg.regs,
	       sd_loop_init_fifo_cfg.data, sd_loop_init_fifo_cfg.irq);
	printk("  Targ FIFO: regs %08x data %08x irq %d\n", sd_loop_targ_fifo_cfg.regs,
	       sd_loop_targ_fifo_cfg.data, sd_loop_targ_fifo_cfg.irq);
	printk("  User FIFO: regs %08x data %08x irq %d\n", sd_loop_user_fifo_cfg.regs,
	       sd_loop_user_fifo_cfg.data, sd_loop_user_fifo_cfg.irq);
	printk("  SRIO Core: regs %08x addr %08x\n",
	       sd_srio_cfg.regs, sd_srio_cfg.addr);
#else
	printk("No CONFIG_USER_MODULES_SRIO_DEV_TEST_LOOP, skipped Loop setup\n");
#endif // CONFIG_USER_MODULES_SRIO_DEV_TEST_LOOP


	return 0;

	goto error; // quash compiler warning if none of the #ifdef's above use it
error:
	sd_xparameters_exit();
	return -EIO;
}


