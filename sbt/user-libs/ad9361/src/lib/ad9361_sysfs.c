/** \file      src/lib/ad9361_sysfs.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ad9361_hal.h"


int ad9361_sysfs_get_enum (unsigned dev, const char *root, const char *leaf,
                           const char **list, int *val)
{
	errno = ENOSYS;
	return -1;
}

int ad9361_sysfs_set_enum (unsigned dev, const char *root, const char *leaf,
                           const char **list, int val)
{
	errno = ENOSYS;
	return -1;
}

int ad9361_sysfs_get_int (unsigned dev, const char *root, const char *leaf, int *val)
{
	return ad9361_sysfs_scanf(dev, root, leaf, "%d", val);
}

int ad9361_sysfs_set_int (unsigned dev, const char *root, const char *leaf, int val)
{
	return ad9361_sysfs_printf(dev, root, leaf, "%d", val);
}

int ad9361_sysfs_get_u32 (unsigned dev, const char *root, const char *leaf, uint32_t *val)
{
	return ad9361_sysfs_scanf(dev, root, leaf, "%u", val);
}

int ad9361_sysfs_set_u32 (unsigned dev, const char *root, const char *leaf, uint32_t val)
{
	return ad9361_sysfs_printf(dev, root, leaf, "%u", val);
}

int ad9361_sysfs_get_u64 (unsigned dev, const char *root, const char *leaf, uint64_t *val)
{
	return ad9361_sysfs_scanf(dev, root, leaf, "%llu", val);
}

int ad9361_sysfs_set_u64 (unsigned dev, const char *root, const char *leaf, uint64_t val)
{
	return ad9361_sysfs_printf(dev, root, leaf, "%llu", val);
}


#define ROOT "/sys/kernel/debug/iio"
#define get_enum(dev,leaf,list,val)  ad9361_sysfs_get_enum(dev,ROOT,leaf,list,val) 
#define set_enum(dev,leaf,list,val)  ad9361_sysfs_set_enum(dev,ROOT,leaf,list,val) 
#define get_int(dev,leaf,val)        ad9361_sysfs_get_int(dev,ROOT,leaf,val) 
#define set_int(dev,leaf,val)        ad9361_sysfs_set_int(dev,ROOT,leaf,val) 
#define get_u32(dev,leaf,val)        ad9361_sysfs_get_u32(dev,ROOT,leaf,val) 
#define set_u32(dev,leaf,val)        ad9361_sysfs_set_u32(dev,ROOT,leaf,val) 
#define get_u64(dev,leaf,val)        ad9361_sysfs_get_u64(dev,ROOT,leaf,val) 
#define set_u64(dev,leaf,val)        ad9361_sysfs_set_u64(dev,ROOT,leaf,val) 


/* Path: calib_mode
 */
static const char *ad9361_enum_calib_mode[] = { };
int ad9361_get_calib_mode (unsigned dev, int *val)
{
	return get_enum(dev, "calib_mode", ad9361_enum_calib_mode, val);
}
int ad9361_set_calib_mode (unsigned dev, int val)
{
	return set_enum(dev, "calib_mode", ad9361_enum_calib_mode, val);
}

/* Path: dcxo_tune_coarse
 * Var : dcxo_coarse
 */
int ad9361_get_dcxo_tune_coarse (unsigned dev, uint32_t *val)
{
	return get_u32(dev, "dcxo_tune_coarse", val);
}
int ad9361_set_dcxo_tune_coarse (unsigned dev, uint32_t val)
{
	return set_u32(dev, "dcxo_tune_coarse", val);
}

/* Path: dcxo_tune_fine
 * Var : dcxo_fine
 */
int ad9361_get_dcxo_tune_fine (unsigned dev, uint32_t *val)
{
	return get_u32(dev, "dcxo_tune_fine", val);
}
int ad9361_set_dcxo_tune_fine (unsigned dev, uint32_t val)
{
	return set_u32(dev, "dcxo_tune_fine", val);
}

/* Path: ensm_mode
 */
static const char *ad9361_enum_ensm_mode[] = { };
int ad9361_get_ensm_mode (unsigned dev, int *val)
{
	return get_enum(dev, "ensm_mode", ad9361_enum_ensm_mode, val);
}
int ad9361_set_ensm_mode (unsigned dev, int val)
{
	return set_enum(dev, "ensm_mode", ad9361_enum_ensm_mode, val);
}

