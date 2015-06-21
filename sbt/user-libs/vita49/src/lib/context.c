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
#include <stdint.h>
#include <string.h>

#include <sbt_common/log.h>
#include <sbt_common/mbuf.h>
#include <sbt_common/mqueue.h>
#include <sbt_common/growlist.h>

#include <vita49/types.h>
#include <vita49/common.h>
#include <vita49/context.h>


LOG_MODULE_STATIC("vita49_context", LOG_LEVEL_INFO);


/** Reset a context struct 
 */
void v49_context_reset (struct v49_context *ctx)
{
	ENTER("ctx %p", ctx);
	if ( !ctx )
		RETURN_ERRNO(EFAULT);

	ctx->indicator = 0;

	RETURN_ERRNO(0);
}


#define FAIL(err) do{ loc = __LINE__; ret = (err); goto fail; }while(0)


/** Parse a VITA49 packet into the context struct
 * 
 *  \note The data is merged into the context struct, to handle paginated messages.  The
 *        caller is responsible for resetting the struct before the first packet
 */
int v49_context_parse (struct v49_common *v49, struct mbuf *mbuf)
{
	struct v49_context *ctx = &v49->context;
	uint32_t            ignored = 0;
	uint32_t            u32;
	uint64_t            u64;
	int                 loc = -1;
	int                 ret = 0;

	ENTER("v49 %p, mbuf %p", v49, mbuf);
	if ( !v49 || !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( mbuf_get_be32(mbuf, &ctx->indicator) != sizeof(ctx->indicator) )
		FAIL(V49_ERR_SHORT);

	// Check indicator for Context Field Change Indicator 
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_FLD_CHG_ID) )
		ignored |= 1 << V49_CTX_IND_BIT_FLD_CHG_ID;

	// Check indicator for Reference Point Identifier
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_REF_PNT_ID) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t)) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_REF_PNT_ID;
	}

	// Check indicator for Bandwidth
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_BANDWIDTH) )
	{
		if ( mbuf_get_be64(mbuf, &ctx->bandwidth) != sizeof(ctx->bandwidth) )
			FAIL(V49_ERR_SHORT);

		if ( ctx->bandwidth & (1 << 19) )
			ctx->bandwidth += 1 << 20;
		ctx->bandwidth >>= 20;
		LOG_DEBUG("  bandwidth %016llx\n", (unsigned long long)ctx->bandwidth);
	}

	// Check indicator for IF Reference Frequency
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_IF_REF_FREQ) )
	{
		if ( mbuf_get_be64(mbuf, &ctx->if_ref_freq) != sizeof(ctx->if_ref_freq) )
			FAIL(V49_ERR_SHORT);

		if ( ctx->if_ref_freq & (1 << 19) )
			ctx->if_ref_freq += 1 << 20;
		ctx->if_ref_freq >>= 20;
		LOG_DEBUG("  if_ref_freq %016llx\n", (unsigned long long)ctx->if_ref_freq);
	}

	// Check indicator for RF Reference Frequency
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_RF_REF_FREQ) )
	{
		if ( mbuf_get_be64(mbuf, &ctx->rf_ref_freq) != sizeof(ctx->rf_ref_freq) )
			FAIL(V49_ERR_SHORT);

		if ( ctx->rf_ref_freq & (1 << 19) )
			ctx->rf_ref_freq += 1 << 20;
		ctx->rf_ref_freq >>= 20;
		LOG_DEBUG("  rf_ref_freq %016llx\n", (unsigned long long)ctx->rf_ref_freq);
	}

	// Check indicator for RF Reference Frequency Offset
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_RF_REF_OFFS) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint64_t)) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_RF_REF_OFFS;
	}

	// Check indicator for IF Band Offset
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_IF_BND_OFFS) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t) * 2) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_IF_BND_OFFS;
	}

	// Check indicator for Reference Level
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_REF_LEVEL) )
	{
		if ( mbuf_get_be32(mbuf, &ctx->ref_level_q7) != sizeof(ctx->ref_level_q7) )
			FAIL(V49_ERR_SHORT);

		LOG_DEBUG("  ref_level %08x\n", ctx->ref_level_q7);
	}

	// Check indicator for Reference Level
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_REF_LEVEL) )
	{
		if ( mbuf_get_be32(mbuf, &u32) != sizeof(u32) )
			FAIL(V49_ERR_SHORT);

		LOG_DEBUG("  gain %08x\n", u32);
		ctx->gain_stage1_q7 = (u32 & 0x0000FFFF);
		ctx->gain_stage2_q7 = (u32 & 0xFFFF0000) >> 16;
	}

	// Check indicator for Over-range Count
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_OVR_RNG_CNT) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t)) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_OVR_RNG_CNT;
	}

	// Check indicator for Sample Rate
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_SAMPLE_RATE) )
	{
		if ( mbuf_get_be64(mbuf, &ctx->sample_rate) != sizeof(ctx->sample_rate) )
			FAIL(V49_ERR_SHORT);

		if ( ctx->sample_rate & (1 << 19) )
			ctx->sample_rate += 1 << 20;
		ctx->sample_rate >>= 20;
	}

	// Check indicator for Timestamp Adjustment
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_TSTAMP_ADJ) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t) * 2) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_TSTAMP_ADJ;
	}

	// Check indicator for Timestamp Calibration Time
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_TSTAMP_CAL) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t)) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_TSTAMP_CAL;
	}

	// Check indicator for Temperature
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_TEMPERATURE) )
	{
		if ( mbuf_get_be32(mbuf, &ctx->temperature_q6) != sizeof(ctx->temperature_q6) )
			FAIL(V49_ERR_SHORT);
	}

	// Check indicator for Device Identifier
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_DEVICE_ID) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t) * 2) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_DEVICE_ID;
	}

	// Check indicator for State and Event Indicators
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_ST_EVT_IND) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t)) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_ST_EVT_IND;
	}

	// Check indicator for Data Packet Payload Format
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_PAYLOAD_FMT) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t) * 2) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_PAYLOAD_FMT;
	}

	// Check indicator for Formatted GPS Geolocation
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_GPS_FMT) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t) * 11) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_GPS_FMT;
	}

	// Check indicator for Formatted INS Geolocation
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_INS_FMT) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t) * 11) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_INS_FMT;
	}

	// Check indicator for ECEF (Earth-Centered, Earth-Fixed) Ephemeris
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_EPHEM_ECEF) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t) * 13) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_EPHEM_ECEF;
	}

	// Check indicator for Relative Ephemeris
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_EPHEM_REL) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t) * 13) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_EPHEM_REL;
	}

	// Check indicator for Ephemeris Reference Identifier
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_EPHEM_ID) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t)) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_EPHEM_ID;
	}

	// Check indicator for GPS ASCII
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_GPS_ASCII) )
	{
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t)) < 0 )
			FAIL(V49_ERR_SHORT);

		if ( mbuf_get_be32(mbuf, &u32) != sizeof(u32) )
			FAIL(V49_ERR_SHORT);

		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t) * u32) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_GPS_ASCII;
	}

	// Check indicator for Context Association Lists (7.1.5.25)
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_ASSOC_LIST) )
	{
		if ( mbuf_get_be64(mbuf, &u64) != sizeof(u64) )
			FAIL(V49_ERR_SHORT);

		// Asynchronous-Channel List Size
		u32 = u64 & 0x7fff;
		u64 >>= 15;

		// When the “A” bit is set to one, the Asynchronous-Channel Tag List shall be
		// included in the Context Association Lists Section, and the size of the
		// Asynchronous-Channel Tag List shall be the same as the size of the
		// Asynchronous-Channel Context Association List, given by the
		// Asynchronous-Channel List Size field.
		if ( u64 & 1 )
			u32 *= 2;
		u64 >>= 1;

		// Vector-Component List Size
		u32  += u64 & 0xffff;
		u64 >>= 16;

		// System List Size
		u32  += u64 & 0x1FF;
		u64 >>= 16;

		// Source List Size
		u32  += u64 & 0x1FF;

		// skip contents
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t) * u32) < 0 )
			FAIL(V49_ERR_SHORT);

		ignored |= 1 << V49_CTX_IND_BIT_GPS_ASCII;
	}

	if ( ignored )
	{
		ctx->indicator &= ~ignored;
		LOG_DEBUG("%s: ignored indicators: %s\n", __func__,
		          v49_context_indicator(ignored));
	}

	RETURN_ERRNO_VALUE(0, "%d", V49_OK_COMPLETE);

