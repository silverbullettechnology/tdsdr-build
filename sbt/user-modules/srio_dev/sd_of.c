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
#include "dsxx/test.h"
#include "loop/test.h"
#include "sd_regs.h"
#include "sd_desc.h"
#include "sd_fifo.h"

struct srio_dev
{
	struct device     *dev;
	struct rio_mport   mport;

	struct sd_fifo    *init;
	struct sd_fifo    *targ;

	uint32_t __iomem  *maint;
	uint32_t __iomem  *sys_regs;
};


#if 0
static void sd_res_printk (const struct resource *r, char *name, int l)
{
	printk("%s: %p", name, r);
	if ( !r )
	{
		printk("\n");
		return;
	}

	printk(": name '%s' start %08x end %08x flags %08lx\n",
	       r->name ? r->name : "(null)",
		   r->start, r->end, r->flags);

	if ( r->parent && l < 3 )
	{
		printk("%*sparent: ", l, "\t\t\t");
		sd_res_printk(r->parent, name, l + 1);
	}
}
#endif


static int sd_of_probe (struct platform_device *pdev)
{
//	struct device_node    *child;
//	struct device_node    *node = pdev->dev.of_node;
	struct srio_dev       *sd;
	struct resource       *maint;
	struct resource       *sys_regs;
	int                    ret = 0;


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
printk("%s: pdev in %p, sd alloc at %p, dev %p\n", __func__, pdev, sd, sd->dev);


	sd->maint = devm_ioremap_resource(sd->dev, maint);
	if ( IS_ERR(sd->maint) )
	{
		dev_err(sd->dev, "maintenance ioremap fail, stop\n");
		goto kfree;
	}
printk("maint mapped %08x -> %p\n", maint->start, sd->maint);

	sd->sys_regs = devm_ioremap_resource(sd->dev, sys_regs);
	if ( IS_ERR(sd->sys_regs) )
	{
		dev_err(sd->dev, "system regs ioremap fail, stop\n");
		goto maint;
	}
printk("sys_regs mapped %08x -> %p\n", sys_regs->start, sd->sys_regs);


	if ( !(sd->init = sd_fifo_probe(pdev, "init")) )
	{
		dev_err(&pdev->dev, "initiator fifo probe fail, stop\n");
		ret = -ENXIO;
		goto sys_regs;
	}
printk("initiator fifo init ok: %p\n", sd->init);

	if ( !(sd->targ = sd_fifo_probe(pdev, "targ")) )
	{
		dev_err(&pdev->dev, "target fifo probe fail, stop\n");
		ret = -ENXIO;
		goto fifo;
	}
printk("target fifo init ok: %p\n", sd->targ);


//	sd_mport_attach(&sd->mport);
//	if ( (ret = rio_register_mport(&sd->mport)) )
//		goto sd_fifo;

	platform_set_drvdata(pdev, sd);

	return ret;

fifo:
	sd_fifo_free(sd->init);

sys_regs:
	devm_iounmap(sd->dev, sd->sys_regs);

maint:
	devm_iounmap(sd->dev, sd->maint);

kfree:
	kfree(sd);

	return ret;
}



static int sd_of_remove (struct platform_device *pdev)
{
	struct srio_dev *sd = platform_get_drvdata(pdev);


printk("%s: pdev in %p, sd alloc at %p, dev %p\n", __func__, pdev, sd, sd->dev);
	platform_set_drvdata(pdev, NULL);

	sd_fifo_free(sd->init);
	sd_fifo_free(sd->targ);

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

