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
#include <sys/mman.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "dsa_main.h"
#include "dsa_sample.h"
#include "dsa_channel.h"
#include "dsa_common.h"

#include "log.h"
LOG_MODULE_STATIC("channel", LOG_LEVEL_INFO);


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
		return 0;

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
	size_t  size = len * sizeof(struct dsa_sample_pair);
	void   *buff;
	long    page;


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

	// free old buffer
	if ( (buff = xfer->smp) )
	{
		size = xfer->len * sizeof(struct dsa_sample_pair);
		munlock(buff, size);
		free(buff);
	}
	xfer->smp = NULL;
	xfer->len = 0;

	if ( posix_memalign(&buff, page, size) )
	{
		LOG_ERROR("Failed to posix_memalign() %zu bytes aligned to %lu: %s\n",
		          size, page, strerror(errno));
		return -1;
	}

	if ( mlock(buff, size) )
	{
		LOG_ERROR("Failed to mlock() %zu bytes: %s\n", size, strerror(errno));
		free(buff);
		return -1;
	}

	LOG_DEBUG("Buffer allocated successfully\n");
	xfer->smp = buff;
	xfer->len = len;
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
						memset(samp, 0x00, len * sizeof(struct dsa_sample_pair));

					// use for loop to paint only channel 1
					else if ( ident & DC_CHAN_1 )
						while ( left-- )
						{
							samp->ch[0].i = 0;
							samp->ch[0].q = 0;
							samp++;
						}

					// use for loop to paint only channel 2
					else
						while ( left-- )
						{
							samp->ch[1].i = 0;
							samp->ch[1].q = 0;
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
				size_t  size = (*xfer)->len * sizeof(struct dsa_sample_pair);
				void   *buff = (*xfer)->smp;

				if ( buff )
				{
					munlock(buff, size);
					free(buff);
				}

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
unsigned long dsa_channel_timeout (struct dsa_channel_event *evt, int msps)
{
	struct dsa_channel_xfer **xfer;
	unsigned long             time;
	unsigned long             max  = 0;
	int                       dev;
	int                       dir;

	LOG_DEBUG("%s(evt, %d msps)\n", __func__, msps);
	if ( msps < 1 )
		msps = 1;
	msps *= 10000;

	for ( dev = DC_DEV_AD1; dev <= DC_DEV_AD2; dev <<= 1 )
		for ( dir = DC_DIR_TX; dir <= DC_DIR_RX; dir <<= 1 )
			if ( (xfer = evt_to_xfer(evt, (dev|dir))) && *xfer )
			{
				time  = (*xfer)->len / msps;
				time += (time >> 2);
				LOG_DEBUG("%s: %zu samples -> %lu jiffies w/+25%%\n", 
				          dsa_channel_desc(dev|dir), (*xfer)->len, time);
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


void dsa_channel_calc_exp (struct dsa_channel_event *evt, int reps)
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
				               (*xfer)->len * sizeof(struct dsa_sample_pair));
				(*xfer)->exp = 0;
				for ( rep = 0; rep < reps; rep++ )
					(*xfer)->exp += exp;
				LOG_DEBUG("%s: %016llx\n",
				        dsa_channel_desc(dev|DC_DIR_TX), (*xfer)->exp);
			}

	LOG_DEBUG("dsa_channel_calc_exp() done\n");
}


