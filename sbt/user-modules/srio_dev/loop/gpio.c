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
#include <linux/hrtimer.h>

#include "loop/gpio.h"
#include "../srio_dev.h"


// OUTPUT PINS
#define LED_0_PIN          (54 + 0)
#define LED_1_PIN          (54 + 1)
#define LED_2_PIN          (54 + 2)
//#define LED_3_PIN        (54 + 3)
#define SRIO_PCIE_SEL_PIN  (54 + 3)

#define SRIO_SYS_RST_PIN           (54 + 4)
#define SRIO_S_AXI_MAINTR_RST_PIN  (54 + 5)
#define SRIO_SIM_TRAIN_EN_PIN      (54 + 6)
#define SRIO_PHY_MCE_PIN           (54 + 7)
#define SRIO_PHY_LINK_RESET_PIN    (54 + 8)
#define SRIO_FORCE_REINIT_PIN      (54 + 9)

// INPUT PINS
#define SRIO_LINK_INITIALIZED_PIN  (54 + 10)
#define SRIO_PORT_INITIALIZED_PIN  (54 + 11)
#define SRIO_CLK_LOCK_OUT_PIN      (54 + 12)
#define SRIO_MODE_1X_PIN           (54 + 13)

// LOOPBACK PINS
#define GT_LOOPBACK_0_PIN           (54 + 14)
#define GT_LOOPBACK_1_PIN           (54 + 15)
#define GT_LOOPBACK_2_PIN           (54 + 16)

// OUTPUT DEBUG PINS
#define GT0_QPLL_LOCK_OUT_PIN      (54 + 17)
#define GT_PCS_RST_OUT_PIN         (54 + 18)
#define GTRX_DISPERR_OR_PIN        (54 + 19)
#define GTRX_NOTINTABLE_OR_PIN     (54 + 20)
#define LOG_RST_OUT_PIN            (54 + 21)
#define PHY_RCVD_LINK_RESET_PIN    (54 + 22)
#define PHY_RCVD_MCE_PIN           (54 + 23)
#define PHY_RST_OUT_PIN            (54 + 24)
#define PORT_ERROR_PIN             (54 + 25)

// TX drive control pins
#define GT_TXDIFFCTRL_0_PIN        (54 + 26)
#define GT_TXDIFFCTRL_1_PIN        (54 + 27)
#define GT_TXDIFFCTRL_2_PIN        (54 + 28)
#define GT_TXDIFFCTRL_3_PIN        (54 + 29)
#define GT_TXPRECURSOR_0_PIN       (54 + 30)
#define GT_TXPRECURSOR_1_PIN       (54 + 31)
#define GT_TXPRECURSOR_2_PIN       (54 + 32)
#define GT_TXPRECURSOR_3_PIN       (54 + 33)
#define GT_TXPRECURSOR_4_PIN       (54 + 34)
#define GT_TXPOSTCURSOR_0_PIN      (54 + 35)
#define GT_TXPOSTCURSOR_1_PIN      (54 + 36)
#define GT_TXPOSTCURSOR_2_PIN      (54 + 37)
#define GT_TXPOSTCURSOR_3_PIN      (54 + 38)
#define GT_TXPOSTCURSOR_4_PIN      (54 + 39)


// GTX CONTROL PINS - no longer used
#if 0
#define GT_RXCDRHOLD_PIN         (54+26)
#define GT_PRBSSEL_0_PIN         (54+27)
#define GT_PRBSSEL_1_PIN         (54+28)
#define GT_PRBSSEL_2_PIN         (54+29)
#define GT_RX_PRBS_CNT_RESET_PIN (54+30)
#define GT_RXPRBS_ERR_PIN        (54+31)
#define GT_TXPRBSFORCEERR_PIN    (54+32)
#endif


// 
#define FIRST_PIN  LED_0_PIN
#define LAST_PIN   GT_TXPOSTCURSOR_4_PIN


static struct hrtimer  sd_loop_gpio_timer;
static unsigned long   sd_loop_gpio_count[14];

static enum hrtimer_restart sd_loop_gpio_sample  (struct hrtimer *timer)
{
	hrtimer_forward_now(timer, ktime_set(0, 1000000));

