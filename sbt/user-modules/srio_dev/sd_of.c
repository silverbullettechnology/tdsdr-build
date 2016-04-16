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
#include <linux/moduleparam.h>
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

static int devid = 0xFFFF;
module_param(devid, int, 0);


#define LINK_MASK (SD_SR_STAT_SRIO_LINK_INITIALIZED_M | SD_SR_STAT_SRIO_CLOCK_OUT_LOCK_M \
                 | SD_SR_STAT_SRIO_PORT_INITIALIZED_M | SD_SR_STAT_PORT_ERROR_M)

#define LINK_WANT (SD_SR_STAT_SRIO_LINK_INITIALIZED_M | SD_SR_STAT_SRIO_CLOCK_OUT_LOCK_M \
                 | SD_SR_STAT_SRIO_PORT_INITIALIZED_M)


static void sd_of_reset (unsigned long param)
{
	struct srio_dev *sd = (struct srio_dev *)param;

	pr_err("Reset SRIO link...\n");
	sd_regs_srio_reset(sd);
	sd_fifo_reset(sd->init_fifo, SD_FR_ALL);
	sd_fifo_reset(sd->targ_fifo, SD_FR_ALL);
}


#define SD_OF_STATUS_CHECK  (HZ / 10)
#define SD_OF_STATUS_CLEAR  150
const char *sd_of_status_bits[] =
{
	"link", "port", "clock", "1x", "error", "notintable", "disperr",
	 "", "phy_rcvd_mce", "phy_rcvd_link_reset"
};
static void sd_of_status_tick (unsigned long param)
{
	struct srio_dev *sd   = (struct srio_dev *)param;
	unsigned         curr = sd_regs_srio_status(sd);
	unsigned         diff = curr ^ sd->status_prev;
	unsigned         mask;
	char             buf[256];
	char            *ptr = buf;
	char            *end = buf + sizeof(buf);
	int              bit;

	mod_timer(&sd->status_timer, jiffies + SD_OF_STATUS_CHECK);

	for ( bit = 0; bit < ARRAY_SIZE(sd_of_status_bits); bit++ )
	{
		mask = 1 << bit;
		if ( diff & mask )
		{
			if ( sd->status_counts[bit] < 5 )
				ptr += scnprintf(ptr, end - ptr, "%s%c%s",
				                 ptr > buf ? ", " : "",
				                 (curr & mask) >> bit ? '+' : '-',
				                 sd_of_status_bits[bit]);

			if ( sd->status_counts[bit] < ULONG_MAX )
				sd->status_counts[bit]++;
		}
	}
	if ( ptr > buf )
		pr_info("%s: %s\n", dev_name(sd->dev), buf);

	if ( !sd->status_every )
	{
		ptr = buf;
		for ( bit = 0; bit < ARRAY_SIZE(sd_of_status_bits); bit++ )
			if ( sd->status_counts[bit] > 5 )
				ptr += scnprintf(ptr, end - ptr, "%s%s:%lu",
				                 ptr > buf ? ", " : "counts: ",
				                 sd_of_status_bits[bit],
				                 sd->status_counts[bit]);
		if ( ptr > buf )
			pr_info("%s: %s\n", dev_name(sd->dev), buf);

		memset(sd->status_counts, 0, sizeof(sd->status_counts));
		sd->status_every = SD_OF_STATUS_CLEAR;
	}
	else
		sd->status_every--;

	// Check for link status change
	if ( diff & LINK_MASK )
	{
		if ( (curr & LINK_MASK) == LINK_WANT )
		{
			pr_info("SRIO link up\n");
			sd_dhcp_resume(sd);
		}
		else
		{
			pr_info("SRIO link down\n");
			sd_dhcp_pause(sd);
		}
	}

	// Check for link status change
	if ( (diff & SD_SR_STAT_PORT_ERROR_M) && (curr & SD_SR_STAT_PORT_ERROR_M) )
	{
		pr_err("SRIO link error, start recovery\n");
		mod_timer(&sd->reset_timer, jiffies + HZ);
	}
	
	sd->status_prev = curr;
}


