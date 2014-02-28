/** \file      src/app/ad9361/map-debug.c
 *  \brief     Extensions, debugging, and hacking
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
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include <ad9361.h>

#include "map.h"
#include "main.h"
#include "util.h"


/******* Generic get/set for sysfs tree *******/


static int map_iio_common_get (const char *root, const char *leaf, const char *name)
{
	char  path[PATH_MAX];
	char  buff[256];
	char *p, *q;

	if ( !dev_info_curr || !dev_info_curr->leaf )
	{
		fprintf(stderr, "No devfs paths set, stop.\n");
		return -1;
	}

	snprintf(path, sizeof(path), "%s/%s/%s", root, dev_info_curr->leaf, leaf);
	if ( proc_read(path, buff, sizeof(buff)) < 0 )
	{
		perror(path);
		return -1;
	}

	for ( p = buff; isspace(*p); p++ ) ;
	for ( q = p; *q; q++ ) ;
	while ( q > p && isspace(*(q - 1)) )
		q--;
	*q = '\0';

	printf("%s=\"%s\"\n", name, p);
	return 0;
}

static int map_iio_common_set (const char *root, const char *leaf, const char *value)
{
	char path[PATH_MAX];

	if ( !dev_info_curr || !dev_info_curr->leaf )
	{
		fprintf(stderr, "No devfs paths set, stop.\n");
		return -1;
	}

	snprintf(path, sizeof(path), "%s/%s/%s", root, dev_info_curr->leaf, leaf);
	if ( proc_printf(path, "%s\n", value) < 0 )
	{
		perror(path);
		return -1;
	}
	
	return 0;
}


/******* Normal operation IIO attributes in /sys/bus/iio/devices/ *******/


static int map_iio_normal_get (int argc, const char **argv)
{
	return map_iio_common_get("/sys/bus/iio/devices", argv[0] + 4, argv[0] + 4);
}
MAP_CMD(get_calib_mode,                           map_iio_normal_get, 1);
MAP_CMD(get_dcxo_tune_coarse,                     map_iio_normal_get, 1);
MAP_CMD(get_dcxo_tune_fine,                       map_iio_normal_get, 1);
MAP_CMD(get_dev,                                  map_iio_normal_get, 1);
MAP_CMD(get_ensm_mode,                            map_iio_normal_get, 1);
MAP_CMD(get_filter_fir_config,                    map_iio_normal_get, 1);
MAP_CMD(get_in_out_voltage_filter_fir_en,         map_iio_normal_get, 1);
MAP_CMD(get_in_temp0_input,                       map_iio_normal_get, 1);
MAP_CMD(get_in_voltage0_gain_control_mode,        map_iio_normal_get, 1);
MAP_CMD(get_in_voltage0_hardwaregain,             map_iio_normal_get, 1);
MAP_CMD(get_in_voltage0_rssi,                     map_iio_normal_get, 1);
MAP_CMD(get_in_voltage1_gain_control_mode,        map_iio_normal_get, 1);
MAP_CMD(get_in_voltage1_hardwaregain,             map_iio_normal_get, 1);
MAP_CMD(get_in_voltage1_rssi,                     map_iio_normal_get, 1);
MAP_CMD(get_in_voltage_bb_dc_offset_tracking_en,  map_iio_normal_get, 1);
MAP_CMD(get_in_voltage_filter_fir_en,             map_iio_normal_get, 1);
MAP_CMD(get_in_voltage_quadrature_tracking_en,    map_iio_normal_get, 1);
MAP_CMD(get_in_voltage_rf_bandwidth,              map_iio_normal_get, 1);
MAP_CMD(get_in_voltage_rf_dc_offset_tracking_en,  map_iio_normal_get, 1);
MAP_CMD(get_in_voltage_sampling_frequency,        map_iio_normal_get, 1);
MAP_CMD(get_name,                                 map_iio_normal_get, 1);
MAP_CMD(get_out_altvoltage0_RX_LO_frequency,      map_iio_normal_get, 1);
MAP_CMD(get_out_altvoltage1_TX_LO_frequency,      map_iio_normal_get, 1);
MAP_CMD(get_out_voltage0_hardwaregain,            map_iio_normal_get, 1);
MAP_CMD(get_out_voltage1_hardwaregain,            map_iio_normal_get, 1);
MAP_CMD(get_out_voltage_filter_fir_en,            map_iio_normal_get, 1);
MAP_CMD(get_out_voltage_rf_bandwidth,             map_iio_normal_get, 1);
MAP_CMD(get_out_voltage_sampling_frequency,       map_iio_normal_get, 1);
MAP_CMD(get_rx_path_rates,                        map_iio_normal_get, 1);
MAP_CMD(get_subsystem,                            map_iio_normal_get, 1);
MAP_CMD(get_trx_rate_governor,                    map_iio_normal_get, 1);
MAP_CMD(get_tx_path_rates,                        map_iio_normal_get, 1);
MAP_CMD(get_uevent,                               map_iio_normal_get, 1);

