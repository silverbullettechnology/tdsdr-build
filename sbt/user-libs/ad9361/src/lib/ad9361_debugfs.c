/** \file      src/lib/ad9361_debugfs.c
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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>


static int sysfs_get_int (unsigned dev, const char *leaf, int *val)
{
	errno = ENOSYS;
	return -1;
}

static int sysfs_set_int (unsigned dev, const char *leaf, int val)
{
	errno = ENOSYS;
	return -1;
}

static int sysfs_get_u32 (unsigned dev, const char *leaf, uint32_t *val)
{
	errno = ENOSYS;
	return -1;
}

static int sysfs_set_u32 (unsigned dev, const char *leaf, uint32_t val)
{
	errno = ENOSYS;
	return -1;
}


/* Path: bist_prbs
 * Var : (none)
 * mode: 0:BIST_DISABLE, 1:BIST_INJ_TX, 2:BIST_INJ_RX
 */
int ad9361_get_bist_prbs (unsigned dev, int *mode)
{
	return sysfs_get_int(dev, "bist_prbs", mode);
}
int ad9361_set_bist_prbs (unsigned dev, int  mode)
{
	return sysfs_set_int(dev, "bist_prbs", mode);
}

/* Path: bist_tone
 * Var : (none)
 */
int ad9361_get_bist_tone (unsigned dev, int *mode)
{
	return sysfs_get_int(dev, "bist_tone", mode);
}
int ad9361_set_bist_tone (unsigned dev, int  mode, uint32_t freq_Hz,
                          uint32_t level_dB, uint32_t mask)
{
	errno = ENOSYS;
	return -1;
}

/* Path: gaininfo_rx1 / gaininfo_rx2
 * Var : (none)
 */
int ad9361_get_gaininfo_rx (unsigned dev, int channel,
                            int32_t  *gain_db,      uint32_t *fgt_lmt_index,
                            uint32_t *digital_gain, uint32_t *lmt_gain,
                            uint32_t *lpf_gain,     uint32_t *lna_index,
                            uint32_t *tia_index,    uint32_t *mixer_index)
{
	int32_t   s32;
	uint32_t  u32;

	if ( !gain_db        ) gain_db       = &s32;
	if ( !fgt_lmt_index  ) fgt_lmt_index = &u32;
	if ( !digital_gain   ) digital_gain  = &u32;
	if ( !lmt_gain       ) lmt_gain      = &u32;
	if ( !lpf_gain       ) lpf_gain      = &u32;
	if ( !lna_index      ) lna_index     = &u32;
	if ( !tia_index      ) tia_index     = &u32;
	if ( !mixer_index    ) mixer_index   = &u32;

	errno = ENOSYS;
	return -1;
}
int ad9361_get_gaininfo_rx1 (unsigned dev,
                             int32_t  *gain_db,      uint32_t *fgt_lmt_index,
                             uint32_t *digital_gain, uint32_t *lmt_gain,
                             uint32_t *lpf_gain,     uint32_t *lna_index,
                             uint32_t *tia_index,    uint32_t *mixer_index)
{
	return ad9361_get_gaininfo_rx(dev, 1,
	                              gain_db, fgt_lmt_index, digital_gain, lmt_gain,
	                              lpf_gain, lna_index, tia_index, mixer_index);
}
int ad9361_get_gaininfo_rx2 (unsigned dev,
                             int32_t  *gain_db,      uint32_t *fgt_lmt_index,
                             uint32_t *digital_gain, uint32_t *lmt_gain,
                             uint32_t *lpf_gain,     uint32_t *lna_index,
                             uint32_t *tia_index,    uint32_t *mixer_index)
{
	return ad9361_get_gaininfo_rx(dev, 2,
	                              gain_db, fgt_lmt_index, digital_gain, lmt_gain,
	                              lpf_gain, lna_index, tia_index, mixer_index);
}


/* Path: initialize
 * Var : (none)
 */
int ad9361_set_initialize (unsigned dev)
{
	return sysfs_set_int(dev, "initialize", 1);
}

/* Path: loopback
 * Var : (none)
 * mode: 0:disable
 *       1:loopback (AD9361 internal) TX->RX
 *       2:loopback (FPGA internal) RX->TX
 */
int ad9361_get_loopback (unsigned dev, int *mode)
{
	return sysfs_get_int(dev, "loopback", mode);
}
int ad9361_set_loopback (unsigned dev, int  mode)
{
	return sysfs_set_int(dev, "loopback", mode);
}

/* Path: bist_timing_analysis
 * Func: ad9361_dig_interface_timing_analysis()
 */
int ad9361_get_bist_timing_analysis (unsigned dev, char *dst, size_t max)
{
	errno = ENOSYS;
	return -1;
}


/* Path: adi,frequency-division-duplex-mode-enable
 * Var : fdd
 */
int ad9361_get_frequency_division_duplex_mode_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,frequency-division-duplex-mode-enable", val);
}
int ad9361_set_frequency_division_duplex_mode_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,frequency-division-duplex-mode-enable", val);
}

/* Path: adi,ensm-enable-pin-pulse-mode-enable
 * Var : ensm_pin_pulse_mode
 */
int ad9361_get_ensm_enable_pin_pulse_mode_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,ensm-enable-pin-pulse-mode-enable", val);
}
int ad9361_set_ensm_enable_pin_pulse_mode_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,ensm-enable-pin-pulse-mode-enable", val);
}

/* Path: adi,ensm-enable-txnrx-control-enable
 * Var : ensm_pin_ctrl
 */
int ad9361_get_ensm_enable_txnrx_control_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,ensm-enable-txnrx-control-enable", val);
}
int ad9361_set_ensm_enable_txnrx_control_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,ensm-enable-txnrx-control-enable", val);
}

/* Path: adi,debug-mode-enable
 * Var : debug_mode
 */
int ad9361_get_debug_mode_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,debug-mode-enable", val);
}
int ad9361_set_debug_mode_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,debug-mode-enable", val);
}