fail:
	LOG_ERROR("%s[%d]: %s\n", __func__, loc, v49_return_desc(ret));
	mbuf_dump(mbuf);
	return ret;
}


/** Format a v49_context struct into VITA49 packets
 * 
 *  \note The caller should provide the MTU for the connection being used, which will be
 *        used to allocate mbufs and to paginate if necessary.
 */
int v49_context_format (struct v49_common *v49, struct mbuf *mbuf)
{
	struct v49_context *ctx = &v49->context;
	uint32_t            u32;
	uint64_t            u64;
	int                 loc = -1;
	int                 ret = V49_OK_COMPLETE;

	ENTER("v49 %p, mbuf %p", v49, mbuf);
	if ( !v49 || !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	// Mute unsupported indicators and write
	if ( ctx->indicator & ~V49_CONTEXT_IND_BIT_MASK )
	{
		LOG_DEBUG("%s: muted indicators: %s\n", __func__,
		          v49_context_indicator(ctx->indicator & ~V49_CONTEXT_IND_BIT_MASK));
		ctx->indicator &= V49_CONTEXT_IND_BIT_MASK;
	}
	if ( mbuf_set_be32(mbuf, ctx->indicator) != sizeof(ctx->indicator) )
		FAIL(V49_ERR_SPACE);

	// Check indicator for Bandwidth
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_BANDWIDTH) )
	{
		u64   = ctx->bandwidth;
		u64 <<= 20;
		if ( mbuf_set_be64(mbuf, u64) != sizeof(u64) )
			FAIL(V49_ERR_SPACE);
		LOG_DEBUG("  bandwidth %016llx\n", (unsigned long long)u64);
	}

	// Check indicator for IF Reference Frequency
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_IF_REF_FREQ) )
	{
		u64   = ctx->if_ref_freq;
		u64 <<= 20;
		if ( mbuf_set_be64(mbuf, u64) != sizeof(u64) )
			FAIL(V49_ERR_SPACE);
		LOG_DEBUG("  if_ref_freq %016llx\n", (unsigned long long)u64);
	}

	// Check indicator for RF Reference Frequency
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_RF_REF_FREQ) )
	{
		u64   = ctx->rf_ref_freq;
		u64 <<= 20;
		if ( mbuf_set_be64(mbuf, u64) != sizeof(u64) )
			FAIL(V49_ERR_SPACE);
		LOG_DEBUG("  rf_ref_freq %016llx\n", (unsigned long long)u64);
	}

	// Check indicator for Reference Level
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_REF_LEVEL) )
	{
		if ( mbuf_set_be32(mbuf, ctx->ref_level_q7) != sizeof(ctx->ref_level_q7) )
			FAIL(V49_ERR_SPACE);
	}

	// Check indicator for Reference Level
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_REF_LEVEL) )
	{
		u32   = ctx->gain_stage2_q7;
		u32 <<= 16;
		u32  |= ctx->gain_stage1_q7;

		if ( mbuf_set_be32(mbuf, u32) != sizeof(u32) )
			FAIL(V49_ERR_SPACE);
	}

	// Check indicator for Sample Rate
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_SAMPLE_RATE) )
	{
		u64   = ctx->sample_rate;
		u64 <<= 20;
		if ( mbuf_set_be64(mbuf, u64) != sizeof(u64) )
			FAIL(V49_ERR_SPACE);
		LOG_DEBUG("  sample_rate %016llx\n", (unsigned long long)u64);
	}

	// Check indicator for Temperature
	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_TEMPERATURE) )
	{
		if ( mbuf_set_be32(mbuf, ctx->temperature_q6) != sizeof(ctx->temperature_q6) )
			FAIL(V49_ERR_SPACE);
		LOG_DEBUG("  temperature %08x\n", ctx->temperature_q6);
	}

	RETURN_ERRNO_VALUE(0, "%d", V49_OK_COMPLETE);

