/** \file       spi_shim.c
 *  \brief      SPI fanout shim - connects as a client to an upstream SPI master
 *              (connected to the real device) and provides a bindable downstream master
 *              (for WIP drivers to connect to) and a spidev for userspace access.
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/of_device.h>


#define DRIVER_NAME "spi_shim"


struct spi_shim
{
	struct spi_device *us_device;
	struct spi_master *us_master;
	struct spi_master *ds_master;
	struct spi_device *spidev;
};


static int spi_shim_transfer (struct spi_device *spi, struct spi_message *mesg)
{
	struct spi_shim *priv = spi_master_get_devdata(spi->master);
	pr_debug("%s(spi %p -> master %p -> priv %p):\n", __func__,
	         spi, spi->master, priv);

	pr_debug("  mode 0x%x\n", spi->mode);
	mesg->spi = priv->us_device;
	return spi_async(priv->us_device, mesg);
}

static void spi_shim_cleanup (struct spi_device *spi)
{
	struct spi_shim *priv = spi_master_get_devdata(spi->master);
	pr_debug("%s(spi %p -> master %p -> priv %p):\n", __func__,
	         spi, spi->master, priv);

	if ( priv->us_master->cleanup )
		priv->us_master->cleanup(priv->us_device);
}

static int spi_shim_setup (struct spi_device *spi)
{
	struct spi_shim *priv = spi_master_get_devdata(spi->master);
	int ret;

	pr_debug("%s(spi %p -> master %p -> priv %p):\n", __func__,
	         spi, spi->master, priv);
  
	if ( !priv->us_master->setup )
		return 0;

	priv->us_device->bits_per_word = spi->bits_per_word;
	priv->us_device->mode          = spi->mode;
	ret = priv->us_master->setup(priv->us_device);
	pr_debug("setup: %d bpw, mode 0x%x: ret %d\n", spi->bits_per_word, spi->mode, ret);
	return ret;
}

static int spi_shim_spi_probe (struct spi_device *spi)
{
	struct spi_shim *priv;
	u32              mode;
	u32              bus_num;

	pr_debug("%s(spi %p -> %s):\n", __func__, spi, dev_name(&spi->dev));

	if ( !(priv = kzalloc(sizeof(struct spi_shim), GFP_KERNEL)) )
		return -ENOMEM;
	pr_debug("%s: spi/us_device %p, us_master %p, priv %p\n", __func__, 
	         spi, spi->master, priv);

	mode = 0;
	of_property_read_u32(spi->dev.of_node, "sbt,mode", &mode);
	pr_debug("Set mode 0x%x\n", mode);
	spi->mode = mode;
	spi->master->setup(spi);

	priv->us_device = spi;
	priv->us_master = spi->master;
	spi_set_drvdata(spi, priv);

	if ( !(priv->ds_master = spi_alloc_master(&spi->dev, 0)) )
		goto free1;

    priv->ds_master->num_chipselect = 2;
    priv->ds_master->mode_bits      = SPI_CPHA | SPI_CPOL;
    priv->ds_master->setup          = spi_shim_setup;
    priv->ds_master->transfer       = spi_shim_transfer;
    priv->ds_master->cleanup        = spi_shim_cleanup;
    priv->ds_master->dev.of_node    = spi->dev.of_node;
	pr_debug("%s: ds_master %p\n", __func__, priv->ds_master);

	if ( !of_property_read_u32(spi->dev.of_node, "bus-num", &bus_num) )
		priv->ds_master->bus_num = bus_num & 0xFFFF;

	spi_master_set_devdata(priv->ds_master, priv);
	if ( spi_register_master(priv->ds_master) )
		goto free2;

	if ( !(priv->spidev = spi_alloc_device(priv->ds_master)) )
		goto free2;

	priv->spidev->chip_select      = 1;
	priv->spidev->max_speed_hz     = priv->ds_master->max_speed_hz;
	priv->spidev->mode             = mode;
	priv->spidev->bits_per_word    = 8;
	priv->spidev->irq              = -1;
	priv->spidev->controller_state = NULL;
	priv->spidev->controller_data  = NULL;
	strlcpy(priv->spidev->modalias, "spidev", SPI_NAME_SIZE);
	pr_debug("%s: spidev %p\n", __func__, priv->spidev);

	spi_set_drvdata(priv->spidev, priv);
	if ( spi_add_device(priv->spidev) < 0 )
		goto free3;

	return 0;

free3:
	spi_dev_put(priv->spidev);
free2:
	spi_unregister_master(priv->ds_master);
	spi_master_put(priv->ds_master);
free1:
	kfree(priv);
	return -ENOMEM;
}

static int spi_shim_spi_remove (struct spi_device *spi)
{
	struct spi_shim *priv = spi_get_drvdata(spi);

	spi_set_drvdata(spi, NULL);

	spi_unregister_device(priv->spidev);
	spi_dev_put(priv->spidev);

	spi_unregister_master(priv->ds_master);
	spi_master_put(priv->ds_master);

	kfree(priv);
	return 0;
}

static const struct spi_device_id spi_shim_spi_ids[] =
{
	{ "spi-shim", 1 },
	{ "spi_shim", 2 },
	{}
};
MODULE_DEVICE_TABLE(spi, spi_shim_spi_ids);

static struct spi_driver spi_shim_spi_driver =
{
	.driver =
	{
		.name  = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe    = spi_shim_spi_probe,
	.remove   = spi_shim_spi_remove,
	.id_table = spi_shim_spi_ids,

};
module_spi_driver(spi_shim_spi_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Morgan Hughes <morgan.hughes@silver-bullet-tech.com>");
MODULE_DESCRIPTION("SPI shim module");