/* Path: in_out_voltage_filter_fir_en
 */
int ad9361_get_in_out_voltage_filter_fir_en (unsigned dev, int *val)
{
	return get_int(dev, "in_out_voltage_filter_fir_en", val);
}
int ad9361_set_in_out_voltage_filter_fir_en (unsigned dev, int val)
{
	return set_int(dev, "in_out_voltage_filter_fir_en", val);
}

/* Path: in_temp0_input
 */
int ad9361_get_in_temp0_input (unsigned dev, int *val)
{
	return get_int(dev, "in_temp0_input", val);
}

/* Path: in_voltage0_gain_control_mode / in_voltage1_gain_control_mode
 */
static const char *ad9361_enum_gain_control_mode[] = { };
int ad9361_get_in_voltage_gain_control_mode (unsigned dev, int channel, int *val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_gain_control_mode", channel);
	return get_enum(dev, leaf, ad9361_enum_gain_control_mode, val);
}
int ad9361_set_in_voltage_gain_control_mode (unsigned dev, int channel, int val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_gain_control_mode", channel);
	return set_enum(dev, leaf, ad9361_enum_gain_control_mode, val);
}
int ad9361_get_in_voltage0_gain_control_mode (unsigned dev, int *val)
{
	return ad9361_get_in_voltage_gain_control_mode(dev, 0, val);
}
int ad9361_set_in_voltage0_gain_control_mode (unsigned dev, int val)
{
	return ad9361_set_in_voltage_gain_control_mode(dev, 0, val);
}
int ad9361_get_in_voltage1_gain_control_mode (unsigned dev, int *val)
{
	return ad9361_get_in_voltage_gain_control_mode(dev, 1, val);
}
int ad9361_set_in_voltage1_gain_control_mode (unsigned dev, int val)
{
	return ad9361_set_in_voltage_gain_control_mode(dev, 1, val);
}

/* Path: in_voltage0_hardwaregain / in_voltage1_hardwaregain
 */
int ad9361_get_in_voltage_hardwaregain (unsigned dev, int channel, uint32_t *val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_hardwaregain", channel);
	return get_u32(dev, leaf, val);
}
int ad9361_set_in_voltage_hardwaregain (unsigned dev, int channel, uint32_t val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_hardwaregain", channel);
	return set_u32(dev, leaf, val);
}
int ad9361_get_in_voltage0_hardwaregain (unsigned dev, uint32_t *val)
{
	return ad9361_get_in_voltage_hardwaregain(dev, 0, val);
}
int ad9361_set_in_voltage0_hardwaregain (unsigned dev, uint32_t val)
{
	return ad9361_set_in_voltage_hardwaregain(dev, 0, val);
}
int ad9361_get_in_voltage1_hardwaregain (unsigned dev, uint32_t *val)
{
	return ad9361_get_in_voltage_hardwaregain(dev, 1, val);
}
int ad9361_set_in_voltage1_hardwaregain (unsigned dev, uint32_t val)
{
	return ad9361_set_in_voltage_hardwaregain(dev, 1, val);
}

/* Path: in_voltage0_rf_port_select
 */
static const char *ad9361_enum_rx_rf_port_select [] = { };
int ad9361_get_in_voltage_rf_port_select (unsigned dev, int channel, int *val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_rf_port_select", channel);
	return get_enum(dev, leaf, ad9361_enum_rx_rf_port_select, val);
}
int ad9361_set_in_voltage_rf_port_select (unsigned dev, int channel, int val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_rf_port_select", channel);
	return set_enum(dev, leaf, ad9361_enum_rx_rf_port_select, val);
}
int ad9361_get_in_voltage0_rf_port_select (unsigned dev, int *val)
{
	return ad9361_get_in_voltage_rf_port_select(dev, 0, val);
}
int ad9361_set_in_voltage0_rf_port_select (unsigned dev, int val)
{
	return ad9361_set_in_voltage_rf_port_select(dev, 0, val);
}
int ad9361_get_in_voltage1_rf_port_select (unsigned dev, int *val)
{
	return ad9361_get_in_voltage_rf_port_select(dev, 1, val);
}
int ad9361_set_in_voltage1_rf_port_select (unsigned dev, int val)
{
	return ad9361_set_in_voltage_rf_port_select(dev, 1, val);
}