static int map_iio_normal_set (int argc, const char **argv)
{
	if ( argc < 2 || !argv[1] )
		return -1;

	return map_iio_common_set("/sys/bus/iio/devices", argv[0] + 4, argv[1]);
}
MAP_CMD(set_calib_mode,                           map_iio_normal_set, 2);
MAP_CMD(set_dcxo_tune_coarse,                     map_iio_normal_set, 2);
MAP_CMD(set_dcxo_tune_fine,                       map_iio_normal_set, 2);
MAP_CMD(set_ensm_mode,                            map_iio_normal_set, 2);
MAP_CMD(set_in_out_voltage_filter_fir_en,         map_iio_normal_set, 2);
MAP_CMD(set_in_temp0_input,                       map_iio_normal_set, 2);
MAP_CMD(set_in_voltage0_gain_control_mode,        map_iio_normal_set, 2);
MAP_CMD(set_in_voltage0_hardwaregain,             map_iio_normal_set, 2);
MAP_CMD(set_in_voltage0_rssi,                     map_iio_normal_set, 2);
MAP_CMD(set_in_voltage1_gain_control_mode,        map_iio_normal_set, 2);
MAP_CMD(set_in_voltage1_hardwaregain,             map_iio_normal_set, 2);
MAP_CMD(set_in_voltage1_rssi,                     map_iio_normal_set, 2);
MAP_CMD(set_in_voltage_bb_dc_offset_tracking_en,  map_iio_normal_set, 2);
MAP_CMD(set_in_voltage_filter_fir_en,             map_iio_normal_set, 2);
MAP_CMD(set_in_voltage_quadrature_tracking_en,    map_iio_normal_set, 2);
MAP_CMD(set_in_voltage_rf_bandwidth,              map_iio_normal_set, 2);
MAP_CMD(set_in_voltage_rf_dc_offset_tracking_en,  map_iio_normal_set, 2);
MAP_CMD(set_in_voltage_sampling_frequency,        map_iio_normal_set, 2);
MAP_CMD(set_out_altvoltage0_RX_LO_frequency,      map_iio_normal_set, 2);
MAP_CMD(set_out_altvoltage1_TX_LO_frequency,      map_iio_normal_set, 2);
MAP_CMD(set_out_voltage0_hardwaregain,            map_iio_normal_set, 2);
MAP_CMD(set_out_voltage1_hardwaregain,            map_iio_normal_set, 2);
MAP_CMD(set_out_voltage_filter_fir_en,            map_iio_normal_set, 2);
MAP_CMD(set_out_voltage_rf_bandwidth,             map_iio_normal_set, 2);
MAP_CMD(set_out_voltage_sampling_frequency,       map_iio_normal_set, 2);
MAP_CMD(set_subsystem,                            map_iio_normal_set, 2);

static int map_iio_filter_set (int argc, const char **argv)
{
	if ( argc < 2 || !argv[1] )
		return -1;

	char  b[4096];
	FILE *f = fopen(argv[1], "r");
	if ( !f )
	{
		char *p;
		if ( !(p = path_match(b, sizeof(b), env_filter_path, argv[1])) )
		{
			fprintf(stderr, "%s: unknown filter\n", argv[1]);
			return -1;
		}
		f = fopen(p, "r");
	}
	if ( !f )
	{
		fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
		return -1;
	}

	char *p = b;
	char *e = b + sizeof(b);
	while ( p < e && fgets(p, e - p, f) )
		while ( *p )
			p++;
	fclose(f);

	return map_iio_common_set("/sys/bus/iio/devices", argv[0] + 4, b);
}
MAP_CMD(set_filter_fir_config,  map_iio_filter_set, 2);


/******* Debug operations attributes in /sys/kernsl/debug/iio/ *******/


