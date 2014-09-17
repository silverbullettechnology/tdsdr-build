/** \file      dsa_channel.h
 *  \brief     interfaces for transfer channels and buffers 
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
#ifndef _INCLUDE_DSA_CHANNEL_H_
#define _INCLUDE_DSA_CHANNEL_H_
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

#include "dsa/format.h"
#include "dsa/sample.h"


#define DC_DEV_AD1  0x01
#define DC_DEV_AD2  0x02
#define DC_DIR_TX   0x04
#define DC_DIR_RX   0x08
#define DC_CHAN_1   0x10
#define DC_CHAN_2   0x20

#define DC_DEV_IDX_TO_MASK(i)  (DC_DEV_AD1 << (i))
#define DC_CHAN_IDX_TO_MASK(i) (DC_CHAN_1  << (i))


// describes the data source / sink for a single channel within a transfer
struct dsa_channel_sxx
{
	struct format *fmt;
	char           loc[0];
};

// describes a single transfer buffer for one ADI part in one direction
struct dsa_channel_xfer
{
	struct dsa_sample_pair *smp;
	size_t                  len;
	uint64_t                exp;
	struct dsa_channel_sxx *src[2];
	struct dsa_channel_sxx *snk[2];
};

// describes transfer buffers for a single transfer event: each event may use either or
// both AD9361 targets, and may transfer in either or both directions
struct dsa_channel_event
{
	struct dsa_channel_xfer *tx[2];
	struct dsa_channel_xfer *rx[2];
};


extern int dsa_channel_ensm_wake;


int  dsa_channel_ident   (const char *argv0);
int  dsa_channel_buffer  (struct dsa_channel_event *evt, int ident, size_t len, int paint);
int  dsa_channel_sxx     (struct dsa_channel_event *evt, int ident, int sxx,
                          struct format *fmt, const char *loc);
void dsa_channel_cleanup (struct dsa_channel_event *evt);

const char *dsa_channel_desc (int ident);
void dsa_channel_event_dump (struct dsa_channel_event *evt);

// Load all setup channels with data - if lsh is nonzero, shift sample data left by 4 bits
int dsa_channel_load (struct dsa_channel_event *evt, int lsh);
int dsa_channel_save (struct dsa_channel_event *evt);

void dsa_channel_calc_exp (struct dsa_channel_event *evt, int reps, int dsxx);
void dsa_format_list(FILE *fp);

// calculates the DMA timeout for the largest allocated buffer at the given sample 
// rate (currently 4MSPS), with a +25% margin.
unsigned long dsa_channel_timeout (struct dsa_channel_event *evt, int msps);

// checks user's requested channel config against setup of the AD9361 chips, returns
// number of buffers which mismatch, or 0 if all allocated buffers/channels are supported.
int dsa_channel_check_active (int ident);

// check the configured directions against the AD9361 ENSM modes; if configured for FDD
// but in sleep, wait, or alert modes the mode will be set to FDD; otherwise if configured
// for TX, RX, or FDD the mode and directions are checked.  returns 0 if transfers can
// proceed, >0 for mismatches, <0 on error
int dsa_channel_check_and_wake (struct dsa_channel_event *evt, int no_change);

// put to sleep AD9361s woken up by dsa_channel_check_and_wake()
int dsa_channel_sleep (void);

unsigned long dsa_channel_ctrl (struct dsa_channel_event *evt, int dev, int old);

#endif // _INCLUDE_DSA_CHANNEL_H_
