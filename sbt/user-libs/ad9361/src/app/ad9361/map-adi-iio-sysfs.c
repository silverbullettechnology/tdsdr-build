/** \file      src/app/ad9361/map-adi-iio-sysfs.c
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


static int map_set_calib_mode (int argc, const char **argv)
{
	MAP_ARG_ENUM(mode, 1, ad9361_enum_calib_mode, "");

	MAP_LIB_CALL(ad9361_set_calib_mode, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_calib_mode, map_set_calib_mode, 2);

static int map_start_calib (int argc, const char **argv)
{
	MAP_ARG_ENUM(mode, 1, ad9361_enum_calib_mode, "");
	MAP_ARG(int, arg,  2, "");

	MAP_LIB_CALL(ad9361_start_calib, ad9361_legacy_dev, mode, arg);

	return 0;
}
MAP_CMD(start_calib,  map_start_calib,  3);

static int map_set_dcxo_tune_coarse (int argc, const char **argv)
{
	MAP_ARG(uint32_t, coarse, 1, "");

	MAP_LIB_CALL(ad9361_set_dcxo_tune_coarse, ad9361_legacy_dev, coarse);

	return 0;
}
MAP_CMD(set_dcxo_tune_coarse, map_set_dcxo_tune_coarse, 2);

static int map_set_dcxo_tune_fine (int argc, const char **argv)
{
	MAP_ARG(uint32_t, fine, 1, "");

	MAP_LIB_CALL(ad9361_set_dcxo_tune_fine, ad9361_legacy_dev, fine);

	return 0;
}
MAP_CMD(set_dcxo_tune_fine, map_set_dcxo_tune_fine, 2);

static int map_set_ensm_mode (int argc, const char **argv)
{
	MAP_ARG_ENUM(mode, 1, ad9361_enum_ensm_mode, "");

	MAP_LIB_CALL(ad9361_set_ensm_mode, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_ensm_mode, map_set_ensm_mode, 2);

static int map_load_filter_fir_config (int argc, const char **argv)
{
	const char  *path = argv[1];
	FILE        *hand;
	MAP_ARG_HELP(path, path, 1, "Path to filter file");

	if ( (hand = fopen(path, "r")) )
		fclose(hand);
	else if ( !(path = path_match(NULL, 0, env_filter_path, argv[1])) )
	{
		fprintf(stderr, "%s: unknown filter\n", argv[1]);
		return -1;
	}

	MAP_LIB_CALL(ad9361_load_filter_fir_config, ad9361_legacy_dev, path);

	return 0;
}
MAP_CMD(set_filter_fir_config,   map_load_filter_fir_config,  2);
MAP_CMD(load_filter_fir_config,  map_load_filter_fir_config,  2);

static int map_set_in_out_voltage_filter_fir_en (int argc, const char **argv)
{
	MAP_ARG(int, val, 1, "");

	MAP_LIB_CALL(ad9361_set_in_out_voltage_filter_fir_en, ad9361_legacy_dev, val);

	return 0;
}
MAP_CMD(set_in_out_voltage_filter_fir_en, map_set_in_out_voltage_filter_fir_en, 2);

static int map_set_in_voltage_gain_control_mode (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	MAP_ARG_ENUM(mode,    2, ad9361_enum_gain_control_mode, "");

	MAP_LIB_CALL(ad9361_set_in_voltage_gain_control_mode, ad9361_legacy_dev, channel, mode);

	return 0;
}
MAP_CMD(set_in_voltage_gain_control_mode, map_set_in_voltage_gain_control_mode, 2);

static int map_set_in_voltage0_gain_control_mode (int argc, const char **argv)
{
	MAP_ARG_ENUM(mode, 1, ad9361_enum_gain_control_mode, "");

	MAP_LIB_CALL(ad9361_set_in_voltage0_gain_control_mode, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_in_voltage0_gain_control_mode, map_set_in_voltage0_gain_control_mode, 2);

static int map_set_in_voltage1_gain_control_mode (int argc, const char **argv)
{
	MAP_ARG_ENUM(mode, 1, ad9361_enum_gain_control_mode, "");

	MAP_LIB_CALL(ad9361_set_in_voltage1_gain_control_mode, ad9361_legacy_dev, mode);

	return 0;
}
MAP_CMD(set_in_voltage1_gain_control_mode, map_set_in_voltage1_gain_control_mode, 2);

static int map_set_in_voltage_hardwaregain (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	MAP_ARG(int, gain,    2, "");

	MAP_LIB_CALL(ad9361_set_in_voltage_hardwaregain, ad9361_legacy_dev, channel, gain);

	return 0;
}
MAP_CMD(set_in_voltage_hardwaregain, map_set_in_voltage_hardwaregain, 2);

static int map_set_in_voltage0_hardwaregain (int argc, const char **argv)
{
	MAP_ARG(int, gain, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage0_hardwaregain, ad9361_legacy_dev, gain);

	return 0;
}
MAP_CMD(set_in_voltage0_hardwaregain, map_set_in_voltage0_hardwaregain, 2);

static int map_set_in_voltage1_hardwaregain (int argc, const char **argv)
{
	MAP_ARG(int, gain, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage1_hardwaregain, ad9361_legacy_dev, gain);

	return 0;
}
MAP_CMD(set_in_voltage1_hardwaregain, map_set_in_voltage1_hardwaregain, 2);

static int map_set_in_voltage_rf_port_select (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	MAP_ARG_ENUM(port,    2, ad9361_enum_rx_rf_port_select, "");

	MAP_LIB_CALL(ad9361_set_in_voltage_rf_port_select, ad9361_legacy_dev, channel, port);

	return 0;
}
MAP_CMD(set_in_voltage_rf_port_select, map_set_in_voltage_rf_port_select, 2);

static int map_set_in_voltage0_rf_port_select (int argc, const char **argv)
{
	MAP_ARG_ENUM(port, 1, ad9361_enum_rx_rf_port_select, "");

	MAP_LIB_CALL(ad9361_set_in_voltage0_rf_port_select, ad9361_legacy_dev, port);

	return 0;
}
MAP_CMD(set_in_voltage0_rf_port_select, map_set_in_voltage0_rf_port_select, 2);

static int map_set_in_voltage1_rf_port_select (int argc, const char **argv)
{
	MAP_ARG_ENUM(port, 1, ad9361_enum_rx_rf_port_select, "");

	MAP_LIB_CALL(ad9361_set_in_voltage1_rf_port_select, ad9361_legacy_dev, port);

	return 0;
}
MAP_CMD(set_in_voltage1_rf_port_select, map_set_in_voltage1_rf_port_select, 2);

static int map_set_in_voltage_rssi (int argc, const char **argv)
{
	MAP_ARG(int,      channel, 1, "");
	MAP_ARG(uint32_t, rssi,    2, "");

	MAP_LIB_CALL(ad9361_set_in_voltage_rssi, ad9361_legacy_dev, channel, rssi);

	return 0;
}
MAP_CMD(set_in_voltage_rssi, map_set_in_voltage_rssi, 2);

static int map_set_in_voltage0_rssi (int argc, const char **argv)
{
	MAP_ARG(uint32_t, rssi, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage0_rssi, ad9361_legacy_dev, rssi);

	return 0;
}
MAP_CMD(set_in_voltage0_rssi, map_set_in_voltage0_rssi, 1);

static int map_set_in_voltage1_rssi (int argc, const char **argv)
{
	MAP_ARG(uint32_t, rssi, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage1_rssi, ad9361_legacy_dev, rssi);

	return 0;
}
MAP_CMD(set_in_voltage1_rssi, map_set_in_voltage1_rssi, 1);

static int map_set_in_voltage2_offset (int argc, const char **argv)
{
	MAP_ARG(int, offset, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage2_offset, ad9361_legacy_dev, offset);

	return 0;
}
MAP_CMD(set_in_voltage2_offset, map_set_in_voltage2_offset, 2);

static int map_set_in_voltage2_raw (int argc, const char **argv)
{
	MAP_ARG(int, raw, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage2_raw, ad9361_legacy_dev, raw);

	return 0;
}
MAP_CMD(set_in_voltage2_raw, map_set_in_voltage2_raw, 2);

static int map_set_in_voltage2_scale (int argc, const char **argv)
{
	MAP_ARG(int, scale, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage2_scale, ad9361_legacy_dev, scale);

	return 0;
}
MAP_CMD(set_in_voltage2_scale, map_set_in_voltage2_scale, 2);

static int map_set_in_voltage_bb_dc_offset_tracking_en (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage_bb_dc_offset_tracking_en, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_in_voltage_bb_dc_offset_tracking_en, map_set_in_voltage_bb_dc_offset_tracking_en, 2);

static int map_set_in_voltage_filter_fir_en (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage_filter_fir_en, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_in_voltage_filter_fir_en, map_set_in_voltage_filter_fir_en, 2);

static int map_set_in_voltage_quadrature_tracking_en (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage_quadrature_tracking_en, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_in_voltage_quadrature_tracking_en, map_set_in_voltage_quadrature_tracking_en, 2);

static int map_set_in_voltage_rf_bandwidth (int argc, const char **argv)
{
	MAP_ARG(uint32_t, bandwidth, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage_rf_bandwidth, ad9361_legacy_dev, bandwidth);

	return 0;
}
MAP_CMD(set_in_voltage_rf_bandwidth, map_set_in_voltage_rf_bandwidth, 2);

static int map_set_in_voltage_rf_dc_offset_tracking_en (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage_rf_dc_offset_tracking_en, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_in_voltage_rf_dc_offset_tracking_en, map_set_in_voltage_rf_dc_offset_tracking_en, 2);

static int map_set_in_voltage_sampling_frequency (int argc, const char **argv)
{
	MAP_ARG(uint32_t, frequency, 1, "");

	MAP_LIB_CALL(ad9361_set_in_voltage_sampling_frequency, ad9361_legacy_dev, frequency);

	return 0;
}
MAP_CMD(set_in_voltage_sampling_frequency, map_set_in_voltage_sampling_frequency, 2);

static int map_set_out_altvoltage0_RX_LO_frequency (int argc, const char **argv)
{
	MAP_ARG(uint64_t, frequency, 1, "");

	MAP_LIB_CALL(ad9361_set_out_altvoltage0_RX_LO_frequency, ad9361_legacy_dev, frequency);

	return 0;
}
MAP_CMD(set_out_altvoltage0_RX_LO_frequency, map_set_out_altvoltage0_RX_LO_frequency, 2);

static int map_set_out_altvoltage1_TX_LO_frequency (int argc, const char **argv)
{
	MAP_ARG(uint64_t, frequency, 1, "");

	MAP_LIB_CALL(ad9361_set_out_altvoltage1_TX_LO_frequency, ad9361_legacy_dev, frequency);

	return 0;
}
MAP_CMD(set_out_altvoltage1_TX_LO_frequency, map_set_out_altvoltage1_TX_LO_frequency, 2);

static long tx_gain (const char *arg)
{
	// convert decimal number to integer x1000
	long g = labs(dec_to_s_fix(arg, '.', 3));

	// if the user specified the number in mdB, divide out the extra x1000
	if ( g > 89750L )
		g /= 1000;

	// library call expects a negative number
	return 0 - g;
}

static int map_set_out_voltage_hardwaregain (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "TX channel, 0 or 1");
	int gain = tx_gain(argv[2]);
	MAP_ARG_HELP(decimal, gain, 2, "Gain in dB (-12.345) or mdB (-12345)");

	MAP_LIB_CALL(ad9361_set_out_voltage_hardwaregain, ad9361_legacy_dev, channel, gain);

	return 0;
}
MAP_CMD(set_out_voltage_hardwaregain, map_set_out_voltage_hardwaregain, 3);

static int map_set_out_voltage0_hardwaregain (int argc, const char **argv)
{
	int gain = tx_gain(argv[1]);
	MAP_ARG_HELP(decimal, gain, 1, "Gain in dB (-12.345) or mdB (-12345)");

	MAP_LIB_CALL(ad9361_set_out_voltage0_hardwaregain, ad9361_legacy_dev, gain);

	return 0;
}
MAP_CMD(set_out_voltage0_hardwaregain, map_set_out_voltage0_hardwaregain, 2);

static int map_set_out_voltage1_hardwaregain (int argc, const char **argv)
{
	int gain = tx_gain(argv[1]);
	MAP_ARG_HELP(decimal, gain, 1, "Gain in dB (-12.345) or mdB (-12345)");

	MAP_LIB_CALL(ad9361_set_out_voltage1_hardwaregain, ad9361_legacy_dev, gain);

	return 0;
}
MAP_CMD(set_out_voltage1_hardwaregain, map_set_out_voltage1_hardwaregain, 2);

static int map_set_out_voltage_rf_port_select (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	MAP_ARG_ENUM(port,    2, ad9361_enum_tx_rf_port_select, "");

	MAP_LIB_CALL(ad9361_set_out_voltage_rf_port_select, ad9361_legacy_dev, channel, port);

	return 0;
}
MAP_CMD(set_out_voltage_rf_port_select, map_set_out_voltage_rf_port_select, 2);

static int map_set_out_voltage0_rf_port_select (int argc, const char **argv)
{
	MAP_ARG_ENUM(port, 1, ad9361_enum_tx_rf_port_select, "");

	MAP_LIB_CALL(ad9361_set_out_voltage0_rf_port_select, ad9361_legacy_dev, port);

	return 0;
}
MAP_CMD(set_out_voltage0_rf_port_select, map_set_out_voltage0_rf_port_select, 2);

static int map_set_out_voltage1_rf_port_select (int argc, const char **argv)
{
	MAP_ARG_ENUM(port, 1, ad9361_enum_tx_rf_port_select, "");

	MAP_LIB_CALL(ad9361_set_out_voltage1_rf_port_select, ad9361_legacy_dev, port);

	return 0;
}
MAP_CMD(set_out_voltage1_rf_port_select, map_set_out_voltage1_rf_port_select, 2);

static int map_set_out_voltage_raw (int argc, const char **argv)
{
	MAP_ARG(int,      channel, 1, "");
	MAP_ARG(uint32_t, raw,     2, "");

	MAP_LIB_CALL(ad9361_set_out_voltage_raw, ad9361_legacy_dev, channel, raw);

	return 0;
}
MAP_CMD(set_out_voltage_raw, map_set_out_voltage_raw, 2);

static int map_set_out_voltage2_raw (int argc, const char **argv)
{
	MAP_ARG(uint32_t, raw, 1, "");

	MAP_LIB_CALL(ad9361_set_out_voltage2_raw, ad9361_legacy_dev, raw);

	return 0;
}
MAP_CMD(set_out_voltage2_raw, map_set_out_voltage2_raw, 2);

static int map_set_out_voltage3_raw (int argc, const char **argv)
{
	MAP_ARG(uint32_t, raw, 1, "");

	MAP_LIB_CALL(ad9361_set_out_voltage3_raw, ad9361_legacy_dev, raw);

	return 0;
}
MAP_CMD(set_out_voltage3_raw, map_set_out_voltage3_raw, 2);

static int map_set_out_voltage_scale (int argc, const char **argv)
{
	MAP_ARG(int,      channel, 1, "");
	MAP_ARG(uint32_t, scale,   2, "");

	MAP_LIB_CALL(ad9361_set_out_voltage_scale, ad9361_legacy_dev, channel, scale);

	return 0;
}
MAP_CMD(set_out_voltage_scale, map_set_out_voltage_scale, 2);

static int map_set_out_voltage2_scale (int argc, const char **argv)
{
	MAP_ARG(uint32_t, scale, 1, "");

	MAP_LIB_CALL(ad9361_set_out_voltage2_scale, ad9361_legacy_dev, scale);

	return 0;
}
MAP_CMD(set_out_voltage2_scale, map_set_out_voltage2_scale, 2);

static int map_set_out_voltage3_scale (int argc, const char **argv)
{
	MAP_ARG(uint32_t, scale, 1, "");

	MAP_LIB_CALL(ad9361_set_out_voltage3_scale, ad9361_legacy_dev, scale);

	return 0;
}
MAP_CMD(set_out_voltage3_scale, map_set_out_voltage3_scale, 2);

static int map_set_out_voltage_filter_fir_en (int argc, const char **argv)
{
	MAP_ARG(int, enable, 1, "");

	MAP_LIB_CALL(ad9361_set_out_voltage_filter_fir_en, ad9361_legacy_dev, enable);

	return 0;
}
MAP_CMD(set_out_voltage_filter_fir_en, map_set_out_voltage_filter_fir_en, 2);

static int map_set_out_voltage_rf_bandwidth (int argc, const char **argv)
{
	MAP_ARG(uint32_t, bandwidth, 1, "");

	MAP_LIB_CALL(ad9361_set_out_voltage_rf_bandwidth, ad9361_legacy_dev, bandwidth);

	return 0;
}
MAP_CMD(set_out_voltage_rf_bandwidth, map_set_out_voltage_rf_bandwidth, 2);

static int map_set_out_voltage_sampling_frequency (int argc, const char **argv)
{
	MAP_ARG(uint32_t, frequency, 1, "");

	MAP_LIB_CALL(ad9361_set_out_voltage_sampling_frequency, ad9361_legacy_dev, frequency);

	return 0;
}
MAP_CMD(set_out_voltage_sampling_frequency, map_set_out_voltage_sampling_frequency, 2);

static int map_set_trx_rate_governor (int argc, const char **argv)
{
	MAP_ARG_ENUM(governor, 1, ad9361_enum_rate_governor, "");

	MAP_LIB_CALL(ad9361_set_trx_rate_governor, ad9361_legacy_dev, governor);

	return 0;
}
MAP_CMD(set_trx_rate_governor, map_set_trx_rate_governor, 2);

static int map_get_calib_mode (int argc, const char **argv)
{
	int  calib_mode;

	MAP_LIB_CALL(ad9361_get_calib_mode, ad9361_legacy_dev, &calib_mode);

	MAP_RES_ENUM(calib_mode, ad9361_enum_calib_mode);
	return 0;
}
MAP_CMD(get_calib_mode, map_get_calib_mode, 1);

static int map_get_dcxo_tune_coarse (int argc, const char **argv)
{
	uint32_t  dcxo_tune_coarse;

	MAP_LIB_CALL(ad9361_get_dcxo_tune_coarse, ad9361_legacy_dev, &dcxo_tune_coarse);

	MAP_RESULT(uint32_t, dcxo_tune_coarse, 1);
	return 0;
}
MAP_CMD(get_dcxo_tune_coarse, map_get_dcxo_tune_coarse, 1);

static int map_get_dcxo_tune_fine (int argc, const char **argv)
{
	uint32_t  dcxo_tune_fine;

	MAP_LIB_CALL(ad9361_get_dcxo_tune_fine, ad9361_legacy_dev, &dcxo_tune_fine);

	MAP_RESULT(uint32_t, dcxo_tune_fine, 1);
	return 0;
}
MAP_CMD(get_dcxo_tune_fine, map_get_dcxo_tune_fine, 1);

static int map_get_ensm_mode (int argc, const char **argv)
{
	int  ensm_mode;

	MAP_LIB_CALL(ad9361_get_ensm_mode, ad9361_legacy_dev, &ensm_mode);

	MAP_RES_ENUM(ensm_mode, ad9361_enum_ensm_mode);
	return 0;
}
MAP_CMD(get_ensm_mode, map_get_ensm_mode, 1);

static int map_get_in_out_voltage_filter_fir_en (int argc, const char **argv)
{
	int  in_out_voltage_filter_fir_en;

	MAP_LIB_CALL(ad9361_get_in_out_voltage_filter_fir_en, ad9361_legacy_dev, &in_out_voltage_filter_fir_en);

	MAP_RESULT(int, in_out_voltage_filter_fir_en, 1);
	return 0;
}
MAP_CMD(get_in_out_voltage_filter_fir_en, map_get_in_out_voltage_filter_fir_en, 1);

static int map_get_filter_fir_config (int argc, const char **argv)
{
	char buff[128];

	MAP_LIB_CALL(ad9361_get_filter_fir_config, ad9361_legacy_dev, buff, sizeof(buff));

	printf("filter_fir_config=\"%s\"\n", trim(buff));
	return 0;
}
MAP_CMD(get_filter_fir_config,  map_get_filter_fir_config,  1);

static int map_get_in_temp0_input (int argc, const char **argv)
{
	int  in_temp0_input;

	MAP_LIB_CALL(ad9361_get_in_temp0_input, ad9361_legacy_dev, &in_temp0_input);

	MAP_RESULT(int, in_temp0_input, 1);
	return 0;
}
MAP_CMD(get_in_temp0_input, map_get_in_temp0_input, 1);

static int map_get_in_voltage_gain_control_mode (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	int  in_voltage_gain_control_mode;

	MAP_LIB_CALL(ad9361_get_in_voltage_gain_control_mode, ad9361_legacy_dev, channel, &in_voltage_gain_control_mode);

	MAP_RES_ENUM(in_voltage_gain_control_mode, ad9361_enum_gain_control_mode);
	return 0;
}
MAP_CMD(get_in_voltage_gain_control_mode, map_get_in_voltage_gain_control_mode, 2);

static int map_get_in_voltage0_gain_control_mode (int argc, const char **argv)
{
	int  in_voltage0_gain_control_mode;

	MAP_LIB_CALL(ad9361_get_in_voltage0_gain_control_mode, ad9361_legacy_dev, &in_voltage0_gain_control_mode);

	MAP_RES_ENUM(in_voltage0_gain_control_mode, ad9361_enum_gain_control_mode);
	return 0;
}
MAP_CMD(get_in_voltage0_gain_control_mode, map_get_in_voltage0_gain_control_mode, 1);

static int map_get_in_voltage1_gain_control_mode (int argc, const char **argv)
{
	int  in_voltage1_gain_control_mode;

	MAP_LIB_CALL(ad9361_get_in_voltage1_gain_control_mode, ad9361_legacy_dev, &in_voltage1_gain_control_mode);

	MAP_RES_ENUM(in_voltage1_gain_control_mode, ad9361_enum_gain_control_mode);
	return 0;
}
MAP_CMD(get_in_voltage1_gain_control_mode, map_get_in_voltage1_gain_control_mode, 1);

static int map_get_in_voltage_hardwaregain (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	int  in_voltage_hardwaregain;

	MAP_LIB_CALL(ad9361_get_in_voltage_hardwaregain, ad9361_legacy_dev, channel, &in_voltage_hardwaregain);

	MAP_RESULT(int, in_voltage_hardwaregain, 1);
	return 0;
}
MAP_CMD(get_in_voltage_hardwaregain, map_get_in_voltage_hardwaregain, 2);

static int map_get_in_voltage0_hardwaregain (int argc, const char **argv)
{
	int  in_voltage0_hardwaregain;

	MAP_LIB_CALL(ad9361_get_in_voltage0_hardwaregain, ad9361_legacy_dev, &in_voltage0_hardwaregain);

	MAP_RESULT(int, in_voltage0_hardwaregain, 1);
	return 0;
}
MAP_CMD(get_in_voltage0_hardwaregain, map_get_in_voltage0_hardwaregain, 1);

static int map_get_in_voltage1_hardwaregain (int argc, const char **argv)
{
	int  in_voltage1_hardwaregain;

	MAP_LIB_CALL(ad9361_get_in_voltage1_hardwaregain, ad9361_legacy_dev, &in_voltage1_hardwaregain);

	MAP_RESULT(int, in_voltage1_hardwaregain, 1);
	return 0;
}
MAP_CMD(get_in_voltage1_hardwaregain, map_get_in_voltage1_hardwaregain, 1);

static int map_get_in_voltage_rf_port_select (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	int  in_voltage_rf_port_select;

	MAP_LIB_CALL(ad9361_get_in_voltage_rf_port_select, ad9361_legacy_dev, channel, &in_voltage_rf_port_select);

	MAP_RES_ENUM(in_voltage_rf_port_select, ad9361_enum_rx_rf_port_select);
	return 0;
}
MAP_CMD(get_in_voltage_rf_port_select, map_get_in_voltage_rf_port_select, 2);

static int map_get_in_voltage0_rf_port_select (int argc, const char **argv)
{
	int  in_voltage0_rf_port_select;

	MAP_LIB_CALL(ad9361_get_in_voltage0_rf_port_select, ad9361_legacy_dev, &in_voltage0_rf_port_select);

	MAP_RES_ENUM(in_voltage0_rf_port_select, ad9361_enum_rx_rf_port_select);
	return 0;
}
MAP_CMD(get_in_voltage0_rf_port_select, map_get_in_voltage0_rf_port_select, 1);

static int map_get_in_voltage1_rf_port_select (int argc, const char **argv)
{
	int  in_voltage1_rf_port_select;

	MAP_LIB_CALL(ad9361_get_in_voltage1_rf_port_select, ad9361_legacy_dev, &in_voltage1_rf_port_select);

	MAP_RES_ENUM(in_voltage1_rf_port_select, ad9361_enum_rx_rf_port_select);
	return 0;
}
MAP_CMD(get_in_voltage1_rf_port_select, map_get_in_voltage1_rf_port_select, 1);

static int map_get_in_voltage_rssi (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	uint32_t  in_voltage_rssi;

	MAP_LIB_CALL(ad9361_get_in_voltage_rssi, ad9361_legacy_dev, channel, &in_voltage_rssi);

	MAP_RESULT(uint32_t, in_voltage_rssi, 1);
	return 0;
}
MAP_CMD(get_in_voltage_rssi, map_get_in_voltage_rssi, 2);

static int map_get_in_voltage0_rssi (int argc, const char **argv)
{
	uint32_t  in_voltage0_rssi;

	MAP_LIB_CALL(ad9361_get_in_voltage0_rssi, ad9361_legacy_dev, &in_voltage0_rssi);

	MAP_RESULT(uint32_t, in_voltage0_rssi, 1);
	return 0;
}
MAP_CMD(get_in_voltage0_rssi, map_get_in_voltage0_rssi, 1);

static int map_get_in_voltage1_rssi (int argc, const char **argv)
{
	uint32_t  in_voltage1_rssi;

	MAP_LIB_CALL(ad9361_get_in_voltage1_rssi, ad9361_legacy_dev, &in_voltage1_rssi);

	MAP_RESULT(uint32_t, in_voltage1_rssi, 1);
	return 0;
}
MAP_CMD(get_in_voltage1_rssi, map_get_in_voltage1_rssi, 1);

static int map_get_in_voltage2_offset (int argc, const char **argv)
{
	int  in_voltage2_offset;

	MAP_LIB_CALL(ad9361_get_in_voltage2_offset, ad9361_legacy_dev, &in_voltage2_offset);

	MAP_RESULT(int, in_voltage2_offset, 1);
	return 0;
}
MAP_CMD(get_in_voltage2_offset, map_get_in_voltage2_offset, 1);

static int map_get_in_voltage2_raw (int argc, const char **argv)
{
	int  in_voltage2_raw;

	MAP_LIB_CALL(ad9361_get_in_voltage2_raw, ad9361_legacy_dev, &in_voltage2_raw);

	MAP_RESULT(int, in_voltage2_raw, 1);
	return 0;
}
MAP_CMD(get_in_voltage2_raw, map_get_in_voltage2_raw, 1);

static int map_get_in_voltage2_scale (int argc, const char **argv)
{
	int  in_voltage2_scale;

	MAP_LIB_CALL(ad9361_get_in_voltage2_scale, ad9361_legacy_dev, &in_voltage2_scale);

	MAP_RESULT(int, in_voltage2_scale, 1);
	return 0;
}
MAP_CMD(get_in_voltage2_scale, map_get_in_voltage2_scale, 1);

static int map_get_in_voltage_bb_dc_offset_tracking_en (int argc, const char **argv)
{
	int  in_voltage_bb_dc_offset_tracking_en;

	MAP_LIB_CALL(ad9361_get_in_voltage_bb_dc_offset_tracking_en, ad9361_legacy_dev, &in_voltage_bb_dc_offset_tracking_en);

	MAP_RESULT(int, in_voltage_bb_dc_offset_tracking_en, 1);
	return 0;
}
MAP_CMD(get_in_voltage_bb_dc_offset_tracking_en, map_get_in_voltage_bb_dc_offset_tracking_en, 1);

static int map_get_in_voltage_filter_fir_en (int argc, const char **argv)
{
	int  in_voltage_filter_fir_en;

	MAP_LIB_CALL(ad9361_get_in_voltage_filter_fir_en, ad9361_legacy_dev, &in_voltage_filter_fir_en);

	MAP_RESULT(int, in_voltage_filter_fir_en, 1);
	return 0;
}
MAP_CMD(get_in_voltage_filter_fir_en, map_get_in_voltage_filter_fir_en, 1);

static int map_get_in_voltage_quadrature_tracking_en (int argc, const char **argv)
{
	int  in_voltage_quadrature_tracking_en;

	MAP_LIB_CALL(ad9361_get_in_voltage_quadrature_tracking_en, ad9361_legacy_dev, &in_voltage_quadrature_tracking_en);

	MAP_RESULT(int, in_voltage_quadrature_tracking_en, 1);
	return 0;
}
MAP_CMD(get_in_voltage_quadrature_tracking_en, map_get_in_voltage_quadrature_tracking_en, 1);

static int map_get_in_voltage_rf_bandwidth (int argc, const char **argv)
{
	uint32_t  in_voltage_rf_bandwidth;

	MAP_LIB_CALL(ad9361_get_in_voltage_rf_bandwidth, ad9361_legacy_dev, &in_voltage_rf_bandwidth);

	MAP_RESULT(uint32_t, in_voltage_rf_bandwidth, 1);
	return 0;
}
MAP_CMD(get_in_voltage_rf_bandwidth, map_get_in_voltage_rf_bandwidth, 1);

static int map_get_in_voltage_rf_dc_offset_tracking_en (int argc, const char **argv)
{
	int  in_voltage_rf_dc_offset_tracking_en;

	MAP_LIB_CALL(ad9361_get_in_voltage_rf_dc_offset_tracking_en, ad9361_legacy_dev, &in_voltage_rf_dc_offset_tracking_en);

	MAP_RESULT(int, in_voltage_rf_dc_offset_tracking_en, 1);
	return 0;
}
MAP_CMD(get_in_voltage_rf_dc_offset_tracking_en, map_get_in_voltage_rf_dc_offset_tracking_en, 1);

static int map_get_in_voltage_sampling_frequency (int argc, const char **argv)
{
	uint32_t  in_voltage_sampling_frequency;

	MAP_LIB_CALL(ad9361_get_in_voltage_sampling_frequency, ad9361_legacy_dev, &in_voltage_sampling_frequency);

	MAP_RESULT(uint32_t, in_voltage_sampling_frequency, 1);
	return 0;
}
MAP_CMD(get_in_voltage_sampling_frequency, map_get_in_voltage_sampling_frequency, 1);

static int map_get_out_altvoltage0_RX_LO_frequency (int argc, const char **argv)
{
	uint64_t  out_altvoltage0_RX_LO_frequency;

	MAP_LIB_CALL(ad9361_get_out_altvoltage0_RX_LO_frequency, ad9361_legacy_dev, &out_altvoltage0_RX_LO_frequency);

	MAP_RESULT(uint64_t, out_altvoltage0_RX_LO_frequency, 1);
	return 0;
}
MAP_CMD(get_out_altvoltage0_RX_LO_frequency, map_get_out_altvoltage0_RX_LO_frequency, 1);

static int map_get_out_altvoltage1_TX_LO_frequency (int argc, const char **argv)
{
	uint64_t  out_altvoltage1_TX_LO_frequency;

	MAP_LIB_CALL(ad9361_get_out_altvoltage1_TX_LO_frequency, ad9361_legacy_dev, &out_altvoltage1_TX_LO_frequency);

	MAP_RESULT(uint64_t, out_altvoltage1_TX_LO_frequency, 1);
	return 0;
}
MAP_CMD(get_out_altvoltage1_TX_LO_frequency, map_get_out_altvoltage1_TX_LO_frequency, 1);

static int map_get_out_voltage_hardwaregain (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	int  out_voltage_hardwaregain;

	MAP_LIB_CALL(ad9361_get_out_voltage_hardwaregain, ad9361_legacy_dev, channel, &out_voltage_hardwaregain);

	printf("out_voltage_hardwaregain=\"%d.%03d\"\n",
	       out_voltage_hardwaregain / 1000, abs(out_voltage_hardwaregain) % 1000);
	return 0;
}
MAP_CMD(get_out_voltage_hardwaregain, map_get_out_voltage_hardwaregain, 2);

static int map_get_out_voltage0_hardwaregain (int argc, const char **argv)
{
	int  out_voltage0_hardwaregain;

	MAP_LIB_CALL(ad9361_get_out_voltage0_hardwaregain, ad9361_legacy_dev, &out_voltage0_hardwaregain);

	printf("out_voltage0_hardwaregain=\"%d.%03d\"\n",
	       out_voltage0_hardwaregain / 1000, abs(out_voltage0_hardwaregain) % 1000);
	return 0;
}
MAP_CMD(get_out_voltage0_hardwaregain, map_get_out_voltage0_hardwaregain, 1);

static int map_get_out_voltage1_hardwaregain (int argc, const char **argv)
{
	int  out_voltage1_hardwaregain;

	MAP_LIB_CALL(ad9361_get_out_voltage1_hardwaregain, ad9361_legacy_dev, &out_voltage1_hardwaregain);

	printf("out_voltage1_hardwaregain=\"%d.%03d\"\n",
	       out_voltage1_hardwaregain / 1000, abs(out_voltage1_hardwaregain) % 1000);
	return 0;
}
MAP_CMD(get_out_voltage1_hardwaregain, map_get_out_voltage1_hardwaregain, 1);

static int map_get_out_voltage_rf_port_select (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	int  out_voltage_rf_port_select;

	MAP_LIB_CALL(ad9361_get_out_voltage_rf_port_select, ad9361_legacy_dev, channel, &out_voltage_rf_port_select);

	MAP_RES_ENUM(out_voltage_rf_port_select, ad9361_enum_tx_rf_port_select);
	return 0;
}
MAP_CMD(get_out_voltage_rf_port_select, map_get_out_voltage_rf_port_select, 2);

static int map_get_out_voltage0_rf_port_select (int argc, const char **argv)
{
	int  out_voltage0_rf_port_select;

	MAP_LIB_CALL(ad9361_get_out_voltage0_rf_port_select, ad9361_legacy_dev, &out_voltage0_rf_port_select);

	MAP_RES_ENUM(out_voltage0_rf_port_select, ad9361_enum_tx_rf_port_select);
	return 0;
}
MAP_CMD(get_out_voltage0_rf_port_select, map_get_out_voltage0_rf_port_select, 1);

static int map_get_out_voltage1_rf_port_select (int argc, const char **argv)
{
	int  out_voltage1_rf_port_select;

	MAP_LIB_CALL(ad9361_get_out_voltage1_rf_port_select, ad9361_legacy_dev, &out_voltage1_rf_port_select);

	MAP_RES_ENUM(out_voltage1_rf_port_select, ad9361_enum_tx_rf_port_select);
	return 0;
}
MAP_CMD(get_out_voltage1_rf_port_select, map_get_out_voltage1_rf_port_select, 1);

static int map_get_out_voltage_raw (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	uint32_t  out_voltage_raw;

	MAP_LIB_CALL(ad9361_get_out_voltage_raw, ad9361_legacy_dev, channel, &out_voltage_raw);

	MAP_RESULT(uint32_t, out_voltage_raw, 1);
	return 0;
}
MAP_CMD(get_out_voltage_raw, map_get_out_voltage_raw, 2);

static int map_get_out_voltage2_raw (int argc, const char **argv)
{
	uint32_t  out_voltage2_raw;

	MAP_LIB_CALL(ad9361_get_out_voltage2_raw, ad9361_legacy_dev, &out_voltage2_raw);

	MAP_RESULT(uint32_t, out_voltage2_raw, 1);
	return 0;
}
MAP_CMD(get_out_voltage2_raw, map_get_out_voltage2_raw, 1);

static int map_get_out_voltage3_raw (int argc, const char **argv)
{
	uint32_t  out_voltage3_raw;

	MAP_LIB_CALL(ad9361_get_out_voltage3_raw, ad9361_legacy_dev, &out_voltage3_raw);

	MAP_RESULT(uint32_t, out_voltage3_raw, 1);
	return 0;
}
MAP_CMD(get_out_voltage3_raw, map_get_out_voltage3_raw, 1);

static int map_get_out_voltage_scale (int argc, const char **argv)
{
	MAP_ARG(int, channel, 1, "");
	uint32_t  out_voltage_scale;

	MAP_LIB_CALL(ad9361_get_out_voltage_scale, ad9361_legacy_dev, channel, &out_voltage_scale);

	MAP_RESULT(uint32_t, out_voltage_scale, 1);
	return 0;
}
MAP_CMD(get_out_voltage_scale, map_get_out_voltage_scale, 2);

static int map_get_out_voltage2_scale (int argc, const char **argv)
{
	uint32_t  out_voltage2_scale;

	MAP_LIB_CALL(ad9361_get_out_voltage2_scale, ad9361_legacy_dev, &out_voltage2_scale);

	MAP_RESULT(uint32_t, out_voltage2_scale, 1);
	return 0;
}
MAP_CMD(get_out_voltage2_scale, map_get_out_voltage2_scale, 1);

static int map_get_out_voltage3_scale (int argc, const char **argv)
{
	uint32_t  out_voltage3_scale;

	MAP_LIB_CALL(ad9361_get_out_voltage3_scale, ad9361_legacy_dev, &out_voltage3_scale);

	MAP_RESULT(uint32_t, out_voltage3_scale, 1);
	return 0;
}
MAP_CMD(get_out_voltage3_scale, map_get_out_voltage3_scale, 1);

static int map_get_out_voltage_filter_fir_en (int argc, const char **argv)
{
	int  out_voltage_filter_fir_en;

	MAP_LIB_CALL(ad9361_get_out_voltage_filter_fir_en, ad9361_legacy_dev, &out_voltage_filter_fir_en);

	MAP_RESULT(int, out_voltage_filter_fir_en, 1);
	return 0;
}
MAP_CMD(get_out_voltage_filter_fir_en, map_get_out_voltage_filter_fir_en, 1);

static int map_get_out_voltage_rf_bandwidth (int argc, const char **argv)
{
	uint32_t  out_voltage_rf_bandwidth;

	MAP_LIB_CALL(ad9361_get_out_voltage_rf_bandwidth, ad9361_legacy_dev, &out_voltage_rf_bandwidth);

	MAP_RESULT(uint32_t, out_voltage_rf_bandwidth, 1);
	return 0;
}
MAP_CMD(get_out_voltage_rf_bandwidth, map_get_out_voltage_rf_bandwidth, 1);

static int map_get_out_voltage_sampling_frequency (int argc, const char **argv)
{
	uint32_t  out_voltage_sampling_frequency;

	MAP_LIB_CALL(ad9361_get_out_voltage_sampling_frequency, ad9361_legacy_dev, &out_voltage_sampling_frequency);

	MAP_RESULT(uint32_t, out_voltage_sampling_frequency, 1);
	return 0;
}
MAP_CMD(get_out_voltage_sampling_frequency, map_get_out_voltage_sampling_frequency, 1);

static int map_get_trx_rate_governor (int argc, const char **argv)
{
	int  trx_rate_governor;

	MAP_LIB_CALL(ad9361_get_trx_rate_governor, ad9361_legacy_dev, &trx_rate_governor);

	MAP_RES_ENUM(trx_rate_governor, ad9361_enum_rate_governor);
	return 0;
}
MAP_CMD(get_trx_rate_governor, map_get_trx_rate_governor, 1);

static int map_get_rx_path_rates (int argc, const char **argv)
{
	uint32_t  bbpll;
	uint32_t  adc;
	uint32_t  r2;
	uint32_t  r1;
	uint32_t  rf;
	uint32_t  rxsamp;

	MAP_LIB_CALL(ad9361_get_rx_path_rates, ad9361_legacy_dev, &bbpll, &adc, &r2, &r1, &rf, &rxsamp);

	MAP_RESULT(uint32_t, bbpll,  1);
	MAP_RESULT(uint32_t, adc,    1);
	MAP_RESULT(uint32_t, r2,     1);
	MAP_RESULT(uint32_t, r1,     1);
	MAP_RESULT(uint32_t, rf,     1);
	MAP_RESULT(uint32_t, rxsamp, 1);
	return 0;
}
MAP_CMD(get_rx_path_rates, map_get_rx_path_rates, 1);

static int map_get_tx_path_rates (int argc, const char **argv)
{
	uint32_t  bbpll;
	uint32_t  dac;
	uint32_t  t2;
	uint32_t  t1;
	uint32_t  tf;
	uint32_t  txsamp;

	MAP_LIB_CALL(ad9361_get_tx_path_rates, ad9361_legacy_dev, &bbpll, &dac, &t2, &t1, &tf, &txsamp);

	MAP_RESULT(uint32_t, bbpll,  1);
	MAP_RESULT(uint32_t, dac,    1);
	MAP_RESULT(uint32_t, t2,     1);
	MAP_RESULT(uint32_t, t1,     1);
	MAP_RESULT(uint32_t, tf,     1);
	MAP_RESULT(uint32_t, txsamp, 1);
	return 0;
}
MAP_CMD(get_tx_path_rates, map_get_tx_path_rates, 1);

