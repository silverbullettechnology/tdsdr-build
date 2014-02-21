/** \file      asfe_ctl.v
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
asfe_ctl_0.1 {
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
		asfe_ctl_err_desc;
		/* src/lib/asfe_ctl_hal.c */
		asfe_ctl_hal_attach;
		asfe_ctl_hal_detach;
		asfe_ctl_hal_spi_reg_get;
		asfe_ctl_hal_spi_reg_clr;
		/* src/lib/asfe_ctl_hal_linux.c */
		asfe_ctl_hal_linux_attach;
		asfe_ctl_hal_linux_cleanup;
		asfe_ctl_hal_linux_gpio_init;
		asfe_ctl_hal_linux_gpio_done;
		asfe_ctl_hal_linux_spi_init;
		asfe_ctl_hal_linux_spi_done;
		asfe_ctl_hal_linux_uart_init;
		asfe_ctl_hal_linux_uart_done;
		/* src/lib/asfe_ctl_hal_sim.c */
		asfe_ctl_hal_sim_attach;
	local:
		*;
};
