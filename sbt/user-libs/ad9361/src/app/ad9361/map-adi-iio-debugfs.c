/** \file      src/app/ad9361/map-adi-iio-debugfs.c
 *  \brief     API to IIO controls in debugfs
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
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include <ad9361.h>

#include "map.h"
#include "main.h"
#include "util.h"
#include "parse-gen.h"
#include "format-gen.h"


static int map_set_bist_prbs (int argc, const char **argv)
{
	MAP_ARG(int, mode, 1, "");

	MAP_LIB_CALL(ad9361_set_bist_prbs, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_bist_prbs, map_set_bist_prbs, 2);

static int map_set_bist_tone (int argc, const char **argv)
{
	MAP_ARG(int,      mode,     1, "");
	MAP_ARG(uint32_t, freq_Hz,  2, "");
	MAP_ARG(uint32_t, level_dB, 3, "");
	MAP_ARG(uint32_t, mask,     4, "");

	MAP_LIB_CALL(ad9361_set_bist_tone, ad9361_legacy_dev, mode, freq_Hz, level_dB, mask);

	return 0;
}
MAP_CMD(set_bist_tone, map_set_bist_tone, 2);

static int map_set_initialize (int argc, const char **argv)
{
	MAP_LIB_CALL(ad9361_set_initialize, ad9361_legacy_dev);

	return 0;
}
MAP_CMD(set_initialize, map_set_initialize, 1);

static int map_set_loopback (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_loopback, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_loopback, map_set_loopback, 2);

static int map_set_frequency_division_duplex_mode_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_frequency_division_duplex_mode_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_frequency_division_duplex_mode_enable, map_set_frequency_division_duplex_mode_enable, 2);

static int map_set_ensm_enable_pin_pulse_mode_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_ensm_enable_pin_pulse_mode_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_ensm_enable_pin_pulse_mode_enable, map_set_ensm_enable_pin_pulse_mode_enable, 2);

static int map_set_ensm_enable_txnrx_control_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_ensm_enable_txnrx_control_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_ensm_enable_txnrx_control_enable, map_set_ensm_enable_txnrx_control_enable, 2);

static int map_set_debug_mode_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_debug_mode_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_debug_mode_enable, map_set_debug_mode_enable, 2);

static int map_set_tdd_use_fdd_vco_tables_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_tdd_use_fdd_vco_tables_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_tdd_use_fdd_vco_tables_enable, map_set_tdd_use_fdd_vco_tables_enable, 2);

static int map_set_tdd_use_dual_synth_mode_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_tdd_use_dual_synth_mode_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_tdd_use_dual_synth_mode_enable, map_set_tdd_use_dual_synth_mode_enable, 2);

static int map_set_tdd_skip_vco_cal_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_tdd_skip_vco_cal_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_tdd_skip_vco_cal_enable, map_set_tdd_skip_vco_cal_enable, 2);

static int map_set_2rx_2tx_mode_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_2rx_2tx_mode_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_2rx_2tx_mode_enable, map_set_2rx_2tx_mode_enable, 2);

static int map_set_split_gain_table_mode_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_split_gain_table_mode_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_split_gain_table_mode_enable, map_set_split_gain_table_mode_enable, 2);

static int map_set_rx_rf_port_input_select (int argc, const char **argv)
{
	MAP_ARG(uint32_t, port, 1, "");

	MAP_LIB_CALL(ad9361_set_rx_rf_port_input_select, ad9361_legacy_dev, port);

	return 0;
}
MAP_CMD(set_rx_rf_port_input_select, map_set_rx_rf_port_input_select, 2);

static int map_set_tx_rf_port_input_select (int argc, const char **argv)
{
	MAP_ARG(uint32_t, port, 1, "");

	MAP_LIB_CALL(ad9361_set_tx_rf_port_input_select, ad9361_legacy_dev, port);

	return 0;
}
MAP_CMD(set_tx_rf_port_input_select, map_set_tx_rf_port_input_select, 2);

static int map_set_external_tx_lo_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_external_tx_lo_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_external_tx_lo_enable, map_set_external_tx_lo_enable, 2);

static int map_set_external_rx_lo_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_external_rx_lo_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_external_rx_lo_enable, map_set_external_rx_lo_enable, 2);

static int map_set_xo_disable_use_ext_refclk_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_xo_disable_use_ext_refclk_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_xo_disable_use_ext_refclk_enable, map_set_xo_disable_use_ext_refclk_enable, 2);

static int map_set_clk_output_mode_select (int argc, const char **argv)
{
	MAP_ARG(uint32_t, mode, 1, "");

	MAP_LIB_CALL(ad9361_set_clk_output_mode_select, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_clk_output_mode_select, map_set_clk_output_mode_select, 2);

static int map_set_dc_offset_tracking_update_event_mask (int argc, const char **argv)
{
	MAP_ARG(uint32_t, mask, 1, "");

	MAP_LIB_CALL(ad9361_set_dc_offset_tracking_update_event_mask, ad9361_legacy_dev, mask);

	return 0;
}
MAP_CMD(set_dc_offset_tracking_update_event_mask, map_set_dc_offset_tracking_update_event_mask, 2);

static int map_set_dc_offset_attenuation_high_range (int argc, const char **argv)
{
	MAP_ARG(uint32_t, attenuation, 1, "");

	MAP_LIB_CALL(ad9361_set_dc_offset_attenuation_high_range, ad9361_legacy_dev, attenuation);

	return 0;
}
MAP_CMD(set_dc_offset_attenuation_high_range, map_set_dc_offset_attenuation_high_range, 2);

static int map_set_dc_offset_attenuation_low_range (int argc, const char **argv)
{
	MAP_ARG(uint32_t, attenuation, 1, "");

	MAP_LIB_CALL(ad9361_set_dc_offset_attenuation_low_range, ad9361_legacy_dev, attenuation);

	return 0;
}
MAP_CMD(set_dc_offset_attenuation_low_range, map_set_dc_offset_attenuation_low_range, 2);

static int map_set_dc_offset_count_high_range (int argc, const char **argv)
{
	MAP_ARG(uint32_t, count, 1, "");

	MAP_LIB_CALL(ad9361_set_dc_offset_count_high_range, ad9361_legacy_dev, count);

	return 0;
}
MAP_CMD(set_dc_offset_count_high_range, map_set_dc_offset_count_high_range, 2);

static int map_set_dc_offset_count_low_range (int argc, const char **argv)
{
	MAP_ARG(uint32_t, count, 1, "");

	MAP_LIB_CALL(ad9361_set_dc_offset_count_low_range, ad9361_legacy_dev, count);

	return 0;
}
MAP_CMD(set_dc_offset_count_low_range, map_set_dc_offset_count_low_range, 2);

static int map_set_rf_rx_bandwidth_hz (int argc, const char **argv)
{
	MAP_ARG(uint32_t, bandwidth, 1, "");

	MAP_LIB_CALL(ad9361_set_rf_rx_bandwidth_hz, ad9361_legacy_dev, bandwidth);

	return 0;
}
MAP_CMD(set_rf_rx_bandwidth_hz, map_set_rf_rx_bandwidth_hz, 2);

static int map_set_rf_tx_bandwidth_hz (int argc, const char **argv)
{
	MAP_ARG(uint32_t, bandwidth, 1, "");

	MAP_LIB_CALL(ad9361_set_rf_tx_bandwidth_hz, ad9361_legacy_dev, bandwidth);

	return 0;
}
MAP_CMD(set_rf_tx_bandwidth_hz, map_set_rf_tx_bandwidth_hz, 2);

static int map_set_tx_attenuation_mdB (int argc, const char **argv)
{
	MAP_ARG(uint32_t, attenuation, 1, "");

	MAP_LIB_CALL(ad9361_set_tx_attenuation_mdB, ad9361_legacy_dev, attenuation);

	return 0;
}
MAP_CMD(set_tx_attenuation_mdB, map_set_tx_attenuation_mdB, 2);

static int map_set_update_tx_gain_in_alert_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_update_tx_gain_in_alert_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_update_tx_gain_in_alert_enable, map_set_update_tx_gain_in_alert_enable, 2);

static int map_set_gc_rx1_mode (int argc, const char **argv)
{
	MAP_ARG(uint32_t, mode, 1, "");

	MAP_LIB_CALL(ad9361_set_gc_rx1_mode, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_gc_rx1_mode, map_set_gc_rx1_mode, 2);

static int map_set_gc_rx2_mode (int argc, const char **argv)
{
	MAP_ARG(uint32_t, mode, 1, "");

	MAP_LIB_CALL(ad9361_set_gc_rx2_mode, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_gc_rx2_mode, map_set_gc_rx2_mode, 2);

static int map_set_gc_adc_ovr_sample_size (int argc, const char **argv)
{
	MAP_ARG(uint32_t, size, 1, "");

	MAP_LIB_CALL(ad9361_set_gc_adc_ovr_sample_size, ad9361_legacy_dev, size);

	return 0;
}
MAP_CMD(set_gc_adc_ovr_sample_size, map_set_gc_adc_ovr_sample_size, 2);

static int map_set_gc_adc_small_overload_thresh (int argc, const char **argv)
{
	MAP_ARG(uint32_t, thresh, 1, "");

	MAP_LIB_CALL(ad9361_set_gc_adc_small_overload_thresh, ad9361_legacy_dev, thresh);

	return 0;
}
MAP_CMD(set_gc_adc_small_overload_thresh, map_set_gc_adc_small_overload_thresh, 2);

static int map_set_gc_adc_large_overload_thresh (int argc, const char **argv)
{
	MAP_ARG(uint32_t, thresh, 1, "");

	MAP_LIB_CALL(ad9361_set_gc_adc_large_overload_thresh, ad9361_legacy_dev, thresh);

	return 0;
}
MAP_CMD(set_gc_adc_large_overload_thresh, map_set_gc_adc_large_overload_thresh, 2);

static int map_set_gc_lmt_overload_high_thresh (int argc, const char **argv)
{
	MAP_ARG(uint32_t, thresh, 1, "");

	MAP_LIB_CALL(ad9361_set_gc_lmt_overload_high_thresh, ad9361_legacy_dev, thresh);

	return 0;
}
MAP_CMD(set_gc_lmt_overload_high_thresh, map_set_gc_lmt_overload_high_thresh, 2);

static int map_set_gc_lmt_overload_low_thresh (int argc, const char **argv)
{
	MAP_ARG(uint32_t, thresh, 1, "");

	MAP_LIB_CALL(ad9361_set_gc_lmt_overload_low_thresh, ad9361_legacy_dev, thresh);

	return 0;
}
MAP_CMD(set_gc_lmt_overload_low_thresh, map_set_gc_lmt_overload_low_thresh, 2);

static int map_set_gc_dec_pow_measurement_duration (int argc, const char **argv)
{
	MAP_ARG(uint32_t, duration, 1, "");

	MAP_LIB_CALL(ad9361_set_gc_dec_pow_measurement_duration, ad9361_legacy_dev, duration);

	return 0;
}
MAP_CMD(set_gc_dec_pow_measurement_duration, map_set_gc_dec_pow_measurement_duration, 2);

static int map_set_gc_low_power_thresh (int argc, const char **argv)
{
	MAP_ARG(uint32_t, thresh, 1, "");

	MAP_LIB_CALL(ad9361_set_gc_low_power_thresh, ad9361_legacy_dev, thresh);

	return 0;
}
MAP_CMD(set_gc_low_power_thresh, map_set_gc_low_power_thresh, 2);

static int map_set_gc_dig_gain_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_gc_dig_gain_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_gc_dig_gain_enable, map_set_gc_dig_gain_enable, 2);

static int map_set_gc_max_dig_gain (int argc, const char **argv)
{
	MAP_ARG(uint32_t, gain, 1, "");

	MAP_LIB_CALL(ad9361_set_gc_max_dig_gain, ad9361_legacy_dev, gain);

	return 0;
}
MAP_CMD(set_gc_max_dig_gain, map_set_gc_max_dig_gain, 2);

static int map_set_mgc_rx1_ctrl_inp_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_mgc_rx1_ctrl_inp_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_mgc_rx1_ctrl_inp_enable, map_set_mgc_rx1_ctrl_inp_enable, 2);

static int map_set_mgc_rx2_ctrl_inp_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_mgc_rx2_ctrl_inp_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_mgc_rx2_ctrl_inp_enable, map_set_mgc_rx2_ctrl_inp_enable, 2);

static int map_set_mgc_inc_gain_step (int argc, const char **argv)
{
	MAP_ARG(uint32_t, step, 1, "");

	MAP_LIB_CALL(ad9361_set_mgc_inc_gain_step, ad9361_legacy_dev, step);

	return 0;
}
MAP_CMD(set_mgc_inc_gain_step, map_set_mgc_inc_gain_step, 2);

static int map_set_mgc_dec_gain_step (int argc, const char **argv)
{
	MAP_ARG(uint32_t, step, 1, "");

	MAP_LIB_CALL(ad9361_set_mgc_dec_gain_step, ad9361_legacy_dev, step);

	return 0;
}
MAP_CMD(set_mgc_dec_gain_step, map_set_mgc_dec_gain_step, 2);

static int map_set_mgc_split_table_ctrl_inp_gain_mode (int argc, const char **argv)
{
	MAP_ARG(uint32_t, mode, 1, "");

	MAP_LIB_CALL(ad9361_set_mgc_split_table_ctrl_inp_gain_mode, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_mgc_split_table_ctrl_inp_gain_mode, map_set_mgc_split_table_ctrl_inp_gain_mode, 2);

static int map_set_agc_attack_delay_extra_margin_us (int argc, const char **argv)
{
	MAP_ARG(uint32_t, margin, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_attack_delay_extra_margin_us, ad9361_legacy_dev, margin);

	return 0;
}
MAP_CMD(set_agc_attack_delay_extra_margin_us, map_set_agc_attack_delay_extra_margin_us, 2);

static int map_set_agc_outer_thresh_high (int argc, const char **argv)
{
	MAP_ARG(uint32_t, thresh, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_outer_thresh_high, ad9361_legacy_dev, thresh);

	return 0;
}
MAP_CMD(set_agc_outer_thresh_high, map_set_agc_outer_thresh_high, 2);

static int map_set_agc_outer_thresh_high_dec_steps (int argc, const char **argv)
{
	MAP_ARG(uint32_t, steps, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_outer_thresh_high_dec_steps, ad9361_legacy_dev, steps);

	return 0;
}
MAP_CMD(set_agc_outer_thresh_high_dec_steps, map_set_agc_outer_thresh_high_dec_steps, 2);

static int map_set_agc_inner_thresh_high (int argc, const char **argv)
{
	MAP_ARG(uint32_t, thresh, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_inner_thresh_high, ad9361_legacy_dev, thresh);

	return 0;
}
MAP_CMD(set_agc_inner_thresh_high, map_set_agc_inner_thresh_high, 2);

static int map_set_agc_inner_thresh_high_dec_steps (int argc, const char **argv)
{
	MAP_ARG(uint32_t, steps, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_inner_thresh_high_dec_steps, ad9361_legacy_dev, steps);

	return 0;
}
MAP_CMD(set_agc_inner_thresh_high_dec_steps, map_set_agc_inner_thresh_high_dec_steps, 2);

static int map_set_agc_inner_thresh_low (int argc, const char **argv)
{
	MAP_ARG(uint32_t, thresh, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_inner_thresh_low, ad9361_legacy_dev, thresh);

	return 0;
}
MAP_CMD(set_agc_inner_thresh_low, map_set_agc_inner_thresh_low, 2);

static int map_set_agc_inner_thresh_low_inc_steps (int argc, const char **argv)
{
	MAP_ARG(uint32_t, steps, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_inner_thresh_low_inc_steps, ad9361_legacy_dev, steps);

	return 0;
}
MAP_CMD(set_agc_inner_thresh_low_inc_steps, map_set_agc_inner_thresh_low_inc_steps, 2);

static int map_set_agc_outer_thresh_low (int argc, const char **argv)
{
	MAP_ARG(uint32_t, thresh, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_outer_thresh_low, ad9361_legacy_dev, thresh);

	return 0;
}
MAP_CMD(set_agc_outer_thresh_low, map_set_agc_outer_thresh_low, 2);

static int map_set_agc_outer_thresh_low_inc_steps (int argc, const char **argv)
{
	MAP_ARG(uint32_t, steps, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_outer_thresh_low_inc_steps, ad9361_legacy_dev, steps);

	return 0;
}
MAP_CMD(set_agc_outer_thresh_low_inc_steps, map_set_agc_outer_thresh_low_inc_steps, 2);

static int map_set_agc_adc_small_overload_exceed_counter (int argc, const char **argv)
{
	MAP_ARG(uint32_t, counter, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_adc_small_overload_exceed_counter, ad9361_legacy_dev, counter);

	return 0;
}
MAP_CMD(set_agc_adc_small_overload_exceed_counter, map_set_agc_adc_small_overload_exceed_counter, 2);

static int map_set_agc_adc_large_overload_exceed_counter (int argc, const char **argv)
{
	MAP_ARG(uint32_t, counter, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_adc_large_overload_exceed_counter, ad9361_legacy_dev, counter);

	return 0;
}
MAP_CMD(set_agc_adc_large_overload_exceed_counter, map_set_agc_adc_large_overload_exceed_counter, 2);

static int map_set_agc_adc_large_overload_inc_steps (int argc, const char **argv)
{
	MAP_ARG(uint32_t, steps, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_adc_large_overload_inc_steps, ad9361_legacy_dev, steps);

	return 0;
}
MAP_CMD(set_agc_adc_large_overload_inc_steps, map_set_agc_adc_large_overload_inc_steps, 2);

static int map_set_agc_adc_lmt_small_overload_prevent_gain_inc_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_adc_lmt_small_overload_prevent_gain_inc_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_agc_adc_lmt_small_overload_prevent_gain_inc_enable, map_set_agc_adc_lmt_small_overload_prevent_gain_inc_enable, 2);

static int map_set_agc_lmt_overload_large_exceed_counter (int argc, const char **argv)
{
	MAP_ARG(uint32_t, counter, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_lmt_overload_large_exceed_counter, ad9361_legacy_dev, counter);

	return 0;
}
MAP_CMD(set_agc_lmt_overload_large_exceed_counter, map_set_agc_lmt_overload_large_exceed_counter, 2);

static int map_set_agc_lmt_overload_small_exceed_counter (int argc, const char **argv)
{
	MAP_ARG(uint32_t, counter, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_lmt_overload_small_exceed_counter, ad9361_legacy_dev, counter);

	return 0;
}
MAP_CMD(set_agc_lmt_overload_small_exceed_counter, map_set_agc_lmt_overload_small_exceed_counter, 2);

static int map_set_agc_lmt_overload_large_inc_steps (int argc, const char **argv)
{
	MAP_ARG(uint32_t, steps, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_lmt_overload_large_inc_steps, ad9361_legacy_dev, steps);

	return 0;
}
MAP_CMD(set_agc_lmt_overload_large_inc_steps, map_set_agc_lmt_overload_large_inc_steps, 2);

static int map_set_agc_dig_saturation_exceed_counter (int argc, const char **argv)
{
	MAP_ARG(uint32_t, counter, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_dig_saturation_exceed_counter, ad9361_legacy_dev, counter);

	return 0;
}
MAP_CMD(set_agc_dig_saturation_exceed_counter, map_set_agc_dig_saturation_exceed_counter, 2);

static int map_set_agc_dig_gain_step_size (int argc, const char **argv)
{
	MAP_ARG(uint32_t, size, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_dig_gain_step_size, ad9361_legacy_dev, size);

	return 0;
}
MAP_CMD(set_agc_dig_gain_step_size, map_set_agc_dig_gain_step_size, 2);

static int map_set_agc_sync_for_gain_counter_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_sync_for_gain_counter_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_agc_sync_for_gain_counter_enable, map_set_agc_sync_for_gain_counter_enable, 2);

static int map_set_agc_gain_update_interval_us (int argc, const char **argv)
{
	MAP_ARG(uint32_t, interval, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_gain_update_interval_us, ad9361_legacy_dev, interval);

	return 0;
}
MAP_CMD(set_agc_gain_update_interval_us, map_set_agc_gain_update_interval_us, 2);

static int map_set_agc_immed_gain_change_if_large_adc_overload_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_immed_gain_change_if_large_adc_overload_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_agc_immed_gain_change_if_large_adc_overload_enable, map_set_agc_immed_gain_change_if_large_adc_overload_enable, 2);

static int map_set_agc_immed_gain_change_if_large_lmt_overload_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_agc_immed_gain_change_if_large_lmt_overload_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_agc_immed_gain_change_if_large_lmt_overload_enable, map_set_agc_immed_gain_change_if_large_lmt_overload_enable, 2);

static int map_set_fagc_dec_pow_measurement_duration (int argc, const char **argv)
{
	MAP_ARG(uint32_t, duration, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_dec_pow_measurement_duration, ad9361_legacy_dev, duration);

	return 0;
}
MAP_CMD(set_fagc_dec_pow_measurement_duration, map_set_fagc_dec_pow_measurement_duration, 2);

static int map_set_fagc_state_wait_time_ns (int argc, const char **argv)
{
	MAP_ARG(uint32_t, time, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_state_wait_time_ns, ad9361_legacy_dev, time);

	return 0;
}
MAP_CMD(set_fagc_state_wait_time_ns, map_set_fagc_state_wait_time_ns, 2);

static int map_set_fagc_allow_agc_gain_increase_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_allow_agc_gain_increase_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_fagc_allow_agc_gain_increase_enable, map_set_fagc_allow_agc_gain_increase_enable, 2);

static int map_set_fagc_lp_thresh_increment_time (int argc, const char **argv)
{
	MAP_ARG(uint32_t, time, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_lp_thresh_increment_time, ad9361_legacy_dev, time);

	return 0;
}
MAP_CMD(set_fagc_lp_thresh_increment_time, map_set_fagc_lp_thresh_increment_time, 2);

static int map_set_fagc_lp_thresh_increment_steps (int argc, const char **argv)
{
	MAP_ARG(uint32_t, steps, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_lp_thresh_increment_steps, ad9361_legacy_dev, steps);

	return 0;
}
MAP_CMD(set_fagc_lp_thresh_increment_steps, map_set_fagc_lp_thresh_increment_steps, 2);

static int map_set_fagc_lock_level (int argc, const char **argv)
{
	MAP_ARG(uint32_t, level, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_lock_level, ad9361_legacy_dev, level);

	return 0;
}
MAP_CMD(set_fagc_lock_level, map_set_fagc_lock_level, 2);

static int map_set_fagc_lock_level_lmt_gain_increase_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_lock_level_lmt_gain_increase_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_fagc_lock_level_lmt_gain_increase_enable, map_set_fagc_lock_level_lmt_gain_increase_enable, 2);

static int map_set_fagc_lock_level_gain_increase_upper_limit (int argc, const char **argv)
{
	MAP_ARG(uint32_t, limit, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_lock_level_gain_increase_upper_limit, ad9361_legacy_dev, limit);

	return 0;
}
MAP_CMD(set_fagc_lock_level_gain_increase_upper_limit, map_set_fagc_lock_level_gain_increase_upper_limit, 2);

static int map_set_fagc_lpf_final_settling_steps (int argc, const char **argv)
{
	MAP_ARG(uint32_t, steps, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_lpf_final_settling_steps, ad9361_legacy_dev, steps);

	return 0;
}
MAP_CMD(set_fagc_lpf_final_settling_steps, map_set_fagc_lpf_final_settling_steps, 2);

static int map_set_fagc_lmt_final_settling_steps (int argc, const char **argv)
{
	MAP_ARG(uint32_t, steps, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_lmt_final_settling_steps, ad9361_legacy_dev, steps);

	return 0;
}
MAP_CMD(set_fagc_lmt_final_settling_steps, map_set_fagc_lmt_final_settling_steps, 2);

static int map_set_fagc_final_overrange_count (int argc, const char **argv)
{
	MAP_ARG(uint32_t, count, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_final_overrange_count, ad9361_legacy_dev, count);

	return 0;
}
MAP_CMD(set_fagc_final_overrange_count, map_set_fagc_final_overrange_count, 2);

static int map_set_fagc_gain_increase_after_gain_lock_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_gain_increase_after_gain_lock_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_fagc_gain_increase_after_gain_lock_enable, map_set_fagc_gain_increase_after_gain_lock_enable, 2);

static int map_set_fagc_gain_index_type_after_exit_rx_mode (int argc, const char **argv)
{
	MAP_ARG(uint32_t, mode, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_gain_index_type_after_exit_rx_mode, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_fagc_gain_index_type_after_exit_rx_mode, map_set_fagc_gain_index_type_after_exit_rx_mode, 2);

static int map_set_fagc_use_last_lock_level_for_set_gain_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_use_last_lock_level_for_set_gain_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_fagc_use_last_lock_level_for_set_gain_enable, map_set_fagc_use_last_lock_level_for_set_gain_enable, 2);

static int map_set_fagc_rst_gla_stronger_sig_thresh_exceeded_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_rst_gla_stronger_sig_thresh_exceeded_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_fagc_rst_gla_stronger_sig_thresh_exceeded_enable, map_set_fagc_rst_gla_stronger_sig_thresh_exceeded_enable, 2);

static int map_set_fagc_optimized_gain_offset (int argc, const char **argv)
{
	MAP_ARG(uint32_t, offset, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_optimized_gain_offset, ad9361_legacy_dev, offset);

	return 0;
}
MAP_CMD(set_fagc_optimized_gain_offset, map_set_fagc_optimized_gain_offset, 2);

static int map_set_fagc_rst_gla_stronger_sig_thresh_above_ll (int argc, const char **argv)
{
	MAP_ARG(uint32_t, thresh, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_rst_gla_stronger_sig_thresh_above_ll, ad9361_legacy_dev, thresh);

	return 0;
}
MAP_CMD(set_fagc_rst_gla_stronger_sig_thresh_above_ll, map_set_fagc_rst_gla_stronger_sig_thresh_above_ll, 2);

static int map_set_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable, map_set_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable, 2);

static int map_set_fagc_rst_gla_engergy_lost_goto_optim_gain_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_rst_gla_engergy_lost_goto_optim_gain_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_fagc_rst_gla_engergy_lost_goto_optim_gain_enable, map_set_fagc_rst_gla_engergy_lost_goto_optim_gain_enable, 2);

static int map_set_fagc_rst_gla_engergy_lost_sig_thresh_below_ll (int argc, const char **argv)
{
	MAP_ARG(uint32_t, thresh, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_rst_gla_engergy_lost_sig_thresh_below_ll, ad9361_legacy_dev, thresh);

	return 0;
}
MAP_CMD(set_fagc_rst_gla_engergy_lost_sig_thresh_below_ll, map_set_fagc_rst_gla_engergy_lost_sig_thresh_below_ll, 2);

static int map_set_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt (int argc, const char **argv)
{
	MAP_ARG(uint32_t, count, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt, ad9361_legacy_dev, count);

	return 0;
}
MAP_CMD(set_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt, map_set_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt, 2);

static int map_set_fagc_rst_gla_large_adc_overload_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_rst_gla_large_adc_overload_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_fagc_rst_gla_large_adc_overload_enable, map_set_fagc_rst_gla_large_adc_overload_enable, 2);

static int map_set_fagc_rst_gla_large_lmt_overload_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_rst_gla_large_lmt_overload_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_fagc_rst_gla_large_lmt_overload_enable, map_set_fagc_rst_gla_large_lmt_overload_enable, 2);

static int map_set_fagc_rst_gla_en_agc_pulled_high_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_rst_gla_en_agc_pulled_high_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_fagc_rst_gla_en_agc_pulled_high_enable, map_set_fagc_rst_gla_en_agc_pulled_high_enable, 2);

static int map_set_fagc_rst_gla_if_en_agc_pulled_high_mode (int argc, const char **argv)
{
	MAP_ARG(uint32_t, mode, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_rst_gla_if_en_agc_pulled_high_mode, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_fagc_rst_gla_if_en_agc_pulled_high_mode, map_set_fagc_rst_gla_if_en_agc_pulled_high_mode, 2);

static int map_set_fagc_power_measurement_duration_in_state5 (int argc, const char **argv)
{
	MAP_ARG(uint32_t, duration, 1, "");

	MAP_LIB_CALL(ad9361_set_fagc_power_measurement_duration_in_state5, ad9361_legacy_dev, duration);

	return 0;
}
MAP_CMD(set_fagc_power_measurement_duration_in_state5, map_set_fagc_power_measurement_duration_in_state5, 2);

static int map_set_rssi_restart_mode (int argc, const char **argv)
{
	MAP_ARG(uint32_t, mode, 1, "");

	MAP_LIB_CALL(ad9361_set_rssi_restart_mode, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_rssi_restart_mode, map_set_rssi_restart_mode, 2);

static int map_set_rssi_unit_is_rx_samples_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_rssi_unit_is_rx_samples_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_rssi_unit_is_rx_samples_enable, map_set_rssi_unit_is_rx_samples_enable, 2);

static int map_set_rssi_delay (int argc, const char **argv)
{
	MAP_ARG(uint32_t, delay, 1, "");

	MAP_LIB_CALL(ad9361_set_rssi_delay, ad9361_legacy_dev, delay);

	return 0;
}
MAP_CMD(set_rssi_delay, map_set_rssi_delay, 2);

static int map_set_rssi_wait (int argc, const char **argv)
{
	MAP_ARG(uint32_t, wait, 1, "");

	MAP_LIB_CALL(ad9361_set_rssi_wait, ad9361_legacy_dev, wait);

	return 0;
}
MAP_CMD(set_rssi_wait, map_set_rssi_wait, 2);

static int map_set_rssi_duration (int argc, const char **argv)
{
	MAP_ARG(uint32_t, duration, 1, "");

	MAP_LIB_CALL(ad9361_set_rssi_duration, ad9361_legacy_dev, duration);

	return 0;
}
MAP_CMD(set_rssi_duration, map_set_rssi_duration, 2);

static int map_set_ctrl_outs_index (int argc, const char **argv)
{
	MAP_ARG(uint32_t, index, 1, "");

	MAP_LIB_CALL(ad9361_set_ctrl_outs_index, ad9361_legacy_dev, index);

	return 0;
}
MAP_CMD(set_ctrl_outs_index, map_set_ctrl_outs_index, 2);

static int map_set_ctrl_outs_enable_mask (int argc, const char **argv)
{
	MAP_ARG(uint32_t, mask, 1, "");

	MAP_LIB_CALL(ad9361_set_ctrl_outs_enable_mask, ad9361_legacy_dev, mask);

	return 0;
}
MAP_CMD(set_ctrl_outs_enable_mask, map_set_ctrl_outs_enable_mask, 2);

static int map_set_elna_settling_delay_ns (int argc, const char **argv)
{
	MAP_ARG(uint32_t, delay, 1, "");

	MAP_LIB_CALL(ad9361_set_elna_settling_delay_ns, ad9361_legacy_dev, delay);

	return 0;
}
MAP_CMD(set_elna_settling_delay_ns, map_set_elna_settling_delay_ns, 2);

static int map_set_elna_gain_mdB (int argc, const char **argv)
{
	MAP_ARG(uint32_t, gain, 1, "");

	MAP_LIB_CALL(ad9361_set_elna_gain_mdB, ad9361_legacy_dev, gain);

	return 0;
}
MAP_CMD(set_elna_gain_mdB, map_set_elna_gain_mdB, 2);

static int map_set_elna_bypass_loss_mdB (int argc, const char **argv)
{
	MAP_ARG(uint32_t, loss, 1, "");

	MAP_LIB_CALL(ad9361_set_elna_bypass_loss_mdB, ad9361_legacy_dev, loss);

	return 0;
}
MAP_CMD(set_elna_bypass_loss_mdB, map_set_elna_bypass_loss_mdB, 2);

static int map_set_elna_rx1_gpo0_control_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_elna_rx1_gpo0_control_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_elna_rx1_gpo0_control_enable, map_set_elna_rx1_gpo0_control_enable, 2);

static int map_set_elna_rx2_gpo1_control_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_elna_rx2_gpo1_control_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_elna_rx2_gpo1_control_enable, map_set_elna_rx2_gpo1_control_enable, 2);

static int map_set_temp_sense_measurement_interval_ms (int argc, const char **argv)
{
	MAP_ARG(uint32_t, interval, 1, "");

	MAP_LIB_CALL(ad9361_set_temp_sense_measurement_interval_ms, ad9361_legacy_dev, interval);

	return 0;
}
MAP_CMD(set_temp_sense_measurement_interval_ms, map_set_temp_sense_measurement_interval_ms, 2);

static int map_set_temp_sense_offset_signed (int argc, const char **argv)
{
	MAP_ARG(int, offset, 1, "");

	MAP_LIB_CALL(ad9361_set_temp_sense_offset_signed, ad9361_legacy_dev, offset);

	return 0;
}
MAP_CMD(set_temp_sense_offset_signed, map_set_temp_sense_offset_signed, 2);

static int map_set_temp_sense_periodic_measurement_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_temp_sense_periodic_measurement_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_temp_sense_periodic_measurement_enable, map_set_temp_sense_periodic_measurement_enable, 2);

static int map_set_temp_sense_decimation (int argc, const char **argv)
{
	MAP_ARG(uint32_t, decimation, 1, "");

	MAP_LIB_CALL(ad9361_set_temp_sense_decimation, ad9361_legacy_dev, decimation);

	return 0;
}
MAP_CMD(set_temp_sense_decimation, map_set_temp_sense_decimation, 2);

static int map_set_aux_adc_rate (int argc, const char **argv)
{
	MAP_ARG(uint32_t, rate, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_adc_rate, ad9361_legacy_dev, rate);

	return 0;
}
MAP_CMD(set_aux_adc_rate, map_set_aux_adc_rate, 2);

static int map_set_aux_adc_decimation (int argc, const char **argv)
{
	MAP_ARG(uint32_t, decimation, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_adc_decimation, ad9361_legacy_dev, decimation);

	return 0;
}
MAP_CMD(set_aux_adc_decimation, map_set_aux_adc_decimation, 2);

static int map_set_aux_dac_manual_mode_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac_manual_mode_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_aux_dac_manual_mode_enable, map_set_aux_dac_manual_mode_enable, 2);

static int map_set_aux_dac1_default_value_mV (int argc, const char **argv)
{
	MAP_ARG(uint32_t, value, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac1_default_value_mV, ad9361_legacy_dev, value);

	return 0;
}
MAP_CMD(set_aux_dac1_default_value_mV, map_set_aux_dac1_default_value_mV, 2);

static int map_set_aux_dac1_active_in_rx_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac1_active_in_rx_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_aux_dac1_active_in_rx_enable, map_set_aux_dac1_active_in_rx_enable, 2);

static int map_set_aux_dac1_active_in_tx_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac1_active_in_tx_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_aux_dac1_active_in_tx_enable, map_set_aux_dac1_active_in_tx_enable, 2);

static int map_set_aux_dac1_active_in_alert_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac1_active_in_alert_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_aux_dac1_active_in_alert_enable, map_set_aux_dac1_active_in_alert_enable, 2);

static int map_set_aux_dac1_rx_delay_us (int argc, const char **argv)
{
	MAP_ARG(uint32_t, delay, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac1_rx_delay_us, ad9361_legacy_dev, delay);

	return 0;
}
MAP_CMD(set_aux_dac1_rx_delay_us, map_set_aux_dac1_rx_delay_us, 2);

static int map_set_aux_dac1_tx_delay_us (int argc, const char **argv)
{
	MAP_ARG(uint32_t, delay, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac1_tx_delay_us, ad9361_legacy_dev, delay);

	return 0;
}
MAP_CMD(set_aux_dac1_tx_delay_us, map_set_aux_dac1_tx_delay_us, 2);

static int map_set_aux_dac2_default_value_mV (int argc, const char **argv)
{
	MAP_ARG(uint32_t, value, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac2_default_value_mV, ad9361_legacy_dev, value);

	return 0;
}
MAP_CMD(set_aux_dac2_default_value_mV, map_set_aux_dac2_default_value_mV, 2);

static int map_set_aux_dac2_active_in_rx_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac2_active_in_rx_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_aux_dac2_active_in_rx_enable, map_set_aux_dac2_active_in_rx_enable, 2);

static int map_set_aux_dac2_active_in_tx_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac2_active_in_tx_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_aux_dac2_active_in_tx_enable, map_set_aux_dac2_active_in_tx_enable, 2);

static int map_set_aux_dac2_active_in_alert_enable (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac2_active_in_alert_enable, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_aux_dac2_active_in_alert_enable, map_set_aux_dac2_active_in_alert_enable, 2);

static int map_set_aux_dac2_rx_delay_us (int argc, const char **argv)
{
	MAP_ARG(uint32_t, delay, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac2_rx_delay_us, ad9361_legacy_dev, delay);

	return 0;
}
MAP_CMD(set_aux_dac2_rx_delay_us, map_set_aux_dac2_rx_delay_us, 2);

static int map_set_aux_dac2_tx_delay_us (int argc, const char **argv)
{
	MAP_ARG(uint32_t, delay, 1, "");

	MAP_LIB_CALL(ad9361_set_aux_dac2_tx_delay_us, ad9361_legacy_dev, delay);

	return 0;
}
MAP_CMD(set_aux_dac2_tx_delay_us, map_set_aux_dac2_tx_delay_us, 2);

static int map_get_bist_prbs (int argc, const char **argv)
{
	int  bist_prbs;

	MAP_LIB_CALL(ad9361_get_bist_prbs, ad9361_legacy_dev, &bist_prbs);

	MAP_RESULT(int, bist_prbs, 1);
	return 0;
}
MAP_CMD(get_bist_prbs, map_get_bist_prbs, 1);

static int map_get_bist_timing_analysis (int argc, const char **argv)
{
	char buff[4096];

	MAP_LIB_CALL(ad9361_get_bist_timing_analysis, ad9361_legacy_dev, buff, sizeof(buff));

	char *q;
	printf("bist_timing_analysis='");
	for ( q = trim(buff); *q; q++ )
		switch ( *q )
		{
			case '\'': fputs("'\\''", stdout); break;
			default:   putchar(*q);            break;
		}
	printf("'\n");
	return 0;
}
MAP_CMD(get_bist_timing_analysis, map_get_bist_timing_analysis, 1);

static int map_get_bist_tone (int argc, const char **argv)
{
	int  bist_tone;

	MAP_LIB_CALL(ad9361_get_bist_tone, ad9361_legacy_dev, &bist_tone);

	MAP_RESULT(int, bist_tone, 1);
	return 0;
}
MAP_CMD(get_bist_tone, map_get_bist_tone, 1);

static int map_get_loopback (int argc, const char **argv)
{
	int  loopback;

	MAP_LIB_CALL(ad9361_get_loopback, ad9361_legacy_dev, &loopback);

	MAP_RESULT(int, loopback, 1);
	return 0;
}
MAP_CMD(get_loopback, map_get_loopback, 1);

static int map_get_frequency_division_duplex_mode_enable (int argc, const char **argv)
{
	int  frequency_division_duplex_mode_enable;

	MAP_LIB_CALL(ad9361_get_frequency_division_duplex_mode_enable, ad9361_legacy_dev, &frequency_division_duplex_mode_enable);

	MAP_RESULT(int, frequency_division_duplex_mode_enable, 1);
	return 0;
}
MAP_CMD(get_frequency_division_duplex_mode_enable, map_get_frequency_division_duplex_mode_enable, 1);

static int map_get_gaininfo_rx1 (int argc, const char **argv)
{
	int32_t   gain_db;
	uint32_t  fgt_lmt_index;
	uint32_t  digital_gain;
	uint32_t  lmt_gain;
	uint32_t  lpf_gain;
	uint32_t  lna_index;
	uint32_t  tia_index;
	uint32_t  mixer_index;

	MAP_LIB_CALL(ad9361_get_gaininfo_rx1, ad9361_legacy_dev, &gain_db, &fgt_lmt_index, &digital_gain, &lmt_gain, &lpf_gain, &lna_index, &tia_index, &mixer_index);

	MAP_RESULT(int32_t,  gain_db,       1);
	MAP_RESULT(uint32_t, fgt_lmt_index, 1);
	MAP_RESULT(uint32_t, digital_gain,  1);
	MAP_RESULT(uint32_t, lmt_gain,      1);
	MAP_RESULT(uint32_t, lpf_gain,      1);
	MAP_RESULT(uint32_t, lna_index,     1);
	MAP_RESULT(uint32_t, tia_index,     1);
	MAP_RESULT(uint32_t, mixer_index,   1);
	return 0;
}
MAP_CMD(get_gaininfo_rx1, map_get_gaininfo_rx1, 1);

static int map_get_gaininfo_rx2 (int argc, const char **argv)
{
	int32_t   gain_db;
	uint32_t  fgt_lmt_index;
	uint32_t  digital_gain;
	uint32_t  lmt_gain;
	uint32_t  lpf_gain;
	uint32_t  lna_index;
	uint32_t  tia_index;
	uint32_t  mixer_index;

	MAP_LIB_CALL(ad9361_get_gaininfo_rx2, ad9361_legacy_dev, &gain_db, &fgt_lmt_index, &digital_gain, &lmt_gain, &lpf_gain, &lna_index, &tia_index, &mixer_index);

	MAP_RESULT(int32_t,  gain_db,       1);
	MAP_RESULT(uint32_t, fgt_lmt_index, 1);
	MAP_RESULT(uint32_t, digital_gain,  1);
	MAP_RESULT(uint32_t, lmt_gain,      1);
	MAP_RESULT(uint32_t, lpf_gain,      1);
	MAP_RESULT(uint32_t, lna_index,     1);
	MAP_RESULT(uint32_t, tia_index,     1);
	MAP_RESULT(uint32_t, mixer_index,   1);
	return 0;
}
MAP_CMD(get_gaininfo_rx2, map_get_gaininfo_rx2, 1);

static int map_get_gaininfo_rx (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	int32_t   gain_db;
	uint32_t  fgt_lmt_index;
	uint32_t  digital_gain;
	uint32_t  lmt_gain;
	uint32_t  lpf_gain;
	uint32_t  lna_index;
	uint32_t  tia_index;
	uint32_t  mixer_index;

	MAP_LIB_CALL(ad9361_get_gaininfo_rx, ad9361_legacy_dev, channel, &gain_db, &fgt_lmt_index, &digital_gain, &lmt_gain, &lpf_gain, &lna_index, &tia_index, &mixer_index);

	MAP_RESULT(int32_t,  gain_db,       1);
	MAP_RESULT(uint32_t, fgt_lmt_index, 1);
	MAP_RESULT(uint32_t, digital_gain,  1);
	MAP_RESULT(uint32_t, lmt_gain,      1);
	MAP_RESULT(uint32_t, lpf_gain,      1);
	MAP_RESULT(uint32_t, lna_index,     1);
	MAP_RESULT(uint32_t, tia_index,     1);
	MAP_RESULT(uint32_t, mixer_index,   1);
	return 0;
}
MAP_CMD(get_gaininfo_rx, map_get_gaininfo_rx, 2);

static int map_get_ensm_enable_pin_pulse_mode_enable (int argc, const char **argv)
{
	int  ensm_enable_pin_pulse_mode_enable;

	MAP_LIB_CALL(ad9361_get_ensm_enable_pin_pulse_mode_enable, ad9361_legacy_dev, &ensm_enable_pin_pulse_mode_enable);

	MAP_RESULT(int, ensm_enable_pin_pulse_mode_enable, 1);
	return 0;
}
MAP_CMD(get_ensm_enable_pin_pulse_mode_enable, map_get_ensm_enable_pin_pulse_mode_enable, 1);

static int map_get_ensm_enable_txnrx_control_enable (int argc, const char **argv)
{
	int  ensm_enable_txnrx_control_enable;

	MAP_LIB_CALL(ad9361_get_ensm_enable_txnrx_control_enable, ad9361_legacy_dev, &ensm_enable_txnrx_control_enable);

	MAP_RESULT(int, ensm_enable_txnrx_control_enable, 1);
	return 0;
}
MAP_CMD(get_ensm_enable_txnrx_control_enable, map_get_ensm_enable_txnrx_control_enable, 1);

static int map_get_debug_mode_enable (int argc, const char **argv)
{
	int  debug_mode_enable;

	MAP_LIB_CALL(ad9361_get_debug_mode_enable, ad9361_legacy_dev, &debug_mode_enable);

	MAP_RESULT(int, debug_mode_enable, 1);
	return 0;
}
MAP_CMD(get_debug_mode_enable, map_get_debug_mode_enable, 1);

static int map_get_tdd_use_fdd_vco_tables_enable (int argc, const char **argv)
{
	int  tdd_use_fdd_vco_tables_enable;

	MAP_LIB_CALL(ad9361_get_tdd_use_fdd_vco_tables_enable, ad9361_legacy_dev, &tdd_use_fdd_vco_tables_enable);

	MAP_RESULT(int, tdd_use_fdd_vco_tables_enable, 1);
	return 0;
}
MAP_CMD(get_tdd_use_fdd_vco_tables_enable, map_get_tdd_use_fdd_vco_tables_enable, 1);

static int map_get_tdd_use_dual_synth_mode_enable (int argc, const char **argv)
{
	int  tdd_use_dual_synth_mode_enable;

	MAP_LIB_CALL(ad9361_get_tdd_use_dual_synth_mode_enable, ad9361_legacy_dev, &tdd_use_dual_synth_mode_enable);

	MAP_RESULT(int, tdd_use_dual_synth_mode_enable, 1);
	return 0;
}
MAP_CMD(get_tdd_use_dual_synth_mode_enable, map_get_tdd_use_dual_synth_mode_enable, 1);

static int map_get_tdd_skip_vco_cal_enable (int argc, const char **argv)
{
	int  tdd_skip_vco_cal_enable;

	MAP_LIB_CALL(ad9361_get_tdd_skip_vco_cal_enable, ad9361_legacy_dev, &tdd_skip_vco_cal_enable);

	MAP_RESULT(int, tdd_skip_vco_cal_enable, 1);
	return 0;
}
MAP_CMD(get_tdd_skip_vco_cal_enable, map_get_tdd_skip_vco_cal_enable, 1);

static int map_get_2rx_2tx_mode_enable (int argc, const char **argv)
{
	int  _2rx_2tx_mode_enable;

	MAP_LIB_CALL(ad9361_get_2rx_2tx_mode_enable, ad9361_legacy_dev, &_2rx_2tx_mode_enable);

	MAP_RESULT(int, _2rx_2tx_mode_enable, 1);
	return 0;
}
MAP_CMD(get_2rx_2tx_mode_enable, map_get_2rx_2tx_mode_enable, 1);

static int map_get_split_gain_table_mode_enable (int argc, const char **argv)
{
	int  split_gain_table_mode_enable;

	MAP_LIB_CALL(ad9361_get_split_gain_table_mode_enable, ad9361_legacy_dev, &split_gain_table_mode_enable);

	MAP_RESULT(int, split_gain_table_mode_enable, 1);
	return 0;
}
MAP_CMD(get_split_gain_table_mode_enable, map_get_split_gain_table_mode_enable, 1);

static int map_get_rx_rf_port_input_select (int argc, const char **argv)
{
	uint32_t  rx_rf_port_input_select;

	MAP_LIB_CALL(ad9361_get_rx_rf_port_input_select, ad9361_legacy_dev, &rx_rf_port_input_select);

	MAP_RESULT(uint32_t, rx_rf_port_input_select, 1);
	return 0;
}
MAP_CMD(get_rx_rf_port_input_select, map_get_rx_rf_port_input_select, 1);

static int map_get_tx_rf_port_input_select (int argc, const char **argv)
{
	uint32_t  tx_rf_port_input_select;

	MAP_LIB_CALL(ad9361_get_tx_rf_port_input_select, ad9361_legacy_dev, &tx_rf_port_input_select);

	MAP_RESULT(uint32_t, tx_rf_port_input_select, 1);
	return 0;
}
MAP_CMD(get_tx_rf_port_input_select, map_get_tx_rf_port_input_select, 1);

static int map_get_external_tx_lo_enable (int argc, const char **argv)
{
	int  external_tx_lo_enable;

	MAP_LIB_CALL(ad9361_get_external_tx_lo_enable, ad9361_legacy_dev, &external_tx_lo_enable);

	MAP_RESULT(int, external_tx_lo_enable, 1);
	return 0;
}
MAP_CMD(get_external_tx_lo_enable, map_get_external_tx_lo_enable, 1);

static int map_get_external_rx_lo_enable (int argc, const char **argv)
{
	int  external_rx_lo_enable;

	MAP_LIB_CALL(ad9361_get_external_rx_lo_enable, ad9361_legacy_dev, &external_rx_lo_enable);

	MAP_RESULT(int, external_rx_lo_enable, 1);
	return 0;
}
MAP_CMD(get_external_rx_lo_enable, map_get_external_rx_lo_enable, 1);

static int map_get_xo_disable_use_ext_refclk_enable (int argc, const char **argv)
{
	int  xo_disable_use_ext_refclk_enable;

	MAP_LIB_CALL(ad9361_get_xo_disable_use_ext_refclk_enable, ad9361_legacy_dev, &xo_disable_use_ext_refclk_enable);

	MAP_RESULT(int, xo_disable_use_ext_refclk_enable, 1);
	return 0;
}
MAP_CMD(get_xo_disable_use_ext_refclk_enable, map_get_xo_disable_use_ext_refclk_enable, 1);

static int map_get_clk_output_mode_select (int argc, const char **argv)
{
	uint32_t  clk_output_mode_select;

	MAP_LIB_CALL(ad9361_get_clk_output_mode_select, ad9361_legacy_dev, &clk_output_mode_select);

	MAP_RESULT(uint32_t, clk_output_mode_select, 1);
	return 0;
}
MAP_CMD(get_clk_output_mode_select, map_get_clk_output_mode_select, 1);

static int map_get_dc_offset_tracking_update_event_mask (int argc, const char **argv)
{
	uint32_t  dc_offset_tracking_update_event_mask;

	MAP_LIB_CALL(ad9361_get_dc_offset_tracking_update_event_mask, ad9361_legacy_dev, &dc_offset_tracking_update_event_mask);

	MAP_RESULT(uint32_t, dc_offset_tracking_update_event_mask, 1);
	return 0;
}
MAP_CMD(get_dc_offset_tracking_update_event_mask, map_get_dc_offset_tracking_update_event_mask, 1);

static int map_get_dc_offset_attenuation_high_range (int argc, const char **argv)
{
	uint32_t  dc_offset_attenuation_high_range;

	MAP_LIB_CALL(ad9361_get_dc_offset_attenuation_high_range, ad9361_legacy_dev, &dc_offset_attenuation_high_range);

	MAP_RESULT(uint32_t, dc_offset_attenuation_high_range, 1);
	return 0;
}
MAP_CMD(get_dc_offset_attenuation_high_range, map_get_dc_offset_attenuation_high_range, 1);

static int map_get_dc_offset_attenuation_low_range (int argc, const char **argv)
{
	uint32_t  dc_offset_attenuation_low_range;

	MAP_LIB_CALL(ad9361_get_dc_offset_attenuation_low_range, ad9361_legacy_dev, &dc_offset_attenuation_low_range);

	MAP_RESULT(uint32_t, dc_offset_attenuation_low_range, 1);
	return 0;
}
MAP_CMD(get_dc_offset_attenuation_low_range, map_get_dc_offset_attenuation_low_range, 1);

static int map_get_dc_offset_count_high_range (int argc, const char **argv)
{
	uint32_t  dc_offset_count_high_range;

	MAP_LIB_CALL(ad9361_get_dc_offset_count_high_range, ad9361_legacy_dev, &dc_offset_count_high_range);

	MAP_RESULT(uint32_t, dc_offset_count_high_range, 1);
	return 0;
}
MAP_CMD(get_dc_offset_count_high_range, map_get_dc_offset_count_high_range, 1);

static int map_get_dc_offset_count_low_range (int argc, const char **argv)
{
	uint32_t  dc_offset_count_low_range;

	MAP_LIB_CALL(ad9361_get_dc_offset_count_low_range, ad9361_legacy_dev, &dc_offset_count_low_range);

	MAP_RESULT(uint32_t, dc_offset_count_low_range, 1);
	return 0;
}
MAP_CMD(get_dc_offset_count_low_range, map_get_dc_offset_count_low_range, 1);

static int map_get_rf_rx_bandwidth_hz (int argc, const char **argv)
{
	uint32_t  rf_rx_bandwidth_hz;

	MAP_LIB_CALL(ad9361_get_rf_rx_bandwidth_hz, ad9361_legacy_dev, &rf_rx_bandwidth_hz);

	MAP_RESULT(uint32_t, rf_rx_bandwidth_hz, 1);
	return 0;
}
MAP_CMD(get_rf_rx_bandwidth_hz, map_get_rf_rx_bandwidth_hz, 1);

static int map_get_rf_tx_bandwidth_hz (int argc, const char **argv)
{
	uint32_t  rf_tx_bandwidth_hz;

	MAP_LIB_CALL(ad9361_get_rf_tx_bandwidth_hz, ad9361_legacy_dev, &rf_tx_bandwidth_hz);

	MAP_RESULT(uint32_t, rf_tx_bandwidth_hz, 1);
	return 0;
}
MAP_CMD(get_rf_tx_bandwidth_hz, map_get_rf_tx_bandwidth_hz, 1);

static int map_get_tx_attenuation_mdB (int argc, const char **argv)
{
	uint32_t  tx_attenuation_mdB;

	MAP_LIB_CALL(ad9361_get_tx_attenuation_mdB, ad9361_legacy_dev, &tx_attenuation_mdB);

	MAP_RESULT(uint32_t, tx_attenuation_mdB, 1);
	return 0;
}
MAP_CMD(get_tx_attenuation_mdB, map_get_tx_attenuation_mdB, 1);

static int map_get_update_tx_gain_in_alert_enable (int argc, const char **argv)
{
	int  update_tx_gain_in_alert_enable;

	MAP_LIB_CALL(ad9361_get_update_tx_gain_in_alert_enable, ad9361_legacy_dev, &update_tx_gain_in_alert_enable);

	MAP_RESULT(int, update_tx_gain_in_alert_enable, 1);
	return 0;
}
MAP_CMD(get_update_tx_gain_in_alert_enable, map_get_update_tx_gain_in_alert_enable, 1);

static int map_get_gc_rx1_mode (int argc, const char **argv)
{
	uint32_t  gc_rx1_mode;

	MAP_LIB_CALL(ad9361_get_gc_rx1_mode, ad9361_legacy_dev, &gc_rx1_mode);

	MAP_RESULT(uint32_t, gc_rx1_mode, 1);
	return 0;
}
MAP_CMD(get_gc_rx1_mode, map_get_gc_rx1_mode, 1);

static int map_get_gc_rx2_mode (int argc, const char **argv)
{
	uint32_t  gc_rx2_mode;

	MAP_LIB_CALL(ad9361_get_gc_rx2_mode, ad9361_legacy_dev, &gc_rx2_mode);

	MAP_RESULT(uint32_t, gc_rx2_mode, 1);
	return 0;
}
MAP_CMD(get_gc_rx2_mode, map_get_gc_rx2_mode, 1);

static int map_get_gc_adc_ovr_sample_size (int argc, const char **argv)
{
	uint32_t  gc_adc_ovr_sample_size;

	MAP_LIB_CALL(ad9361_get_gc_adc_ovr_sample_size, ad9361_legacy_dev, &gc_adc_ovr_sample_size);

	MAP_RESULT(uint32_t, gc_adc_ovr_sample_size, 1);
	return 0;
}
MAP_CMD(get_gc_adc_ovr_sample_size, map_get_gc_adc_ovr_sample_size, 1);

static int map_get_gc_adc_small_overload_thresh (int argc, const char **argv)
{
	uint32_t  gc_adc_small_overload_thresh;

	MAP_LIB_CALL(ad9361_get_gc_adc_small_overload_thresh, ad9361_legacy_dev, &gc_adc_small_overload_thresh);

	MAP_RESULT(uint32_t, gc_adc_small_overload_thresh, 1);
	return 0;
}
MAP_CMD(get_gc_adc_small_overload_thresh, map_get_gc_adc_small_overload_thresh, 1);

static int map_get_gc_adc_large_overload_thresh (int argc, const char **argv)
{
	uint32_t  gc_adc_large_overload_thresh;

	MAP_LIB_CALL(ad9361_get_gc_adc_large_overload_thresh, ad9361_legacy_dev, &gc_adc_large_overload_thresh);

	MAP_RESULT(uint32_t, gc_adc_large_overload_thresh, 1);
	return 0;
}
MAP_CMD(get_gc_adc_large_overload_thresh, map_get_gc_adc_large_overload_thresh, 1);

static int map_get_gc_lmt_overload_high_thresh (int argc, const char **argv)
{
	uint32_t  gc_lmt_overload_high_thresh;

	MAP_LIB_CALL(ad9361_get_gc_lmt_overload_high_thresh, ad9361_legacy_dev, &gc_lmt_overload_high_thresh);

	MAP_RESULT(uint32_t, gc_lmt_overload_high_thresh, 1);
	return 0;
}
MAP_CMD(get_gc_lmt_overload_high_thresh, map_get_gc_lmt_overload_high_thresh, 1);

static int map_get_gc_lmt_overload_low_thresh (int argc, const char **argv)
{
	uint32_t  gc_lmt_overload_low_thresh;

	MAP_LIB_CALL(ad9361_get_gc_lmt_overload_low_thresh, ad9361_legacy_dev, &gc_lmt_overload_low_thresh);

	MAP_RESULT(uint32_t, gc_lmt_overload_low_thresh, 1);
	return 0;
}
MAP_CMD(get_gc_lmt_overload_low_thresh, map_get_gc_lmt_overload_low_thresh, 1);

static int map_get_gc_dec_pow_measurement_duration (int argc, const char **argv)
{
	uint32_t  gc_dec_pow_measurement_duration;

	MAP_LIB_CALL(ad9361_get_gc_dec_pow_measurement_duration, ad9361_legacy_dev, &gc_dec_pow_measurement_duration);

	MAP_RESULT(uint32_t, gc_dec_pow_measurement_duration, 1);
	return 0;
}
MAP_CMD(get_gc_dec_pow_measurement_duration, map_get_gc_dec_pow_measurement_duration, 1);

static int map_get_gc_low_power_thresh (int argc, const char **argv)
{
	uint32_t  gc_low_power_thresh;

	MAP_LIB_CALL(ad9361_get_gc_low_power_thresh, ad9361_legacy_dev, &gc_low_power_thresh);

	MAP_RESULT(uint32_t, gc_low_power_thresh, 1);
	return 0;
}
MAP_CMD(get_gc_low_power_thresh, map_get_gc_low_power_thresh, 1);

static int map_get_gc_dig_gain_enable (int argc, const char **argv)
{
	int  gc_dig_gain_enable;

	MAP_LIB_CALL(ad9361_get_gc_dig_gain_enable, ad9361_legacy_dev, &gc_dig_gain_enable);

	MAP_RESULT(int, gc_dig_gain_enable, 1);
	return 0;
}
MAP_CMD(get_gc_dig_gain_enable, map_get_gc_dig_gain_enable, 1);

static int map_get_gc_max_dig_gain (int argc, const char **argv)
{
	uint32_t  gc_max_dig_gain;

	MAP_LIB_CALL(ad9361_get_gc_max_dig_gain, ad9361_legacy_dev, &gc_max_dig_gain);

	MAP_RESULT(uint32_t, gc_max_dig_gain, 1);
	return 0;
}
MAP_CMD(get_gc_max_dig_gain, map_get_gc_max_dig_gain, 1);

static int map_get_mgc_rx1_ctrl_inp_enable (int argc, const char **argv)
{
	int  mgc_rx1_ctrl_inp_enable;

	MAP_LIB_CALL(ad9361_get_mgc_rx1_ctrl_inp_enable, ad9361_legacy_dev, &mgc_rx1_ctrl_inp_enable);

	MAP_RESULT(int, mgc_rx1_ctrl_inp_enable, 1);
	return 0;
}
MAP_CMD(get_mgc_rx1_ctrl_inp_enable, map_get_mgc_rx1_ctrl_inp_enable, 1);

static int map_get_mgc_rx2_ctrl_inp_enable (int argc, const char **argv)
{
	int  mgc_rx2_ctrl_inp_enable;

	MAP_LIB_CALL(ad9361_get_mgc_rx2_ctrl_inp_enable, ad9361_legacy_dev, &mgc_rx2_ctrl_inp_enable);

	MAP_RESULT(int, mgc_rx2_ctrl_inp_enable, 1);
	return 0;
}
MAP_CMD(get_mgc_rx2_ctrl_inp_enable, map_get_mgc_rx2_ctrl_inp_enable, 1);

static int map_get_mgc_inc_gain_step (int argc, const char **argv)
{
	uint32_t  mgc_inc_gain_step;

	MAP_LIB_CALL(ad9361_get_mgc_inc_gain_step, ad9361_legacy_dev, &mgc_inc_gain_step);

	MAP_RESULT(uint32_t, mgc_inc_gain_step, 1);
	return 0;
}
MAP_CMD(get_mgc_inc_gain_step, map_get_mgc_inc_gain_step, 1);

static int map_get_mgc_dec_gain_step (int argc, const char **argv)
{
	uint32_t  mgc_dec_gain_step;

	MAP_LIB_CALL(ad9361_get_mgc_dec_gain_step, ad9361_legacy_dev, &mgc_dec_gain_step);

	MAP_RESULT(uint32_t, mgc_dec_gain_step, 1);
	return 0;
}
MAP_CMD(get_mgc_dec_gain_step, map_get_mgc_dec_gain_step, 1);

static int map_get_mgc_split_table_ctrl_inp_gain_mode (int argc, const char **argv)
{
	uint32_t  mgc_split_table_ctrl_inp_gain_mode;

	MAP_LIB_CALL(ad9361_get_mgc_split_table_ctrl_inp_gain_mode, ad9361_legacy_dev, &mgc_split_table_ctrl_inp_gain_mode);

	MAP_RESULT(uint32_t, mgc_split_table_ctrl_inp_gain_mode, 1);
	return 0;
}
MAP_CMD(get_mgc_split_table_ctrl_inp_gain_mode, map_get_mgc_split_table_ctrl_inp_gain_mode, 1);

static int map_get_agc_attack_delay_extra_margin_us (int argc, const char **argv)
{
	uint32_t  agc_attack_delay_extra_margin_us;

	MAP_LIB_CALL(ad9361_get_agc_attack_delay_extra_margin_us, ad9361_legacy_dev, &agc_attack_delay_extra_margin_us);

	MAP_RESULT(uint32_t, agc_attack_delay_extra_margin_us, 1);
	return 0;
}
MAP_CMD(get_agc_attack_delay_extra_margin_us, map_get_agc_attack_delay_extra_margin_us, 1);

static int map_get_agc_outer_thresh_high (int argc, const char **argv)
{
	uint32_t  agc_outer_thresh_high;

	MAP_LIB_CALL(ad9361_get_agc_outer_thresh_high, ad9361_legacy_dev, &agc_outer_thresh_high);

	MAP_RESULT(uint32_t, agc_outer_thresh_high, 1);
	return 0;
}
MAP_CMD(get_agc_outer_thresh_high, map_get_agc_outer_thresh_high, 1);

static int map_get_agc_outer_thresh_high_dec_steps (int argc, const char **argv)
{
	uint32_t  agc_outer_thresh_high_dec_steps;

	MAP_LIB_CALL(ad9361_get_agc_outer_thresh_high_dec_steps, ad9361_legacy_dev, &agc_outer_thresh_high_dec_steps);

	MAP_RESULT(uint32_t, agc_outer_thresh_high_dec_steps, 1);
	return 0;
}
MAP_CMD(get_agc_outer_thresh_high_dec_steps, map_get_agc_outer_thresh_high_dec_steps, 1);

static int map_get_agc_inner_thresh_high (int argc, const char **argv)
{
	uint32_t  agc_inner_thresh_high;

	MAP_LIB_CALL(ad9361_get_agc_inner_thresh_high, ad9361_legacy_dev, &agc_inner_thresh_high);

	MAP_RESULT(uint32_t, agc_inner_thresh_high, 1);
	return 0;
}
MAP_CMD(get_agc_inner_thresh_high, map_get_agc_inner_thresh_high, 1);

static int map_get_agc_inner_thresh_high_dec_steps (int argc, const char **argv)
{
	uint32_t  agc_inner_thresh_high_dec_steps;

	MAP_LIB_CALL(ad9361_get_agc_inner_thresh_high_dec_steps, ad9361_legacy_dev, &agc_inner_thresh_high_dec_steps);

	MAP_RESULT(uint32_t, agc_inner_thresh_high_dec_steps, 1);
	return 0;
}
MAP_CMD(get_agc_inner_thresh_high_dec_steps, map_get_agc_inner_thresh_high_dec_steps, 1);

static int map_get_agc_inner_thresh_low (int argc, const char **argv)
{
	uint32_t  agc_inner_thresh_low;

	MAP_LIB_CALL(ad9361_get_agc_inner_thresh_low, ad9361_legacy_dev, &agc_inner_thresh_low);

	MAP_RESULT(uint32_t, agc_inner_thresh_low, 1);
	return 0;
}
MAP_CMD(get_agc_inner_thresh_low, map_get_agc_inner_thresh_low, 1);

static int map_get_agc_inner_thresh_low_inc_steps (int argc, const char **argv)
{
	uint32_t  agc_inner_thresh_low_inc_steps;

	MAP_LIB_CALL(ad9361_get_agc_inner_thresh_low_inc_steps, ad9361_legacy_dev, &agc_inner_thresh_low_inc_steps);

	MAP_RESULT(uint32_t, agc_inner_thresh_low_inc_steps, 1);
	return 0;
}
MAP_CMD(get_agc_inner_thresh_low_inc_steps, map_get_agc_inner_thresh_low_inc_steps, 1);

static int map_get_agc_outer_thresh_low (int argc, const char **argv)
{
	uint32_t  agc_outer_thresh_low;

	MAP_LIB_CALL(ad9361_get_agc_outer_thresh_low, ad9361_legacy_dev, &agc_outer_thresh_low);

	MAP_RESULT(uint32_t, agc_outer_thresh_low, 1);
	return 0;
}
MAP_CMD(get_agc_outer_thresh_low, map_get_agc_outer_thresh_low, 1);

static int map_get_agc_outer_thresh_low_inc_steps (int argc, const char **argv)
{
	uint32_t  agc_outer_thresh_low_inc_steps;

	MAP_LIB_CALL(ad9361_get_agc_outer_thresh_low_inc_steps, ad9361_legacy_dev, &agc_outer_thresh_low_inc_steps);

	MAP_RESULT(uint32_t, agc_outer_thresh_low_inc_steps, 1);
	return 0;
}
MAP_CMD(get_agc_outer_thresh_low_inc_steps, map_get_agc_outer_thresh_low_inc_steps, 1);

static int map_get_agc_adc_small_overload_exceed_counter (int argc, const char **argv)
{
	uint32_t  agc_adc_small_overload_exceed_counter;

	MAP_LIB_CALL(ad9361_get_agc_adc_small_overload_exceed_counter, ad9361_legacy_dev, &agc_adc_small_overload_exceed_counter);

	MAP_RESULT(uint32_t, agc_adc_small_overload_exceed_counter, 1);
	return 0;
}
MAP_CMD(get_agc_adc_small_overload_exceed_counter, map_get_agc_adc_small_overload_exceed_counter, 1);

static int map_get_agc_adc_large_overload_exceed_counter (int argc, const char **argv)
{
	uint32_t  agc_adc_large_overload_exceed_counter;

	MAP_LIB_CALL(ad9361_get_agc_adc_large_overload_exceed_counter, ad9361_legacy_dev, &agc_adc_large_overload_exceed_counter);

	MAP_RESULT(uint32_t, agc_adc_large_overload_exceed_counter, 1);
	return 0;
}
MAP_CMD(get_agc_adc_large_overload_exceed_counter, map_get_agc_adc_large_overload_exceed_counter, 1);

static int map_get_agc_adc_large_overload_inc_steps (int argc, const char **argv)
{
	uint32_t  agc_adc_large_overload_inc_steps;

	MAP_LIB_CALL(ad9361_get_agc_adc_large_overload_inc_steps, ad9361_legacy_dev, &agc_adc_large_overload_inc_steps);

	MAP_RESULT(uint32_t, agc_adc_large_overload_inc_steps, 1);
	return 0;
}
MAP_CMD(get_agc_adc_large_overload_inc_steps, map_get_agc_adc_large_overload_inc_steps, 1);

static int map_get_agc_adc_lmt_small_overload_prevent_gain_inc_enable (int argc, const char **argv)
{
	int  agc_adc_lmt_small_overload_prevent_gain_inc_enable;

	MAP_LIB_CALL(ad9361_get_agc_adc_lmt_small_overload_prevent_gain_inc_enable, ad9361_legacy_dev, &agc_adc_lmt_small_overload_prevent_gain_inc_enable);

	MAP_RESULT(int, agc_adc_lmt_small_overload_prevent_gain_inc_enable, 1);
	return 0;
}
MAP_CMD(get_agc_adc_lmt_small_overload_prevent_gain_inc_enable, map_get_agc_adc_lmt_small_overload_prevent_gain_inc_enable, 1);

static int map_get_agc_lmt_overload_large_exceed_counter (int argc, const char **argv)
{
	uint32_t  agc_lmt_overload_large_exceed_counter;

	MAP_LIB_CALL(ad9361_get_agc_lmt_overload_large_exceed_counter, ad9361_legacy_dev, &agc_lmt_overload_large_exceed_counter);

	MAP_RESULT(uint32_t, agc_lmt_overload_large_exceed_counter, 1);
	return 0;
}
MAP_CMD(get_agc_lmt_overload_large_exceed_counter, map_get_agc_lmt_overload_large_exceed_counter, 1);

static int map_get_agc_lmt_overload_small_exceed_counter (int argc, const char **argv)
{
	uint32_t  agc_lmt_overload_small_exceed_counter;

	MAP_LIB_CALL(ad9361_get_agc_lmt_overload_small_exceed_counter, ad9361_legacy_dev, &agc_lmt_overload_small_exceed_counter);

	MAP_RESULT(uint32_t, agc_lmt_overload_small_exceed_counter, 1);
	return 0;
}
MAP_CMD(get_agc_lmt_overload_small_exceed_counter, map_get_agc_lmt_overload_small_exceed_counter, 1);

static int map_get_agc_lmt_overload_large_inc_steps (int argc, const char **argv)
{
	uint32_t  agc_lmt_overload_large_inc_steps;

	MAP_LIB_CALL(ad9361_get_agc_lmt_overload_large_inc_steps, ad9361_legacy_dev, &agc_lmt_overload_large_inc_steps);

	MAP_RESULT(uint32_t, agc_lmt_overload_large_inc_steps, 1);
	return 0;
}
MAP_CMD(get_agc_lmt_overload_large_inc_steps, map_get_agc_lmt_overload_large_inc_steps, 1);

static int map_get_agc_dig_saturation_exceed_counter (int argc, const char **argv)
{
	uint32_t  agc_dig_saturation_exceed_counter;

	MAP_LIB_CALL(ad9361_get_agc_dig_saturation_exceed_counter, ad9361_legacy_dev, &agc_dig_saturation_exceed_counter);

	MAP_RESULT(uint32_t, agc_dig_saturation_exceed_counter, 1);
	return 0;
}
MAP_CMD(get_agc_dig_saturation_exceed_counter, map_get_agc_dig_saturation_exceed_counter, 1);

static int map_get_agc_dig_gain_step_size (int argc, const char **argv)
{
	uint32_t  agc_dig_gain_step_size;

	MAP_LIB_CALL(ad9361_get_agc_dig_gain_step_size, ad9361_legacy_dev, &agc_dig_gain_step_size);

	MAP_RESULT(uint32_t, agc_dig_gain_step_size, 1);
	return 0;
}
MAP_CMD(get_agc_dig_gain_step_size, map_get_agc_dig_gain_step_size, 1);

static int map_get_agc_sync_for_gain_counter_enable (int argc, const char **argv)
{
	int  agc_sync_for_gain_counter_enable;

	MAP_LIB_CALL(ad9361_get_agc_sync_for_gain_counter_enable, ad9361_legacy_dev, &agc_sync_for_gain_counter_enable);

	MAP_RESULT(int, agc_sync_for_gain_counter_enable, 1);
	return 0;
}
MAP_CMD(get_agc_sync_for_gain_counter_enable, map_get_agc_sync_for_gain_counter_enable, 1);

static int map_get_agc_gain_update_interval_us (int argc, const char **argv)
{
	uint32_t  agc_gain_update_interval_us;

	MAP_LIB_CALL(ad9361_get_agc_gain_update_interval_us, ad9361_legacy_dev, &agc_gain_update_interval_us);

	MAP_RESULT(uint32_t, agc_gain_update_interval_us, 1);
	return 0;
}
MAP_CMD(get_agc_gain_update_interval_us, map_get_agc_gain_update_interval_us, 1);

static int map_get_agc_immed_gain_change_if_large_adc_overload_enable (int argc, const char **argv)
{
	int  agc_immed_gain_change_if_large_adc_overload_enable;

	MAP_LIB_CALL(ad9361_get_agc_immed_gain_change_if_large_adc_overload_enable, ad9361_legacy_dev, &agc_immed_gain_change_if_large_adc_overload_enable);

	MAP_RESULT(int, agc_immed_gain_change_if_large_adc_overload_enable, 1);
	return 0;
}
MAP_CMD(get_agc_immed_gain_change_if_large_adc_overload_enable, map_get_agc_immed_gain_change_if_large_adc_overload_enable, 1);

static int map_get_agc_immed_gain_change_if_large_lmt_overload_enable (int argc, const char **argv)
{
	int  agc_immed_gain_change_if_large_lmt_overload_enable;

	MAP_LIB_CALL(ad9361_get_agc_immed_gain_change_if_large_lmt_overload_enable, ad9361_legacy_dev, &agc_immed_gain_change_if_large_lmt_overload_enable);

	MAP_RESULT(int, agc_immed_gain_change_if_large_lmt_overload_enable, 1);
	return 0;
}
MAP_CMD(get_agc_immed_gain_change_if_large_lmt_overload_enable, map_get_agc_immed_gain_change_if_large_lmt_overload_enable, 1);

static int map_get_fagc_dec_pow_measurement_duration (int argc, const char **argv)
{
	uint32_t  fagc_dec_pow_measurement_duration;

	MAP_LIB_CALL(ad9361_get_fagc_dec_pow_measurement_duration, ad9361_legacy_dev, &fagc_dec_pow_measurement_duration);

	MAP_RESULT(uint32_t, fagc_dec_pow_measurement_duration, 1);
	return 0;
}
MAP_CMD(get_fagc_dec_pow_measurement_duration, map_get_fagc_dec_pow_measurement_duration, 1);

static int map_get_fagc_state_wait_time_ns (int argc, const char **argv)
{
	uint32_t  fagc_state_wait_time_ns;

	MAP_LIB_CALL(ad9361_get_fagc_state_wait_time_ns, ad9361_legacy_dev, &fagc_state_wait_time_ns);

	MAP_RESULT(uint32_t, fagc_state_wait_time_ns, 1);
	return 0;
}
MAP_CMD(get_fagc_state_wait_time_ns, map_get_fagc_state_wait_time_ns, 1);

static int map_get_fagc_allow_agc_gain_increase_enable (int argc, const char **argv)
{
	int  fagc_allow_agc_gain_increase_enable;

	MAP_LIB_CALL(ad9361_get_fagc_allow_agc_gain_increase_enable, ad9361_legacy_dev, &fagc_allow_agc_gain_increase_enable);

	MAP_RESULT(int, fagc_allow_agc_gain_increase_enable, 1);
	return 0;
}
MAP_CMD(get_fagc_allow_agc_gain_increase_enable, map_get_fagc_allow_agc_gain_increase_enable, 1);

static int map_get_fagc_lp_thresh_increment_time (int argc, const char **argv)
{
	uint32_t  fagc_lp_thresh_increment_time;

	MAP_LIB_CALL(ad9361_get_fagc_lp_thresh_increment_time, ad9361_legacy_dev, &fagc_lp_thresh_increment_time);

	MAP_RESULT(uint32_t, fagc_lp_thresh_increment_time, 1);
	return 0;
}
MAP_CMD(get_fagc_lp_thresh_increment_time, map_get_fagc_lp_thresh_increment_time, 1);

static int map_get_fagc_lp_thresh_increment_steps (int argc, const char **argv)
{
	uint32_t  fagc_lp_thresh_increment_steps;

	MAP_LIB_CALL(ad9361_get_fagc_lp_thresh_increment_steps, ad9361_legacy_dev, &fagc_lp_thresh_increment_steps);

	MAP_RESULT(uint32_t, fagc_lp_thresh_increment_steps, 1);
	return 0;
}
MAP_CMD(get_fagc_lp_thresh_increment_steps, map_get_fagc_lp_thresh_increment_steps, 1);

static int map_get_fagc_lock_level (int argc, const char **argv)
{
	uint32_t  fagc_lock_level;

	MAP_LIB_CALL(ad9361_get_fagc_lock_level, ad9361_legacy_dev, &fagc_lock_level);

	MAP_RESULT(uint32_t, fagc_lock_level, 1);
	return 0;
}
MAP_CMD(get_fagc_lock_level, map_get_fagc_lock_level, 1);

static int map_get_fagc_lock_level_lmt_gain_increase_enable (int argc, const char **argv)
{
	int  fagc_lock_level_lmt_gain_increase_enable;

	MAP_LIB_CALL(ad9361_get_fagc_lock_level_lmt_gain_increase_enable, ad9361_legacy_dev, &fagc_lock_level_lmt_gain_increase_enable);

	MAP_RESULT(int, fagc_lock_level_lmt_gain_increase_enable, 1);
	return 0;
}
MAP_CMD(get_fagc_lock_level_lmt_gain_increase_enable, map_get_fagc_lock_level_lmt_gain_increase_enable, 1);

static int map_get_fagc_lock_level_gain_increase_upper_limit (int argc, const char **argv)
{
	uint32_t  fagc_lock_level_gain_increase_upper_limit;

	MAP_LIB_CALL(ad9361_get_fagc_lock_level_gain_increase_upper_limit, ad9361_legacy_dev, &fagc_lock_level_gain_increase_upper_limit);

	MAP_RESULT(uint32_t, fagc_lock_level_gain_increase_upper_limit, 1);
	return 0;
}
MAP_CMD(get_fagc_lock_level_gain_increase_upper_limit, map_get_fagc_lock_level_gain_increase_upper_limit, 1);

static int map_get_fagc_lpf_final_settling_steps (int argc, const char **argv)
{
	uint32_t  fagc_lpf_final_settling_steps;

	MAP_LIB_CALL(ad9361_get_fagc_lpf_final_settling_steps, ad9361_legacy_dev, &fagc_lpf_final_settling_steps);

	MAP_RESULT(uint32_t, fagc_lpf_final_settling_steps, 1);
	return 0;
}
MAP_CMD(get_fagc_lpf_final_settling_steps, map_get_fagc_lpf_final_settling_steps, 1);

static int map_get_fagc_lmt_final_settling_steps (int argc, const char **argv)
{
	uint32_t  fagc_lmt_final_settling_steps;

	MAP_LIB_CALL(ad9361_get_fagc_lmt_final_settling_steps, ad9361_legacy_dev, &fagc_lmt_final_settling_steps);

	MAP_RESULT(uint32_t, fagc_lmt_final_settling_steps, 1);
	return 0;
}
MAP_CMD(get_fagc_lmt_final_settling_steps, map_get_fagc_lmt_final_settling_steps, 1);

static int map_get_fagc_final_overrange_count (int argc, const char **argv)
{
	uint32_t  fagc_final_overrange_count;

	MAP_LIB_CALL(ad9361_get_fagc_final_overrange_count, ad9361_legacy_dev, &fagc_final_overrange_count);

	MAP_RESULT(uint32_t, fagc_final_overrange_count, 1);
	return 0;
}
MAP_CMD(get_fagc_final_overrange_count, map_get_fagc_final_overrange_count, 1);

static int map_get_fagc_gain_increase_after_gain_lock_enable (int argc, const char **argv)
{
	int  fagc_gain_increase_after_gain_lock_enable;

	MAP_LIB_CALL(ad9361_get_fagc_gain_increase_after_gain_lock_enable, ad9361_legacy_dev, &fagc_gain_increase_after_gain_lock_enable);

	MAP_RESULT(int, fagc_gain_increase_after_gain_lock_enable, 1);
	return 0;
}
MAP_CMD(get_fagc_gain_increase_after_gain_lock_enable, map_get_fagc_gain_increase_after_gain_lock_enable, 1);

static int map_get_fagc_gain_index_type_after_exit_rx_mode (int argc, const char **argv)
{
	uint32_t  fagc_gain_index_type_after_exit_rx_mode;

	MAP_LIB_CALL(ad9361_get_fagc_gain_index_type_after_exit_rx_mode, ad9361_legacy_dev, &fagc_gain_index_type_after_exit_rx_mode);

	MAP_RESULT(uint32_t, fagc_gain_index_type_after_exit_rx_mode, 1);
	return 0;
}
MAP_CMD(get_fagc_gain_index_type_after_exit_rx_mode, map_get_fagc_gain_index_type_after_exit_rx_mode, 1);

static int map_get_fagc_use_last_lock_level_for_set_gain_enable (int argc, const char **argv)
{
	int  fagc_use_last_lock_level_for_set_gain_enable;

	MAP_LIB_CALL(ad9361_get_fagc_use_last_lock_level_for_set_gain_enable, ad9361_legacy_dev, &fagc_use_last_lock_level_for_set_gain_enable);

	MAP_RESULT(int, fagc_use_last_lock_level_for_set_gain_enable, 1);
	return 0;
}
MAP_CMD(get_fagc_use_last_lock_level_for_set_gain_enable, map_get_fagc_use_last_lock_level_for_set_gain_enable, 1);

static int map_get_fagc_rst_gla_stronger_sig_thresh_exceeded_enable (int argc, const char **argv)
{
	int  fagc_rst_gla_stronger_sig_thresh_exceeded_enable;

	MAP_LIB_CALL(ad9361_get_fagc_rst_gla_stronger_sig_thresh_exceeded_enable, ad9361_legacy_dev, &fagc_rst_gla_stronger_sig_thresh_exceeded_enable);

	MAP_RESULT(int, fagc_rst_gla_stronger_sig_thresh_exceeded_enable, 1);
	return 0;
}
MAP_CMD(get_fagc_rst_gla_stronger_sig_thresh_exceeded_enable, map_get_fagc_rst_gla_stronger_sig_thresh_exceeded_enable, 1);

static int map_get_fagc_optimized_gain_offset (int argc, const char **argv)
{
	uint32_t  fagc_optimized_gain_offset;

	MAP_LIB_CALL(ad9361_get_fagc_optimized_gain_offset, ad9361_legacy_dev, &fagc_optimized_gain_offset);

	MAP_RESULT(uint32_t, fagc_optimized_gain_offset, 1);
	return 0;
}
MAP_CMD(get_fagc_optimized_gain_offset, map_get_fagc_optimized_gain_offset, 1);

static int map_get_fagc_rst_gla_stronger_sig_thresh_above_ll (int argc, const char **argv)
{
	uint32_t  fagc_rst_gla_stronger_sig_thresh_above_ll;

	MAP_LIB_CALL(ad9361_get_fagc_rst_gla_stronger_sig_thresh_above_ll, ad9361_legacy_dev, &fagc_rst_gla_stronger_sig_thresh_above_ll);

	MAP_RESULT(uint32_t, fagc_rst_gla_stronger_sig_thresh_above_ll, 1);
	return 0;
}
MAP_CMD(get_fagc_rst_gla_stronger_sig_thresh_above_ll, map_get_fagc_rst_gla_stronger_sig_thresh_above_ll, 1);

static int map_get_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable (int argc, const char **argv)
{
	int  fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable;

	MAP_LIB_CALL(ad9361_get_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable, ad9361_legacy_dev, &fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable);

	MAP_RESULT(int, fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable, 1);
	return 0;
}
MAP_CMD(get_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable, map_get_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable, 1);

static int map_get_fagc_rst_gla_engergy_lost_goto_optim_gain_enable (int argc, const char **argv)
{
	int  fagc_rst_gla_engergy_lost_goto_optim_gain_enable;

	MAP_LIB_CALL(ad9361_get_fagc_rst_gla_engergy_lost_goto_optim_gain_enable, ad9361_legacy_dev, &fagc_rst_gla_engergy_lost_goto_optim_gain_enable);

	MAP_RESULT(int, fagc_rst_gla_engergy_lost_goto_optim_gain_enable, 1);
	return 0;
}
MAP_CMD(get_fagc_rst_gla_engergy_lost_goto_optim_gain_enable, map_get_fagc_rst_gla_engergy_lost_goto_optim_gain_enable, 1);

static int map_get_fagc_rst_gla_engergy_lost_sig_thresh_below_ll (int argc, const char **argv)
{
	uint32_t  fagc_rst_gla_engergy_lost_sig_thresh_below_ll;

	MAP_LIB_CALL(ad9361_get_fagc_rst_gla_engergy_lost_sig_thresh_below_ll, ad9361_legacy_dev, &fagc_rst_gla_engergy_lost_sig_thresh_below_ll);

	MAP_RESULT(uint32_t, fagc_rst_gla_engergy_lost_sig_thresh_below_ll, 1);
	return 0;
}
MAP_CMD(get_fagc_rst_gla_engergy_lost_sig_thresh_below_ll, map_get_fagc_rst_gla_engergy_lost_sig_thresh_below_ll, 1);

static int map_get_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt (int argc, const char **argv)
{
	uint32_t  fagc_energy_lost_stronger_sig_gain_lock_exit_cnt;

	MAP_LIB_CALL(ad9361_get_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt, ad9361_legacy_dev, &fagc_energy_lost_stronger_sig_gain_lock_exit_cnt);

	MAP_RESULT(uint32_t, fagc_energy_lost_stronger_sig_gain_lock_exit_cnt, 1);
	return 0;
}
MAP_CMD(get_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt, map_get_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt, 1);

static int map_get_fagc_rst_gla_large_adc_overload_enable (int argc, const char **argv)
{
	int  fagc_rst_gla_large_adc_overload_enable;

	MAP_LIB_CALL(ad9361_get_fagc_rst_gla_large_adc_overload_enable, ad9361_legacy_dev, &fagc_rst_gla_large_adc_overload_enable);

	MAP_RESULT(int, fagc_rst_gla_large_adc_overload_enable, 1);
	return 0;
}
MAP_CMD(get_fagc_rst_gla_large_adc_overload_enable, map_get_fagc_rst_gla_large_adc_overload_enable, 1);

static int map_get_fagc_rst_gla_large_lmt_overload_enable (int argc, const char **argv)
{
	int  fagc_rst_gla_large_lmt_overload_enable;

	MAP_LIB_CALL(ad9361_get_fagc_rst_gla_large_lmt_overload_enable, ad9361_legacy_dev, &fagc_rst_gla_large_lmt_overload_enable);

	MAP_RESULT(int, fagc_rst_gla_large_lmt_overload_enable, 1);
	return 0;
}
MAP_CMD(get_fagc_rst_gla_large_lmt_overload_enable, map_get_fagc_rst_gla_large_lmt_overload_enable, 1);

static int map_get_fagc_rst_gla_en_agc_pulled_high_enable (int argc, const char **argv)
{
	int  fagc_rst_gla_en_agc_pulled_high_enable;

	MAP_LIB_CALL(ad9361_get_fagc_rst_gla_en_agc_pulled_high_enable, ad9361_legacy_dev, &fagc_rst_gla_en_agc_pulled_high_enable);

	MAP_RESULT(int, fagc_rst_gla_en_agc_pulled_high_enable, 1);
	return 0;
}
MAP_CMD(get_fagc_rst_gla_en_agc_pulled_high_enable, map_get_fagc_rst_gla_en_agc_pulled_high_enable, 1);

static int map_get_fagc_rst_gla_if_en_agc_pulled_high_mode (int argc, const char **argv)
{
	uint32_t  fagc_rst_gla_if_en_agc_pulled_high_mode;

	MAP_LIB_CALL(ad9361_get_fagc_rst_gla_if_en_agc_pulled_high_mode, ad9361_legacy_dev, &fagc_rst_gla_if_en_agc_pulled_high_mode);

	MAP_RESULT(uint32_t, fagc_rst_gla_if_en_agc_pulled_high_mode, 1);
	return 0;
}
MAP_CMD(get_fagc_rst_gla_if_en_agc_pulled_high_mode, map_get_fagc_rst_gla_if_en_agc_pulled_high_mode, 1);

static int map_get_fagc_power_measurement_duration_in_state5 (int argc, const char **argv)
{
	uint32_t  fagc_power_measurement_duration_in_state5;

	MAP_LIB_CALL(ad9361_get_fagc_power_measurement_duration_in_state5, ad9361_legacy_dev, &fagc_power_measurement_duration_in_state5);

	MAP_RESULT(uint32_t, fagc_power_measurement_duration_in_state5, 1);
	return 0;
}
MAP_CMD(get_fagc_power_measurement_duration_in_state5, map_get_fagc_power_measurement_duration_in_state5, 1);

static int map_get_rssi_restart_mode (int argc, const char **argv)
{
	uint32_t  rssi_restart_mode;

	MAP_LIB_CALL(ad9361_get_rssi_restart_mode, ad9361_legacy_dev, &rssi_restart_mode);

	MAP_RESULT(uint32_t, rssi_restart_mode, 1);
	return 0;
}
MAP_CMD(get_rssi_restart_mode, map_get_rssi_restart_mode, 1);

static int map_get_rssi_unit_is_rx_samples_enable (int argc, const char **argv)
{
	int  rssi_unit_is_rx_samples_enable;

	MAP_LIB_CALL(ad9361_get_rssi_unit_is_rx_samples_enable, ad9361_legacy_dev, &rssi_unit_is_rx_samples_enable);

	MAP_RESULT(int, rssi_unit_is_rx_samples_enable, 1);
	return 0;
}
MAP_CMD(get_rssi_unit_is_rx_samples_enable, map_get_rssi_unit_is_rx_samples_enable, 1);

static int map_get_rssi_delay (int argc, const char **argv)
{
	uint32_t  rssi_delay;

	MAP_LIB_CALL(ad9361_get_rssi_delay, ad9361_legacy_dev, &rssi_delay);

	MAP_RESULT(uint32_t, rssi_delay, 1);
	return 0;
}
MAP_CMD(get_rssi_delay, map_get_rssi_delay, 1);

static int map_get_rssi_wait (int argc, const char **argv)
{
	uint32_t  rssi_wait;

	MAP_LIB_CALL(ad9361_get_rssi_wait, ad9361_legacy_dev, &rssi_wait);

	MAP_RESULT(uint32_t, rssi_wait, 1);
	return 0;
}
MAP_CMD(get_rssi_wait, map_get_rssi_wait, 1);

static int map_get_rssi_duration (int argc, const char **argv)
{
	uint32_t  rssi_duration;

	MAP_LIB_CALL(ad9361_get_rssi_duration, ad9361_legacy_dev, &rssi_duration);

	MAP_RESULT(uint32_t, rssi_duration, 1);
	return 0;
}
MAP_CMD(get_rssi_duration, map_get_rssi_duration, 1);

static int map_get_ctrl_outs_index (int argc, const char **argv)
{
	uint32_t  ctrl_outs_index;

	MAP_LIB_CALL(ad9361_get_ctrl_outs_index, ad9361_legacy_dev, &ctrl_outs_index);

	MAP_RESULT(uint32_t, ctrl_outs_index, 1);
	return 0;
}
MAP_CMD(get_ctrl_outs_index, map_get_ctrl_outs_index, 1);

static int map_get_ctrl_outs_enable_mask (int argc, const char **argv)
{
	uint32_t  ctrl_outs_enable_mask;

	MAP_LIB_CALL(ad9361_get_ctrl_outs_enable_mask, ad9361_legacy_dev, &ctrl_outs_enable_mask);

	MAP_RESULT(uint32_t, ctrl_outs_enable_mask, 1);
	return 0;
}
MAP_CMD(get_ctrl_outs_enable_mask, map_get_ctrl_outs_enable_mask, 1);

static int map_get_elna_settling_delay_ns (int argc, const char **argv)
{
	uint32_t  elna_settling_delay_ns;

	MAP_LIB_CALL(ad9361_get_elna_settling_delay_ns, ad9361_legacy_dev, &elna_settling_delay_ns);

	MAP_RESULT(uint32_t, elna_settling_delay_ns, 1);
	return 0;
}
MAP_CMD(get_elna_settling_delay_ns, map_get_elna_settling_delay_ns, 1);

static int map_get_elna_gain_mdB (int argc, const char **argv)
{
	uint32_t  elna_gain_mdB;

	MAP_LIB_CALL(ad9361_get_elna_gain_mdB, ad9361_legacy_dev, &elna_gain_mdB);

	MAP_RESULT(uint32_t, elna_gain_mdB, 1);
	return 0;
}
MAP_CMD(get_elna_gain_mdB, map_get_elna_gain_mdB, 1);

static int map_get_elna_bypass_loss_mdB (int argc, const char **argv)
{
	uint32_t  elna_bypass_loss_mdB;

	MAP_LIB_CALL(ad9361_get_elna_bypass_loss_mdB, ad9361_legacy_dev, &elna_bypass_loss_mdB);

	MAP_RESULT(uint32_t, elna_bypass_loss_mdB, 1);
	return 0;
}
MAP_CMD(get_elna_bypass_loss_mdB, map_get_elna_bypass_loss_mdB, 1);

static int map_get_elna_rx1_gpo0_control_enable (int argc, const char **argv)
{
	int  elna_rx1_gpo0_control_enable;

	MAP_LIB_CALL(ad9361_get_elna_rx1_gpo0_control_enable, ad9361_legacy_dev, &elna_rx1_gpo0_control_enable);

	MAP_RESULT(int, elna_rx1_gpo0_control_enable, 1);
	return 0;
}
MAP_CMD(get_elna_rx1_gpo0_control_enable, map_get_elna_rx1_gpo0_control_enable, 1);

static int map_get_elna_rx2_gpo1_control_enable (int argc, const char **argv)
{
	int  elna_rx2_gpo1_control_enable;

	MAP_LIB_CALL(ad9361_get_elna_rx2_gpo1_control_enable, ad9361_legacy_dev, &elna_rx2_gpo1_control_enable);

	MAP_RESULT(int, elna_rx2_gpo1_control_enable, 1);
	return 0;
}
MAP_CMD(get_elna_rx2_gpo1_control_enable, map_get_elna_rx2_gpo1_control_enable, 1);

static int map_get_temp_sense_measurement_interval_ms (int argc, const char **argv)
{
	uint32_t  temp_sense_measurement_interval_ms;

	MAP_LIB_CALL(ad9361_get_temp_sense_measurement_interval_ms, ad9361_legacy_dev, &temp_sense_measurement_interval_ms);

	MAP_RESULT(uint32_t, temp_sense_measurement_interval_ms, 1);
	return 0;
}
MAP_CMD(get_temp_sense_measurement_interval_ms, map_get_temp_sense_measurement_interval_ms, 1);

static int map_get_temp_sense_offset_signed (int argc, const char **argv)
{
	int  temp_sense_offset_signed;

	MAP_LIB_CALL(ad9361_get_temp_sense_offset_signed, ad9361_legacy_dev, &temp_sense_offset_signed);

	MAP_RESULT(int, temp_sense_offset_signed, 1);
	return 0;
}
MAP_CMD(get_temp_sense_offset_signed, map_get_temp_sense_offset_signed, 1);

static int map_get_temp_sense_periodic_measurement_enable (int argc, const char **argv)
{
	int  temp_sense_periodic_measurement_enable;

	MAP_LIB_CALL(ad9361_get_temp_sense_periodic_measurement_enable, ad9361_legacy_dev, &temp_sense_periodic_measurement_enable);

	MAP_RESULT(int, temp_sense_periodic_measurement_enable, 1);
	return 0;
}
MAP_CMD(get_temp_sense_periodic_measurement_enable, map_get_temp_sense_periodic_measurement_enable, 1);

static int map_get_temp_sense_decimation (int argc, const char **argv)
{
	uint32_t  temp_sense_decimation;

	MAP_LIB_CALL(ad9361_get_temp_sense_decimation, ad9361_legacy_dev, &temp_sense_decimation);

	MAP_RESULT(uint32_t, temp_sense_decimation, 1);
	return 0;
}
MAP_CMD(get_temp_sense_decimation, map_get_temp_sense_decimation, 1);

static int map_get_aux_adc_rate (int argc, const char **argv)
{
	uint32_t  aux_adc_rate;

	MAP_LIB_CALL(ad9361_get_aux_adc_rate, ad9361_legacy_dev, &aux_adc_rate);

	MAP_RESULT(uint32_t, aux_adc_rate, 1);
	return 0;
}
MAP_CMD(get_aux_adc_rate, map_get_aux_adc_rate, 1);

static int map_get_aux_adc_decimation (int argc, const char **argv)
{
	uint32_t  aux_adc_decimation;

	MAP_LIB_CALL(ad9361_get_aux_adc_decimation, ad9361_legacy_dev, &aux_adc_decimation);

	MAP_RESULT(uint32_t, aux_adc_decimation, 1);
	return 0;
}
MAP_CMD(get_aux_adc_decimation, map_get_aux_adc_decimation, 1);

static int map_get_aux_dac_manual_mode_enable (int argc, const char **argv)
{
	int  aux_dac_manual_mode_enable;

	MAP_LIB_CALL(ad9361_get_aux_dac_manual_mode_enable, ad9361_legacy_dev, &aux_dac_manual_mode_enable);

	MAP_RESULT(int, aux_dac_manual_mode_enable, 1);
	return 0;
}
MAP_CMD(get_aux_dac_manual_mode_enable, map_get_aux_dac_manual_mode_enable, 1);

static int map_get_aux_dac1_default_value_mV (int argc, const char **argv)
{
	uint32_t  aux_dac1_default_value_mV;

	MAP_LIB_CALL(ad9361_get_aux_dac1_default_value_mV, ad9361_legacy_dev, &aux_dac1_default_value_mV);

	MAP_RESULT(uint32_t, aux_dac1_default_value_mV, 1);
	return 0;
}
MAP_CMD(get_aux_dac1_default_value_mV, map_get_aux_dac1_default_value_mV, 1);

static int map_get_aux_dac1_active_in_rx_enable (int argc, const char **argv)
{
	int  aux_dac1_active_in_rx_enable;

	MAP_LIB_CALL(ad9361_get_aux_dac1_active_in_rx_enable, ad9361_legacy_dev, &aux_dac1_active_in_rx_enable);

	MAP_RESULT(int, aux_dac1_active_in_rx_enable, 1);
	return 0;
}
MAP_CMD(get_aux_dac1_active_in_rx_enable, map_get_aux_dac1_active_in_rx_enable, 1);

static int map_get_aux_dac1_active_in_tx_enable (int argc, const char **argv)
{
	int  aux_dac1_active_in_tx_enable;

	MAP_LIB_CALL(ad9361_get_aux_dac1_active_in_tx_enable, ad9361_legacy_dev, &aux_dac1_active_in_tx_enable);

	MAP_RESULT(int, aux_dac1_active_in_tx_enable, 1);
	return 0;
}
MAP_CMD(get_aux_dac1_active_in_tx_enable, map_get_aux_dac1_active_in_tx_enable, 1);

static int map_get_aux_dac1_active_in_alert_enable (int argc, const char **argv)
{
	int  aux_dac1_active_in_alert_enable;

	MAP_LIB_CALL(ad9361_get_aux_dac1_active_in_alert_enable, ad9361_legacy_dev, &aux_dac1_active_in_alert_enable);

	MAP_RESULT(int, aux_dac1_active_in_alert_enable, 1);
	return 0;
}
MAP_CMD(get_aux_dac1_active_in_alert_enable, map_get_aux_dac1_active_in_alert_enable, 1);

static int map_get_aux_dac1_rx_delay_us (int argc, const char **argv)
{
	uint32_t  aux_dac1_rx_delay_us;

	MAP_LIB_CALL(ad9361_get_aux_dac1_rx_delay_us, ad9361_legacy_dev, &aux_dac1_rx_delay_us);

	MAP_RESULT(uint32_t, aux_dac1_rx_delay_us, 1);
	return 0;
}
MAP_CMD(get_aux_dac1_rx_delay_us, map_get_aux_dac1_rx_delay_us, 1);

static int map_get_aux_dac1_tx_delay_us (int argc, const char **argv)
{
	uint32_t  aux_dac1_tx_delay_us;

	MAP_LIB_CALL(ad9361_get_aux_dac1_tx_delay_us, ad9361_legacy_dev, &aux_dac1_tx_delay_us);

	MAP_RESULT(uint32_t, aux_dac1_tx_delay_us, 1);
	return 0;
}
MAP_CMD(get_aux_dac1_tx_delay_us, map_get_aux_dac1_tx_delay_us, 1);

static int map_get_aux_dac2_default_value_mV (int argc, const char **argv)
{
	uint32_t  aux_dac2_default_value_mV;

	MAP_LIB_CALL(ad9361_get_aux_dac2_default_value_mV, ad9361_legacy_dev, &aux_dac2_default_value_mV);

	MAP_RESULT(uint32_t, aux_dac2_default_value_mV, 1);
	return 0;
}
MAP_CMD(get_aux_dac2_default_value_mV, map_get_aux_dac2_default_value_mV, 1);

static int map_get_aux_dac2_active_in_rx_enable (int argc, const char **argv)
{
	int  aux_dac2_active_in_rx_enable;

	MAP_LIB_CALL(ad9361_get_aux_dac2_active_in_rx_enable, ad9361_legacy_dev, &aux_dac2_active_in_rx_enable);

	MAP_RESULT(int, aux_dac2_active_in_rx_enable, 1);
	return 0;
}
MAP_CMD(get_aux_dac2_active_in_rx_enable, map_get_aux_dac2_active_in_rx_enable, 1);

static int map_get_aux_dac2_active_in_tx_enable (int argc, const char **argv)
{
	int  aux_dac2_active_in_tx_enable;

	MAP_LIB_CALL(ad9361_get_aux_dac2_active_in_tx_enable, ad9361_legacy_dev, &aux_dac2_active_in_tx_enable);

	MAP_RESULT(int, aux_dac2_active_in_tx_enable, 1);
	return 0;
}
MAP_CMD(get_aux_dac2_active_in_tx_enable, map_get_aux_dac2_active_in_tx_enable, 1);

static int map_get_aux_dac2_active_in_alert_enable (int argc, const char **argv)
{
	int  aux_dac2_active_in_alert_enable;

	MAP_LIB_CALL(ad9361_get_aux_dac2_active_in_alert_enable, ad9361_legacy_dev, &aux_dac2_active_in_alert_enable);

	MAP_RESULT(int, aux_dac2_active_in_alert_enable, 1);
	return 0;
}
MAP_CMD(get_aux_dac2_active_in_alert_enable, map_get_aux_dac2_active_in_alert_enable, 1);

static int map_get_aux_dac2_rx_delay_us (int argc, const char **argv)
{
	uint32_t  aux_dac2_rx_delay_us;

	MAP_LIB_CALL(ad9361_get_aux_dac2_rx_delay_us, ad9361_legacy_dev, &aux_dac2_rx_delay_us);

	MAP_RESULT(uint32_t, aux_dac2_rx_delay_us, 1);
	return 0;
}
MAP_CMD(get_aux_dac2_rx_delay_us, map_get_aux_dac2_rx_delay_us, 1);

static int map_get_aux_dac2_tx_delay_us (int argc, const char **argv)
{
	uint32_t  aux_dac2_tx_delay_us;

	MAP_LIB_CALL(ad9361_get_aux_dac2_tx_delay_us, ad9361_legacy_dev, &aux_dac2_tx_delay_us);

	MAP_RESULT(uint32_t, aux_dac2_tx_delay_us, 1);
	return 0;
}
MAP_CMD(get_aux_dac2_tx_delay_us, map_get_aux_dac2_tx_delay_us, 1);

