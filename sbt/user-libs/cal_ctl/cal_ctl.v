/** \file      cal_ctl.v
 *  \brief     Shared-library exports and version control
 *  \copyright Copyright 2013,2014 Silver Bullet Technology
 *
 *             Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *             use this file except in compliance with the License.  You may obtain a copy
 *             of the License at: 
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *             Unless required by applicable law or agreed to in writing, software
 *             distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *             WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 *             License for the specific language governing permissions and limitations
 *             under the License.
 *
 * vim:ts=4:noexpandtab
 */
cal_ctl_0.1 {
	global:
		CMB_init;
		CMB_SPIWriteByte;
		CMB_SPIWriteByteArray;
		CMB_SPIReadByte;
		CMB_uartSendHex;
		CMB_uartSendString;
		CMB_uartReceiveString;
		CMB_hostTxPacket;
		CMB_hostRxPacket;
		CMB_DelayU;
		CMB_gpioInit;
		CMB_gpioWrite;
		CMB_gpioPulse;
		cal_ctl_err_desc;
		/* src/lib/cal_ctl_hal.c */
		cal_ctl_hal_attach;
		cal_ctl_hal_detach;
		cal_ctl_hal_spi_reg_get;
		cal_ctl_hal_spi_reg_clr;
		/* src/lib/cal_ctl_hal_linux.c */
		cal_ctl_hal_linux_attach;
		cal_ctl_hal_linux_cleanup;
		cal_ctl_hal_linux_gpio_init;
		cal_ctl_hal_linux_set_gpio;
		cal_ctl_hal_linux_gpio_done;
		cal_ctl_hal_linux_spi_init;
		cal_ctl_hal_linux_spi_done;
		cal_ctl_hal_linux_uart_init;
		cal_ctl_hal_linux_uart_done;
		/* src/lib/cal_ctl_hal_sim.c */
		cal_ctl_hal_sim_attach;
		/* src/lib/cal_ctl_lib.c */
		cal_test_function;
		cal_ctl_spiwritearray;
		cal_ctl_dev_reopen;
		cal_ctl_spitransaction;
		cal_ctl_cal_spare_1;
		cal_ctl_cal_spare_2;
		cal_ctl_cal_rstn;
		cal_ctl_adi1_tx_en;
		cal_ctl_cal_reserve1;
		cal_ctl_cal_reserve2;
		cal_ctl_cal_reserve4;
		cal_ctl_cal_reserve3;
		cal_ctl_adi2_tx_en;
		cal_ctl_cal_spare_3;
		cal_ctl_cal_spare_4;
	local:
		*;
};