	sd_loop_gpio_count[0]++;
	if ( gpio_get_value(SRIO_LINK_INITIALIZED_PIN) )  sd_loop_gpio_count[ 1]++;
	if ( gpio_get_value(SRIO_PORT_INITIALIZED_PIN) )  sd_loop_gpio_count[ 2]++;
	if ( gpio_get_value(SRIO_CLK_LOCK_OUT_PIN)     )  sd_loop_gpio_count[ 3]++;
	if ( gpio_get_value(SRIO_MODE_1X_PIN)          )  sd_loop_gpio_count[ 4]++;
	if ( gpio_get_value(GT0_QPLL_LOCK_OUT_PIN)     )  sd_loop_gpio_count[ 5]++;
	if ( gpio_get_value(GT_PCS_RST_OUT_PIN)        )  sd_loop_gpio_count[ 6]++;
	if ( gpio_get_value(GTRX_DISPERR_OR_PIN)       )  sd_loop_gpio_count[ 7]++;
	if ( gpio_get_value(GTRX_NOTINTABLE_OR_PIN)    )  sd_loop_gpio_count[ 8]++;
	if ( gpio_get_value(LOG_RST_OUT_PIN)           )  sd_loop_gpio_count[ 9]++;
	if ( gpio_get_value(PHY_RCVD_LINK_RESET_PIN)   )  sd_loop_gpio_count[10]++;
	if ( gpio_get_value(PHY_RCVD_MCE_PIN)          )  sd_loop_gpio_count[11]++;
	if ( gpio_get_value(PHY_RST_OUT_PIN)           )  sd_loop_gpio_count[12]++;
	if ( gpio_get_value(PORT_ERROR_PIN)            )  sd_loop_gpio_count[13]++;

	return HRTIMER_RESTART;
}