/* Path: adi,tdd-use-fdd-vco-tables-enable
 * Var : tdd_use_fdd_tables
 */
int ad9361_get_tdd_use_fdd_vco_tables_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,tdd-use-fdd-vco-tables-enable", val);
}
int ad9361_set_tdd_use_fdd_vco_tables_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,tdd-use-fdd-vco-tables-enable", val);
}

/* Path: adi,tdd-use-dual-synth-mode-enable
 * Var : tdd_use_dual_synth
 */
int ad9361_get_tdd_use_dual_synth_mode_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,tdd-use-dual-synth-mode-enable", val);
}
int ad9361_set_tdd_use_dual_synth_mode_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,tdd-use-dual-synth-mode-enable", val);
}

/* Path: adi,tdd-skip-vco-cal-enable
 * Var : tdd_skip_vco_cal
 */
int ad9361_get_tdd_skip_vco_cal_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,tdd-skip-vco-cal-enable", val);
}
int ad9361_set_tdd_skip_vco_cal_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,tdd-skip-vco-cal-enable", val);
}

/* Path: adi,2rx-2tx-mode-enable
 * Var : rx2tx2
 */
int ad9361_get_2rx_2tx_mode_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,2rx-2tx-mode-enable", val);
}
int ad9361_set_2rx_2tx_mode_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,2rx-2tx-mode-enable", val);
}

/* Path: adi,split-gain-table-mode-enable
 * Var : split_gt
 */
int ad9361_get_split_gain_table_mode_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,split-gain-table-mode-enable", val);
}
int ad9361_set_split_gain_table_mode_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,split-gain-table-mode-enable", val);
}

/* Path: adi,rx-rf-port-input-select
 * Var : rf_rx_input_sel
 */
int ad9361_get_rx_rf_port_input_select (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,rx-rf-port-input-select", val);
}
int ad9361_set_rx_rf_port_input_select (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,rx-rf-port-input-select", val);
}

/* Path: adi,tx-rf-port-input-select
 * Var : rf_tx_output_sel
 */
int ad9361_get_tx_rf_port_input_select (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,tx-rf-port-input-select", val);
}
int ad9361_set_tx_rf_port_input_select (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,tx-rf-port-input-select", val);
}

/* Path: adi,external-tx-lo-enable
 * Var : use_ext_tx_lo
 */
int ad9361_get_external_tx_lo_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,external-tx-lo-enable", val);
}
int ad9361_set_external_tx_lo_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,external-tx-lo-enable", val);
}

/* Path: adi,external-rx-lo-enable
 * Var : use_ext_rx_lo
 */
int ad9361_get_external_rx_lo_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,external-rx-lo-enable", val);
}
int ad9361_set_external_rx_lo_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,external-rx-lo-enable", val);
}

/* Path: adi,xo-disable-use-ext-refclk-enable
 * Var : use_extclk
 */
int ad9361_get_xo_disable_use_ext_refclk_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,xo-disable-use-ext-refclk-enable", val);
}
int ad9361_set_xo_disable_use_ext_refclk_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,xo-disable-use-ext-refclk-enable", val);
}

/* Path: adi,clk-output-mode-select
 * Var : ad9361_clkout_mode
 */
int ad9361_get_clk_output_mode_select (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,clk-output-mode-select", val);
}
int ad9361_set_clk_output_mode_select (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,clk-output-mode-select", val);
}

/* Path: adi,dc-offset-tracking-update-event-mask
 * Var : dc_offset_update_events
 */
int ad9361_get_dc_offset_tracking_update_event_mask (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,dc-offset-tracking-update-event-mask", val);
}
int ad9361_set_dc_offset_tracking_update_event_mask (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,dc-offset-tracking-update-event-mask", val);
}

/* Path: adi,dc-offset-attenuation-high-range
 * Var : dc_offset_attenuation_high
 */
int ad9361_get_dc_offset_attenuation_high_range (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,dc-offset-attenuation-high-range", val);
}
int ad9361_set_dc_offset_attenuation_high_range (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,dc-offset-attenuation-high-range", val);
}

/* Path: adi,dc-offset-attenuation-low-range
 * Var : dc_offset_attenuation_low
 */
int ad9361_get_dc_offset_attenuation_low_range (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,dc-offset-attenuation-low-range", val);
}
int ad9361_set_dc_offset_attenuation_low_range (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,dc-offset-attenuation-low-range", val);
}

/* Path: adi,dc-offset-count-high-range
 * Var : rf_dc_offset_count_high
 */
int ad9361_get_dc_offset_count_high_range (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,dc-offset-count-high-range", val);
}
int ad9361_set_dc_offset_count_high_range (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,dc-offset-count-high-range", val);
}

/* Path: adi,dc-offset-count-low-range
 * Var : rf_dc_offset_count_low
 */
int ad9361_get_dc_offset_count_low_range (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,dc-offset-count-low-range", val);
}
int ad9361_set_dc_offset_count_low_range (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,dc-offset-count-low-range", val);
}

/* Path: adi,rf-rx-bandwidth-hz
 * Var : rf_rx_bandwidth_Hz
 */
int ad9361_get_rf_rx_bandwidth_hz (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,rf-rx-bandwidth-hz", val);
}
int ad9361_set_rf_rx_bandwidth_hz (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,rf-rx-bandwidth-hz", val);
}

/* Path: adi,rf-tx-bandwidth-hz
 * Var : rf_tx_bandwidth_Hz
 */
int ad9361_get_rf_tx_bandwidth_hz (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,rf-tx-bandwidth-hz", val);
}
int ad9361_set_rf_tx_bandwidth_hz (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,rf-tx-bandwidth-hz", val);
}

/* Path: adi,tx-attenuation-mdB
 * Var : tx_atten
 */
int ad9361_get_tx_attenuation_mdB (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,tx-attenuation-mdB", val);
}
int ad9361_set_tx_attenuation_mdB (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,tx-attenuation-mdB", val);
}

