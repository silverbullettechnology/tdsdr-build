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
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/rio.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/ioport.h>

#include "srio_dev.h"
#include "loop/test.h"
#include "loop/proc.h"
#include "sd_regs.h"
#include "sd_desc.h"
#include "sd_fifo.h"
#include "sd_recv.h"


// Temporary: should be in the DT
#ifndef RX_RING_SIZE
#define RX_RING_SIZE 8
#endif


static int sd_of_probe (struct platform_device *pdev)
{
	struct srio_dev       *sd;
	struct resource       *maint;
	struct resource       *sys_regs;
	int                    ret = 0;
	int                    idx;


	if ( !(maint = platform_get_resource_byname(pdev, IORESOURCE_MEM, "maint")) )
	{
		dev_err(&pdev->dev, "resource maint undefined.\n");
		return -ENXIO;
	}

	if ( !(sys_regs = platform_get_resource_byname(pdev, IORESOURCE_MEM, "sys-regs")) )
	{
		dev_err(&pdev->dev, "resource sys-regs undefined.\n");
		return -ENXIO;
	}

	sd = kzalloc(sizeof(*sd), GFP_KERNEL);
	if (!sd) {
		dev_err(&pdev->dev, "memory alloc fail, stop\n");
		return -ENOMEM;
	}
	sd->dev = &pdev->dev;


	/* Default values */
	sd->init_size       = 8;
	sd->targ_size       = 8;
	sd->gt_loopback     = 0;
	sd->gt_diffctrl     = 8;
	sd->gt_txprecursor  = 0;
	sd->gt_txpostcursor = 0;
	sd->gt_rxlpmen      = 0;

	/* Configure from DT */
	of_property_read_u32(pdev->dev.of_node, "sbt,init-size",       &sd->init_size);
	of_property_read_u32(pdev->dev.of_node, "sbt,targ-size",       &sd->targ_size);
	of_property_read_u32(pdev->dev.of_node, "sbt,gt-loopback",     &sd->gt_loopback);
	of_property_read_u32(pdev->dev.of_node, "sbt,gt-diffctrl",     &sd->gt_diffctrl);
	of_property_read_u32(pdev->dev.of_node, "sbt,gt-txprecursor",  &sd->gt_txprecursor);
	of_property_read_u32(pdev->dev.of_node, "sbt,gt-txpostcursor", &sd->gt_txpostcursor);
	of_property_read_u32(pdev->dev.of_node, "sbt,gt-rxlpmen",      &sd->gt_rxlpmen);

	if ( sd->init_size < 4 ) sd->init_size = 4;
	if ( sd->targ_size < 4 ) sd->targ_size = 4;


	if ( !(sd->init_ring = kzalloc(sd->init_size * sizeof(struct sd_desc), GFP_KERNEL)) )
	{
		dev_err(&pdev->dev, "memory alloc fail, stop\n");
		goto sd_free;
	}

	if ( !(sd->targ_ring = kzalloc(sd->targ_size * sizeof(struct sd_desc), GFP_KERNEL)) )
	{
		dev_err(&pdev->dev, "memory alloc fail, stop\n");
		goto sd_free;
	}


	sd->maint = devm_ioremap_resource(sd->dev, maint);
	if ( IS_ERR(sd->maint) )
	{
		dev_err(sd->dev, "maintenance ioremap fail, stop\n");
		goto sd_free;
	}

	sd->sys_regs = devm_ioremap_resource(sd->dev, sys_regs);
	if ( IS_ERR(sd->sys_regs) )
	{
		dev_err(sd->dev, "system regs ioremap fail, stop\n");
		goto maint;
	}


	if ( !(sd->init_fifo = sd_fifo_probe(pdev, "init")) )
	{
		dev_err(&pdev->dev, "initiator fifo probe fail, stop\n");
		ret = -ENXIO;
		goto sys_regs;
	}

	if ( !(sd->targ_fifo = sd_fifo_probe(pdev, "targ")) )
	{
		dev_err(&pdev->dev, "target fifo probe fail, stop\n");
		ret = -ENXIO;
		goto init_fifo;
	}

	ret = sd_desc_setup_ring(sd->init_ring, sd->init_size, BUFF_SIZE,
	                         GFP_KERNEL, sd->dev, sd->init_fifo->rx.chan ? 1 : 0);
	if ( ret ) 
	{
		pr_err("sd_desc_setup_ring() for initator failed: %d\n", ret);
		goto targ_fifo;
	}

	ret = sd_desc_setup_ring(sd->targ_ring, sd->targ_size, BUFF_SIZE,
	                         GFP_KERNEL, sd->dev, sd->targ_fifo->rx.chan ? 1 : 0);
	if ( ret ) 
	{
		pr_err("sd_desc_setup_ring() for target failed: %d\n", ret);
		goto init_ring;
	}

	for ( idx = 0; idx < sd->init_size; idx++ )
	{
		sd->init_ring[idx].info = idx;
		sd_fifo_rx_enqueue(sd->init_fifo, &sd->init_ring[idx]);
	}

	for ( idx = 0; idx < sd->targ_size; idx++ )
	{
		sd->targ_ring[idx].info = idx;
		sd_fifo_rx_enqueue(sd->targ_fifo, &sd->targ_ring[idx]);
	}

	sd_fifo_init_dir(&sd->init_fifo->rx, sd_recv_init, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->rx, sd_recv_targ, HZ);

	// configure transceivers and reset core
	sd_regs_set_gt_loopback(sd,     sd->gt_loopback);
	sd_regs_set_gt_diffctrl(sd,     sd->gt_diffctrl);
	sd_regs_set_gt_txprecursor(sd,  sd->gt_txprecursor);
	sd_regs_set_gt_txpostcursor(sd, sd->gt_txpostcursor);
	sd_regs_set_gt_rxlpmen(sd,      sd->gt_rxlpmen);
	sd_regs_srio_reset(sd);

	if ( sd_test_init(sd) )
	{
		pr_err("Settting up sd_test failed, stop\n");
		ret = -EINVAL;
		goto targ_ring;
	}

#if 0
	sd_fifo_init_dir(&sd->init_fifo->tx, NULL, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->tx, NULL, HZ);

	sd_ops_attach(&sd->ops);
	sd->mport.priv       = (void *)sd;
	sd->mport.ops        = &sd->ops;
	sd->mport.index      = 0; /* Should come from OF */
	sd->mport.sys_size   = 0; /* Should come from OF, match PL image */
	sd->mport.phy_type   = RIO_PHY_SERIAL;
	sd->mport.phys_efptr = 0x100; /* LPS-EF regs offset */

	if ( (ret = rio_register_mport(&sd->mport)) )
	{
		sd_ops_detach(&sd->ops);
		goto targ_ring;
	}
#endif

	platform_set_drvdata(pdev, sd);

	return ret;


targ_ring:
	sd_desc_clean_ring(sd->targ_ring, sd->targ_size, sd->dev);

init_ring:
	sd_desc_clean_ring(sd->init_ring, sd->init_size, sd->dev);

targ_fifo:
	sd_fifo_free(sd->targ_fifo);

init_fifo:
	sd_fifo_free(sd->init_fifo);

sys_regs:
	devm_iounmap(sd->dev, sd->sys_regs);

maint:
	devm_iounmap(sd->dev, sd->maint);

sd_free:
	kfree(sd->targ_ring);
	kfree(sd->init_ring);
	kfree(sd);

	return ret;
}