int dsa_channel_check_active (int ident)
{
	unsigned char  need = 0;
	int            idx = 0;
	int            ret = 0;
	int            dev;
	int            msg = 0;

	for ( dev = DC_DEV_AD1; dev <= DC_DEV_AD2; dev <<= 1 )
		if ( ident & dev )
		{
			switch ( dev | (ident & (DC_DIR_TX|DC_DIR_RX)) )
			{
				case DC_DIR_TX|DC_DEV_AD1: idx = 0; break;
				case DC_DIR_RX|DC_DEV_AD1: idx = 1; break;
				case DC_DIR_TX|DC_DEV_AD2: idx = 2; break;
				case DC_DIR_RX|DC_DEV_AD2: idx = 3; break;
				default:
					errno = EINVAL;
					return -1;
			}
			if ( ident & DC_CHAN_1 )  need |= 0x40;
			if ( ident & DC_CHAN_2 )  need |= 0x80;

			// revised check: error if need has a bit set which is not set in the active 
			// channels list - the user is requesting a channel which is not active.  the
			// other direction isn't as bad, the user's ignoring an active channel.  on TX
			// we'll send 0 paint, on RX we'll discard the samples
			if ( need & ~(dsa_active_channels[idx] & 0xC0) )
			{
				if ( ! msg++ )
					LOG_ERROR("DMA transfer doesn't match channels configured in "
					          "AD9361(s):\n");

				LOG_ERROR("  %s %s: needed:%s%s, configured:%s%s%s\n",
				          (dev == DC_DEV_AD1) ? "AD1" : "AD2",
				          (ident & DC_DIR_TX) ? "TX"  : "RX",
				          need & 0x80 ? " CH2" : "",
				          need & 0x40 ? " CH1" : "",
				          dsa_active_channels[idx] & 0x80 ? " CH2" : "",
				          dsa_active_channels[idx] & 0x40 ? " CH1" : "",
				          dsa_active_channels[idx] & 0xC0 ? "" : " none");
				
				ret++;
			}
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
					ret |= DSM_LVDS_CTRL_TX_ENB;
					ret |= DSM_LVDS_CTRL_T1R1;
					break;

				case 0x80:	// CH2 only  - T1R1, swap
					ret |= DSM_LVDS_CTRL_TX_ENB;
					ret |= DSM_LVDS_CTRL_T1R1;
					ret |= DSM_LVDS_CTRL_TX_SWP;
					break;

				case 0xC0:	// CH1 + CH2 - T2R2, both
					ret |= DSM_LVDS_CTRL_TX_ENB;
					break;
			}

		else // new
			switch ( need )
			{
				case 0x40:	// CH1 only  - T1R1, no swap
					ret |= DSM_NEW_CTRL_TX1;
					break;

				case 0x80:	// CH2 only  - T1R1, swap
					ret |= DSM_NEW_CTRL_TX2;
					break;

				case 0xC0:	// CH1 + CH2 - T2R2, both
					ret |= DSM_NEW_CTRL_TX1;
					ret |= DSM_NEW_CTRL_TX2;
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
					ret |= DSM_LVDS_CTRL_RX_ENB;
					ret |= DSM_LVDS_CTRL_T1R1;
					break;

				case 0x80:	// CH2 only  - T1R1, swap
					ret |= DSM_LVDS_CTRL_RX_ENB;
					ret |= DSM_LVDS_CTRL_T1R1;
					ret |= DSM_LVDS_CTRL_RX_SWP;
					break;

				case 0xC0:	// CH1 + CH2 - T2R2, both
					ret |= DSM_LVDS_CTRL_RX_ENB;
					break;
			}

		else // new
			switch ( need )
			{
				case 0x40:	// CH1 only  - T1R1
					ret |= DSM_NEW_CTRL_RX1;
					break;

				case 0x80:	// CH2 only  - T1R1
					ret |= DSM_NEW_CTRL_RX2;
					break;

				case 0xC0:	// CH1 + CH2 - T2R2
					ret |= DSM_NEW_CTRL_RX1;
					ret |= DSM_NEW_CTRL_RX2;
					break;
			}
	}

	if ( old )
	{
		LOG_DEBUG("dsa_channel_ctrl: old: %lx: { %s%s%s%s%s}\n", ret,
		          ret & DSM_LVDS_CTRL_TX_ENB ? "TX_ENB " : "",
		          ret & DSM_LVDS_CTRL_RX_ENB ? "RX_ENB " : "",
		          ret & DSM_LVDS_CTRL_T1R1   ? "T1R1 "   : "",
		          ret & DSM_LVDS_CTRL_TX_SWP ? "TX_SWP " : "",
		          ret & DSM_LVDS_CTRL_RX_SWP ? "RX_SWP " : "");
	}
	else
	{
		LOG_DEBUG("dsa_channel_ctrl: new: %lx: { %s%s%s%s}\n", ret,
		          ret & DSM_NEW_CTRL_RX1 ? "RX1 " : "",
		          ret & DSM_NEW_CTRL_RX2 ? "RX2 " : "",
		          ret & DSM_NEW_CTRL_TX1 ? "TX1 " : "",
		          ret & DSM_NEW_CTRL_TX2 ? "TX2 " : "");
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
