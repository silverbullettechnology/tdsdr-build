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
#include <ctype.h>
#include <errno.h>

#include "ad9361_hal.h"
#include "ad9361_sysfs.h"
#include "ad9361_private.h"
#include "util.h"


int ad9361_sysfs_get_enum (unsigned dev, const char *root, const char *leaf,
                           const char **word_list, size_t list_size, int *val)
{
	char b[64];
	memset(b, 0, sizeof(b));
	if ( ad9361_sysfs_read(dev, root, leaf, b, sizeof(b) - 1) < 0 )
		return -1;

	char *p, *q;
	for ( p = b; isspace(*p); p++ ) ;
	for ( q = p; *q; q++ ) ;
	while ( q > p && isspace(*(q - 1)) )
		q--;
	
	*q = '\0';

	int n;
	for ( n = 0; n < list_size; n++ )
		if ( word_list[n] && !strcmp(word_list[n], p) )
		{
			*val = n;
			return 0;
		}

	errno = EINVAL;
	return -1;
}

int ad9361_sysfs_set_enum (unsigned dev, const char *root, const char *leaf,
                           const char **word_list, size_t list_size, int val)
{
	if ( val >= list_size || !word_list[val] )
	{
		errno = EINVAL;
		return -1;
	}

	return ad9361_sysfs_printf(dev, root, leaf, "%s\n", word_list[val]);
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


#define ROOT "/sys/bus/iio/devices"
#define get_enum(dev,leaf,list,val) \
	ad9361_sysfs_get_enum(dev,ROOT,leaf,list,(sizeof(list)/sizeof(list[0])),val)
#define set_enum(dev,leaf,list,val) \
	ad9361_sysfs_set_enum(dev,ROOT,leaf,list,(sizeof(list)/sizeof(list[0])),val)
#define get_int(dev,leaf,val)        ad9361_sysfs_get_int(dev,ROOT,leaf,val) 
#define set_int(dev,leaf,val)        ad9361_sysfs_set_int(dev,ROOT,leaf,val) 
#define get_u32(dev,leaf,val)        ad9361_sysfs_get_u32(dev,ROOT,leaf,val) 
#define set_u32(dev,leaf,val)        ad9361_sysfs_set_u32(dev,ROOT,leaf,val) 
#define get_u64(dev,leaf,val)        ad9361_sysfs_get_u64(dev,ROOT,leaf,val) 
#define set_u64(dev,leaf,val)        ad9361_sysfs_set_u64(dev,ROOT,leaf,val) 


/* Path: calib_mode
 */
static const char *ad9361_enum_calib_mode[] =
{
	"auto", "manual", "tx_quad", "rf_dc_offs"
};
int ad9361_get_calib_mode (unsigned dev, int *mode)
{
	return get_enum(dev, "calib_mode", ad9361_enum_calib_mode, mode);
}
int ad9361_set_calib_mode (unsigned dev, int mode)
{
	return set_enum(dev, "calib_mode", ad9361_enum_calib_mode, mode);
}
int ad9361_start_calib (unsigned dev, int mode, int arg)
{
	if ( mode >= (sizeof(ad9361_enum_calib_mode)/sizeof(ad9361_enum_calib_mode[0])) ||
	     !ad9361_enum_calib_mode[mode] )
	{
		errno = EINVAL;
		return -1;
	}

	if ( mode == AD9361_CALIB_MODE_TX_QUAD && arg >= 0 )
		return ad9361_sysfs_printf(dev, ROOT, "calib_mode", "%s %d\n",
		                           ad9361_enum_calib_mode[mode], arg);

	return ad9361_sysfs_printf(dev, ROOT, "calib_mode", "%s\n",
	                           ad9361_enum_calib_mode[mode]);
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
static const char *ad9361_enum_ensm_mode[] =
{
	"sleep", "wait", "alert", "rx", "tx", "fdd", "pinctrl"
};
int ad9361_get_ensm_mode (unsigned dev, int *mode)
{
	return get_enum(dev, "ensm_mode", ad9361_enum_ensm_mode, mode);
}
int ad9361_set_ensm_mode (unsigned dev, int mode)
{
	return set_enum(dev, "ensm_mode", ad9361_enum_ensm_mode, mode);
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

/* Path: filter_fir_config
 */
int ad9361_get_filter_fir_config (unsigned dev, char *dst, size_t max)
{
	memset(dst, 0, max);
	return ad9361_sysfs_read(dev, ROOT, "filter_fir_config", dst, max - 1);
}
int ad9361_set_filter_fir_config (unsigned dev, const char *src)
{
	return ad9361_sysfs_write(dev, ROOT, "filter_fir_config", src, strlen(src));
}
int ad9361_load_filter_fir_config (unsigned dev, const char *path)
{
	FILE *f = fopen(path, "r");
	if ( !f )
		return -1;

	char  b[4096];
	char *p = b;
	char *e = b + sizeof(b);
	while ( p < e && fgets(p, e - p, f) )
		while ( *p )
			p++;

	fclose(f);
	return ad9361_sysfs_write(dev, ROOT, "filter_fir_config", b, p - b);
}

/* Path: in_temp0_input
 */
int ad9361_get_in_temp0_input (unsigned dev, int *val)
{
	return get_int(dev, "in_temp0_input", val);
}

/* Path: in_voltage0_gain_control_mode / in_voltage1_gain_control_mode
 */
static const char *ad9361_enum_gain_control_mode[] =
{
	"manual", "fast_attack", "slow_attack", "hybrid"
};
int ad9361_get_in_voltage_gain_control_mode (unsigned dev, int channel, int *mode)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_gain_control_mode", channel);
	return get_enum(dev, leaf, ad9361_enum_gain_control_mode, mode);
}
int ad9361_set_in_voltage_gain_control_mode (unsigned dev, int channel, int mode)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_gain_control_mode", channel);
	return set_enum(dev, leaf, ad9361_enum_gain_control_mode, mode);
}
int ad9361_get_in_voltage0_gain_control_mode (unsigned dev, int *mode)
{
	return ad9361_get_in_voltage_gain_control_mode(dev, 0, mode);
}
int ad9361_set_in_voltage0_gain_control_mode (unsigned dev, int mode)
{
	return ad9361_set_in_voltage_gain_control_mode(dev, 0, mode);
}
int ad9361_get_in_voltage1_gain_control_mode (unsigned dev, int *mode)
{
	return ad9361_get_in_voltage_gain_control_mode(dev, 1, mode);
}
int ad9361_set_in_voltage1_gain_control_mode (unsigned dev, int mode)
{
	return ad9361_set_in_voltage_gain_control_mode(dev, 1, mode);
}

/* Path: in_voltage0_hardwaregain / in_voltage1_hardwaregain
 */
int ad9361_get_in_voltage_hardwaregain (unsigned dev, int channel, int *val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_hardwaregain", channel);

	char buff[64];
	memset(buff, 0, sizeof(buff));
	if ( ad9361_sysfs_read(dev, ROOT, leaf, buff, sizeof(buff) - 1) < 0 )
		return -1;
	
	*val = dec_to_s_fix(buff, '.', 0);
	return 0;
}
int ad9361_set_in_voltage_hardwaregain (unsigned dev, int channel, int val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_hardwaregain", channel);
	return ad9361_sysfs_printf(dev, ROOT, leaf, "%d.0", val);
}
int ad9361_get_in_voltage0_hardwaregain (unsigned dev, int *val)
{
	return ad9361_get_in_voltage_hardwaregain(dev, 0, val);
}
int ad9361_set_in_voltage0_hardwaregain (unsigned dev, int val)
{
	return ad9361_set_in_voltage_hardwaregain(dev, 0, val);
}
int ad9361_get_in_voltage1_hardwaregain (unsigned dev, int *val)
{
	return ad9361_get_in_voltage_hardwaregain(dev, 1, val);
}
int ad9361_set_in_voltage1_hardwaregain (unsigned dev, int val)
{
	return ad9361_set_in_voltage_hardwaregain(dev, 1, val);
}

/* Path: in_voltage0_rf_port_select
 */
static const char *ad9361_enum_rx_rf_port_select [] =
{
	"A_BALANCED", "B_BALANCED", "C_BALANCED", "A_N", "A_P", "B_N", "B_P", "C_N", "C_P"
};
int ad9361_get_in_voltage_rf_port_select (unsigned dev, int channel, int *port)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_rf_port_select", channel);
	return get_enum(dev, leaf, ad9361_enum_rx_rf_port_select, port);
}
int ad9361_set_in_voltage_rf_port_select (unsigned dev, int channel, int port)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "in_voltage%d_rf_port_select", channel);
	return set_enum(dev, leaf, ad9361_enum_rx_rf_port_select, port);
}
int ad9361_get_in_voltage0_rf_port_select (unsigned dev, int *port)
{
	return ad9361_get_in_voltage_rf_port_select(dev, 0, port);
}
int ad9361_set_in_voltage0_rf_port_select (unsigned dev, int port)
{
	return ad9361_set_in_voltage_rf_port_select(dev, 0, port);
}
int ad9361_get_in_voltage1_rf_port_select (unsigned dev, int *port)
{
	return ad9361_get_in_voltage_rf_port_select(dev, 1, port);
}
int ad9361_set_in_voltage1_rf_port_select (unsigned dev, int port)
{
	return ad9361_set_in_voltage_rf_port_select(dev, 1, port);
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
int ad9361_get_out_voltage_hardwaregain (unsigned dev, int channel, int *val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_hardwaregain", channel);

	char buff[64];
	memset(buff, 0, sizeof(buff));
	if ( ad9361_sysfs_read(dev, ROOT, leaf, buff, sizeof(buff) - 1) < 0 )
		return -1;
	
	*val = dec_to_s_fix(buff, '.', 3);
	return 0;
}
int ad9361_set_out_voltage_hardwaregain (unsigned dev, int channel, int val)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_hardwaregain", channel);
	return ad9361_sysfs_printf(dev, ROOT, leaf, "%d.%03d", val / 1000, abs(val) % 1000);
}
int ad9361_get_out_voltage0_hardwaregain (unsigned dev, int *val)
{
	return ad9361_get_out_voltage_hardwaregain(dev, 0, val);
}
int ad9361_set_out_voltage0_hardwaregain (unsigned dev, int val)
{
	return ad9361_set_out_voltage_hardwaregain(dev, 0, val);
}
int ad9361_get_out_voltage1_hardwaregain (unsigned dev, int *val)
{
	return ad9361_get_out_voltage_hardwaregain(dev, 1, val);
}
int ad9361_set_out_voltage1_hardwaregain (unsigned dev, int val)
{
	return ad9361_set_out_voltage_hardwaregain(dev, 1, val);
}

