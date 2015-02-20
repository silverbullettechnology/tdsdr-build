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
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/rio.h>
#include <linux/slab.h>

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

	struct sd_fifo     init;
	struct sd_fifo     targ;

	uint32_t __iomem  *regs;
};


static int sd_of_fifo_probe (struct sd_fifo *sf, struct device_node *node)
{
	struct sd_fifo_config  cfg;

	cfg.irq = irq_of_parse_and_map(node, 0);

	return -ENOSYS;
}

static int sd_of_probe (struct platform_device *pdev)
{
	struct device_node    *child;
	struct device_node    *node = pdev->dev.of_node;
	struct srio_dev       *sd;
	int                    ret = 0;

	sd = devm_kzalloc(&pdev->dev, sizeof(*sd), GFP_KERNEL);
	if (!sd) {
		dev_err(&pdev->dev, "Memory alloc fail, stop\n");
		return -ENOMEM;
	}
	sd->dev = &pdev->dev;

	if ( !(sd->regs = of_iomap(node, 0)) )
	{
		dev_err(&pdev->dev, "Register iomap fail, stop\n");
		ret = -ENXIO;
		goto sd_kfree;
	}

	child = of_get_child_by_name(node, "init-fifo");
	if ( !child || (ret = sd_of_fifo_probe(&sd->init, child)) )
	{
		dev_err(&pdev->dev, "Probe init FIFO fail, stop\n");
		goto sd_fifo;
	}

	child = of_get_child_by_name(node, "targ-fifo");
	if ( !child || (ret = sd_of_fifo_probe(&sd->targ, child)) )
	{
		dev_err(&pdev->dev, "Probe targ FIFO fail, stop\n");
		goto sd_fifo;
	}

//	sd_mport_attach(&sd->mport);
//	if ( (ret = rio_register_mport(&sd->mport)) )
//		goto sd_fifo;

	platform_set_drvdata(pdev, sd);

	return 0;

sd_fifo:
	sd_fifo_exit(&sd->init);
	sd_fifo_exit(&sd->targ);

sd_kfree:
	kfree(sd);

	return ret;
}



static int sd_of_remove (struct platform_device *pdev)
{
	struct srio_dev *sd = platform_get_drvdata(pdev);

	sd_fifo_exit(&sd->init);
	sd_fifo_exit(&sd->targ);

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