struct iio_debug_map
{
	char *arg0;
	char *leaf;
};
static struct iio_debug_map iio_debug_map[] =
{
	{ "adi_agc_adc_large_overload_exceed_counter",                "adi,agc-adc-large-overload-exceed-counter"                },
	{ "adi_agc_adc_large_overload_inc_steps",                     "adi,agc-adc-large-overload-inc-steps"                     },
	{ "adi_agc_adc_lmt_small_overload_prevent_gain_inc_enable",   "adi,agc-adc-lmt-small-overload-prevent-gain-inc-enable"   },
	{ "adi_agc_adc_small_overload_exceed_counter",                "adi,agc-adc-small-overload-exceed-counter"                },
	{ "adi_agc_attack_delay_extra_margin_us",                     "adi,agc-attack-delay-extra-margin-us"                     },
	{ "adi_agc_dig_gain_step_size",                               "adi,agc-dig-gain-step-size"                               },
	{ "adi_agc_dig_saturation_exceed_counter",                    "adi,agc-dig-saturation-exceed-counter"                    },
	{ "adi_agc_gain_update_interval_us",                          "adi,agc-gain-update-interval-us"                          },
	{ "adi_agc_immed_gain_change_if_large_adc_overload_enable",   "adi,agc-immed-gain-change-if-large-adc-overload-enable"   },
	{ "adi_agc_immed_gain_change_if_large_lmt_overload_enable",   "adi,agc-immed-gain-change-if-large-lmt-overload-enable"   },
	{ "adi_agc_inner_thresh_high",                                "adi,agc-inner-thresh-high"                                },
	{ "adi_agc_inner_thresh_high_dec_steps",                      "adi,agc-inner-thresh-high-dec-steps"                      },
	{ "adi_agc_inner_thresh_low",                                 "adi,agc-inner-thresh-low"                                 },
	{ "adi_agc_inner_thresh_low_inc_steps",                       "adi,agc-inner-thresh-low-inc-steps"                       },
	{ "adi_agc_lmt_overload_large_exceed_counter",                "adi,agc-lmt-overload-large-exceed-counter"                },
	{ "adi_agc_lmt_overload_large_inc_steps",                     "adi,agc-lmt-overload-large-inc-steps"                     },
	{ "adi_agc_lmt_overload_small_exceed_counter",                "adi,agc-lmt-overload-small-exceed-counter"                },
	{ "adi_agc_outer_thresh_high",                                "adi,agc-outer-thresh-high"                                },
	{ "adi_agc_outer_thresh_high_dec_steps",                      "adi,agc-outer-thresh-high-dec-steps"                      },
	{ "adi_agc_outer_thresh_low",                                 "adi,agc-outer-thresh-low"                                 },
	{ "adi_agc_outer_thresh_low_inc_steps",                       "adi,agc-outer-thresh-low-inc-steps"                       },
	{ "adi_agc_sync_for_gain_counter_enable",                     "adi,agc-sync-for-gain-counter-enable"                     },
	{ "adi_aux_adc_decimation",                                   "adi,aux-adc-decimation"                                   },
	{ "adi_aux_adc_rate",                                         "adi,aux-adc-rate"                                         },
	{ "adi_clk_output_mode_select",                               "adi,clk-output-mode-select"                               },
	{ "adi_ctrl_outs_enable_mask",                                "adi,ctrl-outs-enable-mask"                                },
	{ "adi_ctrl_outs_index",                                      "adi,ctrl-outs-index"                                      },
	{ "adi_dc_offset_attenuation_high_range",                     "adi,dc-offset-attenuation-high-range"                     },
	{ "adi_dc_offset_attenuation_low_range",                      "adi,dc-offset-attenuation-low-range"                      },
	{ "adi_dc_offset_count_high_range",                           "adi,dc-offset-count-high-range"                           },
	{ "adi_dc_offset_count_low_range",                            "adi,dc-offset-count-low-range"                            },
	{ "adi_dc_offset_tracking_update_event_mask",                 "adi,dc-offset-tracking-update-event-mask"                 },
	{ "adi_debug_mode_enable",                                    "adi,debug-mode-enable"                                    },
	{ "adi_elna_bypass_loss_mdB",                                 "adi,elna-bypass-loss-mdB"                                 },
	{ "adi_elna_gain_mdB",                                        "adi,elna-gain-mdB"                                        },
	{ "adi_elna_rx1_gpo0_control_enable",                         "adi,elna-rx1-gpo0-control-enable"                         },
	{ "adi_elna_rx2_gpo1_control_enable",                         "adi,elna-rx2-gpo1-control-enable"                         },
	{ "adi_elna_settling_delay_ns",                               "adi,elna-settling-delay-ns"                               },
	{ "adi_ensm_enable_pin_pulse_mode_enable",                    "adi,ensm-enable-pin-pulse-mode-enable"                    },
	{ "adi_ensm_enable_txnrx_control_enable",                     "adi,ensm-enable-txnrx-control-enable"                     },
	{ "adi_external_rx_lo_enable",                                "adi,external-rx-lo-enable"                                },
	{ "adi_external_tx_lo_enable",                                "adi,external-tx-lo-enable"                                },
	{ "adi_fagc_allow_agc_gain_increase_enable",                  "adi,fagc-allow-agc-gain-increase-enable"                  },
	{ "adi_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt",     "adi,fagc-energy-lost-stronger-sig-gain-lock-exit-cnt"     },
	{ "adi_fagc_final_overrange_count",                           "adi,fagc-final-overrange-count"                           },
	{ "adi_fagc_gain_increase_after_gain_lock_enable",            "adi,fagc-gain-increase-after-gain-lock-enable"            },
	{ "adi_fagc_gain_index_type_after_exit_rx_mode",              "adi,fagc-gain-index-type-after-exit-rx-mode"              },
	{ "adi_fagc_lmt_final_settling_steps",                        "adi,fagc-lmt-final-settling-steps"                        },
	{ "adi_fagc_lock_level",                                      "adi,fagc-lock-level"                                      },
	{ "adi_fagc_lock_level_gain_increase_upper_limit",            "adi,fagc-lock-level-gain-increase-upper-limit"            },
	{ "adi_fagc_lock_level_lmt_gain_increase_enable",             "adi,fagc-lock-level-lmt-gain-increase-enable"             },
	{ "adi_fagc_lp_thresh_increment_steps",                       "adi,fagc-lp-thresh-increment-steps"                       },
	{ "adi_fagc_lp_thresh_increment_time",                        "adi,fagc-lp-thresh-increment-time"                        },
	{ "adi_fagc_lpf_final_settling_steps",                        "adi,fagc-lpf-final-settling-steps"                        },
	{ "adi_fagc_optimized_gain_offset",                           "adi,fagc-optimized-gain-offset"                           },
	{ "adi_fagc_power_measurement_duration_in_state5",            "adi,fagc-power-measurement-duration-in-state5"            },
	{ "adi_fagc_rst_gla_en_agc_pulled_high_enable",               "adi,fagc-rst-gla-en-agc-pulled-high-enable"               },
	{ "adi_fagc_rst_gla_engergy_lost_goto_optim_gain_enable",     "adi,fagc-rst-gla-engergy-lost-goto-optim-gain-enable"     },
	{ "adi_fagc_rst_gla_engergy_lost_sig_thresh_below_ll",        "adi,fagc-rst-gla-engergy-lost-sig-thresh-below-ll"        },
	{ "adi_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable", "adi,fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable" },
	{ "adi_fagc_rst_gla_if_en_agc_pulled_high_mode",              "adi,fagc-rst-gla-if-en-agc-pulled-high-mode"              },
	{ "adi_fagc_rst_gla_large_adc_overload_enable",               "adi,fagc-rst-gla-large-adc-overload-enable"               },
	{ "adi_fagc_rst_gla_large_lmt_overload_enable",               "adi,fagc-rst-gla-large-lmt-overload-enable"               },
	{ "adi_fagc_rst_gla_stronger_sig_thresh_above_ll",            "adi,fagc-rst-gla-stronger-sig-thresh-above-ll"            },
	{ "adi_fagc_rst_gla_stronger_sig_thresh_exceeded_enable",     "adi,fagc-rst-gla-stronger-sig-thresh-exceeded-enable"     },
	{ "adi_fagc_state_wait_time_ns",                              "adi,fagc-state-wait-time-ns"                              },
	{ "adi_fagc_use_last_lock_level_for_set_gain_enable",         "adi,fagc-use-last-lock-level-for-set-gain-enable"         },
	{ "adi_frequency_division_duplex_mode_enable",                "adi,frequency-division-duplex-mode-enable"                },
	{ "adi_gc_adc_large_overload_thresh",                         "adi,gc-adc-large-overload-thresh"                         },
	{ "adi_gc_adc_ovr_sample_size",                               "adi,gc-adc-ovr-sample-size"                               },
	{ "adi_gc_adc_small_overload_thresh",                         "adi,gc-adc-small-overload-thresh"                         },
	{ "adi_gc_dec_pow_measurement_duration",                      "adi,gc-dec-pow-measurement-duration"                      },
	{ "adi_gc_dig_gain_enable",                                   "adi,gc-dig-gain-enable"                                   },
	{ "adi_gc_lmt_overload_high_thresh",                          "adi,gc-lmt-overload-high-thresh"                          },
	{ "adi_gc_lmt_overload_low_thresh",                           "adi,gc-lmt-overload-low-thresh"                           },
	{ "adi_gc_low_power_thresh",                                  "adi,gc-low-power-thresh"                                  },
	{ "adi_gc_max_dig_gain",                                      "adi,gc-max-dig-gain"                                      },
	{ "adi_gc_rx1_mode",                                          "adi,gc-rx1-mode"                                          },
	{ "adi_gc_rx2_mode",                                          "adi,gc-rx2-mode"                                          },
	{ "adi_mgc_dec_gain_step",                                    "adi,mgc-dec-gain-step"                                    },
	{ "adi_mgc_inc_gain_step",                                    "adi,mgc-inc-gain-step"                                    },
	{ "adi_mgc_rx1_ctrl_inp_enable",                              "adi,mgc-rx1-ctrl-inp-enable"                              },
	{ "adi_mgc_rx2_ctrl_inp_enable",                              "adi,mgc-rx2-ctrl-inp-enable"                              },
	{ "adi_mgc_split_table_ctrl_inp_gain_mode",                   "adi,mgc-split-table-ctrl-inp-gain-mode"                   },
	{ "adi_rf_rx_bandwidth_hz",                                   "adi,rf-rx-bandwidth-hz"                                   },
	{ "adi_rf_tx_bandwidth_hz",                                   "adi,rf-tx-bandwidth-hz"                                   },
	{ "adi_rssi_delay",                                           "adi,rssi-delay"                                           },
	{ "adi_rssi_duration",                                        "adi,rssi-duration"                                        },
	{ "adi_rssi_restart_mode",                                    "adi,rssi-restart-mode"                                    },
	{ "adi_rssi_unit_is_rx_samples_enable",                       "adi,rssi-unit-is-rx-samples-enable"                       },
	{ "adi_rssi_wait",                                            "adi,rssi-wait"                                            },
	{ "adi_rx_rf_port_input_select",                              "adi,rx-rf-port-input-select"                              },
	{ "adi_split_gain_table_mode_enable",                         "adi,split-gain-table-mode-enable"                         },
	{ "adi_tdd_skip_vco_cal_enable",                              "adi,tdd-skip-vco-cal-enable"                              },
	{ "adi_tdd_use_dual_synth_mode_enable",                       "adi,tdd-use-dual-synth-mode-enable"                       },
	{ "adi_tdd_use_fdd_vco_tables_enable",                        "adi,tdd-use-fdd-vco-tables-enable"                        },
	{ "adi_temp_sense_decimation",                                "adi,temp-sense-decimation"                                },
	{ "adi_temp_sense_measurement_interval_ms",                   "adi,temp-sense-measurement-interval-ms"                   },
	{ "adi_temp_sense_offset_signed",                             "adi,temp-sense-offset-signed"                             },
	{ "adi_temp_sense_periodic_measurement_enable",               "adi,temp-sense-periodic-measurement-enable"               },
	{ "adi_tx_attenuation_mdB",                                   "adi,tx-attenuation-mdB"                                   },
	{ "adi_tx_rf_port_input_select",                              "adi,tx-rf-port-input-select"                              },
	{ "adi_update_tx_gain_in_alert_enable",                       "adi,update-tx-gain-in-alert-enable"                       },
	{ "adi_xo_disable_use_ext_refclk_enable",                     "adi,xo-disable-use-ext-refclk-enable"                     },
	{ "bist_prbs",                                                "bist_prbs"                                                },
	{ "bist_timing_analysis",                                     "bist_timing_analysis"                                     },
	{ "bist_tone",                                                "bist_tone"                                                },
	{ "direct_reg_access",                                        "direct_reg_access"                                        },
	{ "gaininfo_rx1",                                             "gaininfo_rx1"                                             },
	{ "gaininfo_rx2",                                             "gaininfo_rx2"                                             },
	{ "initialize",                                               "initialize"                                               },
	{ "loopback",                                                 "loopback"                                                 },
	{ NULL }
};