/* Path: out_voltage0_rf_port_select / out_voltage1_rf_port_select
 */
static const char *ad9361_enum_tx_rf_port_select [] = { "A", "B" };
int ad9361_get_out_voltage_rf_port_select (unsigned dev, int channel, int *port)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_rf_port_select", channel);
	return get_enum(dev, leaf, ad9361_enum_tx_rf_port_select, port);
}
int ad9361_set_out_voltage_rf_port_select (unsigned dev, int channel, int port)
{
	char leaf[64];
	snprintf(leaf, sizeof(leaf), "out_voltage%d_rf_port_select", channel);
	return set_enum(dev, leaf, ad9361_enum_tx_rf_port_select, port);
}
int ad9361_get_out_voltage0_rf_port_select (unsigned dev, int *port)
{
	return ad9361_get_out_voltage_rf_port_select(dev, 0, port);
}
int ad9361_set_out_voltage0_rf_port_select (unsigned dev, int port)
{
	return ad9361_set_out_voltage_rf_port_select(dev, 0, port);
}
int ad9361_get_out_voltage1_rf_port_select (unsigned dev, int *port)
{
	return ad9361_get_out_voltage_rf_port_select(dev, 1, port);
}
int ad9361_set_out_voltage1_rf_port_select (unsigned dev, int port)
{
	return ad9361_set_out_voltage_rf_port_select(dev, 1, port);
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

/* Path: rx_path_rates
 */
int ad9361_get_rx_path_rates (unsigned dev, uint32_t *bbpll, uint32_t *adc,
                              uint32_t *r2, uint32_t *r1,    uint32_t *rf,
                              uint32_t *rxsamp)
{
	uint32_t  u32;

	if ( !bbpll  ) bbpll  = &u32;
	if ( !adc    ) adc    = &u32;
	if ( !r2     ) r2     = &u32;
	if ( !r1     ) r1     = &u32;
	if ( !rf     ) rf     = &u32;
	if ( !rxsamp ) rxsamp = &u32;

	int ret = ad9361_sysfs_scanf(dev, ROOT, "rx_path_rates",
	                             "BBPLL:%lu ADC:%lu R2:%lu R1:%lu RF:%lu RXSAMP:%lu",
	                             bbpll, adc, r2, r1, rf, rxsamp);

	return ret < 0 ? ret : 6 - ret;
}

/* Path: trx_rate_governor
 */
static const char *ad9361_enum_rate_governor[] = { "highest_osr", "nominal" };
int ad9361_get_trx_rate_governor (unsigned dev, int *val)
{
	return get_enum(dev, "trx_rate_governor", ad9361_enum_rate_governor, val);
}
int ad9361_set_trx_rate_governor (unsigned dev, int val)
{
	return set_enum(dev, "trx_rate_governor", ad9361_enum_rate_governor, val);
}

/* Path: tx_path_rates
 */
int ad9361_get_tx_path_rates (unsigned dev, uint32_t *bbpll, uint32_t *dac,
                              uint32_t *t2, uint32_t *t1,    uint32_t *tf,
                              uint32_t *txsamp)
{
	uint32_t  u32;

	if ( !bbpll  ) bbpll  = &u32;
	if ( !dac    ) dac    = &u32;
	if ( !t2     ) t2     = &u32;
	if ( !t1     ) t1     = &u32;
	if ( !tf     ) tf     = &u32;
	if ( !txsamp ) txsamp = &u32;

	int ret = ad9361_sysfs_scanf(dev, ROOT, "tx_path_rates",
	                             "BBPLL:%lu DAC:%lu T2:%lu T1:%lu TF:%lu TXSAMP:%lu",
	                             bbpll, dac, t2, t1, tf, txsamp);

	return ret < 0 ? ret : 6 - ret;
}