/* Path: adi,update-tx-gain-in-alert-enable
 * Var : update_tx_gain_via_alert
 */
int ad9361_get_update_tx_gain_in_alert_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,update-tx-gain-in-alert-enable", val);
}
int ad9361_set_update_tx_gain_in_alert_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,update-tx-gain-in-alert-enable", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Common Gain Control */
                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,gc-rx1-mode
 * Var : gain_ctrl.rx1_mode
 */
int ad9361_get_gc_rx1_mode (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,gc-rx1-mode", val);
}
int ad9361_set_gc_rx1_mode (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,gc-rx1-mode", val);
}

/* Path: adi,gc-rx2-mode
 * Var : gain_ctrl.rx2_mode
 */
int ad9361_get_gc_rx2_mode (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,gc-rx2-mode", val);
}
int ad9361_set_gc_rx2_mode (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,gc-rx2-mode", val);
}

/* Path: adi,gc-adc-ovr-sample-size
 * Var : gain_ctrl.adc_ovr_sample_size
 */
int ad9361_get_gc_adc_ovr_sample_size (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,gc-adc-ovr-sample-size", val);
}
int ad9361_set_gc_adc_ovr_sample_size (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,gc-adc-ovr-sample-size", val);
}

/* Path: adi,gc-adc-small-overload-thresh
 * Var : gain_ctrl.adc_small_overload_thresh
 */
int ad9361_get_gc_adc_small_overload_thresh (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,gc-adc-small-overload-thresh", val);
}
int ad9361_set_gc_adc_small_overload_thresh (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,gc-adc-small-overload-thresh", val);
}

/* Path: adi,gc-adc-large-overload-thresh
 * Var : gain_ctrl.adc_large_overload_thresh
 */
int ad9361_get_gc_adc_large_overload_thresh (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,gc-adc-large-overload-thresh", val);
}
int ad9361_set_gc_adc_large_overload_thresh (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,gc-adc-large-overload-thresh", val);
}

/* Path: adi,gc-lmt-overload-high-thresh
 * Var : gain_ctrl.lmt_overload_high_thresh
 */
int ad9361_get_gc_lmt_overload_high_thresh (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,gc-lmt-overload-high-thresh", val);
}
int ad9361_set_gc_lmt_overload_high_thresh (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,gc-lmt-overload-high-thresh", val);
}

/* Path: adi,gc-lmt-overload-low-thresh
 * Var : gain_ctrl.lmt_overload_low_thresh
 */
int ad9361_get_gc_lmt_overload_low_thresh (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,gc-lmt-overload-low-thresh", val);
}
int ad9361_set_gc_lmt_overload_low_thresh (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,gc-lmt-overload-low-thresh", val);
}

/* Path: adi,gc-dec-pow-measurement-duration
 * Var : gain_ctrl.dec_pow_measuremnt_duration
 */
int ad9361_get_gc_dec_pow_measurement_duration (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,gc-dec-pow-measurement-duration", val);
}
int ad9361_set_gc_dec_pow_measurement_duration (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,gc-dec-pow-measurement-duration", val);
}

/* Path: adi,gc-low-power-thresh
 * Var : gain_ctrl.low_power_thresh
 */
int ad9361_get_gc_low_power_thresh (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,gc-low-power-thresh", val);
}
int ad9361_set_gc_low_power_thresh (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,gc-low-power-thresh", val);
}

/* Path: adi,gc-dig-gain-enable
 * Var : gain_ctrl.dig_gain_en
 */
int ad9361_get_gc_dig_gain_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,gc-dig-gain-enable", val);
}
int ad9361_set_gc_dig_gain_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,gc-dig-gain-enable", val);
}

/* Path: adi,gc-max-dig-gain
 * Var : gain_ctrl.max_dig_gain
 */
int ad9361_get_gc_max_dig_gain (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,gc-max-dig-gain", val);
}
int ad9361_set_gc_max_dig_gain (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,gc-max-dig-gain", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Manual Gain Control */
                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,mgc-rx1-ctrl-inp-enable
 * Var : gain_ctrl.mgc_rx1_ctrl_inp_en
 */
int ad9361_get_mgc_rx1_ctrl_inp_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,mgc-rx1-ctrl-inp-enable", val);
}
int ad9361_set_mgc_rx1_ctrl_inp_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,mgc-rx1-ctrl-inp-enable", val);
}

/* Path: adi,mgc-rx2-ctrl-inp-enable
 * Var : gain_ctrl.mgc_rx1_ctrl_inp_en
 */
int ad9361_get_mgc_rx2_ctrl_inp_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,mgc-rx2-ctrl-inp-enable", val);
}
int ad9361_set_mgc_rx2_ctrl_inp_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,mgc-rx2-ctrl-inp-enable", val);
}

/* Path: adi,mgc-inc-gain-step
 * Var : gain_ctrl.mgc_inc_gain_step
 */
int ad9361_get_mgc_inc_gain_step (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,mgc-inc-gain-step", val);
}
int ad9361_set_mgc_inc_gain_step (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,mgc-inc-gain-step", val);
}

/* Path: adi,mgc-dec-gain-step
 * Var : gain_ctrl.mgc_dec_gain_step
 */
int ad9361_get_mgc_dec_gain_step (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,mgc-dec-gain-step", val);
}
int ad9361_set_mgc_dec_gain_step (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,mgc-dec-gain-step", val);
}

/* Path: adi,mgc-split-table-ctrl-inp-gain-mode
 * Var : gain_ctrl.mgc_split_table_ctrl_inp_gain_mode
 */
int ad9361_get_mgc_split_table_ctrl_inp_gain_mode (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,mgc-split-table-ctrl-inp-gain-mode", val);
}
int ad9361_set_mgc_split_table_ctrl_inp_gain_mode (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,mgc-split-table-ctrl-inp-gain-mode", val);
}

