/** \file      include/lib/ad9361_sysfs.h
 *  \brief     API to IIO controls in sysfs
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
#ifndef _INCLUDE_LIB_AD9361_SYSFS_H_
#define _INCLUDE_LIB_AD9361_SYSFS_H_

//filter_fir_config


/* Path: calib_mode
 */
int ad9361_get_calib_mode (unsigned dev, int *val);
int ad9361_set_calib_mode (unsigned dev, int  val);

/* Path: dcxo_tune_coarse
 * Var : dcxo_coarse
 */
int ad9361_get_dcxo_tune_coarse (unsigned dev, uint32_t *val);
int ad9361_set_dcxo_tune_coarse (unsigned dev, uint32_t  val);

/* Path: dcxo_tune_fine
 * Var : dcxo_fine
 */
int ad9361_get_dcxo_tune_fine (unsigned dev, uint32_t *val);
int ad9361_set_dcxo_tune_fine (unsigned dev, uint32_t  val);

/* Path: ensm_mode
 */
int ad9361_get_ensm_mode (unsigned dev, int *val);
int ad9361_set_ensm_mode (unsigned dev, int  val);

/* Path: in_out_voltage_filter_fir_en
 */
int ad9361_get_in_out_voltage_filter_fir_en (unsigned dev, int *val);
int ad9361_set_in_out_voltage_filter_fir_en (unsigned dev, int  val);

/* Path: in_temp0_input
 */
int ad9361_get_in_temp0_input (unsigned dev, int *val);

/* Path: in_voltage0_gain_control_mode / in_voltage1_gain_control_mode
 */
int ad9361_get_in_voltage_gain_control_mode  (unsigned dev, int channel, int *val);
int ad9361_set_in_voltage_gain_control_mode  (unsigned dev, int channel, int  val);
int ad9361_get_in_voltage0_gain_control_mode (unsigned dev, int *val);
int ad9361_set_in_voltage0_gain_control_mode (unsigned dev, int  val);
int ad9361_get_in_voltage1_gain_control_mode (unsigned dev, int *val);
int ad9361_set_in_voltage1_gain_control_mode (unsigned dev, int  val);

/* Path: in_voltage0_hardwaregain / in_voltage1_hardwaregain
 */
int ad9361_get_in_voltage_hardwaregain  (unsigned dev, int channel, uint32_t *val);
int ad9361_set_in_voltage_hardwaregain  (unsigned dev, int channel, uint32_t  val);
int ad9361_get_in_voltage0_hardwaregain (unsigned dev, uint32_t *val);
int ad9361_set_in_voltage0_hardwaregain (unsigned dev, uint32_t  val);
int ad9361_get_in_voltage1_hardwaregain (unsigned dev, uint32_t *val);
int ad9361_set_in_voltage1_hardwaregain (unsigned dev, uint32_t  val);

/* Path: in_voltage0_rf_port_select
 */
int ad9361_get_in_voltage_rf_port_select  (unsigned dev, int channel, int *val);
int ad9361_set_in_voltage_rf_port_select  (unsigned dev, int channel, int  val);
int ad9361_get_in_voltage0_rf_port_select (unsigned dev, int *val);
int ad9361_set_in_voltage0_rf_port_select (unsigned dev, int  val);
int ad9361_get_in_voltage1_rf_port_select (unsigned dev, int *val);
int ad9361_set_in_voltage1_rf_port_select (unsigned dev, int  val);


/* Path: in_voltage0_rssi / in_voltage1_rssi 
 */
int ad9361_get_in_voltage_rssi  (unsigned dev, int channel, uint32_t *val);
int ad9361_set_in_voltage_rssi  (unsigned dev, int channel, uint32_t  val);
int ad9361_get_in_voltage0_rssi (unsigned dev, uint32_t *val);
int ad9361_set_in_voltage0_rssi (unsigned dev, uint32_t  val);
int ad9361_get_in_voltage1_rssi (unsigned dev, uint32_t *val);
int ad9361_set_in_voltage1_rssi (unsigned dev, uint32_t  val);

/* Path: in_voltage2_offset
 */
int ad9361_get_in_voltage2_offset (unsigned dev, int *val);
int ad9361_set_in_voltage2_offset (unsigned dev, int  val);

/* Path: in_voltage2_raw
 */
int ad9361_get_in_voltage2_raw (unsigned dev, int *val);
int ad9361_set_in_voltage2_raw (unsigned dev, int  val);

/* Path: in_voltage2_scale
 */
int ad9361_get_in_voltage2_scale (unsigned dev, int *val);
int ad9361_set_in_voltage2_scale (unsigned dev, int  val);

/* Path: in_voltage_bb_dc_offset_tracking_en
 */
int ad9361_get_in_voltage_bb_dc_offset_tracking_en (unsigned dev, int *val);
int ad9361_set_in_voltage_bb_dc_offset_tracking_en (unsigned dev, int  val);

/* Path: in_voltage_filter_fir_en
 */
int ad9361_get_in_voltage_filter_fir_en (unsigned dev, int *val);
int ad9361_set_in_voltage_filter_fir_en (unsigned dev, int  val);

/* Path: in_voltage_quadrature_tracking_en
 */
int ad9361_get_in_voltage_quadrature_tracking_en (unsigned dev, int *val);
int ad9361_set_in_voltage_quadrature_tracking_en (unsigned dev, int  val);