/* Path: in_voltage0_rssi / in_voltage1_rssi 
 */
int ad9361_get_in_voltage_rssi (unsigned dev, int channel, uint32_t *val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_rssi", channel);
	return get_u32(dev, leaf, val);
}
int ad9361_set_in_voltage_rssi (unsigned dev, int channel, uint32_t val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_rssi", channel);
	return set_u32(dev, leaf, val);
}
int ad9361_get_in_voltage0_rssi (unsigned dev, uint32_t *val)
{
	return ad9361_get_in_voltage_rssi(dev, 0, val);
}
int ad9361_set_in_voltage0_rssi (unsigned dev, uint32_t val)
{
	return ad9361_set_in_voltage_rssi(dev, 0, val);
}
int ad9361_get_in_voltage1_rssi (unsigned dev, uint32_t *val)
{
	return ad9361_get_in_voltage_rssi(dev, 1, val);
}
int ad9361_set_in_voltage1_rssi (unsigned dev, uint32_t val)
{
	return ad9361_set_in_voltage_rssi(dev, 1, val);
}

/* Path: in_voltage2_offset
 */
int ad9361_get_in_voltage2_offset (unsigned dev, int *val)
{
	return get_int(dev, "in_voltage2_offset", val);
}
int ad9361_set_in_voltage2_offset (unsigned dev, int val)
{
	return set_int(dev, "in_voltage2_offset", val);
}

/* Path: in_voltage2_raw
 */
int ad9361_get_in_voltage2_raw (unsigned dev, int *val)
{
	return get_int(dev, "in_voltage2_raw", val);
}
int ad9361_set_in_voltage2_raw (unsigned dev, int val)
{
	return set_int(dev, "in_voltage2_raw", val);
}

/* Path: in_voltage2_scale
 */
int ad9361_get_in_voltage2_scale (unsigned dev, int *val)
{
	return get_int(dev, "in_voltage2_scale", val);
}
int ad9361_set_in_voltage2_scale (unsigned dev, int val)
{
	return set_int(dev, "in_voltage2_scale", val);
}

/* Path: in_voltage_bb_dc_offset_tracking_en
 */
int ad9361_get_in_voltage_bb_dc_offset_tracking_en (unsigned dev, int *val)
{
	return get_int(dev, "in_voltage_bb_dc_offset_tracking_en", val);
}
int ad9361_set_in_voltage_bb_dc_offset_tracking_en (unsigned dev, int val)
{
	return set_int(dev, "in_voltage_bb_dc_offset_tracking_en", val);
}

/* Path: in_voltage_filter_fir_en
 */
int ad9361_get_in_voltage_filter_fir_en (unsigned dev, int *val)
{
	return get_int(dev, "in_voltage_filter_fir_en", val);
}
int ad9361_set_in_voltage_filter_fir_en (unsigned dev, int val)
{
	return set_int(dev, "in_voltage_filter_fir_en", val);
}

/* Path: in_voltage_quadrature_tracking_en
 */
int ad9361_get_in_voltage_quadrature_tracking_en (unsigned dev, int *val)
{
	return get_int(dev, "in_voltage_quadrature_tracking_en", val);
}
int ad9361_set_in_voltage_quadrature_tracking_en (unsigned dev, int val)
{
	return set_int(dev, "in_voltage_quadrature_tracking_en", val);
}

/* Path: in_voltage_rf_bandwidth
 */
int ad9361_get_in_voltage_rf_bandwidth (unsigned dev, uint32_t *val)
{
	return get_u32(dev, "in_voltage_rf_bandwidth", val);
}
int ad9361_set_in_voltage_rf_bandwidth (unsigned dev, uint32_t val)
{
	return set_u32(dev, "in_voltage_rf_bandwidth", val);
}

/* Path: in_voltage_rf_dc_offset_tracking_en
 */
int ad9361_get_in_voltage_rf_dc_offset_tracking_en (unsigned dev, int *val)
{
	return get_int(dev, "in_voltage_rf_dc_offset_tracking_en", val);
}
int ad9361_set_in_voltage_rf_dc_offset_tracking_en (unsigned dev, int val)
{
	return set_int(dev, "in_voltage_rf_dc_offset_tracking_en", val);
}