fail:
	LOG_ERROR("%s[%d]: %s\n", __func__, loc, v49_return_desc(ret));
	RETURN_VALUE("%d", ret);
}


/******* Debug / descript code *******/


const char *v49_context_indicator (int ind)
{
	static char  buff[512];

	snprintf(buff, sizeof(buff),
	          "%08x: { %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s }",
	          ind,
	          ind & (1 << V49_CTX_IND_BIT_FLD_CHG_ID  ) ? "FLD_CHG_ID "  : "",
	          ind & (1 << V49_CTX_IND_BIT_REF_PNT_ID  ) ? "REF_PNT_ID "  : "",
	          ind & (1 << V49_CTX_IND_BIT_BANDWIDTH   ) ? "BANDWIDTH "   : "",
	          ind & (1 << V49_CTX_IND_BIT_IF_REF_FREQ ) ? "IF_REF_FREQ " : "",
	          ind & (1 << V49_CTX_IND_BIT_RF_REF_FREQ ) ? "RF_REF_FREQ " : "",
	          ind & (1 << V49_CTX_IND_BIT_RF_REF_OFFS ) ? "RF_REF_OFFS " : "",
	          ind & (1 << V49_CTX_IND_BIT_IF_BND_OFFS ) ? "IF_BND_OFFS " : "",
	          ind & (1 << V49_CTX_IND_BIT_REF_LEVEL   ) ? "REF_LEVEL "   : "",
	          ind & (1 << V49_CTX_IND_BIT_GAIN        ) ? "GAIN "        : "",
	          ind & (1 << V49_CTX_IND_BIT_OVR_RNG_CNT ) ? "OVR_RNG_CNT " : "",
	          ind & (1 << V49_CTX_IND_BIT_SAMPLE_RATE ) ? "SAMPLE_RATE " : "",
	          ind & (1 << V49_CTX_IND_BIT_TSTAMP_ADJ  ) ? "TSTAMP_ADJ "  : "",
	          ind & (1 << V49_CTX_IND_BIT_TSTAMP_CAL  ) ? "TSTAMP_CAL "  : "",
	          ind & (1 << V49_CTX_IND_BIT_TEMPERATURE ) ? "TEMPERATURE " : "",
	          ind & (1 << V49_CTX_IND_BIT_DEVICE_ID   ) ? "DEVICE_ID "   : "",
	          ind & (1 << V49_CTX_IND_BIT_ST_EVT_IND  ) ? "ST_EVT_IND "  : "",
	          ind & (1 << V49_CTX_IND_BIT_PAYLOAD_FMT ) ? "PAYLOAD_FMT " : "",
	          ind & (1 << V49_CTX_IND_BIT_GPS_FMT     ) ? "GPS_FMT "     : "",
	          ind & (1 << V49_CTX_IND_BIT_INS_FMT     ) ? "INS_FMT "     : "",
	          ind & (1 << V49_CTX_IND_BIT_EPHEM_ECEF  ) ? "EPHEM_ECEF "  : "",
	          ind & (1 << V49_CTX_IND_BIT_EPHEM_REL   ) ? "EPHEM_REL "   : "",
	          ind & (1 << V49_CTX_IND_BIT_EPHEM_ID    ) ? "EPHEM_ID "    : "",
	          ind & (1 << V49_CTX_IND_BIT_GPS_ASCII   ) ? "GPS_ASCII "   : "",
	          ind & (1 << V49_CTX_IND_BIT_ASSOC_LIST  ) ? "ASSOC_LIST "  : "");

	return buff;
}


