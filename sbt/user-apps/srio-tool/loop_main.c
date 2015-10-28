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
#define DEF_TIMEOUT     500

unsigned    opt_local    = 0;
size_t      opt_data     = 10 << 20; // 10MB default
size_t      opt_buff     = 0;
size_t      opt_words    = 0;
unsigned    opt_timeout  = DEF_TIMEOUT;
unsigned    opt_npkts    = 0;
FILE       *opt_debug    = NULL;
uint32_t    opt_sid      = 0;

#ifdef SUPPORT_TYPE9
int         opt_type9    = 0;
#endif

unsigned    opt_tx_chan  = DEF_TX_CHAN;
size_t      opt_tx_rand  = 0;
char       *opt_tx_load  = NULL;
FILE       *opt_tx_file  = NULL;
char       *opt_tx_raw   = NULL;

unsigned    opt_rx_chan  = DEF_RX_CHAN;
char       *opt_rx_save  = NULL;
char       *opt_rx_raw   = NULL;

char  tx_load_buff[4096];


struct packet
{
	uint32_t  tuser;
	uint32_t  padding;
	uint32_t  hello[2];
	uint32_t  data[64];
};

struct format_class *sd_fmt_bin = NULL;
static struct format_options sd_fmt_opts =
{
	.channels = 3,
	.single   = DSM_BUS_WIDTH / 2,
	.sample   = DSM_BUS_WIDTH,
	.bits     = 16,
	.packet   = 272,
	.head     = 16,
	.data     = 256,
	.foot     = 0,
};


