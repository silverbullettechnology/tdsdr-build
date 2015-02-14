/** \file       srio_exp.c
 *  \brief      SRIO experiments harness - for developing the FIFO/DMA primitives for
 *              later use in a proper SRIO driver for Xilinx SRIO in Zynq PL.
 *
 *  \copyright Copyright 2014 Silver Bullet Technologies
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
#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include "srio_dev.h"
#include "dsxx/test.h"
#include "loop/test.h"
#include "sd_regs.h"
#include "sd_desc.h"
#include "sd_fifo.h"
#include "sd_xparameters.h"


static int dma = 1;
module_param(dma, int, 0);


static int __init srio_exp_init (void)
{
	int  ret = 0;

	if ( dma )
		dma = (1 << DMA_MEM_TO_DEV) | (1 << DMA_DEV_TO_MEM);

	if ( sd_xparameters_init() )
	{
		pr_err("Failed to iomap regs, stop\n");
		return -ENODEV;
	}

#ifdef CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX
	if ( sd_dsxx_fifo_cfg.regs )
	{
		sd_dsxx_fifo_cfg.dma = dma;
		if ( !sd_dsxx_init() )
		{
			pr_err("DSXX test failed, stop\n");
			ret = -EINVAL;
			goto unmap;
		}
	}
	else
#endif // CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX
#ifdef CONFIG_USER_MODULES_SRIO_DEV_TEST_LOOP
	if ( sd_srio_cfg.regs )
	{
		sd_loop_init_fifo_cfg.dma = dma;
		sd_loop_targ_fifo_cfg.dma = dma;
		sd_loop_user_fifo_cfg.dma = dma;
		if ( !sd_loop_init() )
		{
			pr_err("LOOP test failed, stop\n");
			ret = -EINVAL;
			goto unmap;
		}
	}
	else
#endif // CONFIG_USER_MODULES_SRIO_DEV_TEST_LOOP
	{
		pr_err("Don't know what to do, stop\n");
		ret = -ENOSYS;
		goto unmap;
	}

	// for testing
	panic_timeout = 5;

	printk("SRIO Experiment loaded OK\n");
	return ret;

unmap:
	sd_xparameters_exit();

	return ret;
}
module_init(srio_exp_init);

static void __exit srio_exp_exit (void)
{
#ifdef CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX
	if ( sd_dsxx_fifo_cfg.regs )
		sd_dsxx_exit();
#endif // CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX
#ifdef CONFIG_USER_MODULES_SRIO_DEV_TEST_LOOP
	if ( sd_srio_cfg.regs )
		sd_loop_exit();
#endif // CONFIG_USER_MODULES_SRIO_DEV_TEST_LOOP

	sd_xparameters_exit();
}
module_exit(srio_exp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Morgan Hughes <morgan.hughes@silver-bullet-tech.com>");
MODULE_DESCRIPTION("SRIO experiments module");