/* Path: adi,agc-attack-delay-extra-margin-us
 * Var : gain_ctrl.agc_attack_delay_extra_margin_us
 */
int ad9361_get_agc_attack_delay_extra_margin_us (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-attack-delay-extra-margin-us", val);
}
int ad9361_set_agc_attack_delay_extra_margin_us (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-attack-delay-extra-margin-us", val);
}

/* Path: adi,agc-outer-thresh-high
 * Var : gain_ctrl.agc_outer_thresh_high
 */
int ad9361_get_agc_outer_thresh_high (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-outer-thresh-high", val);
}
int ad9361_set_agc_outer_thresh_high (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-outer-thresh-high", val);
}

/* Path: adi,agc-outer-thresh-high-dec-steps
 * Var : gain_ctrl.agc_outer_thresh_high_dec_steps
 */
int ad9361_get_agc_outer_thresh_high_dec_steps (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-outer-thresh-high-dec-steps", val);
}
int ad9361_set_agc_outer_thresh_high_dec_steps (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-outer-thresh-high-dec-steps", val);
}

/* Path: adi,agc-inner-thresh-high
 * Var : gain_ctrl.agc_inner_thresh_high
 */
int ad9361_get_agc_inner_thresh_high (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-inner-thresh-high", val);
}
int ad9361_set_agc_inner_thresh_high (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-inner-thresh-high", val);
}

/* Path: adi,agc-inner-thresh-high-dec-steps
 * Var : gain_ctrl.agc_inner_thresh_high_dec_steps
 */
int ad9361_get_agc_inner_thresh_high_dec_steps (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-inner-thresh-high-dec-steps", val);
}
int ad9361_set_agc_inner_thresh_high_dec_steps (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-inner-thresh-high-dec-steps", val);
}

/* Path: adi,agc-inner-thresh-low
 * Var : gain_ctrl.agc_inner_thresh_low
 */
int ad9361_get_agc_inner_thresh_low (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-inner-thresh-low", val);
}
int ad9361_set_agc_inner_thresh_low (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-inner-thresh-low", val);
}

/* Path: adi,agc-inner-thresh-low-inc-steps
 * Var : gain_ctrl.agc_inner_thresh_low_inc_steps
 */
int ad9361_get_agc_inner_thresh_low_inc_steps (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-inner-thresh-low-inc-steps", val);
}
int ad9361_set_agc_inner_thresh_low_inc_steps (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-inner-thresh-low-inc-steps", val);
}

/* Path: adi,agc-outer-thresh-low
 * Var : gain_ctrl.agc_outer_thresh_low
 */
int ad9361_get_agc_outer_thresh_low (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-outer-thresh-low", val);
}
int ad9361_set_agc_outer_thresh_low (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-outer-thresh-low", val);
}

/* Path: adi,agc-outer-thresh-low-inc-steps
 * Var : gain_ctrl.agc_outer_thresh_low_inc_steps
 */
int ad9361_get_agc_outer_thresh_low_inc_steps (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-outer-thresh-low-inc-steps", val);
}
int ad9361_set_agc_outer_thresh_low_inc_steps (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-outer-thresh-low-inc-steps", val);
}

/* Path: adi,agc-adc-small-overload-exceed-counter
 * Var : gain_ctrl.adc_small_overload_exceed_counter
 */
int ad9361_get_agc_adc_small_overload_exceed_counter (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-adc-small-overload-exceed-counter", val);
}
int ad9361_set_agc_adc_small_overload_exceed_counter (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-adc-small-overload-exceed-counter", val);
}

/* Path: adi,agc-adc-large-overload-exceed-counter
 * Var : gain_ctrl.adc_large_overload_exceed_counter
 */
int ad9361_get_agc_adc_large_overload_exceed_counter (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-adc-large-overload-exceed-counter", val);
}
int ad9361_set_agc_adc_large_overload_exceed_counter (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-adc-large-overload-exceed-counter", val);
}

/* Path: adi,agc-adc-large-overload-inc-steps
 * Var : gain_ctrl.adc_large_overload_inc_steps
 */
int ad9361_get_agc_adc_large_overload_inc_steps (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-adc-large-overload-inc-steps", val);
}
int ad9361_set_agc_adc_large_overload_inc_steps (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-adc-large-overload-inc-steps", val);
}

/* Path: adi,agc-adc-lmt-small-overload-prevent-gain-inc-enable
 * Var : gain_ctrl.adc_lmt_small_overload_prevent_gain_inc
 */
int ad9361_get_agc_adc_lmt_small_overload_prevent_gain_inc_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,agc-adc-lmt-small-overload-prevent-gain-inc-enable", val);
}
int ad9361_set_agc_adc_lmt_small_overload_prevent_gain_inc_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,agc-adc-lmt-small-overload-prevent-gain-inc-enable", val);
}

/* Path: adi,agc-lmt-overload-large-exceed-counter
 * Var : gain_ctrl.lmt_overload_large_exceed_counter
 */
int ad9361_get_agc_lmt_overload_large_exceed_counter (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-lmt-overload-large-exceed-counter", val);
}
int ad9361_set_agc_lmt_overload_large_exceed_counter (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-lmt-overload-large-exceed-counter", val);
}

/* Path: adi,agc-lmt-overload-small-exceed-counter
 * Var : gain_ctrl.lmt_overload_small_exceed_counter
 */
int ad9361_get_agc_lmt_overload_small_exceed_counter (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-lmt-overload-small-exceed-counter", val);
}
int ad9361_set_agc_lmt_overload_small_exceed_counter (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-lmt-overload-small-exceed-counter", val);
}

/* Path: adi,agc-lmt-overload-large-inc-steps
 * Var : gain_ctrl.lmt_overload_large_inc_steps
 */
int ad9361_get_agc_lmt_overload_large_inc_steps (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-lmt-overload-large-inc-steps", val);
}
int ad9361_set_agc_lmt_overload_large_inc_steps (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-lmt-overload-large-inc-steps", val);
}