static void usage (void)
{
	printf("Usage: srio-loop [-v%spr] [-i file] [-o file] [-s bytes] [-S samples]\n"
	       "                 [-t timeout] [-n npkts] [-C COS] [-b dir:file] [-L local]\n"
	       "                 [stream-id]\n"
	       "Where:\n"
	       "-v          Verbose/debugging enable\n"
#ifdef SUPPORT_TYPE9
	       "-9          Use type 9 when implemented (default type 6)\n"
#endif
	       "-p          Paint transmit payload with counter (default)\n"
	       "-r          Load transmit payload with data from /dev/urandom\n"
	       "-i file     Load transmit payload from file\n"
	       "-o file     Save receive payload to file\n"
	       "-s bytes    Set payload size in bytes (K or M optional)\n"
	       "-S samples  Set payload size in samples (K or M optional)\n"
	       "-t timeout  Set timeout in jiffies (default %u)\n"
	       "-n npkts    Set number of packets for combiner (default from size)\n"
	       "-C COS      Set type 9 COS value (default 0x7F)\n"
	       "-b dir:file Save raw buffer for dir (tx/rx) to file\n"
	       "-L local    SRIO local device-ID (if not auto-probed)\n"
	       "\n"
	       "stream-id is optional and only used for type 9\n"
	       "\n", 
#ifdef SUPPORT_TYPE9
	       "9",
#else
	       "",
#endif
	       DEF_TIMEOUT);

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
	struct packet         *tx_pkt;
	struct packet         *rx_pkt;
	unsigned long          routing;
	unsigned long          reg;
	unsigned long          tuser;
	unsigned long          bad;
	char                  *ptr;
	void                  *tx_buff;
	void                  *rx_buff;
	int                    srio_dev;
	int                    ret = 0;
	int                    opt;
	int                    i, j;

	setbuf(stdout, NULL);

	while ( (opt = getopt(argc, argv, "?hv9rpi:o:s:S:t:n:b:c:L:")) > -1 )
		switch ( opt )
		{
			case 'v':
				opt_debug = stderr;
				format_debug_setup(stderr);
				break;

			case '9':
#ifdef SUPPORT_TYPE9
				opt_type9 = 1;
				break;
#else
				usage();
#endif

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

			case 'o':
				opt_rx_save = optarg;
				break;

			case 's':
				opt_data = (size_bin(optarg) + 7) & ~7;
				break;

			case 'S':
				opt_data  = size_dec(optarg);
				opt_data *= DSM_BUS_WIDTH;
				break;

			case 't': opt_timeout = strtoul(optarg, NULL, 0); break;
			case 'n': opt_npkts   = strtoul(optarg, NULL, 0); break;

			case 'b':
				if ( (ptr = strchr(optarg, ':')) )
					switch ( tolower(*optarg) )
					{
						case 't': opt_tx_raw = ptr + 1; break;
						case 'r': opt_rx_raw = ptr + 1; break;
						default: usage();
					}
				else
					usage();
				break;

			case 'L': opt_local = strtoul(optarg, NULL, 0); break;

			default:
				usage();
		}

	if ( !(sd_fmt_bin = format_class_find("bin")) )
	{
		perror("format_class_find(bin)");
		return 1;
	}

	if ( (argc - optind) >= 1 )
		opt_sid = strtoul(argv[optind + 1], NULL, 0);

	if ( !opt_npkts )
		opt_npkts = format_num_packets_from_data(&sd_fmt_opts, opt_data);
	opt_buff = opt_npkts * sd_fmt_opts.packet;


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

	// allocate page-aligned buffers
	if ( !(tx_buff = dsm_user_alloc(opt_words)) )
	{
		printf("dsm_user_alloc(%zu) failed: %s\n", opt_words, strerror(errno));
		ret = 1;
		goto exit_pipe;
	}
	printf("TX buffer allocated: %lu words / %lu MB\n", opt_words, opt_words >> 17);

	if ( !(rx_buff = dsm_user_alloc(opt_words)) )
	{
		printf("dsm_user_alloc(%zu) failed: %s\n", opt_words, strerror(errno));
		ret = 1;
		goto exit_free_tx;
	}
	printf("RX buffer allocated: %lu words / %lu MB\n", opt_words, opt_words >> 17);


	// Paint RX buffer
	rx_pkt = rx_buff;
	for ( i = 0; i < opt_npkts; i++ )
	{
		// SRIO header
		rx_pkt->tuser    = 0xE3E2E1E0;
		rx_pkt->padding  = 0xE7E6E5E4;
		rx_pkt->hello[0] = 0xEBEAE9E8;
		rx_pkt->hello[1] = 0xEFEEEDEC;

		// Payload area
		memset(rx_pkt->data, 0xDD, sizeof(rx_pkt->data));

		rx_pkt++;
	}

	if ( opt_tx_rand )
	{
		srand(time(NULL));
		if ( (opt_tx_file = tmpfile()) )
		{
			unsigned  word;
			size_t    left = opt_tx_rand;

			printf("Setup random pattern:");
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
		// Paint TX buffer payload area
		tx_pkt = tx_buff;
		for ( i = 0; i < opt_npkts; i++ )
		{
			for ( j = 0; j < sizeof(tx_pkt->data) / sizeof(tx_pkt->data[0]); j++ )
				tx_pkt->data[j] = (j << 26) | (0x3FFFFFF - i);
			tx_pkt++;
		}
	}

	// Setup headers
	tx_pkt = tx_buff;
	for ( i = 0; i < opt_npkts; i++ )
	{
		// SRIO header
		tx_pkt->tuser    = tuser;
		tx_pkt->padding  = 0x00000000;
		tx_pkt->hello[1] = 0x00600000;
		tx_pkt->hello[0] = opt_sid;
		tx_pkt++;
	}


	if ( opt_tx_raw )
		save_buff(opt_tx_raw, tx_buff, opt_buff);


	// hand buffers to kernelspace driver and build scatterlists
	if ( dsm_map_user(opt_tx_chan, 1, tx_buff, opt_words) )
	{
		printf("dsm_map_user() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_free_rx;
	}
	if ( dsm_map_user(opt_rx_chan, 0, rx_buff, opt_words) )
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
	reg &= ~PD_ROUTING_REG_SWRITE_MASK;
	reg |=  PD_ROUTING_REG_SWRITE_DMA;
	pipe_routing_reg_set_adc_sw_dest(reg);

	pipe_srio_dma_split_set_cmd(PD_SRIO_DMA_SPLIT_CMD_RESET);
	pipe_srio_dma_split_set_cmd(0);
	pipe_srio_dma_split_set_npkts(opt_npkts);
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
	tx_pkt = tx_buff;
	rx_pkt = rx_buff;
	for ( i = 0; i < opt_npkts; i++ )
	{
		if ( memcmp(rx_pkt->data, tx_pkt->data, sizeof(rx_pkt->data)) )
		{
			if ( bad < 10 )
				printf("pkt %5d: mismatches:\n", i);
			for ( j = 0; j < sizeof(tx_pkt->data) / sizeof(tx_pkt->data[0]); j++ )
				if ( rx_pkt->data[j] != tx_pkt->data[j] )
				{
					bad++;
					if ( bad < 10 )
						printf("           %2d: %08x / %08x\n", j,
						       rx_pkt->data[j], tx_pkt->data[j]);
				}
		}
	
		tx_pkt++;
		rx_pkt++;
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
	dsm_user_free(rx_buff, opt_words);

exit_free_tx:
	dsm_user_free(tx_buff, opt_words);
	printf("Buffers unlocked and freed\n");

exit_pipe:
	pipe_dev_close();

exit_srio:
	close(srio_dev);

exit_dsm:
	dsm_close();
	return ret;
}

