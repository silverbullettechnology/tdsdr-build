/** \file      ad9361.v
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
ad9361_0.1 {
	global:
		/* src/lib/common.c */
		CMB_init;
		CMB_SPIWriteField;
		CMB_SPIWriteByte;
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
		ad9361_err_desc;
		/* src/lib/ad9361_hal.c */
		ad9361_hal_attach;
		ad9361_hal_detach;
		ad9361_hal_spi_reg_get;
		ad9361_hal_spi_reg_clr;
		/* src/lib/ad9361_hal_linux.c */
		ad9361_hal_linux_attach;
		ad9361_hal_linux_cleanup;
		ad9361_hal_linux_gpio_init;
		ad9361_hal_linux_gpio_done;
		ad9361_hal_linux_spi_init;
		ad9361_hal_linux_spi_done;
		ad9361_hal_linux_uart_init;
		ad9361_hal_linux_uart_done;
		/* src/lib/ad9361_hal_sim.c */
		ad9361_hal_sim_attach;
	local:
		*;
};