static int sd_of_probe (struct platform_device *pdev)
{
	struct srio_dev       *sd;
	struct resource       *maint;
	struct resource       *sys_regs;
	struct resource       *drp_regs;
	int                    ret = 0;
	u32                    flags;


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

	if ( !(drp_regs = platform_get_resource_byname(pdev, IORESOURCE_MEM, "drp-regs")) )
	{
		dev_err(&pdev->dev, "resource drp-regs undefined.\n");
		return -ENXIO;
	}

	sd = kzalloc(sizeof(*sd), GFP_KERNEL);
	if (!sd) {
		dev_err(&pdev->dev, "memory alloc fail, stop\n");
		return -ENOMEM;
	}
	sd->dev = &pdev->dev;

	/* Default values */
	sd->gt_loopback     = 0;
	sd->gt_diffctrl     = 8;
	sd->gt_txprecursor  = 0;
	sd->gt_txpostcursor = 0;
	sd->gt_rxlpmen      = 0;

	/* Configure from DT */
	of_property_read_u32(pdev->dev.of_node, "sbt,gt-loopback",     &sd->gt_loopback);
	of_property_read_u32(pdev->dev.of_node, "sbt,gt-diffctrl",     &sd->gt_diffctrl);
	of_property_read_u32(pdev->dev.of_node, "sbt,gt-txprecursor",  &sd->gt_txprecursor);
	of_property_read_u32(pdev->dev.of_node, "sbt,gt-txpostcursor", &sd->gt_txpostcursor);
	of_property_read_u32(pdev->dev.of_node, "sbt,gt-rxlpmen",      &sd->gt_rxlpmen);


	sd->maint = devm_ioremap_resource(sd->dev, maint);
	if ( IS_ERR(sd->maint) )
	{
		dev_err(sd->dev, "maintenance ioremap fail, stop\n");
		goto sd_free;
	}
	sd->pef = REG_READ(sd->maint + 0x10);

pr_debug("maint: 0x%x -> %p\n", maint->start, sd->maint);
pr_debug("  dev_id  : 0x%08x\n", REG_READ(sd->maint + 0x00));
pr_debug("  dev_info: 0x%08x\n", REG_READ(sd->maint + 0x04));
pr_debug("  asm_id  : 0x%08x\n", REG_READ(sd->maint + 0x08));
pr_debug("  asm_info: 0x%08x\n", REG_READ(sd->maint + 0x0C));
pr_debug("  pef     : 0x%08x\n", REG_READ(sd->maint + 0x10));

	sd->sys_regs = devm_ioremap_resource(sd->dev, sys_regs);
	if ( IS_ERR(sd->sys_regs) )
	{
		dev_err(sd->dev, "system regs ioremap fail, stop\n");
		goto maint;
	}

	sd->drp_regs = devm_ioremap_resource(sd->dev, drp_regs);
	if ( IS_ERR(sd->drp_regs) )
	{
		dev_err(sd->dev, "DRP regs ioremap fail, stop\n");
		goto sys_regs;
	}
pr_debug("DRP: 0x%x -> %p\n", drp_regs->start, sd->drp_regs);
pr_debug("RX_CM_SEL / RX_CM_TRIM: %04x.%04x.%04x.%04x\n",
         REG_READ(sd->drp_regs + 0x0044),
         REG_READ(sd->drp_regs + 0x0844),
         REG_READ(sd->drp_regs + 0x1044),
         REG_READ(sd->drp_regs + 0x1844));

	if ( sd_desc_init(sd) )
	{
		pr_err("Settting up sd_desc failed, stop\n");
		ret = -EINVAL;
		goto drp_regs;
	}

	flags = SD_FL_INFO | SD_FL_WARN | SD_FL_ERROR;
	of_property_read_u32(pdev->dev.of_node, "sbt,init-flags", &flags);
	if ( !(sd->init_fifo = sd_fifo_probe(pdev, "init", flags)) )
	{
		dev_err(&pdev->dev, "initiator fifo probe fail, stop\n");
		ret = -ENXIO;
		goto desc_init;
	}
	sd->init_fifo->sd = sd;

	flags = SD_FL_INFO | SD_FL_WARN | SD_FL_ERROR;
	of_property_read_u32(pdev->dev.of_node, "sbt,targ-flags", &flags);
	if ( !(sd->targ_fifo = sd_fifo_probe(pdev, "targ", flags)) )
	{
		dev_err(&pdev->dev, "target fifo probe fail, stop\n");
		ret = -ENXIO;
		goto init_fifo;
	}
	sd->targ_fifo->sd = sd;


	flags = SD_FL_TRACE | SD_FL_DEBUG | SD_FL_INFO | SD_FL_WARN | SD_FL_ERROR | SD_FL_NO_RX | SD_FL_NO_TX | SD_FL_NO_RSP;
	of_property_read_u32(pdev->dev.of_node, "sbt,shadow-flags", &flags);
	if ( (sd->shadow_fifo = sd_fifo_probe(pdev, "shadow", flags)) )
	{
		pr_debug("Shadow FIFO active\n");
		sd->shadow_fifo->sd = sd;
	}
	else
		pr_debug("No shadow FIFO defined\n");

	/* "ping" function implemented with dbells: if both values are set low enough to be
	 * valid dbell "info" values (ie <= 0xFFFF) then respond to dbell messages with info
	 * equal to ping[0], and respond with a dbell containing info value ping[1].  Pre-
	 * shift the 16-bit values since info is the high half of the HELLO word.  */
	sd->ping[0] = 0xFFFFFFFF;
	sd->ping[1] = 0xFFFFFFFF;
	of_property_read_u32_array(pdev->dev.of_node, "sbt,ping", sd->ping,
	                           ARRAY_SIZE(sd->ping));
	if ( sd->ping[0] <= 0xFFFF && sd->ping[1] <= 0xFFFF )
	{
		sd->ping[0] <<= 16;
		sd->ping[1] <<= 16;
		pr_info("%s: Expect dbell ping requests on %04x, response on %04x\n",
		        dev_name(sd->dev), sd->ping[0] >> 16, sd->ping[1] >> 16);
	}


	if ( sd_test_init(sd) )
	{
		pr_err("Settting up sd_test failed, stop\n");
		ret = -EINVAL;
		goto targ_fifo;
	}

	if ( sd_user_init(sd) )
	{
		pr_err("Settting up sd_user failed, stop\n");
		ret = -EINVAL;
		goto test_exit;
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

	sd_fifo_init_rx(sd->init_fifo, sd_recv_init, HZ);
	sd_fifo_init_rx(sd->targ_fifo, sd_recv_targ, HZ);
	sd_fifo_init_tx(sd->init_fifo, NULL, HZ);
	sd_fifo_init_tx(sd->targ_fifo, NULL, HZ);

	if ( (ret = rio_register_mport(&sd->mport)) )
		goto test_exit;
#endif

	// configure transceivers and reset core
	sd_regs_set_gt_loopback(sd,     sd->gt_loopback);
	sd_regs_set_gt_diffctrl(sd,     sd->gt_diffctrl);
	sd_regs_set_gt_txprecursor(sd,  sd->gt_txprecursor);
	sd_regs_set_gt_txpostcursor(sd, sd->gt_txpostcursor);
	sd_regs_set_gt_rxlpmen(sd,      sd->gt_rxlpmen);
	sd_regs_srio_reset(sd);

	platform_set_drvdata(pdev, sd);

	sd->status_prev  = sd_regs_srio_status(sd);
	sd->status_every = SD_OF_STATUS_CLEAR;
	setup_timer(&sd->status_timer, sd_of_status_tick, (unsigned long)sd);
	mod_timer(&sd->status_timer,   jiffies + SD_OF_STATUS_CHECK);

	setup_timer(&sd->reset_timer, sd_of_reset, (unsigned long)sd);

	/* Module parameter overrides devicetree, devicetree can override default. */
	sd->devid = 0xFFFF;
	if ( devid == 0xFFFF )
	{
		u32 val = 0xFFFF;
		of_property_read_u32(pdev->dev.of_node, "sbt,device-id", &val);
		if ( val < 0xFFFF )
		{
			pr_info("%s: Device-ID set by devicetree at 0x%04x.\n",
			        dev_name(sd->dev), val);
			devid = val;
		}
	}
	else
		pr_info("%s: Device-ID set by user at 0x%04x.\n",
		        dev_name(sd->dev), devid);

	/* Do dynamic probe if device-ID is still not set */
	if ( devid == 0xFFFF )
	{
		u32  min = 1;
		u32  max = 6;
		u32  rep = 5;

		of_property_read_u32(pdev->dev.of_node, "sbt,device-id-min", &min);
		of_property_read_u32(pdev->dev.of_node, "sbt,device-id-max", &max);
		of_property_read_u32(pdev->dev.of_node, "sbt,device-id-rep", &rep);

		pr_info("%s: Start Device-ID probe: min %u, max %u, rep %u\n",
		         dev_name(sd->dev), min, max, rep);
		sd_dhcp_start(sd, min, max, rep);
	}
	else
	{
		sd_regs_set_devid(sd, devid);
		sd_user_attach(sd);
	}

	return ret;

test_exit:
	sd_test_exit();

targ_fifo:
	sd_fifo_free(sd->targ_fifo);

init_fifo:
	sd_fifo_free(sd->init_fifo);

desc_init:
	sd_desc_exit(sd);

drp_regs:
	devm_iounmap(sd->dev, sd->drp_regs);

sys_regs:
	devm_iounmap(sd->dev, sd->sys_regs);

maint:
	devm_iounmap(sd->dev, sd->maint);

sd_free:
	kfree(sd);

	return ret;
}



static int sd_of_remove (struct platform_device *pdev)
{
	struct srio_dev *sd = platform_get_drvdata(pdev);

	sd_dhcp_pause(sd);

	del_timer(&sd->status_timer);
	del_timer(&sd->reset_timer);

	sd_user_exit();
	sd_test_exit();

// no way to unregister an mport yet...
//	rio_unregister_mport(&sd->mport);

	platform_set_drvdata(pdev, NULL);

	sd_fifo_init_rx(sd->init_fifo, NULL, HZ);
	sd_fifo_init_rx(sd->targ_fifo, NULL, HZ);
	sd_fifo_init_tx(sd->init_fifo, NULL, HZ);
	sd_fifo_init_tx(sd->targ_fifo, NULL, HZ);

	sd_fifo_free(sd->init_fifo);
	sd_fifo_free(sd->targ_fifo);
	if ( sd->shadow_fifo )
		sd_fifo_free(sd->shadow_fifo);

	sd_desc_exit(sd); // must come after sd_fifo_free()

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