/* Path: adi,agc-dig-saturation-exceed-counter
 * Var : gain_ctrl.dig_saturation_exceed_counter
 */
int ad9361_get_agc_dig_saturation_exceed_counter (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-dig-saturation-exceed-counter", val);
}
int ad9361_set_agc_dig_saturation_exceed_counter (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-dig-saturation-exceed-counter", val);
}

/* Path: adi,agc-dig-gain-step-size
 * Var : gain_ctrl.dig_gain_step_size
 */
int ad9361_get_agc_dig_gain_step_size (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-dig-gain-step-size", val);
}
int ad9361_set_agc_dig_gain_step_size (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-dig-gain-step-size", val);
}

/* Path: adi,agc-sync-for-gain-counter-enable
 * Var : gain_ctrl.sync_for_gain_counter_en
 */
int ad9361_get_agc_sync_for_gain_counter_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,agc-sync-for-gain-counter-enable", val);
}
int ad9361_set_agc_sync_for_gain_counter_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,agc-sync-for-gain-counter-enable", val);
}

/* Path: adi,agc-gain-update-interval-us
 * Var : gain_ctrl.gain_update_interval_us
 */
int ad9361_get_agc_gain_update_interval_us (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,agc-gain-update-interval-us", val);
}
int ad9361_set_agc_gain_update_interval_us (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,agc-gain-update-interval-us", val);
}

/* Path: adi,agc-immed-gain-change-if-large-adc-overload-enable
 * Var : gain_ctrl.immed_gain_change_if_large_adc_overload
 */
int ad9361_get_agc_immed_gain_change_if_large_adc_overload_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,agc-immed-gain-change-if-large-adc-overload-enable", val);
}
int ad9361_set_agc_immed_gain_change_if_large_adc_overload_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,agc-immed-gain-change-if-large-adc-overload-enable", val);
}

/* Path: adi,agc-immed-gain-change-if-large-lmt-overload-enable
 * Var : gain_ctrl.immed_gain_change_if_large_lmt_overload
 */
int ad9361_get_agc_immed_gain_change_if_large_lmt_overload_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,agc-immed-gain-change-if-large-lmt-overload-enable", val);
}
int ad9361_set_agc_immed_gain_change_if_large_lmt_overload_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,agc-immed-gain-change-if-large-lmt-overload-enable", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Fast AGC */
/* Path: adi,fagc-dec-pow-measurement-duration
 * Var : gain_ctrl.f_agc_dec_pow_measuremnt_duration
 */
int ad9361_get_fagc_dec_pow_measurement_duration (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-dec-pow-measurement-duration", val);
}
int ad9361_set_fagc_dec_pow_measurement_duration (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-dec-pow-measurement-duration", val);
}

/* Path: adi,fagc-state-wait-time-ns
 * Var : gain_ctrl.f_agc_state_wait_time_ns
 */
int ad9361_get_fagc_state_wait_time_ns (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-state-wait-time-ns", val);
}
int ad9361_set_fagc_state_wait_time_ns (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-state-wait-time-ns", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Fast AGC - Low Power */
/* Path: adi,fagc-allow-agc-gain-increase-enable
 * Var : gain_ctrl.f_agc_allow_agc_gain_increase
 */
int ad9361_get_fagc_allow_agc_gain_increase_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,fagc-allow-agc-gain-increase-enable", val);
}
int ad9361_set_fagc_allow_agc_gain_increase_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,fagc-allow-agc-gain-increase-enable", val);
}

/* Path: adi,fagc-lp-thresh-increment-time
 * Var : gain_ctrl.f_agc_lp_thresh_increment_time
 */
int ad9361_get_fagc_lp_thresh_increment_time (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-lp-thresh-increment-time", val);
}
int ad9361_set_fagc_lp_thresh_increment_time (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-lp-thresh-increment-time", val);
}

/* Path: adi,fagc-lp-thresh-increment-steps
 * Var : gain_ctrl.f_agc_lp_thresh_increment_steps
 */
int ad9361_get_fagc_lp_thresh_increment_steps (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-lp-thresh-increment-steps", val);
}
int ad9361_set_fagc_lp_thresh_increment_steps (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-lp-thresh-increment-steps", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Fast AGC - Lock Level */

/* Path: adi,fagc-lock-level
 * Var : gain_ctrl.f_agc_lock_level
 */
int ad9361_get_fagc_lock_level (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-lock-level", val);
}
int ad9361_set_fagc_lock_level (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-lock-level", val);
}

/* Path: adi,fagc-lock-level-lmt-gain-increase-enable
 * Var : gain_ctrl.f_agc_lock_level_lmt_gain_increase_en
 */
int ad9361_get_fagc_lock_level_lmt_gain_increase_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,fagc-lock-level-lmt-gain-increase-enable", val);
}
int ad9361_set_fagc_lock_level_lmt_gain_increase_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,fagc-lock-level-lmt-gain-increase-enable", val);
}

/* Path: adi,fagc-lock-level-gain-increase-upper-limit
 * Var : gain_ctrl.f_agc_lock_level_gain_increase_upper_limit
 */
