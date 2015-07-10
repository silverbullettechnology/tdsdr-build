/** \file      sbt_ad9361_example.c
 *  \brief     Public header wrapper
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
#include <stdio.h>
#include <stdlib.h>
#include <ad9361.h>
#include <asfe_ctl.h>

int main(int argc, char *argv[])
{
	printf("Running SBT Example application.\n");
	printf("cmdline args:\n");
	while(argc--)
		printf("%s\n",*argv++);
	
	UINT16 asfe_data[] = {1,2,3,4,5};
	UINT16 asfe_rd_data[5];
	
	//set up the AD9361 driver HAL
	ad9361_hal_linux_init();
	ad9361_hal_linux_attach();

	//set up the ASFE_CTL HAL
	asfe_ctl_hal_linux_attach();
	asfe_ctl_dev_reopen(0,0);

	//set both ADI devices to fdd mode to permit chip configuration
	ad9361_set_ensm_mode(0,5);
	ad9361_set_ensm_mode(1,5);

	//set the rx port configuration for both AD9361 devices to AD9361_IN_RF_PORT_SELECT_A_BALANCED
	ad9361_set_in_voltage1_rf_port_select(0,0);
	ad9361_set_in_voltage1_rf_port_select(0,0);
	ad9361_set_in_voltage0_rf_port_select(1,0);
	ad9361_set_in_voltage1_rf_port_select(1,0);

	//set the tx port configuration for both AD9361 devices to AD9361_OUT_RF_PORT_SELECT_A
	ad9361_set_out_voltage0_rf_port_select(0,0);
	ad9361_set_out_voltage1_rf_port_select(0,0);
	ad9361_set_out_voltage0_rf_port_select(1,0);
	ad9361_set_out_voltage1_rf_port_select(1,0);
	
	//set the analog filters (rf bandwidth) for tx and rx on both ad9361 devices
	ad9361_set_in_voltage_rf_bandwidth(0,50000000);
	ad9361_set_out_voltage_rf_bandwidth(0,50000000);
	ad9361_set_in_voltage_rf_bandwidth(1,50000000);
	ad9361_set_out_voltage_rf_bandwidth(1,50000000);

	//set the sampling frequency of both ad9361 deviced
	ad9361_set_in_voltage_sampling_frequency(0,38000000);
	ad9361_set_out_voltage_sampling_frequency(0,38000000);
	ad9361_set_in_voltage_sampling_frequency(1,38000000);
	ad9361_set_out_voltage_sampling_frequency(1,38000000);

	//set the tx and rx frequencies to 2400 MHz
	ad9361_set_out_altvoltage0_RX_LO_frequency(0,2400000000ULL);
	ad9361_set_out_altvoltage1_TX_LO_frequency(0,2400000000ULL);
	ad9361_set_out_altvoltage0_RX_LO_frequency(1,2400000000ULL);
	ad9361_set_out_altvoltage1_TX_LO_frequency(1,2400000000ULL);

	//disable the FIR filters.
	ad9361_set_in_out_voltage_filter_fir_en (0, 0);
	ad9361_set_in_out_voltage_filter_fir_en (1, 0);

	//run a TX quadrature calibration
	ad9361_start_calib(0,2,13);
	ad9361_start_calib(1,2,13);

	ad9361_set_ensm_mode(0,0);
	ad9361_set_ensm_mode(1,0);

	printf("AD9361 configured for high sample rates.\n");

	return 0;
}