static int sd_of_remove (struct platform_device *pdev)
{
	struct srio_dev *sd = platform_get_drvdata(pdev);

	sd_test_exit();

// no way to unregister an mport yet...
//	rio_unregister_mport(&sd->mport);

	platform_set_drvdata(pdev, NULL);

	sd_fifo_init_dir(&sd->init_fifo->tx, NULL, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->tx, NULL, HZ);
	sd_fifo_init_dir(&sd->init_fifo->rx, NULL, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->rx, NULL, HZ);

	sd_desc_clean_ring(sd->init_ring, RX_RING_SIZE, sd->dev);
	sd_desc_clean_ring(sd->targ_ring, RX_RING_SIZE, sd->dev);

	sd_fifo_free(sd->init_fifo);
	sd_fifo_free(sd->targ_fifo);

	kfree(sd);

	return 0;
}


static const struct of_device_id sd_of_ids[] = {
	{ .compatible = "sbt,srio-dev" },
	{}
};

static struct platform_driver sd_of_driver = {
	.driver = {
		.name = "srio-dev",
		.owner = THIS_MODULE,
		.of_match_table = sd_of_ids,
	},
	.probe = sd_of_probe,
	.remove = sd_of_remove,
};

static int __init sd_of_init (void)
{
	return platform_driver_register(&sd_of_driver);
}
module_init(sd_of_init);

static void __exit sd_of_exit (void)
{
	platform_driver_unregister(&sd_of_driver);
}
module_exit(sd_of_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Morgan Hughes <morgan.hughes@silver-bullet-tech.com>");
MODULE_DESCRIPTION("SRIO-dev on TDSDR");