int ad9361_get_fagc_lock_level_gain_increase_upper_limit (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-lock-level-gain-increase-upper-limit", val);
}
int ad9361_set_fagc_lock_level_gain_increase_upper_limit (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-lock-level-gain-increase-upper-limit", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Fast AGC - Peak Detectors and Final Settling */

/* Path: adi,fagc-lpf-final-settling-steps
 * Var : gain_ctrl.f_agc_lpf_final_settling_steps
 */
int ad9361_get_fagc_lpf_final_settling_steps (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-lpf-final-settling-steps", val);
}
int ad9361_set_fagc_lpf_final_settling_steps (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-lpf-final-settling-steps", val);
}

/* Path: adi,fagc-lmt-final-settling-steps
 * Var : gain_ctrl.f_agc_lmt_final_settling_steps
 */
int ad9361_get_fagc_lmt_final_settling_steps (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-lmt-final-settling-steps", val);
}
int ad9361_set_fagc_lmt_final_settling_steps (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-lmt-final-settling-steps", val);
}

/* Path: adi,fagc-final-overrange-count
 * Var : gain_ctrl.f_agc_final_overrange_count
 */
int ad9361_get_fagc_final_overrange_count (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-final-overrange-count", val);
}
int ad9361_set_fagc_final_overrange_count (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-final-overrange-count", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Fast AGC - Final Power Test */
/* Path: adi,fagc-gain-increase-after-gain-lock-enable
 * Var : gain_ctrl.f_agc_gain_increase_after_gain_lock_en
 */
int ad9361_get_fagc_gain_increase_after_gain_lock_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,fagc-gain-increase-after-gain-lock-enable", val);
}
int ad9361_set_fagc_gain_increase_after_gain_lock_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,fagc-gain-increase-after-gain-lock-enable", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Fast AGC - Unlocking the Gain */
/* 0 = MAX Gain, 1 = Optimized Gain, 2 = Set Gain */

/* Path: adi,fagc-gain-index-type-after-exit-rx-mode
 * Var : gain_ctrl.f_agc_gain_index_type_after_exit_rx_mode
 */
int ad9361_get_fagc_gain_index_type_after_exit_rx_mode (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-gain-index-type-after-exit-rx-mode", val);
}
int ad9361_set_fagc_gain_index_type_after_exit_rx_mode (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-gain-index-type-after-exit-rx-mode", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,fagc-use-last-lock-level-for-set-gain-enable
 * Var : gain_ctrl.f_agc_use_last_lock_level_for_set_gain_en
 */
int ad9361_get_fagc_use_last_lock_level_for_set_gain_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,fagc-use-last-lock-level-for-set-gain-enable", val);
}
int ad9361_set_fagc_use_last_lock_level_for_set_gain_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,fagc-use-last-lock-level-for-set-gain-enable", val);
}

/* Path: adi,fagc-rst-gla-stronger-sig-thresh-exceeded-enable
 * Var : gain_ctrl.f_agc_rst_gla_stronger_sig_thresh_exceeded_en
 */
int ad9361_get_fagc_rst_gla_stronger_sig_thresh_exceeded_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,fagc-rst-gla-stronger-sig-thresh-exceeded-enable", val);
}
int ad9361_set_fagc_rst_gla_stronger_sig_thresh_exceeded_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,fagc-rst-gla-stronger-sig-thresh-exceeded-enable", val);
}

/* Path: adi,fagc-optimized-gain-offset
 * Var : gain_ctrl.f_agc_optimized_gain_offset
 */
int ad9361_get_fagc_optimized_gain_offset (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-optimized-gain-offset", val);
}
int ad9361_set_fagc_optimized_gain_offset (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-optimized-gain-offset", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,fagc-rst-gla-stronger-sig-thresh-above-ll
 * Var : gain_ctrl.f_agc_rst_gla_stronger_sig_thresh_above_ll
 */
int ad9361_get_fagc_rst_gla_stronger_sig_thresh_above_ll (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-rst-gla-stronger-sig-thresh-above-ll", val);
}
int ad9361_set_fagc_rst_gla_stronger_sig_thresh_above_ll (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-rst-gla-stronger-sig-thresh-above-ll", val);
}

/* Path: adi,fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable
 * Var : gain_ctrl.f_agc_rst_gla_engergy_lost_sig_thresh_exceeded_en
 */
int ad9361_get_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable", val);
}
int ad9361_set_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable", val);
}

/* Path: adi,fagc-rst-gla-engergy-lost-goto-optim-gain-enable
 * Var : gain_ctrl.f_agc_rst_gla_engergy_lost_goto_optim_gain_en
 */
int ad9361_get_fagc_rst_gla_engergy_lost_goto_optim_gain_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,fagc-rst-gla-engergy-lost-goto-optim-gain-enable", val);
}
int ad9361_set_fagc_rst_gla_engergy_lost_goto_optim_gain_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,fagc-rst-gla-engergy-lost-goto-optim-gain-enable", val);
}

/* Path: adi,fagc-rst-gla-engergy-lost-sig-thresh-below-ll
 * Var : gain_ctrl.f_agc_rst_gla_engergy_lost_sig_thresh_below_ll
 */
int ad9361_get_fagc_rst_gla_engergy_lost_sig_thresh_below_ll (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-rst-gla-engergy-lost-sig-thresh-below-ll", val);
}
int ad9361_set_fagc_rst_gla_engergy_lost_sig_thresh_below_ll (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-rst-gla-engergy-lost-sig-thresh-below-ll", val);
}

/* Path: adi,fagc-energy-lost-stronger-sig-gain-lock-exit-cnt
 * Var : gain_ctrl.f_agc_energy_lost_stronger_sig_gain_lock_exit_cnt
 */
int ad9361_get_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-energy-lost-stronger-sig-gain-lock-exit-cnt", val);
}
int ad9361_set_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-energy-lost-stronger-sig-gain-lock-exit-cnt", val);
}

/* Path: adi,fagc-rst-gla-large-adc-overload-enable
 * Var : gain_ctrl.f_agc_rst_gla_large_adc_overload_en
 */
int ad9361_get_fagc_rst_gla_large_adc_overload_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,fagc-rst-gla-large-adc-overload-enable", val);
}
int ad9361_set_fagc_rst_gla_large_adc_overload_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,fagc-rst-gla-large-adc-overload-enable", val);
}

/* Path: adi,fagc-rst-gla-large-lmt-overload-enable
 * Var : gain_ctrl.f_agc_rst_gla_large_lmt_overload_en
 */
int ad9361_get_fagc_rst_gla_large_lmt_overload_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,fagc-rst-gla-large-lmt-overload-enable", val);
}
int ad9361_set_fagc_rst_gla_large_lmt_overload_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,fagc-rst-gla-large-lmt-overload-enable", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,fagc-rst-gla-en-agc-pulled-high-enable
 * Var : gain_ctrl.f_agc_rst_gla_en_agc_pulled_high_en
 */