static char *map_iio_debug_leaf (const char *arg0)
{
	struct iio_debug_map *map = iio_debug_map;

	while ( map->arg0 )
		if ( !strcmp(map->arg0, arg0) )
			return map->leaf;
		else
			map++;

	return NULL;
}


static int map_iio_debug_get (int argc, const char **argv)
{
	const char *arg0 = argv[0] + 4;
	const char *leaf = map_iio_debug_leaf(arg0);

	if ( !leaf )
		return -1;

	return map_iio_common_get("/sys/kernel/debug/iio", leaf, arg0);
}
MAP_CMD(get_adi_2rx_2tx_mode_enable,                                  map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_adc_large_overload_exceed_counter,                map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_adc_large_overload_inc_steps,                     map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_adc_lmt_small_overload_prevent_gain_inc_enable,   map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_adc_small_overload_exceed_counter,                map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_attack_delay_extra_margin_us,                     map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_dig_gain_step_size,                               map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_dig_saturation_exceed_counter,                    map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_gain_update_interval_us,                          map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_immed_gain_change_if_large_adc_overload_enable,   map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_immed_gain_change_if_large_lmt_overload_enable,   map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_inner_thresh_high,                                map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_inner_thresh_high_dec_steps,                      map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_inner_thresh_low,                                 map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_inner_thresh_low_inc_steps,                       map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_lmt_overload_large_exceed_counter,                map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_lmt_overload_large_inc_steps,                     map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_lmt_overload_small_exceed_counter,                map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_outer_thresh_high,                                map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_outer_thresh_high_dec_steps,                      map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_outer_thresh_low,                                 map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_outer_thresh_low_inc_steps,                       map_iio_debug_get, 1);
MAP_CMD(get_adi_agc_sync_for_gain_counter_enable,                     map_iio_debug_get, 1);
MAP_CMD(get_adi_aux_adc_decimation,                                   map_iio_debug_get, 1);
MAP_CMD(get_adi_aux_adc_rate,                                         map_iio_debug_get, 1);
MAP_CMD(get_adi_clk_output_mode_select,                               map_iio_debug_get, 1);
MAP_CMD(get_adi_ctrl_outs_enable_mask,                                map_iio_debug_get, 1);
MAP_CMD(get_adi_ctrl_outs_index,                                      map_iio_debug_get, 1);
MAP_CMD(get_adi_dc_offset_attenuation_high_range,                     map_iio_debug_get, 1);
MAP_CMD(get_adi_dc_offset_attenuation_low_range,                      map_iio_debug_get, 1);
MAP_CMD(get_adi_dc_offset_count_high_range,                           map_iio_debug_get, 1);
MAP_CMD(get_adi_dc_offset_count_low_range,                            map_iio_debug_get, 1);
MAP_CMD(get_adi_dc_offset_tracking_update_event_mask,                 map_iio_debug_get, 1);
MAP_CMD(get_adi_debug_mode_enable,                                    map_iio_debug_get, 1);
MAP_CMD(get_adi_elna_bypass_loss_mdB,                                 map_iio_debug_get, 1);
MAP_CMD(get_adi_elna_gain_mdB,                                        map_iio_debug_get, 1);
MAP_CMD(get_adi_elna_rx1_gpo0_control_enable,                         map_iio_debug_get, 1);
MAP_CMD(get_adi_elna_rx2_gpo1_control_enable,                         map_iio_debug_get, 1);
MAP_CMD(get_adi_elna_settling_delay_ns,                               map_iio_debug_get, 1);
MAP_CMD(get_adi_ensm_enable_pin_pulse_mode_enable,                    map_iio_debug_get, 1);
MAP_CMD(get_adi_ensm_enable_txnrx_control_enable,                     map_iio_debug_get, 1);
MAP_CMD(get_adi_external_rx_lo_enable,                                map_iio_debug_get, 1);
MAP_CMD(get_adi_external_tx_lo_enable,                                map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_allow_agc_gain_increase_enable,                  map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt,     map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_final_overrange_count,                           map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_gain_increase_after_gain_lock_enable,            map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_gain_index_type_after_exit_rx_mode,              map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_lmt_final_settling_steps,                        map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_lock_level,                                      map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_lock_level_gain_increase_upper_limit,            map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_lock_level_lmt_gain_increase_enable,             map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_lp_thresh_increment_steps,                       map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_lp_thresh_increment_time,                        map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_lpf_final_settling_steps,                        map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_optimized_gain_offset,                           map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_power_measurement_duration_in_state5,            map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_rst_gla_en_agc_pulled_high_enable,               map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_rst_gla_engergy_lost_goto_optim_gain_enable,     map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_rst_gla_engergy_lost_sig_thresh_below_ll,        map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable, map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_rst_gla_if_en_agc_pulled_high_mode,              map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_rst_gla_large_adc_overload_enable,               map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_rst_gla_large_lmt_overload_enable,               map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_rst_gla_stronger_sig_thresh_above_ll,            map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_rst_gla_stronger_sig_thresh_exceeded_enable,     map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_state_wait_time_ns,                              map_iio_debug_get, 1);
MAP_CMD(get_adi_fagc_use_last_lock_level_for_set_gain_enable,         map_iio_debug_get, 1);
MAP_CMD(get_adi_frequency_division_duplex_mode_enable,                map_iio_debug_get, 1);
MAP_CMD(get_adi_gc_adc_large_overload_thresh,                         map_iio_debug_get, 1);
MAP_CMD(get_adi_gc_adc_ovr_sample_size,                               map_iio_debug_get, 1);
MAP_CMD(get_adi_gc_adc_small_overload_thresh,                         map_iio_debug_get, 1);
MAP_CMD(get_adi_gc_dec_pow_measurement_duration,                      map_iio_debug_get, 1);
MAP_CMD(get_adi_gc_dig_gain_enable,                                   map_iio_debug_get, 1);
MAP_CMD(get_adi_gc_lmt_overload_high_thresh,                          map_iio_debug_get, 1);
MAP_CMD(get_adi_gc_lmt_overload_low_thresh,                           map_iio_debug_get, 1);
MAP_CMD(get_adi_gc_low_power_thresh,                                  map_iio_debug_get, 1);
MAP_CMD(get_adi_gc_max_dig_gain,                                      map_iio_debug_get, 1);
MAP_CMD(get_adi_gc_rx1_mode,                                          map_iio_debug_get, 1);
MAP_CMD(get_adi_gc_rx2_mode,                                          map_iio_debug_get, 1);
MAP_CMD(get_adi_mgc_dec_gain_step,                                    map_iio_debug_get, 1);
MAP_CMD(get_adi_mgc_inc_gain_step,                                    map_iio_debug_get, 1);
MAP_CMD(get_adi_mgc_rx1_ctrl_inp_enable,                              map_iio_debug_get, 1);
MAP_CMD(get_adi_mgc_rx2_ctrl_inp_enable,                              map_iio_debug_get, 1);
MAP_CMD(get_adi_mgc_split_table_ctrl_inp_gain_mode,                   map_iio_debug_get, 1);
MAP_CMD(get_adi_rf_rx_bandwidth_hz,                                   map_iio_debug_get, 1);
MAP_CMD(get_adi_rf_tx_bandwidth_hz,                                   map_iio_debug_get, 1);
MAP_CMD(get_adi_rssi_delay,                                           map_iio_debug_get, 1);
MAP_CMD(get_adi_rssi_duration,                                        map_iio_debug_get, 1);
MAP_CMD(get_adi_rssi_restart_mode,                                    map_iio_debug_get, 1);
MAP_CMD(get_adi_rssi_unit_is_rx_samples_enable,                       map_iio_debug_get, 1);
MAP_CMD(get_adi_rssi_wait,                                            map_iio_debug_get, 1);
MAP_CMD(get_adi_rx_rf_port_input_select,                              map_iio_debug_get, 1);
MAP_CMD(get_adi_split_gain_table_mode_enable,                         map_iio_debug_get, 1);
MAP_CMD(get_adi_tdd_skip_vco_cal_enable,                              map_iio_debug_get, 1);
MAP_CMD(get_adi_tdd_use_dual_synth_mode_enable,                       map_iio_debug_get, 1);
MAP_CMD(get_adi_tdd_use_fdd_vco_tables_enable,                        map_iio_debug_get, 1);
MAP_CMD(get_adi_temp_sense_decimation,                                map_iio_debug_get, 1);
MAP_CMD(get_adi_temp_sense_measurement_interval_ms,                   map_iio_debug_get, 1);
MAP_CMD(get_adi_temp_sense_offset_signed,                             map_iio_debug_get, 1);
MAP_CMD(get_adi_temp_sense_periodic_measurement_enable,               map_iio_debug_get, 1);
MAP_CMD(get_adi_tx_attenuation_mdB,                                   map_iio_debug_get, 1);
MAP_CMD(get_adi_tx_rf_port_input_select,                              map_iio_debug_get, 1);
MAP_CMD(get_adi_update_tx_gain_in_alert_enable,                       map_iio_debug_get, 1);
MAP_CMD(get_adi_xo_disable_use_ext_refclk_enable,                     map_iio_debug_get, 1);
MAP_CMD(get_bist_prbs,                                                map_iio_debug_get, 1);
MAP_CMD(get_bist_timing_analysis,                                     map_iio_debug_get, 1);
MAP_CMD(get_bist_tone,                                                map_iio_debug_get, 1);
MAP_CMD(get_direct_reg_access,                                        map_iio_debug_get, 1);
MAP_CMD(get_gaininfo_rx1,                                             map_iio_debug_get, 1);
MAP_CMD(get_gaininfo_rx2,                                             map_iio_debug_get, 1);
MAP_CMD(get_initialize,                                               map_iio_debug_get, 1);
MAP_CMD(get_loopback,                                                 map_iio_debug_get, 1);


