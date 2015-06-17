/** \file      sd_main.c
 *  \brief     SRIO-DMA tool, based on minimal.c
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
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pipe_dev.h>
#include <format.h>
#include <dsm.h>

#include "common.h"


unsigned    opt_chan     = 4;
size_t      opt_data     = 2048;
size_t      opt_buff     = 0;
size_t      opt_words    = 0;
unsigned    opt_timeout  = 1500;
unsigned    opt_npkts    = 0;
FILE       *opt_debug    = NULL;
char       *opt_rawfile  = NULL;

char                *opt_out_file = NULL;
struct format_class *opt_out_fmt  = NULL;


struct recv_packet
{
	uint32_t  tuser;
	uint32_t  padding;
	uint32_t  hello[2];
	uint32_t  v49_hdr;
	uint32_t  v49_sid;
	uint32_t  v49_tsi;
	uint32_t  v49_tsf1;
	uint32_t  v49_tsf2;
	uint32_t  data[58];
	uint32_t  v49_trailer;
};


static struct format_options sd_fmt_opts =
{
	.channels = 1,
	.single   = DSM_BUS_WIDTH / 2,
	.sample   = DSM_BUS_WIDTH,
	.bits     = 16,
	.packet   = 272,
	.head     = 36,
	.data     = 232,
	.foot     = 4,
};


static void usage (void)
{
	printf("Usage: srio-recv [-v] [-c channel] [-s bytes] [-t timeout] [-n npkts] [out-file]\n"
	       "Where:\n"
	       "-v          Verbose/debugging enable\n"
	       "-c channel  DMA channel to use\n"
	       "-s bytes    Set payload size in bytes\n"
	       "-t timeout  Set timeout in jiffies\n"
	       "-n npkts    Set number of packets for combiner\n"
	       "-o rawfile  Write raw buffer to rawfile (with packet headers)\n"
	       "\n"
	       "out-file is specified in the typical format of [fmt:]filename[.ext] - if given,\n"
	       "fmt must exactly match a format name, otherwise .ext is used to guess the file\n"
	       "format.  If no out-file is specified, the received data is hexdumped to stdout.\n"
		   "\n");
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

static void progress (size_t done, size_t size)
{
	if ( !size )
		fprintf(stderr, "Failed!");
	else if ( !done )
		fprintf(stderr, "  0%%");
	else if ( done >= size )
		fprintf(stderr, "\b\b\b\b100%%\n");
	else
	{
		unsigned long long prog = done;
		prog *= 100;
		prog /= size;
		fprintf(stderr, "\b\b\b\b%3llu%%", prog);
	}
}

int main (int argc, char **argv)
{
	struct dsm_chan_stats *sb;
	struct recv_packet    *pkt;
	unsigned long          routing;
	unsigned long          reg;
	void                  *buff;
	int                    ret = 0;
	int                    opt;
	int                    idx;

	format_error_setup(stderr);
	while ( (opt = getopt(argc, argv, "?hvc:s:t:n:o:")) > -1 )
		switch ( opt )
		{
			case 'v':
				opt_debug = stderr;
				format_debug_setup(stderr);
				break;

			case 'c': opt_chan    = strtoul(optarg, NULL, 0); break;
			case 's': opt_data    = strtoul(optarg, NULL, 0); break;
			case 't': opt_timeout = strtoul(optarg, NULL, 0); break;
			case 'n': opt_npkts   = strtoul(optarg, NULL, 0); break;
			case 'o': opt_rawfile = optarg;                   break;

			default:
				usage();
				return 1;
		}
	
	if ( optind < argc )
	{
		char *format = argv[optind];

		printf("try to identify '%s'\n", format);

		if ( (opt_out_file = strchr(format, ':')) )
		{
			*opt_out_file++ = '\0';
			opt_out_fmt = format_class_find(format);
		}
		else if ( (opt_out_fmt = format_class_find(format)) )
			opt_out_file = "-";
		else if ( (opt_out_fmt = format_class_guess(format)) )
			opt_out_file = format;

		if ( !opt_out_fmt )
		{
			printf("Couldn't identify format\n");
			usage();
			return 1;
		}
	}

	// open devs
	if ( dsm_reopen("/dev/" DSM_DRIVER_NODE) )
	{
		printf("dsm_reopen(%s): %s\n", "/dev/" DSM_DRIVER_NODE, strerror(errno));
		ret = 1;
		goto exit;
	}
	if ( pipe_dev_reopen("/dev/" PD_DRIVER_NODE) )
	{
		printf("pipe_dev_reopen(%s): %s\n", "/dev/" PD_DRIVER_NODE, strerror(errno));
		ret = 1;
		goto exit_dsm;
	}

	// validate channel number and direction
	if ( opt_chan >= dsm_channels->size )
	{
		printf("Invalid channel %u, available channels:\n", opt_chan);
		dump_channels();
		ret = 1;
		goto exit_pipe;
	}
	if ( !(dsm_channels->list[opt_chan].flags & DSM_CHAN_DIR_RX) )
	{
		printf("Channel %u can't do RX, available channels:\n", opt_chan);
		dump_channels();
		ret = 1;
		goto exit_pipe;
	}

	opt_buff = format_size_buff_from_data(&sd_fmt_opts, opt_data);
	if ( !opt_npkts )
		opt_npkts = format_num_packets_from_data(&sd_fmt_opts, opt_data);

	// validate buffer size against width and channel / total limits
	if ( opt_buff % DSM_BUS_WIDTH || 
	     opt_buff < DSM_BUS_WIDTH || 
	     opt_buff > dsm_channels->list[opt_chan].words ||
	     opt_buff > dsm_limits.total_words )
	{
		printf("Data size %zu gives invalid buffer size %zu - minimum %u, maximum %lu,\n"
		       "and must be a multiple of %u\n",
		       opt_data, opt_buff, DSM_BUS_WIDTH, dsm_limits.total_words * DSM_BUS_WIDTH,
		       DSM_BUS_WIDTH);
		ret = 1;
		goto exit_pipe;
	}
	opt_words = opt_buff / DSM_BUS_WIDTH;
	printf("Data size %zu gives buffer size %zu, npkts %zu, words %zu\n",
	       opt_data, opt_buff, opt_npkts, opt_words);

	// clamp timeout and set
	if ( opt_timeout < 100 )
		opt_timeout = 100;
	else if ( opt_timeout > 6000 )
		opt_timeout = 6000;
	if ( dsm_set_timeout(opt_timeout) ) 
	{
		printf("dsm_set_timeout(%u) failed: %s\n", opt_timeout, strerror(errno));
		ret = 1;
		goto exit_pipe;
	}
	printf("Timeout set to %u jiffies\n", opt_timeout);

	// allocate page-aligned buffer
	if ( !(buff = dsm_user_alloc(opt_words)) )
	{
		printf("dsm_user_alloc(%zu) failed: %s\n", opt_words, strerror(errno));
		ret = 1;
		goto exit_pipe;
	}
	printf("Buffer allocated: %lu words / %lu MB\n", opt_words, opt_words >> 20);
	memset(buff, 0, opt_buff);

	// hand buffer to kernelspace driver and build scatterlist
	if ( dsm_map_user(opt_chan, 0, buff, opt_words) )
	{
		printf("dsm_map_user() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_free;
	}
	printf("Buffer mapped with kernel module\n");


	// setup pipe-dev: route SWRITE to SRIO-DMA, set expected npkts, enable
	if ( pipe_routing_reg_get_adc_sw_dest(&routing) )
	{
		printf("pipe_routing_reg_get_adc_sw_dest() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_free;
	}
	reg  = routing;
	reg &= ~PD_ROUTING_REG_SWRITE_MASK;
	reg |=  PD_ROUTING_REG_SWRITE_DMA;
	pipe_routing_reg_set_adc_sw_dest(reg);

	pipe_srio_dma_comb_set_cmd(PD_SRIO_DMA_COMB_CMD_RESET);
	pipe_srio_dma_comb_set_cmd(0);
	pipe_srio_dma_comb_set_npkts(opt_npkts);
	pipe_srio_dma_comb_set_cmd(PD_SRIO_DMA_COMB_CMD_ENABLE);

	// start DMA transaction on all mapped channels, then start data-source module
	printf("Triggering DMA and starting DSRC... ");
	if ( dsm_oneshot_start(~0) )
	{
		printf("dsm_oneshot_start() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_unmap;
	}

	// wait for started DMA channels to finish or timeout
	printf("\nWaiting for DMA to finish... ");
	if ( dsm_oneshot_wait(~0) )
	{
		printf("dsm_oneshot_wait() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_unmap;
	}
	printf("\nDMA finished.");

	// Reset afterwards: restore routing, reset and idle combiner
	pipe_routing_reg_set_adc_sw_dest(routing);
	pipe_srio_dma_comb_set_cmd(PD_SRIO_DMA_COMB_CMD_RESET);
	pipe_srio_dma_comb_set_cmd(0);

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

	// write out data received if we have an output file/format, otherwise just hexdump
	if ( opt_out_file && opt_out_fmt )
	{
		sd_fmt_opts.prog_func = progress;
		sd_fmt_opts.prog_step = opt_buff / 100;

		FILE *fp = fopen(opt_out_file, "w");
		if ( !fp )
			perror(opt_out_file);
		else if ( !format_write(opt_out_fmt, &sd_fmt_opts, buff, opt_buff, fp) )
			perror("format_write");

		fclose(fp);
	}
	else
	{
		pkt = buff;
		for ( idx = 0; idx < opt_npkts; idx++ )
		{
			printf("\n%d: SWRITE %02x->%02x, HELLO %08x.%08x, bytes:\n", idx,
			       pkt->tuser >> 16, pkt->tuser & 0xFFFF, pkt->hello[1], pkt->hello[0]);
			hexdump_buff(pkt->data, sizeof(pkt->data));
		}
	}

	if ( opt_rawfile )
	{
		int  fd = open(opt_rawfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if ( fd > -1 )
		{
			void   *walk = buff;
			size_t  left = opt_buff;
			int     ret;

			while ( left )
			{
				if ( (ret = write(fd, walk, 4096)) < 0 )
				{
					perror("write");
					break;
				}
				walk += ret;
				left -= ret;
			}
			close(fd);
		}
		else
			perror(opt_rawfile);
	}

exit_unmap:
	if ( dsm_cleanup() )
		printf("dsm_cleanup() failed: %s\n", strerror(errno));
	else
		printf("Buffer unmapped with kernel module\n");

exit_free:
	dsm_user_free(buff, opt_words);
	printf("Buffer unlocked and freed\n");

exit_pipe:
	pipe_dev_close();

exit_dsm:
	dsm_close();

exit:
	return ret;
}

