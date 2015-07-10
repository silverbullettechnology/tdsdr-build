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
#include <unistd.h>
#include <arpa/inet.h>

#include <sd_user.h>
#include <pipe_dev.h>
#include <format.h>
#include <dsm.h>

#include "common.h"

#define DEF_DEST       2
#define DEF_CHAN       5
#define DEF_TIMEOUT    1500

unsigned    opt_remote   = DEF_DEST;
unsigned    opt_local    = 0;
unsigned    opt_chan     = DEF_CHAN;
size_t      opt_data     = 0;
size_t      opt_buff     = 0;
size_t      opt_words    = 0;
unsigned    opt_timeout  = DEF_TIMEOUT;
unsigned    opt_npkts    = 0;
FILE       *opt_debug    = NULL;
uint32_t    opt_sid      = 0;
uint8_t     opt_paint    = 0xFF;
char       *opt_rawfile  = NULL;

char                *opt_in_file = NULL;
struct format_class *opt_in_fmt  = NULL;


struct send_packet
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
	.channels = 3,
	.single   = DSM_BUS_WIDTH / 2,
	.sample   = DSM_BUS_WIDTH,
	.bits     = 16,
	.packet   = 272,
	.head     = 36,
	.data     = 232,
	.foot     = 0,
	.flags    = FO_ENDIAN,
};