static int map_iio_debug_set (int argc, const char **argv)
{
	const char *arg0 = argv[0] + 4;
	const char *leaf = map_iio_debug_leaf(arg0);

	if ( !leaf )
		return -1;

	if ( argc < 2 || !argv[1] )
		return -1;

	return map_iio_common_set("/sys/kernel/debug/iio", leaf, argv[1]);
}
MAP_CMD(set_adi_agc_adc_large_overload_exceed_counter,                map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_adc_large_overload_inc_steps,                     map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_adc_lmt_small_overload_prevent_gain_inc_enable,   map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_adc_small_overload_exceed_counter,                map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_attack_delay_extra_margin_us,                     map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_dig_gain_step_size,                               map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_dig_saturation_exceed_counter,                    map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_gain_update_interval_us,                          map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_immed_gain_change_if_large_adc_overload_enable,   map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_immed_gain_change_if_large_lmt_overload_enable,   map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_inner_thresh_high,                                map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_inner_thresh_high_dec_steps,                      map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_inner_thresh_low,                                 map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_inner_thresh_low_inc_steps,                       map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_lmt_overload_large_exceed_counter,                map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_lmt_overload_large_inc_steps,                     map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_lmt_overload_small_exceed_counter,                map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_outer_thresh_high,                                map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_outer_thresh_high_dec_steps,                      map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_outer_thresh_low,                                 map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_outer_thresh_low_inc_steps,                       map_iio_debug_set, 2);
MAP_CMD(set_adi_agc_sync_for_gain_counter_enable,                     map_iio_debug_set, 2);
MAP_CMD(set_adi_aux_adc_decimation,                                   map_iio_debug_set, 2);
MAP_CMD(set_adi_aux_adc_rate,                                         map_iio_debug_set, 2);
MAP_CMD(set_adi_clk_output_mode_select,                               map_iio_debug_set, 2);
MAP_CMD(set_adi_ctrl_outs_enable_mask,                                map_iio_debug_set, 2);
MAP_CMD(set_adi_ctrl_outs_index,                                      map_iio_debug_set, 2);
MAP_CMD(set_adi_dc_offset_attenuation_high_range,                     map_iio_debug_set, 2);
MAP_CMD(set_adi_dc_offset_attenuation_low_range,                      map_iio_debug_set, 2);
MAP_CMD(set_adi_dc_offset_count_high_range,                           map_iio_debug_set, 2);
MAP_CMD(set_adi_dc_offset_count_low_range,                            map_iio_debug_set, 2);
MAP_CMD(set_adi_dc_offset_tracking_update_event_mask,                 map_iio_debug_set, 2);
MAP_CMD(set_adi_debug_mode_enable,                                    map_iio_debug_set, 2);
MAP_CMD(set_adi_elna_bypass_loss_mdB,                                 map_iio_debug_set, 2);
MAP_CMD(set_adi_elna_gain_mdB,                                        map_iio_debug_set, 2);
MAP_CMD(set_adi_elna_rx1_gpo0_control_enable,                         map_iio_debug_set, 2);
MAP_CMD(set_adi_elna_rx2_gpo1_control_enable,                         map_iio_debug_set, 2);
MAP_CMD(set_adi_elna_settling_delay_ns,                               map_iio_debug_set, 2);
MAP_CMD(set_adi_ensm_enable_pin_pulse_mode_enable,                    map_iio_debug_set, 2);
MAP_CMD(set_adi_ensm_enable_txnrx_control_enable,                     map_iio_debug_set, 2);
MAP_CMD(set_adi_external_rx_lo_enable,                                map_iio_debug_set, 2);
MAP_CMD(set_adi_external_tx_lo_enable,                                map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_allow_agc_gain_increase_enable,                  map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_energy_lost_stronger_sig_gain_lock_exit_cnt,     map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_final_overrange_count,                           map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_gain_increase_after_gain_lock_enable,            map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_gain_index_type_after_exit_rx_mode,              map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_lmt_final_settling_steps,                        map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_lock_level,                                      map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_lock_level_gain_increase_upper_limit,            map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_lock_level_lmt_gain_increase_enable,             map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_lp_thresh_increment_steps,                       map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_lp_thresh_increment_time,                        map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_lpf_final_settling_steps,                        map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_optimized_gain_offset,                           map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_power_measurement_duration_in_state5,            map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_rst_gla_en_agc_pulled_high_enable,               map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_rst_gla_engergy_lost_goto_optim_gain_enable,     map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_rst_gla_engergy_lost_sig_thresh_below_ll,        map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_rst_gla_engergy_lost_sig_thresh_exceeded_enable, map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_rst_gla_if_en_agc_pulled_high_mode,              map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_rst_gla_large_adc_overload_enable,               map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_rst_gla_large_lmt_overload_enable,               map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_rst_gla_stronger_sig_thresh_above_ll,            map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_rst_gla_stronger_sig_thresh_exceeded_enable,     map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_state_wait_time_ns,                              map_iio_debug_set, 2);
MAP_CMD(set_adi_fagc_use_last_lock_level_for_set_gain_enable,         map_iio_debug_set, 2);
MAP_CMD(set_adi_frequency_division_duplex_mode_enable,                map_iio_debug_set, 2);
MAP_CMD(set_adi_gc_adc_large_overload_thresh,                         map_iio_debug_set, 2);
MAP_CMD(set_adi_gc_adc_ovr_sample_size,                               map_iio_debug_set, 2);
MAP_CMD(set_adi_gc_adc_small_overload_thresh,                         map_iio_debug_set, 2);
MAP_CMD(set_adi_gc_dec_pow_measurement_duration,                      map_iio_debug_set, 2);
MAP_CMD(set_adi_gc_dig_gain_enable,                                   map_iio_debug_set, 2);
MAP_CMD(set_adi_gc_lmt_overload_high_thresh,                          map_iio_debug_set, 2);
MAP_CMD(set_adi_gc_lmt_overload_low_thresh,                           map_iio_debug_set, 2);
MAP_CMD(set_adi_gc_low_power_thresh,                                  map_iio_debug_set, 2);
MAP_CMD(set_adi_gc_max_dig_gain,                                      map_iio_debug_set, 2);
MAP_CMD(set_adi_gc_rx1_mode,                                          map_iio_debug_set, 2);
MAP_CMD(set_adi_gc_rx2_mode,                                          map_iio_debug_set, 2);
MAP_CMD(set_adi_mgc_dec_gain_step,                                    map_iio_debug_set, 2);
MAP_CMD(set_adi_mgc_inc_gain_step,                                    map_iio_debug_set, 2);
MAP_CMD(set_adi_mgc_rx1_ctrl_inp_enable,                              map_iio_debug_set, 2);
MAP_CMD(set_adi_mgc_rx2_ctrl_inp_enable,                              map_iio_debug_set, 2);
MAP_CMD(set_adi_mgc_split_table_ctrl_inp_gain_mode,                   map_iio_debug_set, 2);
MAP_CMD(set_adi_rf_rx_bandwidth_hz,                                   map_iio_debug_set, 2);
MAP_CMD(set_adi_rf_tx_bandwidth_hz,                                   map_iio_debug_set, 2);
MAP_CMD(set_adi_rssi_delay,                                           map_iio_debug_set, 2);
MAP_CMD(set_adi_rssi_duration,                                        map_iio_debug_set, 2);
MAP_CMD(set_adi_rssi_restart_mode,                                    map_iio_debug_set, 2);
MAP_CMD(set_adi_rssi_unit_is_rx_samples_enable,                       map_iio_debug_set, 2);
MAP_CMD(set_adi_rssi_wait,                                            map_iio_debug_set, 2);
MAP_CMD(set_adi_rx_rf_port_input_select,                              map_iio_debug_set, 2);
MAP_CMD(set_adi_split_gain_table_mode_enable,                         map_iio_debug_set, 2);
MAP_CMD(set_adi_tdd_skip_vco_cal_enable,                              map_iio_debug_set, 2);
MAP_CMD(set_adi_tdd_use_dual_synth_mode_enable,                       map_iio_debug_set, 2);
MAP_CMD(set_adi_tdd_use_fdd_vco_tables_enable,                        map_iio_debug_set, 2);
MAP_CMD(set_adi_temp_sense_decimation,                                map_iio_debug_set, 2);
MAP_CMD(set_adi_temp_sense_measurement_interval_ms,                   map_iio_debug_set, 2);
MAP_CMD(set_adi_temp_sense_offset_signed,                             map_iio_debug_set, 2);
MAP_CMD(set_adi_temp_sense_periodic_measurement_enable,               map_iio_debug_set, 2);
MAP_CMD(set_adi_tx_attenuation_mdB,                                   map_iio_debug_set, 2);
MAP_CMD(set_adi_tx_rf_port_input_select,                              map_iio_debug_set, 2);
MAP_CMD(set_adi_update_tx_gain_in_alert_enable,                       map_iio_debug_set, 2);
MAP_CMD(set_adi_xo_disable_use_ext_refclk_enable,                     map_iio_debug_set, 2);
MAP_CMD(set_bist_prbs,                                                map_iio_debug_set, 2);
MAP_CMD(set_bist_timing_analysis,                                     map_iio_debug_set, 2);
MAP_CMD(set_bist_tone,                                                map_iio_debug_set, 2);
MAP_CMD(set_direct_reg_access,                                        map_iio_debug_set, 2);
MAP_CMD(set_gaininfo_rx1,                                             map_iio_debug_set, 2);
MAP_CMD(set_gaininfo_rx2,                                             map_iio_debug_set, 2);
MAP_CMD(set_initialize,                                               map_iio_debug_set, 2);
MAP_CMD(set_loopback,                                                 map_iio_debug_set, 2);


static int map_iio_illegal (int argc, const char **argv)
{
	const char *arg0 = argv[0] + 4;
	fprintf(stderr, "%s: cannot set at runtime\n", arg0);
	return -1;
}
MAP_CMD(set_adi_2rx_2tx_mode_enable,                                  map_iio_illegal, 2);
