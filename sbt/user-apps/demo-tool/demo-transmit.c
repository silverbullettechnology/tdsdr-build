/** \file      demo-transmit.c
 *  \brief     Demo tool for transmitting waveform through TD-SDR over SRIO
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
#include <time.h>
#include <arpa/inet.h>
#include <ctype.h>

#include <sd_user.h>
#include <pipe_dev.h>
#include <format.h>
#include <dsm.h>

#include <sbt_common/log.h>
#include <v49_message/resource.h>
#include <v49_client/socket.h>

#include "common.h"
#include "demo-common.h"

LOG_MODULE_STATIC("demo-transmit", LOG_LEVEL_INFO);

#define DEF_DEST       5
#define DEF_CHAN       5
#define DEF_TIMEOUT    500

unsigned    opt_remote   = DEF_DEST;
unsigned    opt_chan     = DEF_CHAN;
size_t      opt_data     = 0;
size_t      opt_buff     = 0;
size_t      opt_words    = 0;
size_t      opt_trail    = 0;
unsigned    opt_timeout  = DEF_TIMEOUT;
unsigned    opt_npkts    = 0;
size_t      opt_body     = 256;
int         opt_paint    = 0;
char       *opt_rawfile  = NULL;
FILE       *opt_debug    = NULL;
uint8_t     opt_cos      = 0x7F;

char                *opt_in_file = NULL;
struct format_class *opt_in_fmt  = NULL;

struct socket *sock;


static struct format_options sd_fmt_opts =
{
	.channels = 0,
	.single   = DSM_BUS_WIDTH / 2,
	.sample   = DSM_BUS_WIDTH,
	.bits     = 12,
	.head     = SRIO_HEAD + VITA_HEAD,
	.flags    = FO_ENDIAN | FO_IQ_SWAP,
};


static void usage (void)
{
	printf("Usage: demo-transmit [-vei12] [-c channel] [-s bytes] [-S samples] [-t timeout]\n"
	       "                     [-R remote] [-p paint] [-z words] [-b bytes] adi-name in-file\n"
	       "Where:\n"
	       "-v            Verbose/debugging enable\n"
	       "-e            Toggle endian swap (default on)\n"
	       "-i            Toggle I/Q swap (default on)\n"
	       "-1            Load channel 1 (default is both)\n"
	       "-2            Load channel 2 (default is both) \n"
	       "-d [mod:]lvl  Debug: set module or global message verbosity (0/focus - 5/trace)\n"
	       "-R remote     SRIO destination device-ID (default %d)\n"
	       "-c channel    DMA channel to use (default %d)\n"
	       "-S samples    Set payload size in samples (K or M optional)\n"
	       "-s bytes      Set payload size in bytes (K or M optional)\n"
	       "-t timeout    Set timeout in jiffies (default %u)\n"
	       "-p paint      Set byte to paint buffer before load (default 0)\n"
	       "-z words      Leave some number of paint samples at the buffer end (default none)\n"
	       "-b bytes      Set PDU body size in bytes (K optional, default 256)\n"
	       "\n"
	       "adi-name is specified as a UUID or human-readable name, and must operate in the\n"
	       "TX direction for transmit\n"
	       "in-file is specified in the typical format of [fmt:]filename[.ext] - if given,\n"
	       "fmt must exactly match a format name, otherwise .ext is used to guess the file\n"
	       "format.\n"
	       "\n", 
		   DEF_DEST, DEF_CHAN, DEF_TIMEOUT);

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

int main (int argc, char **argv)
{
	struct dsm_chan_stats *sb;
	struct srio_header    *srio_hdr;
	struct vita_header    *vita_hdr;
	unsigned long          routing;
	unsigned long          reg;
	unsigned long          tuser;
	uint32_t               hdr;
	uint32_t               smp = 0;
	char                  *ptr;
	char                  *format;
	FILE                  *in_fp = NULL;
	void                  *buff;
	void                  *pkt_base;
	void                  *pkt_head;
	int                    srio_dev;
	int                    ret = 0;
	int                    opt;
	int                    idx;

	log_init_module_list();
	log_dupe(stderr);
	format_error_setup(stderr);

	while ( (opt = getopt(argc, argv, "?hvei12d:R:L:c:s:S:t:p:o:z:b:")) > -1 )
		switch ( opt )
		{
			case 'v':
				opt_debug = stderr;
				format_error_setup(stderr);
				format_debug_setup(stderr);
				break;

			case 'e': sd_fmt_opts.flags ^= FO_ENDIAN;      break;
			case 'i': sd_fmt_opts.flags ^= FO_IQ_SWAP;     break;
			case '1': sd_fmt_opts.channels |= (1 << 0);    break;
			case '2': sd_fmt_opts.channels |= (1 << 1);    break;

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

			case 'R': opt_remote  = strtoul(optarg, NULL, 0); break;
			case 'c': opt_chan    = strtoul(optarg, NULL, 0); break;
			case 't': opt_timeout = strtoul(optarg, NULL, 0); break;
			case 'p': opt_paint   = strtoul(optarg, NULL, 0); break;
			case 'o': opt_rawfile = optarg;                   break;

			case 'z':
				opt_trail = strtoul(optarg, NULL, 0);
				opt_trail *= DSM_BUS_WIDTH;
				break;

			case 's':
				opt_data = (size_bin(optarg) + 7) & ~7;
				break;

			case 'S':
				opt_data  = size_dec(optarg);
				opt_data *= DSM_BUS_WIDTH;
				break;

			case 'b': opt_body = (size_bin(optarg) + 7) & ~7; break;

			default:
				usage();
				return 1;
		}

	sd_fmt_opts.data   = opt_body;
	sd_fmt_opts.packet = sd_fmt_opts.head + sd_fmt_opts.data + sd_fmt_opts.foot;

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

	if ( argc - optind < 2 )
		usage();

	format = argv[optind + 1];
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
		LOG_ERROR("Couldn't identify format\n");
		usage();
		return 1;
	}

	switch ( format_class_read_channels(opt_in_fmt) )
	{
		case FC_CHAN_BUFFER:
			if ( sd_fmt_opts.channels )
				LOG_WARN("Note: -1 or -2 ignored with format %s, loads both channels.\n", 
				         format_class_name(opt_in_fmt));
			break;

		case FC_CHAN_SINGLE:
			if ( sd_fmt_opts.channels == 3 )
			{
				LOG_ERROR("Note: both -1 and -2 given with format %s, only one allowed.\n", 
				         format_class_name(opt_in_fmt));
				return 1;
			}
			break;
	}
	if ( !sd_fmt_opts.channels )
		sd_fmt_opts.channels = 3;

	if ( !opt_data )
	{
		if ( !(in_fp = fopen(opt_in_file, "r")) )
			perror(opt_in_file);
		else if ( !(opt_buff = format_size(opt_in_fmt, &sd_fmt_opts, in_fp)) )
			perror("format_size");
		else
			opt_data = format_size_data_from_buff(&sd_fmt_opts, opt_buff);

		if ( !in_fp )
			return 1;

		if ( !opt_buff )
		{
			fclose(in_fp);
			return 1;
		}
	}
	else if ( !(in_fp = fopen(opt_in_file, "r")) )
		perror(opt_in_file);

	opt_npkts = format_num_packets_from_data(&sd_fmt_opts, opt_data + opt_trail);
	opt_buff  = opt_npkts * sd_fmt_opts.packet;
	opt_words = opt_buff / DSM_BUS_WIDTH;
	printf("Data size %zu (%zu data + %zu trail) gives buffer size %zu, npkts %zu, words %zu\n",
	       opt_data + opt_trail, opt_data, opt_trail, opt_buff, opt_npkts, opt_words);

	LOG_DEBUG("Sizes: buffer %zu, data %zu, npkts %zu\n", opt_buff, opt_data, opt_npkts);


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
	resource_dump(LOG_LEVEL_DEBUG, "Found requested resource:\n  ", &demo_res);

	if ( ! demo_res.txch )
	{
		LOG_ERROR("Resource '%s' has no TX channels, can't transmit with it\n", demo_adi);
		goto exit_sock;
	}


	opt_remote &= 0xFFFF;
	if ( !opt_remote || opt_remote >= 0xFFFF )
	{
		LOG_ERROR("Remote Device-ID 0x%x invalid, must be 0x0001-0xFFFE, stop\n", opt_remote);
		return 1;
	}

	// open DSM dev
	if ( dsm_reopen("/dev/" DSM_DRIVER_NODE) )
	{
		LOG_ERROR("dsm_reopen(%s): %s\n", "/dev/" DSM_DRIVER_NODE, strerror(errno));
		return 1;
	}

	// open SRIO dev, get or set local address
	if ( (srio_dev = open("/dev/" SD_USER_DEV_NODE, O_RDWR|O_NONBLOCK)) < 0 )
	{
		LOG_ERROR("SRIO dev open(%s): %s\n", "/dev/" SD_USER_DEV_NODE, strerror(errno));
		ret = 1;
		goto exit_dsm;
	}
	if ( ioctl(srio_dev, SD_USER_IOCG_LOC_DEV_ID, &tuser) )
	{
		LOG_ERROR("SD_USER_IOCG_LOC_DEV_ID: %s\n", strerror(errno));
		ret = 1;
		goto exit_srio;
	}
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
	     opt_buff > (dsm_channels->list[opt_chan].words * DSM_BUS_WIDTH) ||
	     opt_buff > (dsm_limits.total_words * DSM_BUS_WIDTH) )
	{
		LOG_ERROR("Data size %zu gives invalid buffer size %zu - minimum %u, maximum %lu\n"
		       "per channel, %lu aggregate, and must be a multiple of %u\n",
		       opt_data, opt_buff, DSM_BUS_WIDTH,
		       dsm_channels->list[opt_chan].words * DSM_BUS_WIDTH,
		       dsm_limits.total_words * DSM_BUS_WIDTH,
		       DSM_BUS_WIDTH);
		ret = 1;
		goto exit_pipe;
	}

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


	// setup pipe-dev: route SWRITE to SRIO-DMA, set expected npkts, enable
	if ( pipe_routing_reg_get_adc_sw_dest(&routing) )
	{
		LOG_ERROR("pipe_routing_reg_get_adc_sw_dest() failed: %s\n", strerror(errno));
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
	pipe_srio_dma_split_set_psize((sd_fmt_opts.packet / DSM_BUS_WIDTH) - 1);
	pipe_srio_dma_split_set_cmd(PD_SRIO_DMA_SPLIT_CMD_ENABLE);


	// get access, open device, setup burst length
	if ( seq_access(sock, &demo_cid, &demo_rid, &demo_sid) < 1 )
		goto exit_free;
	LOG_DEBUG("Access granted with SID 0x%x\n", (unsigned)demo_sid);

	if ( seq_open(sock, demo_sid) < 1 )
		goto exit_release;
	LOG_DEBUG("Resource opened OK\n");

	if ( seq_stop(sock, demo_sid, TSTAMP_INT_RELATIVE, 0, opt_data / 8) < 1 )
		goto exit_close;
	LOG_DEBUG("Stop point set at %zu samples\n", opt_data / 8);

	// assemble tuser word with local and remote addresses
	tuser <<= 16;
	tuser  |= opt_remote;
	LOG_DEBUG("tuser word: 0x%08lx\n", tuser);

	// build packet headers - precalc header fields as much as possible
	// length in HELLO header is total packet minus TUSER+HELLO, may need -1
	size_t    packet = sd_fmt_opts.packet - SRIO_HEAD;
	uint32_t  hello0 = (demo_sid << 16) | packet;
	uint32_t  hello1 = (opt_cos  <<  4) | 0x00900000;
	size_t    left   = opt_data;

	// build packet headers - just tuser initially
	pkt_base = buff;
	smp = 0;
	for ( idx = 0; idx < opt_npkts; idx++ )
	{
		srio_hdr = pkt_head = pkt_base;

		// SRIO header - TUSER, alignment, and HELLO header
		srio_hdr->tuser    = tuser;
		srio_hdr->padding  = 0;
		srio_hdr->hello[1] = hello1;
		srio_hdr->hello[0] = hello0;
		if ( left < packet )
		{
			srio_hdr->hello[0] &= 0xFFFF0000;
			srio_hdr->hello[0] |= left & 0xFFFF;
		}

		// VITA49 header always enabled
		pkt_head += sizeof(*srio_hdr);
		vita_hdr = pkt_head;

		if ( left < packet )
			hdr = left;
		else
			hdr = packet;
		hdr >>= 2;
		hdr  |= (idx & 0xf) << 16;
		hdr  |= 0x10300000;	// With SID + V49_HDR_TSF

		vita_hdr->hdr  = ntohl(hdr);
		vita_hdr->sid  = ntohl(demo_sid);
		vita_hdr->tsf1 = 0;
		vita_hdr->tsf2 = ntohl(smp);
		smp += sd_fmt_opts.data / 8;

		// sample advance based on data payload, packet based on payload + headers
		pkt_base += sd_fmt_opts.packet;
		left     -= sd_fmt_opts.data;
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



	// hand buffer to kernelspace driver and build scatterlist
	if ( dsm_map_user(opt_chan, 1, buff, opt_words) )
	{
		LOG_ERROR("dsm_map_user() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_close;
	}
	LOG_DEBUG("Buffer mapped with kernel module\n");


	LOG_FOCUS("Ready to start, press a key to transmit...");
	terminal_pause();

	LOG_INFO("Send START command... ");
	if ( seq_start(sock, demo_sid, TSTAMP_INT_IMMEDIATE, 0, 0) < 1 )
		goto exit_close;

	// start DMA transaction on all mapped channels, then start data-source module
	LOG_INFO("Sent and ack'd, triggering DMA... ");
	if ( dsm_oneshot_start(~0) )
	{
		LOG_ERROR("dsm_oneshot_start() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_unmap;
	}

	// wait for started DMA channels to finish or timeout
	LOG_INFO("triggered, wait for DMA to finish... ");
	if ( dsm_oneshot_wait(~0) )
	{
		LOG_ERROR("dsm_oneshot_wait() failed: %s\n", strerror(errno));
		ret = 1;
		goto exit_unmap;
	}
	LOG_INFO("\nDMA finished, waiting for buffer to drain...");

	sleep(5);
	LOG_INFO("drained.\n");

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

		LOG_INFO("Transfer statistics:\n");

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


exit_unmap:
	if ( dsm_cleanup() )
		LOG_ERROR("dsm_cleanup() failed: %s\n", strerror(errno));
	else
		LOG_INFO("Buffer unmapped with kernel module\n");

exit_close:
	seq_close(sock, demo_sid);

exit_release:
	seq_release(sock, demo_sid);

exit_free:
	dsm_user_free(buff, opt_words);
	LOG_ERROR("Buffer unlocked and freed\n");

exit_pipe:
	pipe_dev_close();

exit_srio:
	close(srio_dev);

exit_dsm:
	dsm_close();

exit_sock:
	socket_close(sock);

	return ret;
}