/* Path: in_voltage_sampling_frequency
 */
int ad9361_get_in_voltage_sampling_frequency (unsigned dev, uint32_t *val)
{
	return get_u32(dev, "in_voltage_sampling_frequency", val);
}
int ad9361_set_in_voltage_sampling_frequency (unsigned dev, uint32_t val)
{
	return set_u32(dev, "in_voltage_sampling_frequency", val);
}

/* Path: out_altvoltage0_RX_LO_frequency
 */
int ad9361_get_out_altvoltage0_RX_LO_frequency (unsigned dev, uint64_t *val)
{
	return get_u64(dev, "out_altvoltage0_RX_LO_frequency", val);
}
int ad9361_set_out_altvoltage0_RX_LO_frequency (unsigned dev, uint64_t val)
{
	return set_u64(dev, "out_altvoltage0_RX_LO_frequency", val);
}

/* Path: out_altvoltage1_TX_LO_frequency
 */
int ad9361_get_out_altvoltage1_TX_LO_frequency (unsigned dev, uint64_t *val)
{
	return get_u64(dev, "out_altvoltage1_TX_LO_frequency", val);
}
int ad9361_set_out_altvoltage1_TX_LO_frequency (unsigned dev, uint64_t val)
{
	return set_u64(dev, "out_altvoltage1_TX_LO_frequency", val);
}

/* Path: out_voltage0_hardwaregain / out_voltage1_hardwaregain
 */
int ad9361_get_out_voltage_hardwaregain (unsigned dev, int channel, uint32_t *val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_hardwaregain", channel);
	return get_u32(dev, leaf, val);	// FIXME: special case for units conversion
}
int ad9361_set_out_voltage_hardwaregain (unsigned dev, int channel, uint32_t val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_hardwaregain", channel);
	return set_u32(dev, leaf, val);	// FIXME: special case for units conversion
}
int ad9361_get_out_voltage0_hardwaregain (unsigned dev, uint32_t *val)
{
	return ad9361_get_out_voltage_hardwaregain(dev, 0, val);
}
int ad9361_set_out_voltage0_hardwaregain (unsigned dev, uint32_t val)
{
	return ad9361_set_out_voltage_hardwaregain(dev, 0, val);
}
int ad9361_get_out_voltage1_hardwaregain (unsigned dev, uint32_t *val)
{
	return ad9361_get_out_voltage_hardwaregain(dev, 1, val);
}
int ad9361_set_out_voltage1_hardwaregain (unsigned dev, uint32_t val)
{
	return ad9361_set_out_voltage_hardwaregain(dev, 1, val);
}

/* Path: out_voltage0_rf_port_select / out_voltage1_rf_port_select
 */
static const char *ad9361_enum_tx_rf_port_select [] = { };
int ad9361_get_out_voltage_rf_port_select (unsigned dev, int channel, int *val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_rf_port_select", channel);
	return get_enum(dev, leaf, ad9361_enum_tx_rf_port_select, val);
}
int ad9361_set_out_voltage_rf_port_select (unsigned dev, int channel, int val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_rf_port_select", channel);
	return set_enum(dev, leaf, ad9361_enum_tx_rf_port_select, val);
}
int ad9361_get_out_voltage0_rf_port_select (unsigned dev, int *val)
{
	return ad9361_get_out_voltage_rf_port_select(dev, 0, val);
}
int ad9361_set_out_voltage0_rf_port_select (unsigned dev, int val)
{
	return ad9361_set_out_voltage_rf_port_select(dev, 0, val);
}
int ad9361_get_out_voltage1_rf_port_select (unsigned dev, int *val)
{
	return ad9361_get_out_voltage_rf_port_select(dev, 1, val);
}
int ad9361_set_out_voltage1_rf_port_select (unsigned dev, int val)
{
	return ad9361_set_out_voltage_rf_port_select(dev, 1, val);
}

/* Path: out_voltage2_raw / out_voltage3_raw
 */