const char *v49_context_return_desc (int err)
{
//	switch ( err )
//	{
//		case V49_PARSE_LIST_SIZE:  return "Packet too short for list";
//		case V49_PARSE_TSTAMP_INT: return "Integer TSF: bad type";
//	}

	static char buff[32];
	snprintf(buff, sizeof(buff), "Unknown(0x%08x)", err);
	return buff;
}


void v49_context_dump (int level, struct v49_context *ctx)
{
	LOG_MESSAGE(level, "context:\n");
	LOG_MESSAGE(level, "indicator  : %s\n",     v49_context_indicator(ctx->indicator));

	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_BANDWIDTH) )
		LOG_MESSAGE(level, "bandwidth  : %llu\n", (unsigned long long)ctx->bandwidth);

	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_IF_REF_FREQ) )
		LOG_MESSAGE(level, "if_ref_freq: %llu\n", (unsigned long long)ctx->if_ref_freq);

	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_RF_REF_FREQ) )
		LOG_MESSAGE(level, "rf_ref_freq: %llu\n", (unsigned long long)ctx->rf_ref_freq);

	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_REF_LEVEL) )
		LOG_MESSAGE(level, "ref_level  : %d.%03u\n", ctx->ref_level_q7 / 128,
		            (1000 * abs(ctx->ref_level_q7 & 127)) >> 7);


	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_REF_LEVEL) )
	{
		LOG_MESSAGE(level, "gain_stage1: %d.%03u\n", ctx->gain_stage1_q7 / 128,
		            (1000 * abs(ctx->gain_stage1_q7 % 128)) >> 7);

		LOG_MESSAGE(level, "gain_stage2: %d.%03u\n", ctx->gain_stage2_q7 / 128,
		            (1000 * abs(ctx->gain_stage2_q7 % 128)) >> 7);
	}

	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_SAMPLE_RATE) )
		LOG_MESSAGE(level, "sample_rate: %llu\n", (unsigned long long)ctx->sample_rate);

	if ( ctx->indicator & (1 << V49_CTX_IND_BIT_TEMPERATURE) )
		LOG_MESSAGE(level, "temperature: %d.%02u\n", ctx->temperature_q6 / 64,
		            (100 * abs(ctx->temperature_q6 % 64)) >> 6);
}


/* Ends */