int sd_loop_gpio_init (void)
{
	int pin, ret;

	printk("%s: request GPIOs...\n", SD_DRIVER_NODE);
	for ( pin = FIRST_PIN; pin <= LAST_PIN; pin++ )
		if ( (ret = gpio_request(pin, SD_DRIVER_NODE)) )
		{
			printk("%s: GPIO %d failed (%d), unwind and stop.\n",
			       SD_DRIVER_NODE, pin, ret);

			for ( pin--; pin >= FIRST_PIN; pin-- )
				gpio_free(pin);

			return ret;
		}

	printk("%s: got GPIOs, setup direction/value...\n", SD_DRIVER_NODE);
	gpio_direction_output(LED_0_PIN,                0);
	gpio_direction_output(LED_1_PIN,                0);
	gpio_direction_output(LED_2_PIN,                0);
//	gpio_direction_output(LED_3_PIN,                0); // SRIO / PCIE SELECT
	gpio_direction_output(SRIO_PCIE_SEL_PIN,        1); // Select AMC 8/9
	gpio_direction_output(SRIO_SYS_RST_PIN,         0);
	gpio_direction_output(SRIO_S_AXI_MAINTR_RST_PIN,0);
	gpio_direction_output(SRIO_SIM_TRAIN_EN_PIN,    0);
	gpio_direction_output(SRIO_PHY_MCE_PIN,         0);
	gpio_direction_output(SRIO_PHY_LINK_RESET_PIN,  0);
	gpio_direction_output(SRIO_FORCE_REINIT_PIN,    0);

	gpio_direction_input(SRIO_LINK_INITIALIZED_PIN);
	gpio_direction_input(SRIO_PORT_INITIALIZED_PIN);
	gpio_direction_input(SRIO_CLK_LOCK_OUT_PIN);
	gpio_direction_input(SRIO_MODE_1X_PIN);

	gpio_direction_output(GT_LOOPBACK_0_PIN,            0);
	gpio_direction_output(GT_LOOPBACK_1_PIN,            0);
	gpio_direction_output(GT_LOOPBACK_2_PIN,            0);

	gpio_direction_input(GT0_QPLL_LOCK_OUT_PIN);
	gpio_direction_input(GT_PCS_RST_OUT_PIN);
	gpio_direction_input(GTRX_DISPERR_OR_PIN);
	gpio_direction_input(GTRX_NOTINTABLE_OR_PIN);
	gpio_direction_input(LOG_RST_OUT_PIN);
	gpio_direction_input(PHY_RCVD_LINK_RESET_PIN);
	gpio_direction_input(PHY_RCVD_MCE_PIN);
	gpio_direction_input(PHY_RST_OUT_PIN);
	gpio_direction_input(PORT_ERROR_PIN);

#if 0
	gpio_direction_output(GT_RXCDRHOLD_PIN,         0);
	gpio_direction_output(GT_PRBSSEL_0_PIN,          0);
	gpio_direction_output(GT_PRBSSEL_1_PIN,          0);
	gpio_direction_output(GT_PRBSSEL_2_PIN,          0);
	gpio_direction_output(GT_RX_PRBS_CNT_RESET_PIN, 0);

	gpio_direction_output(GT_TXPRBSFORCEERR_PIN,    0);
	gpio_direction_input(GT_RXPRBS_ERR_PIN);
#endif

	hrtimer_init(&sd_loop_gpio_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	sd_loop_gpio_timer.function = sd_loop_gpio_sample;

	printk("%s: GPIO setup success\n", SD_DRIVER_NODE);

	return 0;
}

void sd_loop_gpio_exit (void)
{
	int pin;

	for ( pin = FIRST_PIN; pin <= LAST_PIN; pin++ )
		gpio_free(pin);
}

void sd_loop_gpio_srio_reset (void)
{
	gpio_set_value(SRIO_SYS_RST_PIN,      1);
	gpio_set_value(SRIO_PHY_LINK_RESET_PIN, 0);
	gpio_set_value(SRIO_FORCE_REINIT_PIN, 0);

	msleep(10);
	memset(sd_loop_gpio_count, 0, sizeof(sd_loop_gpio_count));
	hrtimer_start(&sd_loop_gpio_timer, ktime_set(0, 1000000), HRTIMER_MODE_REL);

	gpio_set_value(SRIO_SYS_RST_PIN,        0);
	gpio_set_value(SRIO_PHY_LINK_RESET_PIN, 0);
	gpio_set_value(SRIO_FORCE_REINIT_PIN,   0);

	msleep(10);
}

void sd_loop_gpio_prbs_reset (void)
{
#if 0
	gpio_set_value(GT_RX_PRBS_CNT_RESET_PIN, 1);
	msleep(1000);
	gpio_set_value(GT_RX_PRBS_CNT_RESET_PIN, 0);
#endif
}

size_t sd_loop_gpio_print_srio (char *dst, size_t max)
{
	char *p = dst;
	char *e = dst + max;

	p += scnprintf(p, e - p, "link_initialized   [%d]\n", gpio_get_value(SRIO_LINK_INITIALIZED_PIN));
	p += scnprintf(p, e - p, "port_initialized   [%d]\n", gpio_get_value(SRIO_PORT_INITIALIZED_PIN));
	p += scnprintf(p, e - p, "clk_lock_out       [%d]\n", gpio_get_value(SRIO_CLK_LOCK_OUT_PIN));
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

//	p += scnprintf(p, e - p, "rx_cdr_hold      [%d]\n",     gpio_get_value(GT_RXCDRHOLD_PIN));
	p += scnprintf(p, e - p, "rx_loopback      [%d%d%d]\n", gpio_get_value(GT_LOOPBACK_2_PIN), gpio_get_value(GT_LOOPBACK_1_PIN), gpio_get_value(GT_LOOPBACK_0_PIN));
//	p += scnprintf(p, e - p, "rx_prbs_sel      [%d%d%d]\n", gpio_get_value(GT_PRBSSEL_2_PIN), gpio_get_value(GT_PRBSSEL_1_PIN), gpio_get_value(GT_PRBSSEL_0_PIN));
//	p += scnprintf(p, e - p, "rx_prbs_cnt_rst  [%d]\n",     gpio_get_value(GT_RX_PRBS_CNT_RESET_PIN));
//	p += scnprintf(p, e - p, "rx_prbs_err      [%d]\n",     gpio_get_value(GT_RXPRBS_ERR_PIN));
//	p += scnprintf(p, e - p, "prbs_force_err   [%d]\n",     gpio_get_value(GT_TXPRBSFORCEERR_PIN));

	return p - dst;
}

size_t sd_loop_gpio_print_counts (char *dst, size_t max)
{
	char *p = dst;
	char *e = dst + max;

	hrtimer_cancel(&sd_loop_gpio_timer);

	p += scnprintf(p, e - p, "samples              : %lu\n", sd_loop_gpio_count[ 0]);
	p += scnprintf(p, e - p, "srio_link_initialized: %lu\n", sd_loop_gpio_count[ 1]);
	p += scnprintf(p, e - p, "srio_port_initialized: %lu\n", sd_loop_gpio_count[ 2]);
	p += scnprintf(p, e - p, "srio_clk_lock_out    : %lu\n", sd_loop_gpio_count[ 3]);
	p += scnprintf(p, e - p, "srio_mode_1x         : %lu\n", sd_loop_gpio_count[ 4]);
	p += scnprintf(p, e - p, "gt0_qpll_lock_out    : %lu\n", sd_loop_gpio_count[ 5]);
	p += scnprintf(p, e - p, "gt_pcs_rst_out       : %lu\n", sd_loop_gpio_count[ 6]);
	p += scnprintf(p, e - p, "gtrx_disperr_or      : %lu\n", sd_loop_gpio_count[ 7]);
	p += scnprintf(p, e - p, "gtrx_notintable_or   : %lu\n", sd_loop_gpio_count[ 8]);
	p += scnprintf(p, e - p, "log_rst_out          : %lu\n", sd_loop_gpio_count[ 9]);
	p += scnprintf(p, e - p, "phy_rcvd_link_reset  : %lu\n", sd_loop_gpio_count[10]);
	p += scnprintf(p, e - p, "phy_rcvd_mce         : %lu\n", sd_loop_gpio_count[11]);
	p += scnprintf(p, e - p, "phy_rst_out          : %lu\n", sd_loop_gpio_count[12]);
	p += scnprintf(p, e - p, "port_error           : %lu\n", sd_loop_gpio_count[13]);

	return p - dst;
}

#if 0
void sd_loop_gpio_set_prbs_force_err (int en)
{
//	gpio_set_value(GT_TXPRBSFORCEERR_PIN, !!en);
}

int sd_loop_gpio_get_prbs_force_err (void)
{
	return 0; // gpio_get_value(GT_TXPRBSFORCEERR_PIN);
}

void sd_loop_gpio_set_gt_prbs (int mode)
{
	switch (mode)
	{
		case 0:
			printk("PRBS: Normal \n");
			gpio_set_value(GT_PRBSSEL_2_PIN, 0);
			gpio_set_value(GT_PRBSSEL_1_PIN, 0);
			gpio_set_value(GT_PRBSSEL_0_PIN, 0);
			break;
		case 1:
			printk("PRBS: PRBS-7 \n");
			gpio_set_value(GT_PRBSSEL_2_PIN, 0);
			gpio_set_value(GT_PRBSSEL_1_PIN, 0);
			gpio_set_value(GT_PRBSSEL_0_PIN, 1);
			break;
		case 2:
			printk("PRBS: PRBS-15 \n");
			gpio_set_value(GT_PRBSSEL_2_PIN, 0);
			gpio_set_value(GT_PRBSSEL_1_PIN, 1);
			gpio_set_value(GT_PRBSSEL_0_PIN, 0);
			break;
		case 3:
			printk("PRBS: PRBS-23 \n");
			gpio_set_value(GT_PRBSSEL_2_PIN, 0);
			gpio_set_value(GT_PRBSSEL_1_PIN, 1);
			gpio_set_value(GT_PRBSSEL_0_PIN, 1);
			break;
		case 4:
			printk("PRBS: PRBS-31 \n");
			gpio_set_value(GT_PRBSSEL_2_PIN, 1);
			gpio_set_value(GT_PRBSSEL_1_PIN, 0);
			gpio_set_value(GT_PRBSSEL_0_PIN, 0);
			break;
		default:
			printk("PRBS: Normal \n");
			gpio_set_value(GT_PRBSSEL_2_PIN, 0);
			gpio_set_value(GT_PRBSSEL_1_PIN, 0);
			gpio_set_value(GT_PRBSSEL_0_PIN, 0);
			break;
	}
}

void sd_loop_gpio_set_gt_loopback (int mode)
{
	switch (mode)
	{
		case 0:
			 printk("Loopback: Normal  \n");
			 gpio_set_value(GT_LOOPBACK_0_PIN,    0);
			 gpio_set_value(GT_LOOPBACK_1_PIN,    0);
			 gpio_set_value(GT_LOOPBACK_2_PIN,    0);
			 gpio_set_value(GT_RXCDRHOLD_PIN, 0);
			 break;

		case 1:
			 printk("Loopback: PCS (1)  \n");
			 gpio_set_value(GT_LOOPBACK_0_PIN,    1);
			 gpio_set_value(GT_LOOPBACK_1_PIN,    0);
			 gpio_set_value(GT_LOOPBACK_2_PIN,    0);
			 gpio_set_value(GT_RXCDRHOLD_PIN, 1);
			 break;

		case 2:
			 printk("Loopback: PMA (2)  \n");
			 gpio_set_value(GT_LOOPBACK_0_PIN,    0);
			 gpio_set_value(GT_LOOPBACK_1_PIN,    1);
			 gpio_set_value(GT_LOOPBACK_2_PIN,    0);
			 gpio_set_value(GT_RXCDRHOLD_PIN, 0);
			 break;

		default:
			 printk("Loopback: Normal  \n");
			 gpio_set_value(GT_LOOPBACK_0_PIN,    0);
			 gpio_set_value(GT_LOOPBACK_1_PIN,    0);
			 gpio_set_value(GT_LOOPBACK_2_PIN,    0);
			 gpio_set_value(GT_RXCDRHOLD_PIN, 0);
			 break;
	}
}
#endif

void sd_loop_gpio_set_pcie_srio_sel (int en)
{
	gpio_set_value(SRIO_PCIE_SEL_PIN, !!en);
}

int sd_loop_gpio_get_pcie_srio_sel (void)
{
	return gpio_get_value(SRIO_PCIE_SEL_PIN);
}


// TX drive control pins
void sd_loop_gpio_set_gt_txdiffctrl (int val)
{
	gpio_set_value(GT_TXDIFFCTRL_0_PIN, val & (1 << 0) ? 1 : 0);
	gpio_set_value(GT_TXDIFFCTRL_1_PIN, val & (1 << 1) ? 1 : 0);
	gpio_set_value(GT_TXDIFFCTRL_2_PIN, val & (1 << 2) ? 1 : 0);
	gpio_set_value(GT_TXDIFFCTRL_3_PIN, val & (1 << 3) ? 1 : 0);
}
int sd_loop_gpio_get_gt_txdiffctrl (void)
{
	int val = 0;
	if ( gpio_get_value(GT_TXDIFFCTRL_0_PIN) ) val |= (1 << 0);
	if ( gpio_get_value(GT_TXDIFFCTRL_1_PIN) ) val |= (1 << 1);
	if ( gpio_get_value(GT_TXDIFFCTRL_2_PIN) ) val |= (1 << 2);
	if ( gpio_get_value(GT_TXDIFFCTRL_3_PIN) ) val |= (1 << 3);
	return val;
}

void sd_loop_gpio_set_gt_txprecursor (int val)
{
	gpio_set_value(GT_TXPRECURSOR_0_PIN, val & (1 << 0) ? 1 : 0);
	gpio_set_value(GT_TXPRECURSOR_1_PIN, val & (1 << 1) ? 1 : 0);
	gpio_set_value(GT_TXPRECURSOR_2_PIN, val & (1 << 2) ? 1 : 0);
	gpio_set_value(GT_TXPRECURSOR_3_PIN, val & (1 << 3) ? 1 : 0);
	gpio_set_value(GT_TXPRECURSOR_4_PIN, val & (1 << 4) ? 1 : 0);
}
int sd_loop_gpio_get_gt_txprecursor (void)
{
	int val = 0;
	if ( gpio_get_value(GT_TXPRECURSOR_0_PIN) ) val |= (1 << 0);
	if ( gpio_get_value(GT_TXPRECURSOR_1_PIN) ) val |= (1 << 1);
	if ( gpio_get_value(GT_TXPRECURSOR_2_PIN) ) val |= (1 << 2);
	if ( gpio_get_value(GT_TXPRECURSOR_3_PIN) ) val |= (1 << 3);
	if ( gpio_get_value(GT_TXPRECURSOR_4_PIN) ) val |= (1 << 4);
	return val;
}

void sd_loop_gpio_set_gt_txpostcursor (int val)
{
	gpio_set_value(GT_TXPOSTCURSOR_0_PIN, val & (1 << 0) ? 1 : 0);
	gpio_set_value(GT_TXPOSTCURSOR_1_PIN, val & (1 << 1) ? 1 : 0);
	gpio_set_value(GT_TXPOSTCURSOR_2_PIN, val & (1 << 2) ? 1 : 0);
	gpio_set_value(GT_TXPOSTCURSOR_3_PIN, val & (1 << 3) ? 1 : 0);
	gpio_set_value(GT_TXPOSTCURSOR_4_PIN, val & (1 << 4) ? 1 : 0);
}
int sd_loop_gpio_get_gt_txpostcursor (void)
{
	int val = 0;
	if ( gpio_get_value(GT_TXPOSTCURSOR_0_PIN) ) val |= (1 << 0);
	if ( gpio_get_value(GT_TXPOSTCURSOR_1_PIN) ) val |= (1 << 1);
	if ( gpio_get_value(GT_TXPOSTCURSOR_2_PIN) ) val |= (1 << 2);
	if ( gpio_get_value(GT_TXPOSTCURSOR_3_PIN) ) val |= (1 << 3);
	if ( gpio_get_value(GT_TXPOSTCURSOR_4_PIN) ) val |= (1 << 4);
	return val;
}

