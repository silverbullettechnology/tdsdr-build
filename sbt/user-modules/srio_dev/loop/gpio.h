/** \file      loop/gpio.h
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
#ifndef _INCLUDE_LOOP_GPIO_H_
#define _INCLUDE_LOOP_GPIO_H_
#include <linux/kernel.h>


// OUTPUT PINS
#define LED0_PIN           (54 + 0)
#define LED1_PIN           (54 + 1)
#define LED2_PIN           (54 + 2)
//#define LED3_PIN           (54 + 3)
#define SRIO_PCIE_SEL_PIN  (54 + 3)

#define SYS_RST_PIN        (54 + 4)
#define AXI_MAINTR_RST_PIN (54 + 5)
#define SIM_TRAIN_PIN      (54 + 6)
#define PHY_MCE_PIN        (54 + 7)
#define PHY_LINK_RST_PIN   (54 + 8)
#define FORCE_REINIT_PIN   (54 + 9)

// INPUT PINS
#define LINK_INITIALIZED_PIN (54+10)
#define PORT_INITIALIZED_PIN (54+11)
#define CLK_LOCK_OUT_PIN     (54+12)
#define SRIO_MODE_1X_PIN     (54+13)

// LOOPBACK PINS
#define LOOPBACK0_PIN  (54+14)
#define LOOPBACK1_PIN  (54+15)
#define LOOPBACK2_PIN  (54+16)

// OUTPUT DEBUG PINS
#define GT0_QPLL_LOCK_OUT_PIN   (54+17)
#define GT_PCS_RST_OUT_PIN      (54+18)
#define GTRX_DISPERR_OR_PIN     (54+19)
#define GTRX_NOTINTABLE_OR_PIN  (54+20)
#define LOG_RST_OUT_PIN         (54+21)
#define PHY_RCVD_LINK_RESET_PIN (54+22)
#define PHY_RCVD_MCE_PIN        (54+23)
#define PHY_RST_OUT_PIN         (54+24)
#define PORT_ERROR_PIN          (54+25)

// GTX CONTROL PINS
#define GT_RXCDRHOLD_PIN         (54+26)
#define GT_PRBSSEL0_PIN          (54+27)
#define GT_PRBSSEL1_PIN          (54+28)
#define GT_PRBSSEL2_PIN          (54+29)
#define GT_RX_PRBS_CNT_RESET_PIN (54+30)
#define GT_RXPRBS_ERR_PIN        (54+31)
#define GT_TXPRBSFORCEERR_PIN    (54+32)


int  sd_loop_gpio_init (void);
void sd_loop_gpio_exit (void);

void sd_loop_gpio_srio_reset (void);
void sd_loop_gpio_prbs_reset (void);

void sd_loop_gpio_print_srio (void);
void sd_loop_gpio_print_gt_prbs (void);

void sd_loop_gpio_prbs_force_err (int en);

void sd_loop_gpio_set_gt_prbs (int mode);
void sd_loop_gpio_set_gt_loopback (int mode);


#endif /* _INCLUDE_LOOP_GPIO_H_ */
