/** 
 *
 *  \author    Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
 *  \version   v0.0
 *  \copyright Copyright 2014,2015 Silver Bullet Technology
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
 *  vim:ts=4:noexpandtab
 */
#ifndef _INCLUDE_V49_MESSAGE_VITA49_CONTEXT_H_
#define _INCLUDE_V49_MESSAGE_VITA49_CONTEXT_H_
#include <stdint.h>

#include <sbt_common/mbuf.h>
#include <sbt_common/mqueue.h>
#include <sbt_common/growlist.h>


#define V49_CTX_PAYLOAD_FMT_1  0x00000000
#define V49_CTX_PAYLOAD_FMT_2  0x00000000


typedef enum
{
	V49_CTX_IND_BIT_FLD_CHG_ID  = 31, // Context Field Change Indicator
	V49_CTX_IND_BIT_REF_PNT_ID  = 30, // Reference Point Identifier
	V49_CTX_IND_BIT_BANDWIDTH   = 29, // Bandwidth
	V49_CTX_IND_BIT_IF_REF_FREQ = 28, // IF Reference Frequency
	V49_CTX_IND_BIT_RF_REF_FREQ = 27, // RF Reference Frequency
	V49_CTX_IND_BIT_RF_REF_OFFS = 26, // RF Reference Frequency Offset
	V49_CTX_IND_BIT_IF_BND_OFFS = 25, // IF Band Offset
	V49_CTX_IND_BIT_REF_LEVEL   = 24, // Reference Level
	V49_CTX_IND_BIT_GAIN        = 23, // Gain
	V49_CTX_IND_BIT_OVR_RNG_CNT = 22, // Over-range Count
	V49_CTX_IND_BIT_SAMPLE_RATE = 21, // Sample Rate
	V49_CTX_IND_BIT_TSTAMP_ADJ  = 20, // Timestamp Adjustment
	V49_CTX_IND_BIT_TSTAMP_CAL  = 19, // Timestamp Calibration Time
	V49_CTX_IND_BIT_TEMPERATURE = 18, // Temperature
	V49_CTX_IND_BIT_DEVICE_ID   = 17, // Device Identifier
	V49_CTX_IND_BIT_ST_EVT_IND  = 16, // State and Event Indicators
	V49_CTX_IND_BIT_PAYLOAD_FMT = 15, // Data Packet Payload Format
	V49_CTX_IND_BIT_GPS_FMT     = 14, // Formatted GPS Geolocation
	V49_CTX_IND_BIT_INS_FMT     = 13, // Formatted INS Geolocation
	V49_CTX_IND_BIT_EPHEM_ECEF  = 12, // ECEF (Earth-Centered, Earth-Fixed) Ephemeris
	V49_CTX_IND_BIT_EPHEM_REL   = 11, // Relative Ephemeris
	V49_CTX_IND_BIT_EPHEM_ID    = 10, // Ephemeris Reference Identifier
	V49_CTX_IND_BIT_GPS_ASCII   = 9,  // GPS ASCII
	V49_CTX_IND_BIT_ASSOC_LIST  = 8,  // Context Association Lists
}
v49_context_ind_bit_t;


#define V49_CONTEXT_IND_BIT_MASK (0 \
	| (1 << V49_CTX_IND_BIT_FLD_CHG_ID)  \
	| (1 << V49_CTX_IND_BIT_BANDWIDTH)   \
	| (1 << V49_CTX_IND_BIT_IF_REF_FREQ) \
	| (1 << V49_CTX_IND_BIT_RF_REF_FREQ) \
	| (1 << V49_CTX_IND_BIT_REF_LEVEL)   \
	| (1 << V49_CTX_IND_BIT_REF_LEVEL)   \
	| (1 << V49_CTX_IND_BIT_SAMPLE_RATE) \
	| (1 << V49_CTX_IND_BIT_TEMPERATURE) \
)


struct v49_context
{
	// Indicator bitmap
	uint32_t  indicator;

	uint64_t  bandwidth;
	uint64_t  if_ref_freq;
	uint64_t  rf_ref_freq;
	uint32_t  ref_level_q7;
	uint16_t  gain_stage1_q7;
	uint16_t  gain_stage2_q7;
	uint64_t  sample_rate;
	uint32_t  temperature_q6;

};


/** Reset a v49_context struct 
 */
void v49_context_reset (struct v49_context *ctx);

/** Parse a VITA49 packet into the v49_context struct
 * 
 *  \note The data is merged into the v49_context struct, to handle paginated messages.  The
 *        caller is responsible for resetting the struct before the first packet
 */
int v49_context_parse (struct v49_common *v49, struct mbuf *mbuf);

/** Format a v49_context struct into VITA49 packets
 * 
 *  \note The caller should provide the MTU for the connection being used, which will be
 *        used to allocate mbufs and to paginate if necessary.
 */
int v49_context_format (struct v49_common *v49, struct mbuf *mbuf);


const char *v49_context_indicator   (int ind);
const char *v49_context_return_desc (int err);

void v49_context_dump (int level, struct v49_context *ctx);


#endif /* _INCLUDE_V49_MESSAGE_VITA49_CONTEXT_H_ */
/* Ends */
