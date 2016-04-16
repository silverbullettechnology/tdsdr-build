/** \file      loop_main.c
 *  \brief     SRIO-DMA loopback tool
 *  \copyright Copyright 2015 Silver Bullet Technology
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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include <sd_user.h>
#include <pipe_dev.h>
#include <format.h>
#include <dsm.h>

#include "common.h"

#define DEF_TX_CHAN       5
#define DEF_RX_CHAN       4
#define DEF_TIMEOUT    1000
#define DEF_DATA      (10 << 20) // 10MB default
#define DEF_SID           0
#define DEF_BODY        256
#define DEF_COS        0x55

unsigned    opt_local    = 0;
size_t      opt_data     = DEF_DATA;
size_t      opt_buff     = 0;
unsigned    opt_timeout  = DEF_TIMEOUT;
unsigned    opt_npkts    = 0;
size_t      opt_body     = DEF_BODY;
FILE       *opt_debug    = NULL;
uint32_t    opt_sid      = DEF_SID;
uint32_t    opt_cos      = DEF_COS;

unsigned    opt_tx_chan  = DEF_TX_CHAN;
size_t      opt_tx_rand  = 0;
char       *opt_tx_load  = NULL;
FILE       *opt_tx_file  = NULL;
char       *opt_tx_raw   = NULL;

unsigned    opt_rx_chan  = DEF_RX_CHAN;
char       *opt_rx_save  = NULL;
char       *opt_rx_raw   = NULL;

char  tx_load_buff[4096];


struct format_class *sd_fmt_bin = NULL;
static struct format_options sd_fmt_opts =
{
	.channels = 3,
	.single   = DSM_BUS_WIDTH / 2,
	.sample   = DSM_BUS_WIDTH,
	.bits     = 16,
	.head     = SRIO_HEAD,
	.flags    = FO_ENDIAN,
};


static void usage (void)
{
	printf("Usage: srio-loop [-vpr] [-i file] [-o [buf:]file] [-s bytes] [-S samples]\n"
	       "                 [-t timeout] [-C COS] [-L local] [-b bytes] [stream-id]\n"
	       "Where:\n"
	       "-v          Verbose/debugging enable\n"
	       "-p          Paint transmit payload with counter (default)\n"
	       "-r          Load transmit payload with data from /dev/urandom\n"
	       "-i file     Load transmit payload from file\n"
	       "-o file     Save receive payload (without headers) to file\n"
	       "-o buf:file Save buffer (tx: or rx:) to file\n"
	       "-s bytes    Set payload size in bytes (K or M optional)\n"
	       "-S samples  Set payload size in samples (K or M optional)\n"
	       "-b bytes    Set PDU body size in bytes (K optional, default %d)\n"
	       "-t timeout  Set timeout in jiffies (default %u)\n"
	       "-C COS      Set type 9 COS value (default 0x%02x)\n"
	       "-b dir:file Save raw buffer for dir (tx/rx) to file\n"
	       "-L local    SRIO local device-ID (if not auto-probed)\n"
	       "\n"
	       "stream-id is optional and only used for type 9 (default 0x%04x)\n"
	       "\n", 
	       DEF_DATA, DEF_TIMEOUT, DEF_COS, DEF_SID);

	exit(1);
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

static int save_buff (const char *file, void *buff, size_t size)
{
	int  fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, 0666);

	if ( fd < 0 )
	{
		perror(file);
		return -1;
	}

	void   *walk = buff;
	size_t  left = size;
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

	return close(fd);
}


static void show_stats (const char *dir, const struct dsm_xfer_stats *st)
{
	unsigned long long  usec = st->total.tv_sec;
	unsigned long long  rate = 0;

	printf("%s transfer statistics:\n", dir);

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

int main (int argc, char **argv)
{
	struct dsm_chan_stats *sb;
	struct srio_header    *srio_hdr;
	uint32_t              *tx_data;
	uint32_t              *rx_data;
	size_t                 ofs;
	size_t                 left;
	size_t                 size;
	unsigned long          routing;
	unsigned long          reg;
	unsigned long          tuser;
	unsigned long          bad;
	void                  *ptr;
	void                  *tx_buff;
	void                  *rx_buff;
	int                    srio_dev;
	int                    ret = 0;
	int                    opt;
	int                    i, j;

	setbuf(stdout, NULL);

	while ( (opt = getopt(argc, argv, "?hvrpi:o:s:S:t:C:L:b:")) > -1 )
		switch ( opt )
		{
			case 'v':
				opt_debug = stderr;
				format_debug_setup(stderr);
				break;

			case 'r':
				opt_tx_load = NULL;
				opt_tx_rand = 10 << 20;
				break;

			case 'p':
				opt_tx_load = NULL;
				opt_tx_rand = 0;
				break;

			case 'i':
				opt_tx_load = optarg;
				opt_tx_rand = 0;
				break;


			case 's':
				opt_data = (size_bin(optarg) + 7) & ~7;
				break;

			case 'S':
				opt_data  = size_dec(optarg);
				opt_data *= DSM_BUS_WIDTH;
				break;

			case 'o':
				if ( (ptr = strchr(optarg, ':')) )
					switch ( tolower(*optarg) )
					{
						case 't': opt_tx_raw = ptr + 1; break;
						case 'r': opt_rx_raw = ptr + 1; break;
						default: usage();
					}
				else
					opt_rx_save = optarg;
				break;

			case 't': opt_timeout = strtoul(optarg, NULL, 0);        break;
			case 'L': opt_local   = strtoul(optarg, NULL, 0);        break;
			case 'C': opt_cos     = strtoul(optarg, NULL, 0) & 0xFF; break;
			case 'b': opt_body    = (size_bin(optarg) + 7) & ~7;     break;

			default:
				usage();
		}

	sd_fmt_opts.data   = opt_body;
	sd_fmt_opts.packet = sd_fmt_opts.head + sd_fmt_opts.data;

	if ( !(sd_fmt_bin = format_class_find("bin")) )
	{
		perror("format_class_find(bin)");
		return 1;
	}

	if ( (argc - optind) >= 1 )
		opt_sid = strtoul(argv[optind + 1], NULL, 0);

	opt_npkts = format_num_packets_from_data(&sd_fmt_opts, opt_data);
	opt_buff  = format_size_buff_from_data(&sd_fmt_opts,   opt_data);

	if ( opt_debug )
	{
		size_t  chk_npkts;

		size_t  chk_buff;
		printf("data size %zu\nformat -> %u pkts, %zu buff size.\n",
		       opt_data, opt_npkts, opt_buff);

		chk_npkts = opt_data / sd_fmt_opts.data;
		chk_buff  = sd_fmt_opts.packet * chk_npkts;
		if ( opt_data % sd_fmt_opts.data )
		{
			chk_buff += sd_fmt_opts.head;
			chk_buff += opt_data % sd_fmt_opts.data;
			chk_npkts++;
		}
		printf("check  -> %zu pkts, %zu buff size.\n", chk_npkts, chk_buff);

		if ( opt_npkts != chk_npkts || opt_buff != chk_buff )
		{
			printf("Buffer math is wrong at these settings, stop\n");
			return 1;
		}
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

	// assemble tuser word with local and remote addresses
	tuser  &= 0xFFFF;
	tuser  |= (tuser << 16);
	printf("tuser word: 0x%08x\n", tuser);


	// open pipe-dev
	if ( pipe_dev_reopen("/dev/" PD_DRIVER_NODE) )
	{
		printf("pipe_dev_reopen(%s): %s\n", "/dev/" PD_DRIVER_NODE, strerror(errno));
		ret = 1;
		goto exit_srio;
	}

	// validate channel number and direction
	if ( opt_tx_chan >= dsm_channels->size )
	{
		printf("Invalid channel %u, available channels:\n", opt_tx_chan);
		dump_channels();
		ret = 1;
		goto exit_pipe;
	}
	if ( !(dsm_channels->list[opt_tx_chan].flags & DSM_CHAN_DIR_TX) )
	{
		printf("Channel %u can't do TX, available channels:\n", opt_tx_chan);
		dump_channels();
		ret = 1;
		goto exit_pipe;
	}

	if ( opt_rx_chan >= dsm_channels->size )
	{
		printf("Invalid channel %u, available channels:\n", opt_rx_chan);
		dump_channels();
		ret = 1;
		goto exit_pipe;
	}
	if ( !(dsm_channels->list[opt_rx_chan].flags & DSM_CHAN_DIR_RX) )
	{
		printf("Channel %u can't do RX, available channels:\n", opt_rx_chan);
		dump_channels();
		ret = 1;
		goto exit_pipe;
	}


	// validate buffer size against width and channel / total limits
	if ( opt_buff % DSM_BUS_WIDTH || 
	     opt_buff < DSM_BUS_WIDTH || 
	     opt_buff > (dsm_channels->list[opt_tx_chan].words * DSM_BUS_WIDTH) ||
	     opt_buff > (dsm_channels->list[opt_rx_chan].words * DSM_BUS_WIDTH) ||
	     opt_buff > (dsm_limits.total_words * DSM_BUS_WIDTH) )
	{
		printf("Data size %zu gives invalid buffer size %zu - minimum %u, maximum %lu,\n"
		       "and must be a multiple of %u\n",
		       opt_data, opt_buff, DSM_BUS_WIDTH, dsm_limits.total_words * DSM_BUS_WIDTH,
		       DSM_BUS_WIDTH);
		ret = 1;
		goto exit_pipe;
	}
	printf("Data size %zu gives buffer size %zu, npkts %zu, words %zu\n",
	       opt_data, opt_buff, opt_npkts, opt_buff / DSM_BUS_WIDTH);

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

	// allocate page-aligned buffers
	if ( !(tx_buff = dsm_user_alloc(opt_buff / DSM_BUS_WIDTH)) )
	{
		printf("dsm_user_alloc(%zu) failed: %s\n", opt_buff / DSM_BUS_WIDTH, strerror(errno));
		ret = 1;
		goto exit_pipe;
	}
	printf("TX buffer allocated: %lu words / %lu MB\n",
	       opt_buff / DSM_BUS_WIDTH, opt_buff >> 20);

	if ( !(rx_buff = dsm_user_alloc(opt_buff / DSM_BUS_WIDTH)) )
	{
		printf("dsm_user_alloc(%zu) failed: %s\n", opt_buff / DSM_BUS_WIDTH, strerror(errno));
		ret = 1;
		goto exit_free_tx;
	}
	printf("RX buffer allocated: %lu words / %lu MB\n",
	       opt_buff / DSM_BUS_WIDTH, opt_buff >> 20);


	// Paint RX buffer payload area
	memset(rx_buff, 0xDD, opt_buff);

	// Paint RX buffer header areas
	ptr = rx_buff;
	for ( i = 0; i < opt_npkts; i++ )
	{
		srio_hdr = ptr;

		// SRIO header
		srio_hdr->tuser    = 0xE3E2E1E0;
		srio_hdr->padding  = 0xE7E6E5E4;
		srio_hdr->hello[0] = 0xEBEAE9E8;
		srio_hdr->hello[1] = 0xEFEEEDEC;

		ptr += sd_fmt_opts.packet;
	}

	if ( opt_tx_rand )
	{
		srand(time(NULL));
		if ( (opt_tx_file = tmpfile()) )
		{
			unsigned  word;

			printf("Setup random pattern:");
			left = opt_tx_rand;
			while ( left >= sizeof(unsigned) )
			{
				word   = rand() & 0xFFFF;
				word <<= 16;
				word  |= (rand() & 0xFFFF);
				left -= fwrite(&word, sizeof(word), 1, opt_tx_file);
			}
			printf("ok, %zu bytes\n", opt_tx_rand);

			rewind(opt_tx_file);
		}
	}
	else if ( opt_tx_load )
		opt_tx_file = fopen(opt_tx_load, "r");
	
	if ( opt_tx_file )
	{
		sd_fmt_opts.prog_func = progress;
		sd_fmt_opts.prog_step = opt_buff / 100;
		if ( !format_read(sd_fmt_bin, &sd_fmt_opts, tx_buff, opt_buff, opt_tx_file) )
			perror("format_read");
		fclose(opt_tx_file);
	}
	else if ( opt_tx_load || opt_tx_rand )
	{
		perror("TX data file");
		goto exit_free_tx;
	}
	else
	{
		// Paint TX buffer payload area - offset by header length once, then advance by
		// entire amount
		ofs = 0;
		left = opt_data;
		for ( i = 0; i < opt_npkts; i++ )
		{
			tx_data = (tx_buff + sd_fmt_opts.head + ofs);
			for ( j = 0; j < sd_fmt_opts.data / sizeof(*tx_data) && left; j++ )
			{
				tx_data[j] = (j << 26) | (0x3FFFFFF - i);
				left -= sizeof(*tx_data);
			}

			// packet based on payload + headers
			ofs += sd_fmt_opts.packet;
		}
	}

	// build packet headers - precalc header fields as much as possible
	// length in HELLO header is total packet minus TUSER+HELLO, may need -1
	uint32_t  hello0 = (opt_sid << 16) | (sd_fmt_opts.packet - SRIO_HEAD);
	uint32_t  hello1 = (opt_cos <<  4) | 0x00900000;

	// Setup headers
	left = opt_data;
	ptr  = tx_buff;
	for ( i = 0; i < opt_npkts; i++ )
	{
		srio_hdr = ptr;

		// SRIO header - TUSER, alignment, and HELLO header
		srio_hdr->tuser    = tuser;
		srio_hdr->hello[1] = hello1;
		srio_hdr->hello[0] = hello0;
		if ( left < sd_fmt_opts.data )
		{
			srio_hdr->hello[0] &= 0xFFFF0000;
			srio_hdr->hello[0] |= left & 0xFFFF;
		}
		else
			left -= sd_fmt_opts.data;

		// packet based on payload + headers
		ptr += sd_fmt_opts.packet;
	}


	if ( opt_tx_raw )
		save_buff(opt_tx_raw, tx_buff, opt_buff);


	// hand buffers to kernelspace driver and build scatterlists
	if ( dsm_map_user(opt_tx_chan, 1, tx_buff, opt_buff / DSM_BUS_WIDTH) )
	{
		printf("dsm_map_user() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_free_rx;
	}
	if ( dsm_map_user(opt_rx_chan, 0, rx_buff, opt_buff / DSM_BUS_WIDTH) )
	{
		printf("dsm_map_user() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_unmap;
	}
	printf("Buffers mapped with kernel module\n");


	// setup pipe-dev: route SWRITE to SRIO-DMA, set expected npkts, enable
	if ( pipe_routing_reg_get_adc_sw_dest(&routing) )
	{
		printf("pipe_routing_reg_get_adc_sw_dest() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_unmap;
	}
	reg  = routing;
	reg &= ~PD_ROUTING_REG_TYPE9_MASK;
	reg |=  PD_ROUTING_REG_TYPE9_DMA;
	pipe_routing_reg_set_adc_sw_dest(reg);

	pipe_srio_dma_split_set_cmd(PD_SRIO_DMA_SPLIT_CMD_RESET);
	pipe_srio_dma_split_set_cmd(0);
	pipe_srio_dma_split_set_npkts(opt_npkts);

	// New splitter for type 9 needs length including HELLO headers but excluding TUSER +
	// padding, in 64-bit words.
	pipe_srio_dma_split_set_psize((sd_fmt_opts.packet / DSM_BUS_WIDTH) - 1);
	pipe_srio_dma_split_set_cmd(PD_SRIO_DMA_SPLIT_CMD_ENABLE);

	pipe_srio_dma_comb_set_cmd(PD_SRIO_DMA_COMB_CMD_RESET);
	pipe_srio_dma_comb_set_cmd(0);
	pipe_srio_dma_comb_set_npkts(opt_npkts);
	pipe_srio_dma_comb_set_cmd(PD_SRIO_DMA_COMB_CMD_ENABLE);


	// start DMA transaction on RX, then TX
	printf("Triggering DMA... ");
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
	printf("\nDMA finished\n");


	if ( opt_rx_raw )
		save_buff(opt_rx_raw, rx_buff, opt_buff);


	// Reset afterwards: restore routing, reset and idle splitter
	pipe_routing_reg_set_adc_sw_dest(routing);
	pipe_srio_dma_split_set_cmd(PD_SRIO_DMA_SPLIT_CMD_RESET);
	pipe_srio_dma_split_set_cmd(0);
	pipe_srio_dma_comb_set_cmd(PD_SRIO_DMA_COMB_CMD_RESET);
	pipe_srio_dma_comb_set_cmd(0);

	// print stats for the transfer
	if ( (sb = dsm_get_stats()) )
	{
		show_stats("TX", &sb->list[opt_tx_chan]);
		show_stats("RX", &sb->list[opt_rx_chan]);
		free(sb);
	}
	else
		printf("dsm_get_stats() failed: %s\n", strerror(errno));


	// Compare buffers
	bad = 0;
	ofs = 0;
	left = opt_data;
	for ( i = 0; i < opt_npkts; i++ )
	{
		tx_data = (tx_buff + sd_fmt_opts.head + ofs);
		rx_data = (rx_buff + sd_fmt_opts.head + ofs);
		size    = (left < sd_fmt_opts.data) ? left : sd_fmt_opts.data;

		if ( memcmp(rx_data, tx_data, size) )
		{
			if ( bad < 10 )
				printf("pkt %5d: mismatches:\n", i);
			for ( j = 0; j < sd_fmt_opts.data / sizeof(*tx_data) && size; j++ )
			{
				if ( rx_data[j] != tx_data[j] )
				{
					bad++;
					if ( bad < 10 )
						printf("           %2d: %08x / %08x\n", j,
						       rx_data[j], tx_data[j]);
				}
				size -= sizeof(*tx_data);
			}
		}
	
		left -= sd_fmt_opts.data;
		ofs += sd_fmt_opts.packet;
	}
	if ( bad )
		printf("Total %lu words mismatched\n", bad);
	else
		printf("No mismatched payload words\n");

	if ( opt_rx_save )
	{
		FILE *fp = fopen(opt_rx_save, "w");
		if ( fp )
		{
			sd_fmt_opts.prog_func = progress;
			sd_fmt_opts.prog_step = opt_buff / 100;
			if ( !format_write(sd_fmt_bin, &sd_fmt_opts, rx_buff, opt_buff, fp) )
				perror("format_read");
			fclose(fp);
		}
		else
			perror(opt_rx_save);
	}

exit_unmap:
	if ( dsm_cleanup() )
		printf("dsm_cleanup() failed: %s\n", strerror(errno));
	else
		printf("Buffer unmapped with kernel module\n");

exit_free_rx:
	dsm_user_free(rx_buff, opt_buff / DSM_BUS_WIDTH);

exit_free_tx:
	dsm_user_free(tx_buff, opt_buff / DSM_BUS_WIDTH);
	printf("Buffers unlocked and freed\n");

exit_pipe:
	pipe_dev_close();

exit_srio:
	close(srio_dev);

exit_dsm:
	dsm_close();
	return ret;
}