int ad9361_get_fagc_rst_gla_en_agc_pulled_high_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,fagc-rst-gla-en-agc-pulled-high-enable", val);
}
int ad9361_set_fagc_rst_gla_en_agc_pulled_high_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,fagc-rst-gla-en-agc-pulled-high-enable", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,fagc-rst-gla-if-en-agc-pulled-high-mode
 * Var : gain_ctrl.f_agc_rst_gla_if_en_agc_pulled_high_mode
 */
int ad9361_get_fagc_rst_gla_if_en_agc_pulled_high_mode (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-rst-gla-if-en-agc-pulled-high-mode", val);
}
int ad9361_set_fagc_rst_gla_if_en_agc_pulled_high_mode (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-rst-gla-if-en-agc-pulled-high-mode", val);
}

/* Path: adi,fagc-power-measurement-duration-in-state5
 * Var : gain_ctrl.f_agc_power_measurement_duration_in_state5
 */
int ad9361_get_fagc_power_measurement_duration_in_state5 (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,fagc-power-measurement-duration-in-state5", val);
}
int ad9361_set_fagc_power_measurement_duration_in_state5 (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,fagc-power-measurement-duration-in-state5", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* RSSI Control */
                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,rssi-restart-mode
 * Var : rssi_ctrl.restart_mode
 */
int ad9361_get_rssi_restart_mode (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,rssi-restart-mode", val);
}
int ad9361_set_rssi_restart_mode (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,rssi-restart-mode", val);
}

/* Path: adi,rssi-unit-is-rx-samples-enable
 * Var : rssi_ctrl.rssi_unit_is_rx_samples
 */
int ad9361_get_rssi_unit_is_rx_samples_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,rssi-unit-is-rx-samples-enable", val);
}
int ad9361_set_rssi_unit_is_rx_samples_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,rssi-unit-is-rx-samples-enable", val);
}

/* Path: adi,rssi-delay
 * Var : rssi_ctrl.rssi_delay
 */
int ad9361_get_rssi_delay (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,rssi-delay", val);
}
int ad9361_set_rssi_delay (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,rssi-delay", val);
}

/* Path: adi,rssi-wait
 * Var : rssi_ctrl.rssi_wait
 */
int ad9361_get_rssi_wait (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,rssi-wait", val);
}
int ad9361_set_rssi_wait (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,rssi-wait", val);
}

/* Path: adi,rssi-duration
 * Var : rssi_ctrl.rssi_duration
 */
int ad9361_get_rssi_duration (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,rssi-duration", val);
}
int ad9361_set_rssi_duration (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,rssi-duration", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Control Outs Control */
                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,ctrl-outs-index
 * Var : ctrl_outs_ctrl.index
 */
int ad9361_get_ctrl_outs_index (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,ctrl-outs-index", val);
}
int ad9361_set_ctrl_outs_index (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,ctrl-outs-index", val);
}

/* Path: adi,ctrl-outs-enable-mask
 * Var : ctrl_outs_ctrl.en_mask
 */
int ad9361_get_ctrl_outs_enable_mask (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,ctrl-outs-enable-mask", val);
}
int ad9361_set_ctrl_outs_enable_mask (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,ctrl-outs-enable-mask", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* eLNA Control */
                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,elna-settling-delay-ns
 * Var : elna_ctrl.settling_delay_ns
 */
int ad9361_get_elna_settling_delay_ns (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,elna-settling-delay-ns", val);
}
int ad9361_set_elna_settling_delay_ns (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,elna-settling-delay-ns", val);
}

/* Path: adi,elna-gain-mdB
 * Var : elna_ctrl.gain_mdB
 */
int ad9361_get_elna_gain_mdB (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,elna-gain-mdB", val);
}
int ad9361_set_elna_gain_mdB (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,elna-gain-mdB", val);
}

/* Path: adi,elna-bypass-loss-mdB
 * Var : elna_ctrl.bypass_loss_mdB
 */
int ad9361_get_elna_bypass_loss_mdB (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,elna-bypass-loss-mdB", val);
}
int ad9361_set_elna_bypass_loss_mdB (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,elna-bypass-loss-mdB", val);
}

/* Path: adi,elna-rx1-gpo0-control-enable
 * Var : elna_ctrl.elna_1_control_en
 */
int ad9361_get_elna_rx1_gpo0_control_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,elna-rx1-gpo0-control-enable", val);
}
int ad9361_set_elna_rx1_gpo0_control_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,elna-rx1-gpo0-control-enable", val);
}

/* Path: adi,elna-rx2-gpo1-control-enable
 * Var : elna_ctrl.elna_2_control_en
 */
int ad9361_get_elna_rx2_gpo1_control_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,elna-rx2-gpo1-control-enable", val);
}
int ad9361_set_elna_rx2_gpo1_control_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,elna-rx2-gpo1-control-enable", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* AuxADC Temp Sense Control */
                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,temp-sense-measurement-interval-ms
 * Var : auxadc_ctrl.temp_time_inteval_ms
 */
int ad9361_get_temp_sense_measurement_interval_ms (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,temp-sense-measurement-interval-ms", val);
}
int ad9361_set_temp_sense_measurement_interval_ms (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,temp-sense-measurement-interval-ms", val);
}

/* Path: adi,temp-sense-offset-signed
 * Var : auxadc_ctrl.offset
 */
int ad9361_get_temp_sense_offset_signed (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,temp-sense-offset-signed", val);
}
int ad9361_set_temp_sense_offset_signed (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,temp-sense-offset-signed", val);
}

/* Path: adi,temp-sense-periodic-measurement-enable
 * Var : auxadc_ctrl.periodic_temp_measuremnt
 */
int ad9361_get_temp_sense_periodic_measurement_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,temp-sense-periodic-measurement-enable", val);
}
int ad9361_set_temp_sense_periodic_measurement_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,temp-sense-periodic-measurement-enable", val);
}

