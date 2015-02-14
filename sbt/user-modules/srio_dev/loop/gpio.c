/** \file      loop/gpio.c
 *  \brief     GPIO control/status for Loopback test
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
#include <linux/delay.h>
#include <linux/gpio.h>

#include "gpio.h"
#include "../srio_dev.h"


int sd_loop_gpio_init (void)
{
	int pin, ret;

	printk("%s: request GPIOs...\n", SD_DRIVER_NODE);
	for ( pin = LED0_PIN; pin <= GT_TXPRBSFORCEERR_PIN; pin++ )
		if ( (ret = gpio_request(pin, SD_DRIVER_NODE)) )
		{
			printk("%s: GPIO %d failed (%d), unwind and stop.\n",
			       SD_DRIVER_NODE, pin, ret);

			for ( pin--; pin >= LED0_PIN; pin-- )
				gpio_free(pin);

			return ret;
		}

	printk("%s: got GPIOs, setup direction/value...\n", SD_DRIVER_NODE);
	gpio_direction_output(LED0_PIN,                 0);
	gpio_direction_output(LED1_PIN,                 0);
	gpio_direction_output(LED2_PIN,                 0);
//	gpio_direction_output(LED3_PIN,                 0); // SRIO / PCIE SELECT
	gpio_direction_output(SRIO_PCIE_SEL_PIN,        1); // Select AMC 8/9
	gpio_direction_output(SYS_RST_PIN,              0);
	gpio_direction_output(AXI_MAINTR_RST_PIN,       0);
	gpio_direction_output(SIM_TRAIN_PIN,            0);
	gpio_direction_output(PHY_MCE_PIN,              0);
	gpio_direction_output(PHY_LINK_RST_PIN,         0);
	gpio_direction_output(FORCE_REINIT_PIN,         0);

	gpio_direction_input(LINK_INITIALIZED_PIN);
	gpio_direction_input(PORT_INITIALIZED_PIN);
	gpio_direction_input(CLK_LOCK_OUT_PIN);
	gpio_direction_input(SRIO_MODE_1X_PIN);

	gpio_direction_output(LOOPBACK0_PIN,            0);
	gpio_direction_output(LOOPBACK1_PIN,            0);
	gpio_direction_output(LOOPBACK2_PIN,            0);

	gpio_direction_output(GT_RXCDRHOLD_PIN,         0);
	gpio_direction_output(GT_PRBSSEL0_PIN,          0);
	gpio_direction_output(GT_PRBSSEL1_PIN,          0);
	gpio_direction_output(GT_PRBSSEL2_PIN,          0);
	gpio_direction_output(GT_RX_PRBS_CNT_RESET_PIN, 0);

	gpio_direction_output(GT_TXPRBSFORCEERR_PIN,    0);
	gpio_direction_input(GT_RXPRBS_ERR_PIN);

	printk("%s: GPIO setup success\n", SD_DRIVER_NODE);
	return 0;
}

void sd_loop_gpio_exit (void)
{
	int pin;

	for ( pin = LED0_PIN; pin <= GT_TXPRBSFORCEERR_PIN; pin++ )
		gpio_free(pin);
}

void sd_loop_gpio_srio_reset (void)
{
	gpio_set_value(SYS_RST_PIN,      1);
	gpio_set_value(PHY_LINK_RST_PIN, 0);
	gpio_set_value(FORCE_REINIT_PIN, 0);

	msleep(10);

	gpio_set_value(SYS_RST_PIN,      0);
	gpio_set_value(PHY_LINK_RST_PIN, 0);
	gpio_set_value(FORCE_REINIT_PIN, 0);

	msleep(10);
}

void sd_loop_gpio_prbs_reset (void)
{
	gpio_set_value(GT_RX_PRBS_CNT_RESET_PIN, 1);
	msleep(1000);
	gpio_set_value(GT_RX_PRBS_CNT_RESET_PIN, 0);
}

size_t sd_loop_gpio_print_srio (char *dst, size_t max)
{
	char *p = dst;
	char *e = dst + max;

	p += scnprintf(p, e - p, "link_initialized   [%d]\n", gpio_get_value(LINK_INITIALIZED_PIN));
	p += scnprintf(p, e - p, "port_initialized   [%d]\n", gpio_get_value(PORT_INITIALIZED_PIN));
	p += scnprintf(p, e - p, "clk_lock_out       [%d]\n", gpio_get_value(CLK_LOCK_OUT_PIN));
	p += scnprintf(p, e - p, "srio_mode_1x       [%d]\n", gpio_get_value(SRIO_MODE_1X_PIN));
	p += scnprintf(p, e - p, "port_error         [%d]\n", gpio_get_value(PORT_ERROR_PIN));
	p += scnprintf(p, e - p, "gtrx_disperr_or    [%d]\n", gpio_get_value(GTRX_DISPERR_OR_PIN));
	p += scnprintf(p, e - p, "gtrx_notintable_or [%d]\n", gpio_get_value(GTRX_NOTINTABLE_OR_PIN));
	p += scnprintf(p, e - p, "\n");

	return p - dst;
}

size_t sd_loop_gpio_print_gt_prbs (char *dst, size_t max)
{
	char *p = dst;
	char *e = dst + max;

	p += scnprintf(p, e - p, "rx_cdr_hold      [%d]\n",     gpio_get_value(GT_RXCDRHOLD_PIN));
	p += scnprintf(p, e - p, "rx_loopback      [%d%d%d]\n", gpio_get_value(LOOPBACK2_PIN), gpio_get_value(LOOPBACK1_PIN), gpio_get_value(LOOPBACK0_PIN));
	p += scnprintf(p, e - p, "rx_prbs_sel      [%d%d%d]\n", gpio_get_value(GT_PRBSSEL2_PIN), gpio_get_value(GT_PRBSSEL1_PIN), gpio_get_value(GT_PRBSSEL0_PIN));
	p += scnprintf(p, e - p, "rx_prbs_cnt_rst  [%d]\n",     gpio_get_value(GT_RX_PRBS_CNT_RESET_PIN));
	p += scnprintf(p, e - p, "rx_prbs_err      [%d]\n",     gpio_get_value(GT_RXPRBS_ERR_PIN));
	p += scnprintf(p, e - p, "prbs_force_err   [%d]\n",     gpio_get_value(GT_TXPRBSFORCEERR_PIN));

	return p - dst;
}

void sd_loop_gpio_prbs_force_err (int en)
{
	if (en)
		gpio_set_value(GT_TXPRBSFORCEERR_PIN, 1);
	else
		gpio_set_value(GT_TXPRBSFORCEERR_PIN, 0);
}

void sd_loop_gpio_set_gt_prbs (int mode)
{
	switch (mode)
	{
		case 0:
			printk("PRBS: Normal \n");
			gpio_set_value(GT_PRBSSEL2_PIN, 0);
			gpio_set_value(GT_PRBSSEL1_PIN, 0);
			gpio_set_value(GT_PRBSSEL0_PIN, 0);
			break;
		case 1:
			printk("PRBS: PRBS-7 \n");
			gpio_set_value(GT_PRBSSEL2_PIN, 0);
			gpio_set_value(GT_PRBSSEL1_PIN, 0);
			gpio_set_value(GT_PRBSSEL0_PIN, 1);
			break;
		case 2:
			printk("PRBS: PRBS-15 \n");
			gpio_set_value(GT_PRBSSEL2_PIN, 0);
			gpio_set_value(GT_PRBSSEL1_PIN, 1);
			gpio_set_value(GT_PRBSSEL0_PIN, 0);
			break;
		case 3:
			printk("PRBS: PRBS-23 \n");
			gpio_set_value(GT_PRBSSEL2_PIN, 0);
			gpio_set_value(GT_PRBSSEL1_PIN, 1);
			gpio_set_value(GT_PRBSSEL0_PIN, 1);
			break;
		case 4:
			printk("PRBS: PRBS-31 \n");
			gpio_set_value(GT_PRBSSEL2_PIN, 1);
			gpio_set_value(GT_PRBSSEL1_PIN, 0);
			gpio_set_value(GT_PRBSSEL0_PIN, 0);
			break;
		default:
			printk("PRBS: Normal \n");
			gpio_set_value(GT_PRBSSEL2_PIN, 0);
			gpio_set_value(GT_PRBSSEL1_PIN, 0);
			gpio_set_value(GT_PRBSSEL0_PIN, 0);
			break;
	}
}

void sd_loop_gpio_set_gt_loopback (int mode)
{
	switch (mode)
	{
		case 0:
			 printk("Loopback: Normal  \n");
			 gpio_set_value(LOOPBACK0_PIN,    0);
			 gpio_set_value(LOOPBACK1_PIN,    0);
			 gpio_set_value(LOOPBACK2_PIN,    0);
			 gpio_set_value(GT_RXCDRHOLD_PIN, 0);
			 break;

		case 1:
			 printk("Loopback: PCS (1)  \n");
			 gpio_set_value(LOOPBACK0_PIN,    1);
			 gpio_set_value(LOOPBACK1_PIN,    0);
			 gpio_set_value(LOOPBACK2_PIN,    0);
			 gpio_set_value(GT_RXCDRHOLD_PIN, 1);
			 break;

		case 2:
			 printk("Loopback: PMA (2)  \n");
			 gpio_set_value(LOOPBACK0_PIN,    0);
			 gpio_set_value(LOOPBACK1_PIN,    1);
			 gpio_set_value(LOOPBACK2_PIN,    0);
			 gpio_set_value(GT_RXCDRHOLD_PIN, 0);
			 break;

		default:
			 printk("Loopback: Normal  \n");
			 gpio_set_value(LOOPBACK0_PIN,    0);
			 gpio_set_value(LOOPBACK1_PIN,    0);
			 gpio_set_value(LOOPBACK2_PIN,    0);
			 gpio_set_value(GT_RXCDRHOLD_PIN, 0);
			 break;
	}
}



