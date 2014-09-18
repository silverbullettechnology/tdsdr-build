/** \file      minimal.c
 *  \brief     Minimal example for DMA stack
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
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include "dsm/dsm.h"
#include "fifo/dev.h"
#include "fifo/dsxx.h"
#include "common/common.h"


unsigned    opt_chan     = 0;        // first RX channel default 
size_t      opt_size     = 10000000; // 10MS default
unsigned    opt_timeout  = 1500;     // 15sec default
const char *opt_dsm_dev  = "/dev/" DSM_DRIVER_NODE;
const char *opt_fifo_dev = "/dev/" FD_DRIVER_NODE;
int         opt_interact = 0;


static void usage (void)
{
	printf("Usage: minimal [-i] [-n node] [-N node] [-c channel] [-s bytes[K|M]]\n"
	       "               [-S samples[K|M]] [-t timeout]\n"
	       "Where:\n"
	       "-i          Pause before starting DMA and wait for keypress\n"
	       "-n node     Device node for DMA module\n"
	       "-N node     Device node for FIFO module\n"
	       "-c channel  DMA channel to use\n"
	       "-s bytes    Set buffer size in bytes, add K/B for KB/MB\n"
	       "-S samples  Set buffer size in samples, add K/M for kilo-samples/mega-samples\n"
	       "-t timeout  Set timeout in jiffies\n\n");
}

static void dump_channels (void)
{
	int i;

	for ( i = 0; i < dsm_channels->size; i++ )
	{
		char *dir = "???";
		switch ( dsm_channels->list[i].flags & DSM_CHAN_DIR_MASK )
		{
			case DSM_CHAN_DIR_TX: dir = "TX-only"; break;
			case DSM_CHAN_DIR_RX: dir = "RX-only"; break;
			case (DSM_CHAN_DIR_TX | DSM_CHAN_DIR_RX): dir = "bidir"; break;
		}

		printf("%2d: %s %s (%s)\n",
		       i, dsm_channels->list[i].device, dir, dsm_channels->list[i].driver);
	}
}

int main (int argc, char **argv)
{
	struct dsm_chan_stats *sb;
	uint64_t              *ptr;
	uint64_t               val = 0;
	uint32_t               cnt = 0;
	size_t                 idx = 0;
	void                  *buff;
	int                    ret = 0;
	int                    opt;

	setbuf(stdout, NULL);

	while ( (opt = posix_getopt(argc, argv, "?hic:s:S:t:n:N:")) > -1 )
		switch ( opt )
		{
			case 'i': opt_interact = 1;
			case 'n': opt_dsm_dev  = optarg;                           break;
			case 'N': opt_fifo_dev = optarg;                           break;
			case 'c': opt_chan     = strtoul(optarg, NULL, 0);         break;
			case 's': opt_size     = size_bin(optarg) / DSM_BUS_WIDTH; break;
			case 'S': opt_size     = size_dec(optarg);                 break;
			case 't': opt_timeout  = strtoul(optarg, NULL, 0);         break;

			default:
				usage();
				return 1;
		}

	// open devs
	if ( dsm_reopen(opt_dsm_dev) )
	{
		printf("dsm_reopen(%s): %s\n", opt_dsm_dev, strerror(errno));
		ret = 1;
		goto exit;
	}
	if ( fifo_dev_reopen(opt_fifo_dev) )
	{
		printf("fifo_dev_reopen(%s): %s\n", opt_fifo_dev, strerror(errno));
		ret = 1;
		goto exit_dsm;
	}

	// verify data-source module is available
	if ( !(fifo_dev_target_mask & FD_TARGT_DSX0) )
	{
		printf("fifo_dev doesn't support DSXX0, targets: %s\n",
		       fifo_dev_target_desc(fifo_dev_target_mask));
		ret = 1;
		goto exit_dsm;
	}

	// validate channel number and direction
	if ( opt_chan >= dsm_channels->size )
	{
		printf("Invalid channel %u, available channels:\n", opt_chan);
		dump_channels();
		ret = 1;
		goto exit_fifo;
	}
	if ( !(dsm_channels->list[opt_chan].flags & DSM_CHAN_DIR_RX) )
	{
		printf("Channel %u can't do RX, available channels:\n", opt_chan);
		dump_channels();
		ret = 1;
		goto exit_fifo;
	}

	// validate buffer size against width and channel / total limits
	if ( opt_size % DSM_BUS_WIDTH || 
	     opt_size < DSM_BUS_WIDTH || 
	     opt_size > dsm_channels->list[opt_chan].words ||
	     opt_size > dsm_limits.total_words )
	{
		printf("Invalid buffer size %zu - minimum %u, maximum %lu,\n"
		       "and must be a multiple of %u\n",
		       opt_size, DSM_BUS_WIDTH, dsm_limits.total_words * DSM_BUS_WIDTH,
		       DSM_BUS_WIDTH);
		ret = 1;
		goto exit_fifo;
	}

	// clamp timeout and set
	if ( opt_timeout < 100 )
		opt_timeout = 100;
	else if ( opt_timeout > 6000 )
		opt_timeout = 6000;
	if ( dsm_set_timeout(opt_timeout) ) 
	{
		printf("dsm_set_timeout(%u) failed: %s\n", opt_timeout, strerror(errno));
		ret = 1;
		goto exit_fifo;
	}
	printf("Timeout set to %u jiffies\n", opt_timeout);

	// allocate page-aligned buffer
	if ( !(buff = dsm_alloc(opt_size)) )
	{
		printf("fifo_dev_reopen(%s) failed: %s\n", opt_fifo_dev, strerror(errno));
		ret = 1;
		goto exit_fifo;
	}
	printf("Buffer allocated: %lu words / %lu MB\n", opt_size, opt_size >> 7);

	// hand buffer to kernelspace driver and build scatterlist
	if ( dsm_map_user(opt_chan, 0, buff, opt_size) )
	{
		printf("dsm_map_user() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_free;
	}
	printf("Buffer mapped with kernel module\n");

	// disable and reset data-source and wait for user
	fifo_dsrc_set_ctrl(0, FD_DSXX_CTRL_DISABLE);
	fifo_dsrc_set_ctrl(0, FD_DSXX_CTRL_RESET);
	fifo_dsrc_set_bytes(0, opt_size * DSM_BUS_WIDTH);
	fifo_dsrc_set_reps(0, 1);

	if ( opt_interact )
	{
		printf("Ready to trigger DMA, press a key...\n");
		terminal_pause();
	}

	// start DMA transaction on all mapped channels, then start data-source module
	printf("Triggering DMA and starting DSRC... ");
	if ( dsm_oneshot_start(~0) )
	{
		printf("dsm_oneshot_start() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_unmap;
	}
	fifo_dsrc_set_ctrl(0, FD_DSXX_CTRL_ENABLE);

	// wait for started DMA channels to finish or timeout
	printf("\nWaiting for DMA to finish... ");
	if ( dsm_oneshot_wait(~0) )
	{
		printf("dsm_oneshot_wait() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_unmap;
	}
	printf("\nDMA finished, analyzing %d words in buffer: ", opt_size);

	// analyze for expected pattern produced by DSRC: 64-bit counter value should match
	// the 64-bit word offset in the buffer, starting with 0.
	ptr = (uint64_t *)buff;
	while ( idx < opt_size )
	{
		if ( *ptr != val )
		{
			if ( cnt < 10 )
				printf("\nIdx %zu: want %llx, got %llx instead\n",
				       idx, (unsigned long long)val, (unsigned long long)*ptr);
			val = *ptr;
			cnt++;
		}
		ptr++;
		val++;
		idx++;
	}
	if ( cnt >= 10 )
		printf("\n%lu messages suppressed, ", cnt - 10);
	printf("%u deviations\n", cnt);

	// print stats for the transfer
	if ( (sb = dsm_get_stats()) )
	{
		const struct dsm_xfer_stats *st   = &sb->list[opt_chan];
		unsigned long long           usec = st->total.tv_sec;
		unsigned long long           rate = 0;

		printf("Transfer statistics:\n");

		usec *= 1000000000;
		usec += st->total.tv_nsec;
		usec /= 1000;

		if ( usec > 0 )
			rate = st->bytes / usec;

		printf("  bytes    : %llu\n",        st->bytes);
		printf("  time     : %lu.%09lu s\n", st->total.tv_sec, st->total.tv_nsec);
		printf("  rate     : %llu MB/s\n"  , rate);
		printf("  starts   : %lu\n",         st->starts);
		printf("  completes: %lu\n",         st->completes);
		printf("  errors   : %lu\n",         st->errors);
		printf("  timeouts : %lu\n",         st->timeouts);
	}
	else
		printf("dsm_get_stats() failed: %s\n", strerror(errno));
	free(sb);

exit_unmap:
	if ( dsm_cleanup() )
		printf("dsm_cleanup() failed: %s\n", strerror(errno));
	else
		printf("Buffer unmapped with kernel module\n");

exit_free:
	dsm_free(buff, opt_size);
	printf("Buffer unlocked and freed\n");

exit_fifo:
	fifo_dev_close();

exit_dsm:
	dsm_close();

exit:
	return ret;
}

