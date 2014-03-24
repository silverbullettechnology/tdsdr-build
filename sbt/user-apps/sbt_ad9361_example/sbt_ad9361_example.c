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
	ad9361_set_in_voltage_rf_bandwidth(0,3500000);
	ad9361_set_out_voltage_rf_bandwidth(0,3500000);
	ad9361_set_in_voltage_rf_bandwidth(1,3500000);
	ad9361_set_out_voltage_rf_bandwidth(1,3500000);

	//set the sampling frequency of both ad9361 devices to 4 MHz
	ad9361_set_in_voltage_sampling_frequency(0,4000000);
	ad9361_set_out_voltage_sampling_frequency(0,4000000);
	ad9361_set_in_voltage_sampling_frequency(1,4000000);
	ad9361_set_out_voltage_sampling_frequency(1,4000000);

	//set the tx and rx frequencies to 2400 MHz
	ad9361_set_out_altvoltage0_RX_LO_frequency(0,2400000000);
	ad9361_set_out_altvoltage1_TX_LO_frequency(0,2400000000);
	ad9361_set_out_altvoltage0_RX_LO_frequency(1,2400000000);
	ad9361_set_out_altvoltage1_TX_LO_frequency(1,2400000000);

	//load the FIR filter configuration and activate the FIR filters.  Filters must be loaded twice.	
	ad9361_load_filter_fir_config (0, "/usr/lib/ad9361/filter/wimax");
	ad9361_load_filter_fir_config (1, "/usr/lib/ad9361/filter/wimax");

	ad9361_set_in_out_voltage_filter_fir_en (0, 1);
	ad9361_set_in_out_voltage_filter_fir_en (1, 1);

	ad9361_load_filter_fir_config (0, "/usr/lib/ad9361/filter/wimax");
	ad9361_load_filter_fir_config (1, "/usr/lib/ad9361/filter/wimax");

	//set some GPIOs and write some data to the ASFE spi interface in preparation for transmit.	
	printf("Running asfe functions.\n");
	asfe_ctl_spitransaction(&asfe_data[0], &asfe_rd_data[0], 5);
	asfe_ctl_adi1_tx_en(1);
	asfe_ctl_adi2_tx_en(1);

	//start transmitting the example waveform	
	system("dma_streamer_app adt /usr/lib/ad9361/data/802.16d-64q-cp16.iqw cont");

	return 0;
}