int ad9361_get_out_voltage_raw (unsigned dev, int channel, uint32_t *val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_raw", channel);
	return get_u32(dev, leaf, val);
}
int ad9361_set_out_voltage_raw (unsigned dev, int channel, uint32_t val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_raw", channel);
	return set_u32(dev, leaf, val);
}
int ad9361_get_out_voltage2_raw (unsigned dev, uint32_t *val)
{
	return ad9361_get_out_voltage_raw(dev, 2, val);
}
int ad9361_set_out_voltage2_raw (unsigned dev, uint32_t val)
{
	return ad9361_set_out_voltage_raw(dev, 2, val);
}
int ad9361_get_out_voltage3_raw (unsigned dev, uint32_t *val)
{
	return ad9361_get_out_voltage_raw(dev, 3, val);
}
int ad9361_set_out_voltage3_raw (unsigned dev, uint32_t val)
{
	return ad9361_set_out_voltage_raw(dev, 3, val);
}


/* Path: out_voltage2_scale
 */
int ad9361_get_out_voltage_scale (unsigned dev, int channel, uint32_t *val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_scale", channel);
	return get_u32(dev, leaf, val);
}
int ad9361_set_out_voltage_scale (unsigned dev, int channel, uint32_t val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_scale", channel);
	return set_u32(dev, leaf, val);
}
int ad9361_get_out_voltage2_scale (unsigned dev, uint32_t *val)
{
	return ad9361_get_out_voltage_scale(dev, 2, val);
}
int ad9361_set_out_voltage2_scale (unsigned dev, uint32_t val)
{
	return ad9361_set_out_voltage_scale(dev, 2, val);
}
int ad9361_get_out_voltage3_scale (unsigned dev, uint32_t *val)
{
	return ad9361_get_out_voltage_scale(dev, 3, val);
}
int ad9361_set_out_voltage3_scale (unsigned dev, uint32_t val)
{
	return ad9361_set_out_voltage_scale(dev, 3, val);
}

/* Path: out_voltage_filter_fir_en
 */
int ad9361_get_out_voltage_filter_fir_en (unsigned dev, int *val)
{
	return get_int(dev, "out_voltage_filter_fir_en", val);
}
int ad9361_set_out_voltage_filter_fir_en (unsigned dev, int val)
{
	return set_int(dev, "out_voltage_filter_fir_en", val);
}

/* Path: out_voltage_rf_bandwidth
 */
int ad9361_get_out_voltage_rf_bandwidth (unsigned dev, uint32_t *val)
{
	return get_u32(dev, "out_voltage_rf_bandwidth", val);
}
int ad9361_set_out_voltage_rf_bandwidth (unsigned dev, uint32_t val)
{
	return set_u32(dev, "out_voltage_rf_bandwidth", val);
}

/* Path: out_voltage_sampling_frequency
 */
int ad9361_get_out_voltage_sampling_frequency (unsigned dev, uint32_t *val)
{
	return get_u32(dev, "out_voltage_sampling_frequency", val);
}
int ad9361_set_out_voltage_sampling_frequency (unsigned dev, uint32_t val)
{
	return set_u32(dev, "out_voltage_sampling_frequency", val);
}

/* Path: out_rx_path_rates - FIXME: return an array
 */
int ad9361_get_out_rx_path_rates (unsigned dev, uint32_t *val)
{
	return get_u32(dev, "out_rx_path_rates", val);
}
int ad9361_set_out_rx_path_rates (unsigned dev, uint32_t val)
{
	return set_u32(dev, "out_rx_path_rates", val);
}

/* Path: out_trx_rate_governor
 */
static const char *ad9361_enum_rate_governor[] = { };
int ad9361_get_out_trx_rate_governor (unsigned dev, int *val)
{
	return get_enum(dev, "out_trx_rate_governor", ad9361_enum_rate_governor, val);
}
int ad9361_set_out_trx_rate_governor (unsigned dev, int val)
{
	return set_enum(dev, "out_trx_rate_governor", ad9361_enum_rate_governor, val);
}

/* Path: out_tx_path_rates - FIXME: return an array
 */
int ad9361_get_out_tx_path_rates (unsigned dev, uint32_t *val)
{
	return get_u32(dev, "out_tx_path_rates", val);
}
int ad9361_set_out_tx_path_rates (unsigned dev, uint32_t val)
{
	return set_u32(dev, "out_tx_path_rates", val);
}



