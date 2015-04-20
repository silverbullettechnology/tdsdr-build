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
#include <linux/rio_drv.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/ioport.h>

#include "srio_dev.h"
#include "sd_regs.h"
#include "sd_desc.h"
#include "sd_fifo.h"
#include "sd_recv.h"
#include "sd_test.h"
#include "sd_ops.h"
#include "sd_rdma.h"
#include "sd_mbox.h"
#include "sd_user.h"



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


#ifdef SHADOW_FIFO
	if ( !(sd->shadow_ring = kzalloc(4 * sizeof(struct sd_desc), GFP_KERNEL)) )
	{
		dev_err(&pdev->dev, "memory alloc fail, stop\n");
		goto sd_free;
	}
#endif

	sd->maint = devm_ioremap_resource(sd->dev, maint);
	if ( IS_ERR(sd->maint) )
	{
		dev_err(sd->dev, "maintenance ioremap fail, stop\n");
		goto sd_free;
	}
#if 0
printk("maint: 0x%x -> %p\n", maint->start, sd->maint);
printk("  dev_id  : 0x%08x\n", REG_READ(sd->maint + 0x00));
printk("  dev_info: 0x%08x\n", REG_READ(sd->maint + 0x04));
printk("  asm_id  : 0x%08x\n", REG_READ(sd->maint + 0x08));
printk("  asm_info: 0x%08x\n", REG_READ(sd->maint + 0x0C));
printk("  pef     : 0x%08x\n", REG_READ(sd->maint + 0x10));
printk("  swp_info: 0x%08x\n", REG_READ(sd->maint + 0x14));
printk("  src_ops : 0x%08x\n", REG_READ(sd->maint + 0x18));
printk("  dst_ops : 0x%08x\n", REG_READ(sd->maint + 0x1c));
#endif

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


#ifdef SHADOW_FIFO
	if ( !(sd->shadow_fifo = sd_fifo_probe(pdev, "shadow")) )
	{
		dev_err(&pdev->dev, "shadow fifo probe fail, stop\n");
		ret = -ENXIO;
		BUG();
	}
//	sd->shadow_fifo->sd = sd;

	ret = sd_desc_setup_ring(sd->shadow_ring, 4, BUFF_SIZE,
	                         GFP_KERNEL, sd->dev, sd->shadow_fifo->rx.chan ? 1 : 0);
	if ( ret ) 
		BUG();

	for ( idx = 0; idx < 4; idx++ )
	{
		sd->shadow_ring[idx].info = idx;
		sd_fifo_rx_enqueue(sd->shadow_fifo, &sd->shadow_ring[idx]);
	}
#endif


	if ( sd_test_init(sd) )
	{
		pr_err("Settting up sd_test failed, stop\n");
		ret = -EINVAL;
		goto targ_ring;
	}

#if 0
	/* Local and remote register access */
	sd->ops.lcread  = sd_ops_lcread;
	sd->ops.lcwrite = sd_ops_lcwrite;
	sd->ops.cread   = sd_rdma_cread;
	sd->ops.cwrite  = sd_rdma_cwrite;

	/* Send doorbell */
	sd->ops.dsend = sd_ops_dsend;

	/* Mailboxes */
	sd->ops.open_outb_mbox   = sd_mbox_open_outb_mbox;
	sd->ops.close_outb_mbox  = sd_mbox_close_outb_mbox;
	sd->ops.open_inb_mbox    = sd_mbox_open_inb_mbox;
	sd->ops.close_inb_mbox   = sd_mbox_close_inb_mbox;
	sd->ops.add_outb_message = sd_mbox_add_outb_message;
	sd->ops.add_inb_buffer   = sd_mbox_add_inb_buffer;
	sd->ops.get_inb_message  = sd_mbox_get_inb_message;

	sd->mport.priv       = (void *)sd;
	sd->mport.ops        = &sd->ops;
	sd->mport.index      = 0; /* Should come from OF */
	sd->mport.sys_size   = 0; /* Should come from OF, match PL image */
	sd->mport.phy_type   = RIO_PHY_SERIAL;
	sd->mport.phys_efptr = 0x100; /* LPS-EF regs offset */

	INIT_LIST_HEAD(&sd->mport.dbells);

	rio_init_dbell_res(&sd->mport.riores[RIO_DOORBELL_RESOURCE], 0, 0xffff);
	rio_init_mbox_res(&sd->mport.riores[RIO_INB_MBOX_RESOURCE], 0, 3);
	rio_init_mbox_res(&sd->mport.riores[RIO_OUTB_MBOX_RESOURCE], 0, 3);

	snprintf(sd->mport.name, RIO_MAX_MPORT_NAME, "%s", dev_name(sd->dev));

	sd_fifo_init_dir(&sd->init_fifo->rx, sd_recv_init, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->rx, sd_recv_targ, HZ);
	sd_fifo_init_dir(&sd->init_fifo->tx, NULL, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->tx, NULL, HZ);

	if ( (ret = rio_register_mport(&sd->mport)) )
		goto test_exit;
#else
	if ( sd_user_init(sd) )
	{
		pr_err("Settting up sd_user failed, stop\n");
		ret = -EINVAL;
		goto test_exit;
	}
#endif

	// configure transceivers and reset core
	sd_regs_set_gt_loopback(sd,     sd->gt_loopback);
	sd_regs_set_gt_diffctrl(sd,     sd->gt_diffctrl);
	sd_regs_set_gt_txprecursor(sd,  sd->gt_txprecursor);
	sd_regs_set_gt_txpostcursor(sd, sd->gt_txpostcursor);
	sd_regs_set_gt_rxlpmen(sd,      sd->gt_rxlpmen);
	sd_regs_srio_reset(sd);

	platform_set_drvdata(pdev, sd);

	return ret;

test_exit:
	sd_test_exit();

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

	sd_user_exit();
	sd_test_exit();

// no way to unregister an mport yet...
//	rio_unregister_mport(&sd->mport);

	platform_set_drvdata(pdev, NULL);

	sd_fifo_init_dir(&sd->init_fifo->tx, NULL, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->tx, NULL, HZ);
	sd_fifo_init_dir(&sd->init_fifo->rx, NULL, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->rx, NULL, HZ);

	sd_desc_clean_ring(sd->init_ring, sd->init_size, sd->dev);
	sd_desc_clean_ring(sd->targ_ring, sd->targ_size, sd->dev);

	sd_fifo_free(sd->init_fifo);
	sd_fifo_free(sd->targ_fifo);
#ifdef SHADOW_FIFO
	sd_fifo_free(sd->shadow_fifo);
#endif

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

