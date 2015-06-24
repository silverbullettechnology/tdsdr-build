/** \file      demo-sample.c
 *  \brief     Demo tool for sampling waveform from TD-SDR over SRIO
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
#include <time.h>
#include <ctype.h>

#include <pipe_dev.h>
#include <format.h>
#include <dsm.h>

#include <sbt_common/log.h>
#include <v49_message/log.h>
#include <v49_client/log.h>

#include <v49_message/resource.h>
#include <v49_client/socket.h>

#include "common.h"
#include "demo-common.h"


LOG_MODULE_STATIC("demo-sample", LOG_LEVEL_DEBUG);


unsigned    opt_chan     = 4;
unsigned    opt_remote   = 5;
size_t      opt_data     = 8000000;
size_t      opt_buff     = 0;
size_t      opt_words    = 0;
unsigned    opt_timeout  = 1500;
unsigned    opt_npkts    = 0;
FILE       *opt_debug    = NULL;
char       *opt_rawfile  = NULL;
char       *opt_sock_addr = NULL;

char                *opt_out_file = NULL;
struct format_class *opt_out_fmt  = NULL;

struct socket *sock;


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
	.channels = 3,
	.single   = DSM_BUS_WIDTH / 2,
	.sample   = DSM_BUS_WIDTH,
	.bits     = 16,
	.packet   = 272,
	.head     = 36,
	.data     = 232,
	.foot     = 4,
	.flags    = FO_ENDIAN,
};


static void usage (void)
{
	printf("Usage: demo-sample [-v] [-d lvl] [-R remote] [-s|-S size] [-t timeout] [-o raw]\n"
	       "                  adi-name out-file\n"
	       "Where:\n"
	       "-v            Verbose/debugging enable\n"
	       "-d [mod:]lvl  Debug: set module or global message verbosity (0/focus - 5/trace)\n"
	       "-S size       Set payload size in samples (K or M optional)\n"
	       "-s size       Set payload size in bytes (K or M optional)\n"
	       "-t timeout    Set timeout in jiffies\n"
	       "-o rawfile    Write raw buffer to rawfile (with packet headers)\n"
	       "\n"
	       "adi-name is specified as a UUID or human-readable name, and must operate in the\n"
	       "RX direction for sampling\n"
	       "\n"
	       "out-file is specified in the typical format of [fmt:]filename[.ext] - if given,\n"
	       "fmt must exactly match a format name, otherwise .ext is used to guess the file\n"
	       "format.\n"
		   "\n");
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
		LOG_ERROR("Failed!");
	else if ( !done )
		LOG_INFO("  0%%");
	else if ( done >= size )
		LOG_INFO("\b\b\b\b100%%\n");
	else
	{
		unsigned long long prog = done;
		prog *= 100;
		prog /= size;
		LOG_INFO("\b\b\b\b%3llu%%", prog);
	}
}

int main (int argc, char **argv)
{
	struct dsm_chan_stats *sb;
//	struct recv_packet    *pkt;
	unsigned long          routing;
	unsigned long          reg;
	char                  *format;
	char                  *ptr;
	void                  *buff;
	int                    ret = 0;
	int                    opt;
//	int                    idx;

	log_init_module_list();
	log_dupe(stderr);
	log_set_global_level(module_level);

	format_error_setup(stderr);
	while ( (opt = getopt(argc, argv, "?hvd:c:s:S:t:n:o:")) > -1 )
		switch ( opt )
		{
			case 'v':
				opt_debug = stderr;
				format_debug_setup(stderr);
				break;

			// set debug verbosity level and enable trace
			case 'd':
				if ( (ptr = strchr(optarg, ':')) || (ptr = strchr(optarg, '=')) ) // for particular module(s)
				{
					*ptr++ = '\0';
					if ( (opt = log_level_for_label(ptr)) < 0 && isdigit(*ptr) )
						opt = strtoul(ptr, NULL, 0);
					if ( log_set_module_level(optarg, opt) )
					{
						if ( errno == ENOENT )
						{
							char **list = log_get_module_list();
							char **walk;
							printf("Available modules: {");
							for ( walk = list; *walk; walk++ )
								printf(" %s", *walk);
							printf(" }\n");
							free(list);
						}
						usage();
					}
				}
				else // globally
				{
					if ( (opt = log_level_for_label(optarg)) < 0 && isdigit(*optarg) )
						opt = strtoul(optarg, NULL, 0);
					fprintf(stderr, "optarg '%s' -> %d\n", optarg, opt);
					if ( log_set_global_level(opt) )
						usage();
				}
				log_trace(1);
				break;

			case 'c': opt_chan    = strtoul(optarg, NULL, 0); break;
			case 't': opt_timeout = strtoul(optarg, NULL, 0); break;
			case 'n': opt_npkts   = strtoul(optarg, NULL, 0); break;
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
		}
	
	if ( argc - optind < 2 )
		usage();

	format = argv[optind + 1];
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
		LOG_ERROR("Couldn't identify format for '%s'\n", argv[optind]);
		usage();
	}


	if ( !(sock = socket_alloc("srio")) )
	{
		LOG_ERROR("SRIO socket alloc failed: %s\n", strerror(errno));
		usage();
	}
	else
	{
		char  addr[32];
		snprintf(addr, sizeof(addr), "0,%u", opt_remote);
		if ( socket_cmdline(sock, "addr", addr) )
		{
			LOG_ERROR("Bad socket address: '%s'\n", addr);
			goto exit_sock;
		}
		LOG_DEBUG("Configured sock with addr '%s'\n", addr);
	}

	// open connection 
	if ( socket_check(sock) < 0 )
	{
		LOG_ERROR("Failed to connect to server: %s", strerror(errno));
		goto exit_sock;
	}

	srand(time(NULL));
	uuid_random(&demo_cid);

	// query server for requested resource, demo_adi can be an ASCII UUID or name
	demo_adi = argv[optind];
	LOG_DEBUG("demo_adi '%s'\n", demo_adi);
	if ( seq_enum(sock, &demo_cid, &demo_res, demo_adi) < 1 )
		goto exit_sock;

	memcpy(&demo_rid, &demo_res.uuid, sizeof(demo_rid));
	resource_dump(LOG_LEVEL_DEBUG, "Found requested resource: ", &demo_res);


	// open devs
	if ( dsm_reopen("/dev/" DSM_DRIVER_NODE) )
	{
		LOG_ERROR("dsm_reopen(%s): %s\n", "/dev/" DSM_DRIVER_NODE, strerror(errno));
		ret = 1;
		goto exit_sock;
	}
	if ( pipe_dev_reopen("/dev/" PD_DRIVER_NODE) )
	{
		LOG_ERROR("pipe_dev_reopen(%s): %s\n", "/dev/" PD_DRIVER_NODE, strerror(errno));
		ret = 1;
		goto exit_dsm;
	}

	// validate channel number and direction
	if ( opt_chan >= dsm_channels->size )
	{
		LOG_ERROR("Invalid channel %u, available channels:\n", opt_chan);
		dump_channels();
		ret = 1;
		goto exit_pipe;
	}
	if ( !(dsm_channels->list[opt_chan].flags & DSM_CHAN_DIR_RX) )
	{
		LOG_ERROR("Channel %u can't do RX, available channels:\n", opt_chan);
		dump_channels();
		ret = 1;
		goto exit_pipe;
	}

	if ( !opt_npkts )
		opt_npkts = format_num_packets_from_data(&sd_fmt_opts, opt_data);

	// size buffer in total number of packets, since swrite block rounds up... TBD whether
	// this is a good idea or not
	opt_buff = opt_npkts * sd_fmt_opts.packet;

	// validate buffer size against width and channel / total limits
	if ( opt_buff % DSM_BUS_WIDTH || 
	     opt_buff < DSM_BUS_WIDTH || 
	     opt_buff > dsm_channels->list[opt_chan].words ||
	     opt_buff > dsm_limits.total_words )
	{
		LOG_ERROR("Data size %zu gives invalid buffer size %zu - minimum %u, maximum %lu,\n"
		       "and must be a multiple of %u\n",
		       opt_data, opt_buff, DSM_BUS_WIDTH, dsm_limits.total_words * DSM_BUS_WIDTH,
		       DSM_BUS_WIDTH);
		ret = 1;
		goto exit_pipe;
	}
	opt_words = opt_buff / DSM_BUS_WIDTH;
	LOG_DEBUG("Data size %zu gives buffer size %zu, npkts %zu, words %zu\n",
	          opt_data, opt_buff, opt_npkts, opt_words);

	// clamp timeout and set
	if ( opt_timeout < 100 )
		opt_timeout = 100;
	else if ( opt_timeout > 6000 )
		opt_timeout = 6000;
	if ( dsm_set_timeout(opt_timeout) ) 
	{
		LOG_ERROR("dsm_set_timeout(%u) failed: %s\n", opt_timeout, strerror(errno));
		ret = 1;
		goto exit_pipe;
	}
	LOG_DEBUG("Timeout set to %u jiffies\n", opt_timeout);

	// allocate page-aligned buffer
	if ( !(buff = dsm_user_alloc(opt_words)) )
	{
		LOG_ERROR("dsm_user_alloc(%zu) failed: %s\n", opt_words, strerror(errno));
		ret = 1;
		goto exit_pipe;
	}
	LOG_DEBUG("Buffer allocated: %zu words / %zu MB\n", opt_words, opt_words >> 17);
	memset(buff, 0, opt_buff);

	// hand buffer to kernelspace driver and build scatterlist
	if ( dsm_map_user(opt_chan, 0, buff, opt_words) )
	{
		LOG_ERROR("dsm_map_user() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_free;
	}
	LOG_DEBUG("Buffer mapped with kernel module\n");


	// setup pipe-dev: route SWRITE to SRIO-DMA, set expected npkts, enable
	if ( pipe_routing_reg_get_adc_sw_dest(&routing) )
	{
		LOG_ERROR("pipe_routing_reg_get_adc_sw_dest() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_unmap;
	}
	reg  = routing;
	reg &= ~PD_ROUTING_REG_SWRITE_MASK;
	reg |=  PD_ROUTING_REG_SWRITE_DMA;
	pipe_routing_reg_set_adc_sw_dest(reg);

	pipe_srio_dma_comb_set_cmd(PD_SRIO_DMA_COMB_CMD_RESET);
	pipe_srio_dma_comb_set_cmd(0);
	pipe_srio_dma_comb_set_npkts(opt_npkts);
	pipe_srio_dma_comb_set_cmd(PD_SRIO_DMA_COMB_CMD_ENABLE);


	// get access, open device, setup burst length
	if ( seq_access(sock, &demo_cid, &demo_rid, &demo_sid) < 1 )
		goto exit_unmap;
	LOG_DEBUG("Access granted with SID %x\n", (unsigned)demo_sid);

	if ( seq_open(sock, demo_sid) < 1 )
		goto exit_release;
	LOG_DEBUG("Resource opened OK\n");

	if ( seq_stop(sock, demo_sid, TSTAMP_INT_RELATIVE, 0, opt_words) < 1 )
		goto exit_close;
	LOG_DEBUG("Stop point set at %zu samples\n", opt_words);


	LOG_FOCUS("Ready to start...");
	terminal_pause();


	// start DMA transaction on all mapped channels, then start data-source module
	LOG_INFO("Triggering DMA... ");
	if ( dsm_oneshot_start(~0) )
	{
		LOG_ERROR("dsm_oneshot_start() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_close;
	}

	seq_start(sock, demo_sid, TSTAMP_INT_IMMEDIATE, 0, 0);

	// wait for started DMA channels to finish or timeout
	LOG_INFO("\nWaiting for DMA to finish... ");
	if ( dsm_oneshot_wait(~0) )
	{
		printf("dsm_oneshot_wait() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_close;
	}
	LOG_INFO("\nDMA finished.");

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

		LOG_INFO("DMA statistics:\n");

		usec *= 1000000000;
		usec += st->total.tv_nsec;
		usec /= 1000;

		if ( usec > 0 )
			rate = st->bytes / usec;

		LOG_INFO("  bytes    : %llu\n",        st->bytes);
		LOG_INFO("  time     : %lu.%09lu s\n", st->total.tv_sec, st->total.tv_nsec);
		LOG_INFO("  rate     : %llu MB/s\n"  , rate);
		LOG_INFO("  starts   : %lu\n",         st->starts);
		LOG_INFO("  completes: %lu\n",         st->completes);
		LOG_INFO("  errors   : %lu\n",         st->errors);
		LOG_INFO("  timeouts : %lu\n",         st->timeouts);
	}
	else
		LOG_ERROR("dsm_get_stats() failed: %s\n", strerror(errno));
	free(sb);

	// write out data received if we have an output file/format, otherwise just hexdump
	if ( opt_out_file && opt_out_fmt )
	{
		sd_fmt_opts.prog_func = progress;
		sd_fmt_opts.prog_step = opt_buff / 100;

		opt_buff = format_size_buff_from_data(&sd_fmt_opts, opt_data);

		FILE *fp = fopen(opt_out_file, "w");
		if ( !fp )
			perror(opt_out_file);
		else if ( !format_write(opt_out_fmt, &sd_fmt_opts, buff, opt_buff, fp) )
			perror("format_write");

		fclose(fp);
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


exit_close:
	seq_close(sock, demo_sid);

exit_release:
	seq_release(sock, demo_sid);

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

exit_sock:
	socket_close(sock);

	return ret;
}