static void usage (void)
{
	printf("Usage: srio-send [-v] [-c channel] [-s bytes] [-S samples] [-t timeout]\n"
	       "                 [-n npkts] [-L local] [-R remote] in-file stream-id\n"
	       "Where:\n"
	       "-v          Verbose/debugging enable\n"
	       "-d remote   SRIO destination device-ID (default %d)\n"
	       "-L local    SRIO local device-ID (if not auto-probed)\n"
	       "-c channel  DMA channel to use (default %d)\n"
	       "-S sammples Set payload size in samples (K or M optional)\n"
	       "-s bytes    Set payload size in bytes (K or M optional)\n"
	       "-t timeout  Set timeout in jiffies (default %u)\n"
	       "-n npkts    Set number of packets for combiner (default from size)\n"
	       "-p paint    Paint transmit buffer with byte value before loading\n"
	       "\n"
	       "in-file is specified in the typical format of [fmt:]filename[.ext] - if given,\n"
	       "fmt must exactly match a format name, otherwise .ext is used to guess the file\n"
	       "format.\n"
	       "stream-id is a required value, and must match the expected stream-id on the\n"
	       "receiver side, if that receiver implements stream-ID filtering.\n"
	       "\n", 
		   DEF_DEST, DEF_CHAN, DEF_TIMEOUT);
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
	struct send_packet    *pkt;
	unsigned long          routing;
	unsigned long          reg;
	unsigned long          tuser;
	uint32_t               hdr;
	uint32_t               smp = 0;
	char                  *format;
	FILE                  *in_fp = NULL;
	void                  *buff;
	int                    srio_dev;
	int                    ret = 0;
	int                    opt;
	int                    idx;

	setbuf(stdout, NULL);

	while ( (opt = getopt(argc, argv, "?hvR:L:c:s:S:t:n:p:o:")) > -1 )
		switch ( opt )
		{
			case 'v':
				opt_debug = stderr;
				format_debug_setup(stderr);
				break;

			case 'R': opt_remote  = strtoul(optarg, NULL, 0); break;
			case 'L': opt_local   = strtoul(optarg, NULL, 0); break;
			case 'c': opt_chan    = strtoul(optarg, NULL, 0); break;
			case 't': opt_timeout = strtoul(optarg, NULL, 0); break;
			case 'n': opt_npkts   = strtoul(optarg, NULL, 0); break;
			case 'p': opt_paint   = strtoul(optarg, NULL, 0); break;
			case 'o': opt_rawfile = optarg;                   break;

			case 's':
				opt_data = (size_bin(optarg) + 7) & ~7;
				break;

			case 'S':
				opt_data  = size_dec(optarg);
				opt_data *= DSM_BUS_WIDTH;
				break;


			default:
				usage();
				return 1;
		}

	if ( opt_debug )
	{
		fprintf(opt_debug, "format:\n");
		fprintf(opt_debug, "  channels: %lu\n",  sd_fmt_opts.channels);
		fprintf(opt_debug, "  single  : %zu\n",  sd_fmt_opts.single);
		fprintf(opt_debug, "  sample  : %zu\n",  sd_fmt_opts.sample);
		fprintf(opt_debug, "  bits    : %zu\n",  sd_fmt_opts.bits);
		fprintf(opt_debug, "  packet  : %zu\n",  sd_fmt_opts.packet);
		fprintf(opt_debug, "  head    : %zu\n",  sd_fmt_opts.head);
		fprintf(opt_debug, "  data    : %zu\n",  sd_fmt_opts.data);
		fprintf(opt_debug, "  foot    : %zu\n",  sd_fmt_opts.foot);
	}

	if ( (argc - optind) < 2 )
	{
		usage();
		return 1;
	}

	format = argv[optind];
	if ( (opt_in_file = strchr(format, ':')) )
	{
		*opt_in_file++ = '\0';
		opt_in_fmt = format_class_find(format);
	}
	else if ( (opt_in_fmt = format_class_find(format)) )
		opt_in_file = "-";
	else if ( (opt_in_fmt = format_class_guess(format)) )
		opt_in_file = format;

	if ( !opt_in_fmt )
	{
		printf("Couldn't identify format\n");
		usage();
		return 1;
	}

	if ( !opt_data )
	{
		if ( !(in_fp = fopen(opt_in_file, "r")) )
			perror(opt_in_file);
		else if ( !(opt_buff = format_size(opt_in_fmt, &sd_fmt_opts, in_fp)) )
			perror("format_size");
		else
			opt_data = format_size_data_from_buff(&sd_fmt_opts, opt_buff);

		if ( !in_fp || !opt_buff )
		{
			fclose(in_fp);
			return 1;
		}
	}
	else if ( !(in_fp = fopen(opt_in_file, "r")) )
		perror(opt_in_file);

	if ( !opt_npkts )
		opt_npkts = format_num_packets_from_data(&sd_fmt_opts, opt_data);
	opt_buff = opt_npkts * sd_fmt_opts.packet;

	printf("Sizes: buffer %zu, data %zu, npkts %zu\n", opt_buff, opt_data, opt_npkts);

	opt_sid = strtoul(argv[optind + 1], NULL, 0);


	opt_remote &= 0xFFFF;
	if ( !opt_remote || opt_remote >= 0xFFFF )
	{
		printf("Remote Device-ID 0x%x invalid, must be 0x0001-0xFFFE, stop\n", opt_remote);
		return 1;
	}

	// open DSM dev
	if ( dsm_reopen("/dev/" DSM_DRIVER_NODE) )
	{
		printf("dsm_reopen(%s): %s\n", "/dev/" DSM_DRIVER_NODE, strerror(errno));
		return 1;
	}

	// open SRIO dev, get or set local address
	if ( (srio_dev = open("/dev/" SD_USER_DEV_NODE, O_RDWR|O_NONBLOCK)) < 0 )
	{
		printf("SRIO dev open(%s): %s\n", "/dev/" SD_USER_DEV_NODE, strerror(errno));
		ret = 1;
		goto exit_dsm;
	}

	// user gave -L - set local address and tuser
	if ( opt_local && ioctl(srio_dev, SD_USER_IOCS_LOC_DEV_ID, opt_local) )
	{
		printf("SD_USER_IOCS_LOC_DEV_ID: %s\n", strerror(errno));
		ret = 1;
		goto exit_srio;
	}
	tuser = opt_local;

	// no -L - get local address into tuser
	if ( !opt_local && ioctl(srio_dev, SD_USER_IOCG_LOC_DEV_ID, &tuser) )
	{
		printf("SD_USER_IOCG_LOC_DEV_ID: %s\n", strerror(errno));
		ret = 1;
		goto exit_srio;
	}

	// validate
	if ( !tuser || tuser >= 0xFFFF )
	{
		printf("Local Device-ID invalid, wait for discovery to complete or specify with -L\n");
		ret = 1;
		goto exit_srio;
	}

	// open pipe-dev
	if ( pipe_dev_reopen("/dev/" PD_DRIVER_NODE) )
	{
		printf("pipe_dev_reopen(%s): %s\n", "/dev/" PD_DRIVER_NODE, strerror(errno));
		ret = 1;
		goto exit_srio;
	}

	// validate channel number and direction
	if ( opt_chan >= dsm_channels->size )
	{
		printf("Invalid channel %u, available channels:\n", opt_chan);
		dump_channels();
		ret = 1;
		goto exit_pipe;
	}
	if ( !(dsm_channels->list[opt_chan].flags & DSM_CHAN_DIR_TX) )
	{
		printf("Channel %u can't do TX, available channels:\n", opt_chan);
		dump_channels();
		ret = 1;
		goto exit_pipe;
	}


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
	printf("Buffer allocated: %lu words / %lu MB\n", opt_words, opt_words >> 17);

	// load data-file
	sd_fmt_opts.prog_func = progress;
	sd_fmt_opts.prog_step = opt_buff / 100;
	memset(buff, opt_paint, opt_buff);
	if ( in_fp )
	{
		if ( !format_read(opt_in_fmt, &sd_fmt_opts, buff, opt_buff, in_fp) )
			perror("format_read");
		fclose(in_fp);
	}

	// assemble tuser word with local and remote addresses
	tuser <<= 16;
	tuser  |= opt_remote;
	printf("tuser word: 0x%08x\n", tuser);

	// build packet headers - just tuser initially
	pkt = buff;
	for ( idx = 0; idx < opt_npkts; idx++ )
	{
		// SRIO header
		pkt->tuser    = tuser;
		pkt->padding  = 0xFFFFFFFF;
		pkt->hello[1] = 0x00600000;
		pkt->hello[0] = opt_sid;

		// VITA49 header / trailer
		hdr = ((idx & 0xf) << 16) | 0x10F0003F;
		pkt->v49_hdr  = ntohl(hdr);
		pkt->v49_sid  = ntohl(opt_sid);
		pkt->v49_tsi  = 0;
		pkt->v49_tsf1 = 0;
		pkt->v49_tsf2 = ntohl(smp);
		pkt->v49_trailer = 0xaaaaaaaa;

		pkt++;
		smp += 29;
	}

	if ( opt_rawfile )
	{
		int  fd = open(opt_rawfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if ( fd > -1 )
		{
			void   *walk = buff;
			size_t  left = opt_buff;
			int     ret;

			while ( left >= 4096 )
			{
				if ( (ret = write(fd, walk, 4096)) < 0 )
				{
					perror("write");
					break;
				}
				walk += ret;
				left -= ret;
			}
			if ( left && (ret = write(fd, walk, left)) < 0 )
				perror("write");
			close(fd);
		}
		else
			perror(opt_rawfile);
	}

	if ( opt_debug )
		hexdump_buff(buff, opt_buff);


	// hand buffer to kernelspace driver and build scatterlist
	if ( dsm_map_user(opt_chan, 1, buff, opt_words) )
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
	pipe_srio_dma_split_set_cmd(PD_SRIO_DMA_SPLIT_CMD_RESET);
	pipe_srio_dma_split_set_cmd(0);
	pipe_srio_dma_split_set_npkts(opt_npkts);
	pipe_srio_dma_split_set_cmd(PD_SRIO_DMA_SPLIT_CMD_ENABLE);

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
	printf("\nDMA finished, buffer:");

	// Reset afterwards: restore routing, reset and idle splitter
	pipe_routing_reg_set_adc_sw_dest(routing);
	pipe_srio_dma_split_set_cmd(PD_SRIO_DMA_SPLIT_CMD_RESET);
	pipe_srio_dma_split_set_cmd(0);

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
	dsm_user_free(buff, opt_words);
	printf("Buffer unlocked and freed\n");

exit_pipe:
	pipe_dev_close();

exit_srio:
	close(srio_dev);

exit_dsm:
	dsm_close();
	return ret;
}