/* Path: in_voltage_rf_bandwidth
 */
int ad9361_get_in_voltage_rf_bandwidth (unsigned dev, uint32_t *val);
int ad9361_set_in_voltage_rf_bandwidth (unsigned dev, uint32_t  val);

/* Path: in_voltage_rf_dc_offset_tracking_en
 */
int ad9361_get_in_voltage_rf_dc_offset_tracking_en (unsigned dev, int *val);
int ad9361_set_in_voltage_rf_dc_offset_tracking_en (unsigned dev, int  val);

/* Path: in_voltage_sampling_frequency
 */
int ad9361_get_in_voltage_sampling_frequency (unsigned dev, uint32_t *val);
int ad9361_set_in_voltage_sampling_frequency (unsigned dev, uint32_t  val);

/* Path: out_altvoltage0_RX_LO_frequency
 */
int ad9361_get_out_altvoltage0_RX_LO_frequency (unsigned dev, uint64_t *val);
int ad9361_set_out_altvoltage0_RX_LO_frequency (unsigned dev, uint64_t  val);

/* Path: out_altvoltage1_TX_LO_frequency
 */
int ad9361_get_out_altvoltage1_TX_LO_frequency (unsigned dev, uint64_t *val);
int ad9361_set_out_altvoltage1_TX_LO_frequency (unsigned dev, uint64_t  val);

/* Path: out_voltage0_hardwaregain / out_voltage1_hardwaregain
 */
int ad9361_get_out_voltage_hardwaregain  (unsigned dev, int channel, uint32_t *val);
int ad9361_set_out_voltage_hardwaregain  (unsigned dev, int channel, uint32_t  val);
int ad9361_get_out_voltage0_hardwaregain (unsigned dev, uint32_t *val);
int ad9361_set_out_voltage0_hardwaregain (unsigned dev, uint32_t  val);
int ad9361_get_out_voltage1_hardwaregain (unsigned dev, uint32_t *val);
int ad9361_set_out_voltage1_hardwaregain (unsigned dev, uint32_t  val);

/* Path: out_voltage0_rf_port_select / out_voltage1_rf_port_select
 */
int ad9361_get_out_voltage_rf_port_select  (unsigned dev, int channel, int *val);
int ad9361_set_out_voltage_rf_port_select  (unsigned dev, int channel, int  val);
int ad9361_get_out_voltage0_rf_port_select (unsigned dev, int *val);
int ad9361_set_out_voltage0_rf_port_select (unsigned dev, int  val);
int ad9361_get_out_voltage1_rf_port_select (unsigned dev, int *val);
int ad9361_set_out_voltage1_rf_port_select (unsigned dev, int  val);

/* Path: out_voltage2_raw / out_voltage3_raw
 */
int ad9361_get_out_voltage_raw  (unsigned dev, int channel, uint32_t *val);
int ad9361_set_out_voltage_raw  (unsigned dev, int channel, uint32_t  val);
int ad9361_get_out_voltage2_raw (unsigned dev, uint32_t *val);
int ad9361_set_out_voltage2_raw (unsigned dev, uint32_t  val);
int ad9361_get_out_voltage3_raw (unsigned dev, uint32_t *val);
int ad9361_set_out_voltage3_raw (unsigned dev, uint32_t  val);


/* Path: out_voltage2_scale
 */
int ad9361_get_out_voltage_scale  (unsigned dev, int channel, uint32_t *val);
int ad9361_set_out_voltage_scale  (unsigned dev, int channel, uint32_t  val);
int ad9361_get_out_voltage2_scale (unsigned dev, uint32_t *val);
int ad9361_set_out_voltage2_scale (unsigned dev, uint32_t  val);
int ad9361_get_out_voltage3_scale (unsigned dev, uint32_t *val);
int ad9361_set_out_voltage3_scale (unsigned dev, uint32_t  val);

/* Path: out_voltage_filter_fir_en
 */
int ad9361_get_out_voltage_filter_fir_en (unsigned dev, int *val);
int ad9361_set_out_voltage_filter_fir_en (unsigned dev, int  val);

/* Path: out_voltage_rf_bandwidth
 */
int ad9361_get_out_voltage_rf_bandwidth (unsigned dev, uint32_t *val);
int ad9361_set_out_voltage_rf_bandwidth (unsigned dev, uint32_t  val);

/* Path: out_voltage_sampling_frequency
 */
int ad9361_get_out_voltage_sampling_frequency (unsigned dev, uint32_t *val);
int ad9361_set_out_voltage_sampling_frequency (unsigned dev, uint32_t  val);

/* Path: out_rx_path_rates - FIXME: return an array
 */
int ad9361_get_out_rx_path_rates (unsigned dev, uint32_t *val);
int ad9361_set_out_rx_path_rates (unsigned dev, uint32_t  val);

/* Path: out_trx_rate_governor
 */
int ad9361_get_out_trx_rate_governor (unsigned dev, int *val);
int ad9361_set_out_trx_rate_governor (unsigned dev, int  val);

/* Path: out_tx_path_rates - FIXME: return an array
 */
int ad9361_get_out_tx_path_rates (unsigned dev, uint32_t *val);
int ad9361_set_out_tx_path_rates (unsigned dev, uint32_t  val);


#endif /* _INCLUDE_LIB_AD9361_SYSFS_H_ */
