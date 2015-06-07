/** \file      dsa_channel.c
 *  \brief     implementation of transfer channels and buffers 
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
#include <stddef.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include <ad9361.h>
#include <pd_main.h>
#include <fifo_dev.h>

#include "dsa/main.h"
#include "dsa/sample.h"
#include "dsa/channel.h"
#include "dsm/dsm.h"
#include "pipe/access.h"
#include "common/common.h"

#include "common/log.h"
LOG_MODULE_STATIC("channel", LOG_LEVEL_INFO);


int dsa_channel_ensm_wake = 0;


const char *dsa_channel_desc (int ident)
{
	static char buff[64];

	snprintf(buff, sizeof(buff), "%02x { %s%s%s%s%s%s}", ident,
	         ident & DC_DEV_AD1 ? "DC_DEV_AD1 " : "",
	         ident & DC_DEV_AD2 ? "DC_DEV_AD2 " : "",
	         ident & DC_DIR_TX  ? "DC_DIR_TX "  : "",
	         ident & DC_DIR_RX  ? "DC_DIR_RX "  : "",
	         ident & DC_CHAN_1  ? "DC_CHAN_1 "  : "",
	         ident & DC_CHAN_2  ? "DC_CHAN_2 "  : "");

	return buff;
}


// Parse and validate a chip/direction/channel into an ident bitmask:
//   AD 12 T 12
//   ^^ ^^ ^ ^^-- Channel: 1 or 2 for single, 12 or omit for both
//   |  |  `----- Direction: T for TX, R for RX, never both
//   |  `-------- ADI device: 1 or 2 for single, 12 or omit for both
//   `----------- "AD" prefix: required
int dsa_channel_ident (const char *argv0)
{
	const char *ptr = argv0 + 2;
	int         ret = DC_DEV_AD1|DC_DEV_AD2|DC_CHAN_1|DC_CHAN_2;

	// if the string starts with "AD" in any case it's parsed, otherwise it's passed over
	errno = 0;
	if ( tolower(argv0[0]) != 'a' || tolower(argv0[1]) != 'd' )
	{
		// allow DS for data source/sink module instead, but not mixed with AD ident
		if ( tolower(argv0[0]) != 'd' || tolower(argv0[1]) != 's' )
			return 0;
		dsa_dsxx = 1;
	}
	else if ( dsa_dsxx )
		return -1;

	// skip punctation
	while ( *ptr && !isalnum(*ptr) )
		ptr++;

	// device specifier digits after the "AD" are optional: if present then clear default
	// "both" default and check digits.  "AD12" is legal and equivalent to the default.
	if ( isdigit(*ptr) )
	{
		ret &= ~(DC_DEV_AD1|DC_DEV_AD2);
		while ( isdigit(*ptr) )
		{
			switch ( *ptr )
			{
				case '1':
					ret |= DC_DEV_AD1;
					break;
				case '2':
					ret |= DC_DEV_AD2;
					break;
				default:
					errno = EINVAL;
					return -1;
			}

			// skip punctation
			do
				ptr++;
			while ( *ptr && !isalnum(*ptr) );
		}
	}

	// direction char follows and is mandatory
	switch ( tolower(*ptr) )
	{
		case 't': ret |= DC_DIR_TX; break;
		case 'r': ret |= DC_DIR_RX; break;
		default:
			errno = EINVAL;
			return -1;
	}

	// skip punctation
	do
		ptr++;
	while ( *ptr && !isalnum(*ptr) );

	// channel specifier digits after the "T" or "R" are optional: if present then clear
	// default "both" default and check digits.  "T12" is legal and equivalent to the
	// default.
	if ( isdigit(*ptr) )
	{
		ret &= ~(DC_CHAN_1|DC_CHAN_2);
		while ( isdigit(*ptr) )
		{
			switch ( *ptr )
			{
				case '1':
					ret |= DC_CHAN_1;
					break;
				case '2':
					ret |= DC_CHAN_2;
					break;
				default:
					errno = EINVAL;
					return -1;
			}

			// skip punctation
			do
				ptr++;
			while ( *ptr && !isalnum(*ptr) );
		}
	}

	// catch garbage after direction and/or channel numbers
	if ( *ptr && isalpha(*ptr) )
		return -1;

	return ret;
}


// Maps a user-specified bitmask parsed with dsa_channel_ident() onto the appropriate
// dsa_channel_xfer pointer within the dsa_channel_event struct.  Returns a pointer to the
// pointer, so the caller can set/clear the pointer.
static struct dsa_channel_xfer **evt_to_xfer (struct dsa_channel_event *evt, int mask)
{
	switch ( mask & (DC_DEV_AD1|DC_DEV_AD2|DC_DIR_TX|DC_DIR_RX) )
	{
		case DC_DIR_TX|DC_DEV_AD1: return &evt->tx[0];
		case DC_DIR_TX|DC_DEV_AD2: return &evt->tx[1];
		case DC_DIR_RX|DC_DEV_AD1: return &evt->rx[0];
		case DC_DIR_RX|DC_DEV_AD2: return &evt->rx[1];
	}

	return NULL;
}


static int realloc_buffer (struct dsa_channel_xfer *xfer, size_t len)
{
	long  page;

	// need page size for posix_memalign()
	if ( (page = sysconf(_SC_PAGESIZE)) < 0 )
	{
		LOG_ERROR("Failed to get system page size: %s\n", strerror(errno));
		return -1;
	}

	if ( !len || len == xfer->len )
	{
		LOG_DEBUG("Requested size is %zu, current %zu, no action taken\n",
		          len, xfer->len);
		return 0;
	}

	// free old buffer with reusable dsm_user_free() function
	if ( xfer->smp )
		dsm_user_free(xfer->smp, xfer->len);

	// allocate with reusable dsm_user_alloc() function
	xfer->len = 0;
	if ( !(xfer->smp = dsm_user_alloc(len)) )
	{
		LOG_ERROR("Failed to dsm_user_alloc() %zu samples aligned to %lu: %s\n",
		          len, page, strerror(errno));
		return -1;
	}
	xfer->len = len;

	LOG_DEBUG("Buffer allocated successfully\n");
	return len;
}


// Sets up the relevant DMA buffers based on the user-specified bits.  Resizes and
// repaints as necessary.  Returns 0 on success, <0 on error.
int dsa_channel_buffer (struct dsa_channel_event *evt, int ident, size_t len, int paint)
{
	struct dsa_channel_xfer **xfer;
	int                       dev;
	int                       dir;


	for ( dev = DC_DEV_AD1; dev <= DC_DEV_AD2; dev <<= 1 )
		for ( dir = DC_DIR_TX; dir <= DC_DIR_RX; dir <<= 1 )
			if ( (xfer = evt_to_xfer(evt, ident & (dev|dir))) )
			{
				// allocate buffer management struct 
				if ( ! *xfer )
					*xfer = calloc(1, sizeof(struct dsa_channel_xfer));
				if ( ! *xfer )
					return -1;

				// (re)allocate sample buffer
				if ( realloc_buffer(*xfer, len) < 0 )
					return -1;

				// paint buffer if requested and allocated
				if ( paint && (*xfer)->smp && (ident & (DC_CHAN_1|DC_CHAN_2)) )
				{
					struct dsa_sample_pair *samp = (*xfer)->smp;
					size_t                  left = len;

					// use memset to paint both channels at once
					if ( (ident & (DC_CHAN_1|DC_CHAN_2)) == (DC_CHAN_1|DC_CHAN_2) )
						memset(samp, 0xFE, len * sizeof(struct dsa_sample_pair));

					// use for loop to paint only channel 1
					else if ( ident & DC_CHAN_1 )
						while ( left-- )
						{
							samp->ch[0].i = 0xFD;
							samp->ch[0].q = 0xFD;
							samp++;
						}

					// use for loop to paint only channel 2
					else
						while ( left-- )
						{
							samp->ch[1].i = 0xFC;
							samp->ch[1].q = 0xFC;
							samp++;
						}
				}
			}

	return 0;
}


// Maps a bitmask containig user-specified channel bits, OR'd with a TX/RX bits selecting
// the src or snk pointer, onto the appropriate dsa_channel_sxx within the
// dsa_channel_xfer struct.  Note the TX/RX bits may be distinct from the TX/RX bits in
// the user ident parsed with dsa_channel_ident(), in unusual cases where the user wants
// to fill an RX buffer before DMA, or write out a TX buffer after, to check for
// completion/corruption.  Returns a pointer to the pointer, so the caller can set/clear
// the pointer.
static struct dsa_channel_sxx **xfer_to_sxx (struct dsa_channel_xfer *xfer, int mask)
{
	switch ( mask & (DC_CHAN_1|DC_CHAN_2|DC_DIR_TX|DC_DIR_RX) )
	{
		case DC_DIR_TX|DC_CHAN_1: return &xfer->src[0];
		case DC_DIR_TX|DC_CHAN_2: return &xfer->src[1];
		case DC_DIR_RX|DC_CHAN_1: return &xfer->snk[0];
		case DC_DIR_RX|DC_CHAN_2: return &xfer->snk[1];
	}

	return NULL;
}

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
// sizes, allocates, initializes, and returns a dsa_channel_sxx for the given user
// bitmask, format, and location string.  TODO is location expansion with printf-style
// format characters, which will complicate sizing the string and thus the buffer.
static struct dsa_channel_sxx *sxx_int (struct format *fmt, const char *loc,
                                        int dev, int dir, int sxx, int chan)
{
	struct dsa_channel_sxx *ret;
	const char             *s = loc;
	char                   *d;
	char                   *e;
	char                   *f;

	if ( !(ret = malloc(offsetof(struct dsa_channel_sxx, loc) + PATH_MAX)) )
		return NULL;

	ret->fmt = fmt;

	d = ret->loc;
	e = ret->loc + PATH_MAX - 1;
	s = loc;
	while ( *s && d < e )
	{
		// no (more) printfs left: copy clear text
		if ( !(f = strchr(s, '%')) )
		{
			strncpy(d, s, e - d);
			*e = '\0';
			break;
		}

		// straight copy of clear text
		if ( f > s )
		{
			strncpy(d, s, min(f - s, e - d));
			d += f - s;
		}

		// expand known % escapes, pass-through unrecognized
		s = f + 1;
		switch ( *s )
		{
			case '%': // literal % sign
				*d++ = '%';
				break;

			case 'd':
			case 'D': 
				*d = 'x';
				if ( dir == DC_DIR_TX ) *d = 't';
				if ( dir == DC_DIR_RX ) *d = 'r';
				if ( isupper(*s) )
					*d = toupper(*d);
				d++;
				break;

			case 's': 
			case 'S': 
				*d = 'x';
				if ( sxx == DC_DIR_TX ) *d = 's';
				if ( sxx == DC_DIR_RX ) *d = 'd';
				if ( isupper(*s) )
					*d = toupper(*d);
				d++;
				break;

			case 'a':
				*d = 'x';
				if ( dev == DC_DEV_AD1 ) *d = '1';
				if ( dev == DC_DEV_AD2 ) *d = '2';
				d++;
				break;

			case 'c':
				*d = 'x';
				if ( chan == DC_CHAN_1 ) *d = '1';
				if ( chan == DC_CHAN_2 ) *d = '2';
				d++;
				break;

			default: // pass-through anything unrecognized
				*d++ = *f;
				*d++ = *s;
				break;
		}
		*d = '\0';
		s++;
	}

	return ret;
}


// sets up the data source/sink structs for each combination of device, direction, and
// channel specified in the ident bitmask, parsed with dsa_channel_ident().  mask should
// usually be the same as ident, so a TX buffer gets a source spec and an RX gets a sink
// spec, but may be different to allow for unusual cases (see xfer_to_sxx above).  Returns
// 0 on success, <0 on error.
int dsa_channel_sxx (struct dsa_channel_event *evt, int ident, int mask,
                     struct format *fmt, const char *loc)
{
	struct dsa_channel_xfer **xfer;
	struct dsa_channel_sxx  **sxx;
	int                       dev;
	int                       dir1;
	int                       dir2;
	int                       chan;

	mask &= DC_DIR_TX|DC_DIR_RX;
	mask |= ident & (DC_CHAN_1|DC_CHAN_2);

	for ( dev = DC_DEV_AD1; dev <= DC_DEV_AD2; dev <<= 1 )
		for ( dir1 = DC_DIR_TX; dir1 <= DC_DIR_RX; dir1 <<= 1 )
			if ( (xfer = evt_to_xfer(evt, ident & (dev|dir1))) && *xfer )
				for ( chan = DC_CHAN_1; chan <= DC_CHAN_2; chan <<= 1 )
					for ( dir2 = DC_DIR_TX; dir2 <= DC_DIR_RX; dir2 <<= 1 )
						if ( (sxx = xfer_to_sxx(*xfer, mask & (chan|dir2))) )
						{
							free(*sxx);
							if ( !(*sxx = sxx_int(fmt, loc, dev, dir1, dir2, chan)) )
								return -1;
						}

	return 0;
}


// cleans up evt, freeing all dynamically-allocated buffers and pointers.  evt itself is
// not freed, just the objects within it allocated in these functions.
void dsa_channel_cleanup (struct dsa_channel_event *evt)
{
	struct dsa_channel_xfer **xfer;
	struct dsa_channel_sxx  **sxx;
	int                       dev;
	int                       dir1;
	int                       dir2;
	int                       chan;

	for ( dev = DC_DEV_AD1; dev <= DC_DEV_AD2; dev <<= 1 )
		for ( dir1 = DC_DIR_TX; dir1 <= DC_DIR_RX; dir1 <<= 1 )
		{
			xfer = evt_to_xfer(evt, dev|dir1);
			assert(xfer);

			if ( *xfer )
			{
				if ( (*xfer)->smp )
					dsm_user_free( (*xfer)->smp, (*xfer)->len );

				for ( chan = DC_CHAN_1; chan <= DC_CHAN_2; chan <<= 1 )
					for ( dir2 = DC_DIR_TX; dir2 <= DC_DIR_RX; dir2 <<= 1 )
					{
						sxx = xfer_to_sxx(*xfer, chan|dir2);
						assert(sxx);

						free(*sxx);
					}

				free(*xfer);
				*xfer = NULL;
			}

		}
}


// calculates the DMA timeout for the largest allocated buffer at the given sample 
// rate (currently 4MSPS), with a +25% margin.
unsigned long dsa_channel_timeout (struct dsa_channel_event *evt)
{
	struct dsa_channel_xfer **xfer;
	unsigned long             time;
	unsigned long             max  = 0;
	uint32_t                  rate;
	int                       dev;
	int                       dir;

	LOG_DEBUG("%s(evt)\n", __func__);
	for ( dev = DC_DEV_AD1; dev <= DC_DEV_AD2; dev <<= 1 )
		for ( dir = DC_DIR_TX; dir <= DC_DIR_RX; dir <<= 1 )
			if ( (xfer = evt_to_xfer(evt, (dev|dir))) && *xfer )
			{
				// get sample-rate using library calls based on device and direction
				switch ( (dev|dir) & (DC_DEV_AD1|DC_DEV_AD2|DC_DIR_TX|DC_DIR_RX) )
				{
					case DC_DIR_TX|DC_DEV_AD1: 
						if ( ad9361_get_out_voltage_sampling_frequency(0, &rate) < 0 )
							return -1;
						break;

					case DC_DIR_TX|DC_DEV_AD2: 
						if ( ad9361_get_out_voltage_sampling_frequency(1, &rate) < 0 )
							return -1;
						break;

					case DC_DIR_RX|DC_DEV_AD1: 
						if ( ad9361_get_in_voltage_sampling_frequency(0, &rate) < 0 )
							return -1;
						break;

					case DC_DIR_RX|DC_DEV_AD2: 
						if ( ad9361_get_in_voltage_sampling_frequency(1, &rate) < 0 )
							return -1;
						break;

					default:
						return -1;
				}

				// rate in Hz, divide by 100 here makes output into jiffies and add 25%
				rate /= 100;
				if ( rate < 1 )
					rate = 1;
				time  = (*xfer)->len / rate;
				time += (time >> 2);
				LOG_DEBUG("%s: %zu samples @ %u -> %lu jiffies w/+25%%\n", 
				          dsa_channel_desc(dev|dir), (*xfer)->len, rate * 100, time);
				if ( time > max )
					max = time;
			}

	LOG_DEBUG("max timeout: %lu jiffies\n", max);
	return max;
}


static void dump_sxx (struct dsa_channel_sxx *sxx)
{
	if ( !sxx )
		return;

	LOG_DEBUG("\t\tfmt: %p",   sxx->fmt);
	if ( sxx->fmt )
		LOG_DEBUG(" (%s)", sxx->fmt->name);
	LOG_DEBUG("\n");
	LOG_DEBUG("\t\tloc: %s\n", sxx->loc);
}

#ifndef MIN
#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#endif
static void dump_xfer (struct dsa_channel_xfer *xfer)
{
	if ( !xfer )
		return;

	LOG_DEBUG("\tsmp   : %p\n",  xfer->smp);
	LOG_DEBUG("\tlen   : %zu\n", xfer->len);
	if ( xfer->smp && xfer->len )
		LOG_HEXDUMP(xfer->smp, MIN(xfer->len, 64));

	LOG_DEBUG("\tsrc[0]: %p:\n", xfer->src[0]); dump_sxx(xfer->src[0]);
	LOG_DEBUG("\tsrc[1]: %p:\n", xfer->src[1]); dump_sxx(xfer->src[1]);
	LOG_DEBUG("\tsnk[0]: %p:\n", xfer->snk[0]); dump_sxx(xfer->snk[0]);
	LOG_DEBUG("\tsnk[1]: %p:\n", xfer->snk[1]); dump_sxx(xfer->snk[1]);
}

void dsa_channel_event_dump (struct dsa_channel_event *evt)
{
	LOG_DEBUG("tx[0]: %p:\n", evt->tx[0]); dump_xfer(evt->tx[0]);
	LOG_DEBUG("tx[1]: %p:\n", evt->tx[1]); dump_xfer(evt->tx[1]);
	LOG_DEBUG("rx[0]: %p:\n", evt->rx[0]); dump_xfer(evt->rx[0]);
	LOG_DEBUG("rx[1]: %p:\n", evt->rx[1]); dump_xfer(evt->rx[1]);
}


int dsa_channel_load (struct dsa_channel_event *evt, int lsh)
{
	struct dsa_channel_xfer **xfer;
	struct dsa_channel_sxx  **sxx;
	int                       dev;
	int                       dir;
	int                       chan;
	FILE                     *fp;
	int                       ret = 0;
	long                      len;
	char                      loc[PATH_MAX];

	LOG_DEBUG("Load data for evt %p:\n", evt);

	for ( dev = DC_DEV_AD1; dev <= DC_DEV_AD2; dev <<= 1 )
		for ( dir = DC_DIR_TX; dir <= DC_DIR_RX; dir <<= 1 )
			if ( (xfer = evt_to_xfer(evt, dev|dir)) && *xfer )
				for ( chan = DC_CHAN_1; chan <= DC_CHAN_2; chan <<= 1 )
					if ( (sxx = xfer_to_sxx(*xfer, DC_DIR_TX|chan)) && *sxx )
					{
						LOG_DEBUG("  load src for dev/dir/chan %s: %s:%s\n",
						        dsa_channel_desc(dev|dir|chan),
						        (*sxx)->fmt ? (*sxx)->fmt->name : "???",
						        (*sxx)->loc);

						if ( !(*sxx)->fmt )
						{
							LOG_ERROR("No format set, stop\n");
							return -1;
						}

						// Loading data: search data path
						snprintf(loc, sizeof(loc), "%s", (*sxx)->loc);
						if ( !(fp = fopen((*sxx)->loc, "r")) )
						{
							if ( !path_match(loc, sizeof(loc), env_data_path, (*sxx)->loc) )
							{
								LOG_ERROR("%s: %s\n", (*sxx)->loc, strerror(errno));
								return -1;
							}
						
							LOG_DEBUG("%s: using %s\n", (*sxx)->loc, loc);
							fp = fopen(loc, "r");
						}

						// late allocation of buffer size based on input file size
						if ( ! (*xfer)->smp )
						{
							LOG_DEBUG("Late buffer alloc attempt\n");
							if ( !(*sxx)->fmt->size )
							{
								LOG_ERROR("Format %s can't estimate size\n",
						                  (*sxx)->fmt ? (*sxx)->fmt->name : "???");
								errno = ENOSYS;
								return -1;
							}
							if ( (len = format_size((*sxx)->fmt, fp, chan)) < 0 )
							{
								LOG_ERROR("Format size failed: %s\n",
						                  strerror(errno));
								return -1;
							}
							len /= sizeof(struct dsa_sample_pair);
							if ( realloc_buffer(*xfer, len) < 0 )
								return -1;

							LOG_DEBUG("Late buffer alloc success: %ld samples\n", len);
						}

						// TODO: unwrap channel loop, pass dsa_channel_xfer, format size
						// change to len, pass channel mask.  (dual-channel formats can
						// then detect internally)
						LOG_INFO("  Loading buffer from %s...\r", loc);
						ret = format_read((*sxx)->fmt, fp, (*xfer)->smp,
						                  (*xfer)->len * sizeof(struct dsa_sample_pair),
						                  chan, lsh);

						if ( ret < 0 )
						{
							LOG_ERROR("format_read(%s, %s, %zu) failed: %s",
							          (*sxx)->fmt->name, loc,
							          (*xfer)->len * sizeof(struct dsa_sample_pair),
									  strerror(errno));
							fclose(fp);
							return ret;
						}

						fclose(fp);
						LOG_INFO("\r\e[KLoaded %s\n", loc);
					}

	LOG_DEBUG("dsa_channel_load(): %d\n", ret);
	return ret;
}


int dsa_channel_save (struct dsa_channel_event *evt)
{
	struct dsa_channel_xfer **xfer;
	struct dsa_channel_sxx  **sxx;
	int                       dev;
	int                       dir;
	int                       chan;
	FILE                     *fp;
	int                       ret = 0;

	LOG_DEBUG("Save data for evt %p:\n", evt);
	for ( dev = DC_DEV_AD1; dev <= DC_DEV_AD2; dev <<= 1 )
		for ( dir = DC_DIR_TX; dir <= DC_DIR_RX; dir <<= 1 )
			if ( (xfer = evt_to_xfer(evt, dev|dir)) && *xfer )
				for ( chan = DC_CHAN_1; chan <= DC_CHAN_2; chan <<= 1 )
					if ( (sxx = xfer_to_sxx(*xfer, DC_DIR_RX|chan)) && *sxx )
					{
						LOG_DEBUG("  save snk for dev/dir/chan %s: %s:%s\n",
						        dsa_channel_desc(dev|dir|chan),
						        (*sxx)->fmt ? (*sxx)->fmt->name : "???",
						        (*sxx)->loc);

						if ( !(*sxx)->fmt )
						{
							LOG_ERROR("No format set, stop\n");
							return -1;
						}
						if ( !(fp = fopen((*sxx)->loc, "w")) )
						{
							LOG_ERROR("%s: %s\n", (*sxx)->loc, strerror(errno));
							return -1;
						}

						// TODO: unwrap channel loop, pass dsa_channel_xfer, format size
						// change to len, pass channel mask.  (dual-channel formats can
						// then detect internally)
						LOG_INFO("  Saving buffer to %s...\r", (*sxx)->loc);
						ret = format_write((*sxx)->fmt, fp, (*xfer)->smp,
						                   (*xfer)->len * sizeof(struct dsa_sample_pair),
						                   chan);

						if ( ret < 0 )
						{
							LOG_ERROR("format_write(%s, %s, %zu) failed: %s\n",
							          (*sxx)->fmt->name, (*sxx)->loc,
							          (*xfer)->len * sizeof(struct dsa_sample_pair),
									  strerror(errno));
							fclose(fp);
							return ret;
						}

						fclose(fp);
						LOG_INFO("\r\e[KSaved %s\n", (*sxx)->loc);
					}

	return ret;
}


void dsa_channel_calc_exp (struct dsa_channel_event *evt, int reps, int dsxx)
{
	struct dsa_channel_xfer **xfer;
	unsigned long long        exp;
	int                       dev;
	int                       rep;

	LOG_DEBUG("dsa_channel_calc_exp() calculating TX expected sums:\n");
	for ( dev = DC_DEV_AD1; dev <= DC_DEV_AD2; dev <<= 1 )
		if ( (xfer = evt_to_xfer(evt, dev|DC_DIR_TX)) && *xfer )
			if ( (*xfer)->smp && (*xfer)->len )
			{
				exp = dsnk_sum((*xfer)->smp,
				               (*xfer)->len * sizeof(struct dsa_sample_pair),
				               dsxx);
				(*xfer)->exp = 0;
				for ( rep = 0; rep < reps; rep++ )
					(*xfer)->exp += exp;
				LOG_DEBUG("%s: %016llx\n",
				        dsa_channel_desc(dev|DC_DIR_TX), (*xfer)->exp);
			}

	LOG_DEBUG("dsa_channel_calc_exp() done\n");
}


int dsa_channel_access_request (int ident, unsigned long priority)
{
	unsigned long  bits = 0;
	int            ret;

	if ( (ident & (DC_DIR_TX|DC_DEV_AD1)) == (DC_DIR_TX|DC_DEV_AD1) )
		bits |= PD_ACCESS_AD1_TX;

	if ( (ident & (DC_DIR_TX|DC_DEV_AD2)) == (DC_DIR_TX|DC_DEV_AD2) )
		bits |= PD_ACCESS_AD2_TX;

	if ( (ident & (DC_DIR_RX|DC_DEV_AD1)) == (DC_DIR_RX|DC_DEV_AD1) )
		bits |= PD_ACCESS_AD1_RX;

	if ( (ident & (DC_DIR_RX|DC_DEV_AD2)) == (DC_DIR_RX|DC_DEV_AD2) )
		bits |= PD_ACCESS_AD2_RX;

	if ( (ret = pipe_access_request(bits)) )
	{
		LOG_ERROR("Pipe-dev access request for %s (%lx) denied: %s\n",
		          dsa_channel_desc(ident & (DC_DEV_AD1|DC_DEV_AD2|DC_DIR_TX|DC_DIR_RX)),
				  bits, strerror(errno));
		return ret;
	}

	// Note, assumes the PD_ACCESS bits and FD_ACCESS bits are equal (currently true)
	if ( (ret = fifo_access_request(bits)) )
	{
		LOG_ERROR("FIFO-dev access request for %s (%lx) denied: %s\n",
		          dsa_channel_desc(ident & (DC_DEV_AD1|DC_DEV_AD2|DC_DIR_TX|DC_DIR_RX)),
				  bits, strerror(errno));
		pipe_access_release(bits);
		return ret;
	}

	LOG_ERROR("Access granted to %s\n",
	          dsa_channel_desc(ident & (DC_DEV_AD1|DC_DEV_AD2|DC_DIR_TX|DC_DIR_RX)));
	return 0;
}


int dsa_channel_check_active (int ident)
{
	unsigned char  need = 0;
	unsigned char  regs[2];
	int            idx = 0;
	int            ret = 0;
	int            dev;
	int            msg = 0;

	for ( dev = 0; dev <= 1; dev++ )
		if ( ident & DC_DEV_IDX_TO_MASK(dev) )
		{
			ad9361_spi_read_byte(dev, 0x002, &regs[0]);
			ad9361_spi_read_byte(dev, 0x003, &regs[1]);
			regs[0] &= 0xC0;
			regs[1] &= 0xC0;

			idx = ident & DC_DIR_TX ? 0 : 1;
			if ( ident & DC_CHAN_1 )  need |= 0x40;
			if ( ident & DC_CHAN_2 )  need |= 0x80;

			// revised check: error if need has a bit set which is not set in the active 
			// channels list - the user is requesting a channel which is not active.  the
			// other direction isn't as bad, the user's ignoring an active channel.  on TX
			// we'll send 0 paint, on RX we'll discard the samples
			if ( need & ~(regs[idx] & 0xC0) )
			{
				if ( ! msg++ )
					LOG_ERROR("DMA transfer doesn't match channels configured in "
					          "AD9361(s):\n");

				LOG_ERROR("  %s %s: needed:%s%s, configured:%s%s%s\n",
				          dev ? "AD2" : "AD1",
				          (ident & DC_DIR_TX) ? "TX"  : "RX",
				          need & 0x80 ? " CH2" : "",
				          need & 0x40 ? " CH1" : "",
				          regs[idx] & 0x80 ? " CH2" : "",
				          regs[idx] & 0x40 ? " CH1" : "",
				          regs[idx] & 0xC0 ? "" : " none");
				
				ret++;
			}
		}

	return ret;
}


int dsa_channel_check_and_wake (struct dsa_channel_event *evt, int no_change)
{
	struct dsa_channel_xfer **xfer;
	unsigned char             regs[2];
	int                       fdd;
	int                       mode;
	int                       need;
	int                       wake;
	int                       cycles = 0;
	int                       dev;
	int                       dir;
	int                       ret = 0;

	LOG_INFO("Checking AD9361 mode:\n");
	for ( dev = 0; dev <= 1; dev++ )
	{
		if ( !evt->rx[dev] && !evt->tx[dev] )
			continue;

		// get current ENSM mode and FDD config setting
		if ( ad9361_get_ensm_mode(dev, &mode) < 0 )
		{
			LOG_ERROR("ad9361_get_ensm_mode: %s\n", strerror(errno));
			return -1;
		}
		if ( ad9361_get_frequency_division_duplex_mode_enable(dev, &fdd) < 0 )
		{
			LOG_ERROR("ad9361_get_frequency_division_duplex_mode_enable: %s\n", strerror(errno));
			return -1;
		}

		// check for transfers setup for each direction
		need = 0;
		for ( dir = DC_DIR_TX; dir <= DC_DIR_RX; dir <<= 1 )
		{
			LOG_DEBUG("%s: %s\n",
			          dev ? "AD2" : "AD1",
			          dsa_channel_desc(DC_DEV_IDX_TO_MASK(dev)|dir));
			if ( (xfer = evt_to_xfer(evt, DC_DEV_IDX_TO_MASK(dev)|dir)) && *xfer )
			{
				LOG_DEBUG("xfer %p, *xfer %p\n", xfer, xfer ? *xfer : NULL);
				need |= dir;
			}
			else
				LOG_DEBUG("!xfer || !*xfer\n");
		}
		LOG_DEBUG("%s needs %s\n", dev ? "AD2" : "AD1", dsa_channel_desc(need));

		// nothing setup for this ADI: move on
		if ( !need )
		{
			LOG_DEBUG("%s needs nothing\n", dev ? "AD2" : "AD1");
			continue;
		}

		LOG_DEBUG("%s is in mode %s, DMA requested is %s\n", 
		          dev ? "AD2" : "AD1",
		          ad9361_enum_get_string(ad9361_enum_ensm_mode, mode),
		          dsa_channel_desc(need));

		switch ( mode )
		{
			// inactive modes: if configured for FDD, we can activate FDD and proceed
			case AD9361_ENSM_MODE_SLEEP:
			case AD9361_ENSM_MODE_WAIT:
			case AD9361_ENSM_MODE_ALERT:
				if ( !fdd )
				{
					LOG_ERROR("%s is in mode %s, but not configured for FDD: "
					          "can't set to FDD for DMA\n",
					          dev ? "AD2" : "AD1",
					          ad9361_enum_get_string(ad9361_enum_ensm_mode, mode));
					ret++;
				}
				else if ( no_change )
				{
					LOG_ERROR("%s is in mode %s, but user config forbids changing mode\n",
					          dev ? "AD2" : "AD1",
					          ad9361_enum_get_string(ad9361_enum_ensm_mode, mode));
					ret++;
				}
				else
				{
					// put the chip in FDD mode
					LOG_INFO("%s: setting to FDD\n", dev ? "AD2" : "AD1");
					if ( ad9361_set_ensm_mode(dev, AD9361_ENSM_MODE_FDD) < 0 )
					{
						LOG_ERROR("ad9361_set_ensm_mode: %s\n", strerror(errno));
						return -1;
					}

					// record this device was activated to wait for it to be ready
					dsa_channel_ensm_wake |= DC_DEV_IDX_TO_MASK(dev);
				}
				break;

			case AD9361_ENSM_MODE_RX:
				if ( need != DC_DIR_RX )
				{
					LOG_ERROR("%s is in state %s, but RX DMA requested\n",
					          dev ? "AD2" : "AD1",
					          ad9361_enum_get_string(ad9361_enum_ensm_mode, mode));
					ret++;
				}
				break;

			case AD9361_ENSM_MODE_TX:
				if ( need != DC_DIR_TX )
				{
					LOG_ERROR("%s is in state %s, but TX DMA requested\n",
					          dev ? "AD2" : "AD1",
					          ad9361_enum_get_string(ad9361_enum_ensm_mode, mode));
					ret++;
				}
				break;

			case AD9361_ENSM_MODE_FDD:
				LOG_DEBUG("%s is in state %s: normal operation\n",
				          dev ? "AD2" : "AD1",
				          ad9361_enum_get_string(ad9361_enum_ensm_mode, mode));
				break;

			case AD9361_ENSM_MODE_PINCTRL:
				LOG_WARN("%s is in state %s: this is not well-tested yet\n",
				         dev ? "AD2" : "AD1",
				         ad9361_enum_get_string(ad9361_enum_ensm_mode, mode));
				break;
		}
	}

	// wait for wakeup: currently checking TX quad cal
	if ( dsa_channel_ensm_wake )
		LOG_DEBUG("Waiting for %s...\n", dsa_channel_desc(dsa_channel_ensm_wake));
	for ( wake = dsa_channel_ensm_wake; wake && cycles < 1000; cycles++ )
	{
		usleep(2500); // 2.5ms delay
		for ( dev = 0; dev <= 1; dev++ )
			if ( dsa_channel_ensm_wake & DC_DEV_IDX_TO_MASK(dev) )
			{
				// read TX quad cal status and mask for convergence status
				ad9361_spi_read_byte(dev, 0x0A7, &regs[0]);
				ad9361_spi_read_byte(dev, 0x0A8, &regs[1]);
				regs[0] = 0x03;
				regs[1] = 0x03;
				LOG_DEBUG("%02x/%02x\n", regs[0], regs[1]);

				// if converged clear this dev
				if ( regs[0] == 0x03 && regs[1] == 0x03 )
					wake &= ~DC_DEV_IDX_TO_MASK(dev);
			}
	}
	if ( wake )
	{
		LOG_ERROR("Failed to wake up %s\n", dsa_channel_desc(wake));
		errno = ETIME;
		return -1;
	}

	if ( !ret )
		LOG_INFO("No problems found\n");

	return ret;
}


int dsa_channel_sleep (void)
{
	int  dev;
	int  ret = 0;

	LOG_DEBUG("Putting %s to sleep\n", dsa_channel_desc(dsa_channel_ensm_wake));
	for ( dev = 0; dev <= 1; dev++ )
		if ( dsa_channel_ensm_wake & DC_DEV_IDX_TO_MASK(dev) )
		{
			LOG_DEBUG("  %s...\n", dev ? "AD2" : "AD1");
			dsa_channel_ensm_wake &= ~DC_DEV_IDX_TO_MASK(dev);
			if ( ad9361_set_ensm_mode(dev, AD9361_ENSM_MODE_SLEEP) < 0 )
				ret++;
		}

	return ret;
}


unsigned long dsa_channel_ctrl (struct dsa_channel_event *evt, int dev, int old)
{
	struct dsa_channel_xfer **xfer;
	struct dsa_channel_sxx  **sxx;
	unsigned char             need;
	unsigned long             ret = 0;

	need = 0;
	if ( (xfer = evt_to_xfer(evt, dev|DC_DIR_TX)) && *xfer )
	{
		if ( (sxx = xfer_to_sxx(*xfer, DC_DIR_TX|DC_CHAN_1)) && *sxx )
			need |= 0x40;
		if ( (sxx = xfer_to_sxx(*xfer, DC_DIR_TX|DC_CHAN_2)) && *sxx )
			need |= 0x80;

		LOG_DEBUG("dsa_channel_ctrl: %s -> need %02x ( %s%s%s)\n",
		          dsa_channel_desc(dev|DC_DIR_TX), need,
				  need & 0x40 ? "TX1 " : "",
				  need & 0x80 ? "TX2 " : "",
				  !need       ? "none " : "");

		if ( old )
			switch ( need )
			{
				case 0x40:	// CH1 only  - T1R1, no swap
					ret |= FD_LVDS_CTRL_TX_ENB;
					ret |= FD_LVDS_CTRL_T1R1;
					break;

				case 0x80:	// CH2 only  - T1R1, swap
					ret |= FD_LVDS_CTRL_TX_ENB;
					ret |= FD_LVDS_CTRL_T1R1;
					ret |= FD_LVDS_CTRL_TX_SWP;
					break;

				case 0xC0:	// CH1 + CH2 - T2R2, both
					ret |= FD_LVDS_CTRL_TX_ENB;
					break;
			}

		else // new
			switch ( need )
			{
				case 0x40:	// CH1 only  - T1R1, no swap
					ret |= FD_NEW_CTRL_TX1;
					break;

				case 0x80:	// CH2 only  - T1R1, swap
					ret |= FD_NEW_CTRL_TX2;
					break;

				case 0xC0:	// CH1 + CH2 - T2R2, both
					ret |= FD_NEW_CTRL_TX1;
					ret |= FD_NEW_CTRL_TX2;
					break;
			}
	}

	need = 0;
	if ( (xfer = evt_to_xfer(evt, dev|DC_DIR_RX)) && *xfer )
	{
		if ( (sxx = xfer_to_sxx(*xfer, DC_DIR_RX|DC_CHAN_1)) && *sxx )
			need |= 0x40;
		if ( (sxx = xfer_to_sxx(*xfer, DC_DIR_RX|DC_CHAN_2)) && *sxx )
			need |= 0x80;

		LOG_DEBUG("dsa_channel_ctrl: %s -> need %02x ( %s%s%s)\n",
		          dsa_channel_desc(dev|DC_DIR_RX), need,
				  need & 0x40 ? "RX1 " : "",
				  need & 0x80 ? "RX2 " : "",
				  !need       ? "none " : "");

		if ( old )
			switch ( need )
			{
				case 0x40:	// CH1 only  - T1R1, no swap
					ret |= FD_LVDS_CTRL_RX_ENB;
					ret |= FD_LVDS_CTRL_T1R1;
					break;

				case 0x80:	// CH2 only  - T1R1, swap
					ret |= FD_LVDS_CTRL_RX_ENB;
					ret |= FD_LVDS_CTRL_T1R1;
					ret |= FD_LVDS_CTRL_RX_SWP;
					break;

				case 0xC0:	// CH1 + CH2 - T2R2, both
					ret |= FD_LVDS_CTRL_RX_ENB;
					break;
			}

		else // new
			switch ( need )
			{
				case 0x40:	// CH1 only  - T1R1
					ret |= FD_NEW_CTRL_RX1;
					break;

				case 0x80:	// CH2 only  - T1R1
					ret |= FD_NEW_CTRL_RX2;
					break;

				case 0xC0:	// CH1 + CH2 - T2R2
					ret |= FD_NEW_CTRL_RX1;
					ret |= FD_NEW_CTRL_RX2;
					break;
			}
	}

	if ( old )
	{
		LOG_DEBUG("dsa_channel_ctrl: old: %lx: { %s%s%s%s%s}\n", ret,
		          ret & FD_LVDS_CTRL_TX_ENB ? "TX_ENB " : "",
		          ret & FD_LVDS_CTRL_RX_ENB ? "RX_ENB " : "",
		          ret & FD_LVDS_CTRL_T1R1   ? "T1R1 "   : "",
		          ret & FD_LVDS_CTRL_TX_SWP ? "TX_SWP " : "",
		          ret & FD_LVDS_CTRL_RX_SWP ? "RX_SWP " : "");
	}
	else
	{
		LOG_DEBUG("dsa_channel_ctrl: new: %lx: { %s%s%s%s}\n", ret,
		          ret & FD_NEW_CTRL_RX1 ? "RX1 " : "",
		          ret & FD_NEW_CTRL_RX2 ? "RX2 " : "",
		          ret & FD_NEW_CTRL_TX1 ? "TX1 " : "",
		          ret & FD_NEW_CTRL_TX2 ? "TX2 " : "");
	}

	return ret;
}


#ifdef UNIT_TEST
struct format             fmt;
struct dsa_channel_event  evt;

int main (int argc, char **argv)
{
	int  num = 1;

	while ( argv[num] )
	{
		int ident = dsa_channel_ident(argv[num]);
		if ( dsa_channel_buffer(&evt, ident, 1000, 1) < 0 )
		{
			perror("dsa_channel_buffer");
			return 1;
		}
		num++;

		if ( argv[num] )
		{
			if ( dsa_channel_sxx(&evt, ident, ident, &fmt, argv[num]) < 0 )
			{
				perror("dsa_channel_sxx");
				return 1;
			}
			num++;
		}
	}

	dsa_channel_event_dump(&evt);

	dsa_channel_cleanup(&evt);
	return 0;
}
#endif