/* Path: adi,temp-sense-decimation
 * Var : auxadc_ctrl.temp_sensor_decimation
 */
int ad9361_get_temp_sense_decimation (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,temp-sense-decimation", val);
}
int ad9361_set_temp_sense_decimation (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,temp-sense-decimation", val);
}

/* Path: adi,aux-adc-rate
 * Var : auxadc_ctrl.auxadc_clock_rate
 */
int ad9361_get_aux_adc_rate (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,aux-adc-rate", val);
}
int ad9361_set_aux_adc_rate (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,aux-adc-rate", val);
}

/* Path: adi,aux-adc-decimation
 * Var : auxadc_ctrl.auxadc_decimation
 */
int ad9361_get_aux_adc_decimation (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,aux-adc-decimation", val);
}
int ad9361_set_aux_adc_decimation (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,aux-adc-decimation", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* AuxDAC Control */
                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,aux-dac-manual-mode-enable
 * Var : auxdac_ctrl.auxdac_manual_mode_en
 */
int ad9361_get_aux_dac_manual_mode_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,aux-dac-manual-mode-enable", val);
}
int ad9361_set_aux_dac_manual_mode_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,aux-dac-manual-mode-enable", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,aux-dac1-default-value-mV
 * Var : auxdac_ctrl.dac1_default_value
 */
int ad9361_get_aux_dac1_default_value_mV (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,aux-dac1-default-value-mV", val);
}
int ad9361_set_aux_dac1_default_value_mV (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,aux-dac1-default-value-mV", val);
}

/* Path: adi,aux-dac1-active-in-rx-enable
 * Var : auxdac_ctrl.dac1_in_rx_en
 */
int ad9361_get_aux_dac1_active_in_rx_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,aux-dac1-active-in-rx-enable", val);
}
int ad9361_set_aux_dac1_active_in_rx_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,aux-dac1-active-in-rx-enable", val);
}

/* Path: adi,aux-dac1-active-in-tx-enable
 * Var : auxdac_ctrl.dac1_in_tx_en
 */
int ad9361_get_aux_dac1_active_in_tx_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,aux-dac1-active-in-tx-enable", val);
}
int ad9361_set_aux_dac1_active_in_tx_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,aux-dac1-active-in-tx-enable", val);
}

/* Path: adi,aux-dac1-active-in-alert-enable
 * Var : auxdac_ctrl.dac1_in_alert_en
 */
int ad9361_get_aux_dac1_active_in_alert_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,aux-dac1-active-in-alert-enable", val);
}
int ad9361_set_aux_dac1_active_in_alert_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,aux-dac1-active-in-alert-enable", val);
}

/* Path: adi,aux-dac1-rx-delay-us
 * Var : auxdac_ctrl.dac1_rx_delay_us
 */
int ad9361_get_aux_dac1_rx_delay_us (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,aux-dac1-rx-delay-us", val);
}
int ad9361_set_aux_dac1_rx_delay_us (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,aux-dac1-rx-delay-us", val);
}

/* Path: adi,aux-dac1-tx-delay-us
 * Var : auxdac_ctrl.dac1_tx_delay_us
 */
int ad9361_get_aux_dac1_tx_delay_us (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,aux-dac1-tx-delay-us", val);
}
int ad9361_set_aux_dac1_tx_delay_us (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,aux-dac1-tx-delay-us", val);
}

                                                                                                                                                                                                                                                                                                                                                               
/* Path: adi,aux-dac2-default-value-mV
 * Var : auxdac_ctrl.dac2_default_value
 */
int ad9361_get_aux_dac2_default_value_mV (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,aux-dac2-default-value-mV", val);
}
int ad9361_set_aux_dac2_default_value_mV (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,aux-dac2-default-value-mV", val);
}

/* Path: adi,aux-dac2-active-in-rx-enable
 * Var : auxdac_ctrl.dac2_in_rx_en
 */
int ad9361_get_aux_dac2_active_in_rx_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,aux-dac2-active-in-rx-enable", val);
}
int ad9361_set_aux_dac2_active_in_rx_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,aux-dac2-active-in-rx-enable", val);
}

/* Path: adi,aux-dac2-active-in-tx-enable
 * Var : auxdac_ctrl.dac2_in_tx_en
 */
int ad9361_get_aux_dac2_active_in_tx_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,aux-dac2-active-in-tx-enable", val);
}
int ad9361_set_aux_dac2_active_in_tx_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,aux-dac2-active-in-tx-enable", val);
}

/* Path: adi,aux-dac2-active-in-alert-enable
 * Var : auxdac_ctrl.dac2_in_alert_en
 */
int ad9361_get_aux_dac2_active_in_alert_enable (unsigned dev, int *val)
{
	return sysfs_get_int(dev, "adi,aux-dac2-active-in-alert-enable", val);
}
int ad9361_set_aux_dac2_active_in_alert_enable (unsigned dev, int val)
{
	return sysfs_set_int(dev, "adi,aux-dac2-active-in-alert-enable", val);
}

/* Path: adi,aux-dac2-rx-delay-us
 * Var : auxdac_ctrl.dac2_rx_delay_us
 */
int ad9361_get_aux_dac2_rx_delay_us (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,aux-dac2-rx-delay-us", val);
}
int ad9361_set_aux_dac2_rx_delay_us (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,aux-dac2-rx-delay-us", val);
}

/* Path: adi,aux-dac2-tx-delay-us
 * Var : auxdac_ctrl.dac2_tx_delay_us
 */
int ad9361_get_aux_dac2_tx_delay_us (unsigned dev, uint32_t *val)
{
	return sysfs_get_u32(dev, "adi,aux-dac2-tx-delay-us", val);
}
int ad9361_set_aux_dac2_tx_delay_us (unsigned dev, uint32_t val)
{
	return sysfs_set_u32(dev, "adi,aux-dac2-tx-delay-us", val);
}

