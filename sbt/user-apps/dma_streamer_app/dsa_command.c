/** \file      dsa_command.c
 *  \brief     implementation of DSA commands
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
#include <unistd.h>
#include <errno.h>

#include <dma_streamer_mod.h>

#include "dsa_main.h"
#include "dsa_ioctl.h"
#include "dsa_ioctl_adi_old.h"
#include "dsa_ioctl_adi_new.h"
#include "dsa_sample.h"
#include "dsa_channel.h"
#include "dsa_common.h"

#include "log.h"
LOG_MODULE_STATIC("command", LOG_LEVEL_INFO);


void dsa_command_options_usage (void)
{
	printf("\nGlobal options: [-qv] [-D mod:lvl] [-s bytes[K|M]] [-S samples[K|M]]\n"
	       "                [-f format] [-t timeout] [-n node]\n"
	       "Where:\n"
	       "-q          Quiet messages: warnings and errors only\n"
	       "-v          Verbose messages: enable debugging\n"
	       "-D mod:lvl  Set debugging level for module\n"
	       "-s bytes    Set buffer size in bytes, add K/B for KB/MB\n"
	       "-S samples  Set buffer size in samples, add K/M for kilo-samples/mega-samples\n"
	       "-f format   Set data format for sample data\n"
	       "-t timeout  Set timeout in jiffies\n"
	       "-n node     Device node for kernelspace module\n\n");
}

int dsa_command_options (int argc, char **argv)
{
	char *ptr;
	int   opt;
	while ( (opt = posix_getopt(argc, argv, "?hqvs:S:f:t:n:D:")) > -1 )
	{
		LOG_DEBUG("dsa_getopt: global opt '%c' with arg '%s'\n", opt, optarg);
		switch ( opt )
		{
			case 'q':
				log_set_global_level(LOG_LEVEL_WARN);
				break;

			case 'v':
				log_set_global_level(LOG_LEVEL_DEBUG);
				break;

			case 's':
				dsa_opt_len = size_bin(optarg);
				if ( dsa_opt_len % DSM_BUS_WIDTH || dsa_opt_len < DSM_BUS_WIDTH )
				{
					LOG_ERROR("Invalid buffer size '%s' - minimum %u, maximum %u,\n"
					          "and must be a multiple of %u\n",
					          optarg, DSM_BUS_WIDTH, DSM_MAX_SIZE, DSM_BUS_WIDTH);
					return -1;
				}
				dsa_opt_len /= DSM_BUS_WIDTH;
				break;

			case 'S':
				dsa_opt_len = size_dec(optarg);
				if ( dsa_opt_len < 1 )
				{
					LOG_ERROR("Invalid buffer size '%s' - minimum %u, maximum %u\n",
					          optarg, 1, DSM_MAX_SIZE / DSM_BUS_WIDTH);
					return -1;
				}
				break;

			case 't':
				errno = 0;
				dsa_opt_timeout = strtoul(optarg, NULL, 0);
				if ( errno )
					return -1;
				LOG_INFO("DMA timeout set to %u jiffies\n", dsa_opt_timeout);
				break;

			case 'n': dsa_opt_device  = optarg; break;

			case 'f':
				if ( !(dsa_opt_format = format_find(optarg)) )
				{
					LOG_ERROR("Invalid format name '%s'\n", optarg);
					return -1;
				}
				break;

			// set debug level for particular module(s)
			case 'D':
				if ( !(ptr = strchr(optarg, ':')) )
					return -1;

				*ptr++ = '\0';
				opt = strtoul(ptr, NULL, 0);
				log_set_module_level(optarg, opt);
				break;

			case '?':
			case 'h':
				return -2;

			default:
				return -1;
		}
	}

	if ( !dsa_opt_format )
		dsa_opt_format = format_find(DEF_FORMAT);
	if ( !dsa_opt_format )
	{
		LOG_DEBUG("No %s format, stop.", DEF_FORMAT);
		return -1;
	}

	return optind;
}


void dsa_command_setup_usage (void)
{
	printf("\nBuffer options: transfer [-zZ] [-s bytes[K|M]] [-S samples[K|M]] [-f format]\n"
	       "                [format:]filename\n"
	       "Where:\n"
	       "-z          Paint buffer before transfer (default)\n"
	       "-Z          Suppress buffer painting\n"
	       "-s bytes    Set buffer size in bytes, add K/B for KB/MB\n"
	       "-S samples  Set buffer size in samples, add K/M for kilo-samples/mega-samples\n"
	       "-f format   Set data format for sample data\n"
	       "The \"transfer\" specifier describes which ADI part(s) and channel(s) to use,\n"
	       "and the direction for each transfer.  It always takes the following form:\n"
	       "  AD 12 T 12\n"
	       "  ^^ ^^ ^ ^^-- Channel: 1 or 2 for single, 12 or omit for both\n"
	       "  |  |  `----- Direction: T for TX, R for RX, never both\n"
	       "  |  `-------- ADI device: 1 or 2 for single, 12 or omit for both\n"
	       "  `----------- AD prefix: required\n\n"
	       "Examples: spaces are not allowed, but any punctuation is allowed for clarity.\n"
	       "Case is ignored, so these are all valid transfer specifications:\n"
	       "- \"ad1:T1\" - Transmit on TX1 on AD1\n"
	       "- \"ad.2/R\" - Receive on RX1 and RX2 of AD2\n"
	       "- \"AD2R12\" - Receive on RX1 and RX2 of AD2\n"
	       "- \"ADR\"    - Receive on RX1 and RX2 of AD1 and AD2\n"
	       "- \"AD12-t\" - Transmit on TX1 and TX2 on AD1 and AD2\n"
	       "- \"ad:T12\" - Transmit on TX1 and TX2 on AD1 and AD2\n"
	       "- \"adt\"    - Transmit on TX1 and TX2 on AD1 and AD2\n\n"
	       "The \"filename\" specifier describes the source of sample data for TX transfers,\n"
	       "or the destination of sample data for RX transfers.  The format may be given\n"
	       "as a prefix, specified with the -f option, or guessed from the filename\n"
	       "extension.  The following printf-style sequences may be used, which are useful\n"
	       "when using multiple ADIs or channels:\n"
	       "- \"%%a\" - expands to the ADI device (1/2)\n"
	       "- \"%%c\" - expands to the channel (1/2)\n"
	       "- \"%%d\" - expands to the direction (t/r, use \"%%D\" for T/R)\n\n"
	       "Examples:\n"
	       "- \"ADR ad%%ar%%c.iqw\" - save four output files:\n"
	       "  \"ad1r1.iqw\" - I/Q data from AD1 RX channel 1\n"
	       "  \"ad1r2.iqw\" - I/Q data from AD1 RX channel 2\n"
	       "  \"ad2r1.iqw\" - I/Q data from AD2 RX channel 1\n"
	       "  \"ad2r2.iqw\" - I/Q data from AD2 RX channel 2\n"
	       "- \"AD1T waveform.iqw\" - load waveform into both TX channels of AD1\n\n");
}

int dsa_command_setup (int sxx, int argc, char **argv)
{
	struct format *fmt   = NULL;
	size_t         len   = 0;
	int            paint = 1;
	int            ret;
	int            ident;

	LOG_DEBUG("dsa_parse_channel(argc %d, argv):\n", argc);
	for ( ret = 0; ret <= argc; ret++ )
		LOG_DEBUG("  argv[%d]: '%s'\n", ret, argv[ret]);

	LOG_DEBUG("  argv[0] '%s' ", argv[0]);
	if ( (ident = dsa_channel_ident(argv[0])) < 1 )
	{
		if ( ident < 0 )
			LOG_ERROR("%s is malformed\n", argv[0]);
		else
			LOG_DEBUG("%s is not a channel-desc\n", argv[0]);
		return ident;
	}
	LOG_DEBUG("  argv[0] '%s' -> %s\n", argv[0], dsa_channel_desc(ident));

	// check the requested buffer mask is supported by the active channels in the
	// ADI9361(s)
	if ( (ret = dsa_channel_check_active(ident)) )
		return ret;

	// sxx < 0 means to use the ident flags instead, when used on the command-line
	// sxx = 0 means no setup of a source/sink allowed, for dmaBuffer
	if ( sxx < 0 )
		sxx = ident;

	optind = 1;
	while ( (ret = posix_getopt(argc, argv, "zZs:S:f:")) > -1 )
	{
		switch ( ret )
		{
			case 'z': paint = 1; break;
			case 'Z': paint = 0; break;

			case 's':
				if ( (len = size_bin(optarg)) < DSM_BUS_WIDTH || len % DSM_BUS_WIDTH )
				{
					LOG_ERROR("Invalid buffer size '%s' - minimum %u, maximum %u,\n"
					          "and must be a multiple of %u\n",
					          optarg, DSM_BUS_WIDTH, DSM_MAX_SIZE, DSM_BUS_WIDTH);
					return -1;
				}
				len /= DSM_BUS_WIDTH;
				break;

			case 'S':
				if ( (len = size_dec(optarg)) < 1 )
				{
					LOG_ERROR("Invalid buffer size '%s' - minimum %u, maximum %u\n",
					          optarg, 1, DSM_MAX_SIZE / DSM_BUS_WIDTH);
					return -1;
				}
				break;

			case 'f':
				if ( !(fmt = format_find(optarg)) )
				{
					LOG_ERROR("Invalid format name '%s'\n", optarg);
					return -1;
				}
				break;

			default:
				return -1;
		}
	}
LOG_DEBUG("len %zu, paint %d, fmt %s, loc %s\n", 
        len, paint, fmt ? fmt->name : "(none)",
		argv[optind] ? argv[optind] : "(none)");

	// for an RX buffer, prefer the local value, fallback to the default value, must be >0
	if ( (ident & DC_DIR_RX) && !len )
	{
		len = dsa_opt_len;
		LOG_INFO("Using RX buffer size %zu samples\n", len);
	}
	if ( (ident & DC_DIR_RX) && !len )
	{
		LOG_ERROR("Invalid RX buffer size, minimum 1 sample\n");
		return -1;
	}

	// for either direction check against the max size
	if ( len > DSM_MAX_SIZE / DSM_BUS_WIDTH )
	{
		LOG_ERROR("Invalid buffer size, maximum %u samples\n", DSM_MAX_SIZE / DSM_BUS_WIDTH);
		return -1;
	}

	// for source buffers, len may be passed as 0 here, and we'll try to get buffer size
	// from the source data file when we _load
	if ( (ret = dsa_channel_buffer(&dsa_evt, ident, len, paint)) < 0 )
	{
		LOG_ERROR("Early buffer alloc (%zu samples) failed: %s\n", len, strerror(errno));
		return ret;
	}
	if ( len )
		LOG_DEBUG("Buffer alloc (%zu samples) passed\n", len);
	else
		LOG_DEBUG("Buffer alloc deferred to datafile load\n");

	if ( sxx && optind < argc && argv[optind] )
	{
		char *p;

		// specifying a format with a prefix, imagemagick-style, equivalent to -f 
		if ( (p = strchr(argv[optind], ':')) )
		{
			*p++ = '\0';
			if ( !(fmt = format_find(argv[optind])) )
			{
				LOG_ERROR("Format '%s' not known\n", argv[optind]);
				return -1;
			}
		}
		// fallback: guess from filename
		else if ( !fmt && (fmt = format_guess(argv[optind])) )
		{
			p = argv[optind];
			LOG_DEBUG("Format '%s' guessed from filename '%s'\n", fmt->name, argv[optind]);
		}
		// fallback: use default format 
		else if ( dsa_opt_format )
			p = argv[optind];
		// failure: no format specified, guessed, or default
		else
		{
			LOG_ERROR("Format neither specified nor guessable from filename\n");
			return -1;
		}

		// if no format specified locally or guessed from filename, use default
		if ( !fmt )
			fmt = dsa_opt_format;

		// expand filename escapes and load data files if appropriate. this may also
		// allocate the buffers if no 
		if ( (ret = dsa_channel_sxx(&dsa_evt, ident, sxx, fmt, p)) < 0 )
		{
			LOG_ERROR("%s: %s\n", p, strerror(errno));
			return ret;
		}

		optind++;
	}
	else
	{
		LOG_ERROR("No filename given.\n");
		return -1;
	}

LOG_DEBUG("dsa_parse_channel(): return %d\n", optind);
	return optind;
}


void dsa_command_trigger_usage (void)
{
	printf("\nTrigger options: [-sSefuc] [reps|once]\n"
	       "Where:\n"
	       "-s  Show statistics for DMA transfers after completion (default)\n"
	       "-S  Suppress statistics display\n"
	       "-e  Debugging: compare expected TX checksum with FPGA value\n"
	       "-f  Debugging: show FIFO counters before and after transfer\n"
	       "-u  Debugging: un-transpose RX data after transfer in software\n"
	       "-c  Debugging: debug FIFO control registers before and after transfer\n"
	       "The \"reps\" may be a number of repetitions to run before returning, or\n"
	       "the word \"once\" for a single run, which is the default if omitted\n\n");
}

int dsa_command_trigger (int argc, char **argv)
{
	unsigned long long  u64;
	unsigned long       sum[2];
	unsigned long       last[2];
	unsigned long       reps     = 1;
	unsigned long       timeout;
	int                 fifo     = 0;
	int                 stats    = 1;
	int                 exp      = 0;
	int                 utp      = 0;
	int                 ctrl     = 0;
	int                 ret;
	int                 dev;

LOG_DEBUG("dsa_command_trigger(argc %d, argv):\n", argc);
for ( ret = 0; ret <= argc; ret++ )
	LOG_DEBUG("  argv[%d]: '%s'\n", ret, argv[ret]);

	//
	optind = 1;
	while ( (ret = posix_getopt(argc, argv, "fsSeuc")) > -1 )
		switch ( ret )
		{
			case 'f': fifo  = 1; break;
			case 's': stats = 1; break;
			case 'S': stats = 0; break;
			case 'e': exp = 1;   break;
			case 'u': utp = 1;   break;
			case 'c': ctrl = 1;  break;

			default:
				return -1;
		}

	// figure number of reps from argument
	if ( !argv[optind] || !strcasecmp(argv[optind], "once") )
	{
		reps = 1;
		LOG_INFO("Set for single trigger...\n");
	}
//	else if ( !strcasecmp(argv[optind], "cont") )
//	{
//		reps = 0;
//		LOG_INFO("Set continuous triggers...\n");
//	}
//	else if ( !strcasecmp(argv[optind], "pause") )
//	{
//		reps  = 0;
//		LOG_INFO("Set paused triggers...\n");
//	}
	else if ( (reps = size_dec(argv[optind])) < 1 )
	{
		LOG_ERROR("Invalid repetition count '%s'\n", argv[optind]);
		return -1;
	}
	else
		LOG_INFO("Set for %lu repetitions...\n", reps);

	// load source buffers before map - allows buffer sized to input data size
	// pass dsa_adi_new as lsh: new ADI PL enforces a 4-bit right-shift on TX data
	if ( dsa_channel_load(&dsa_evt, dsa_adi_new) < 0 )
	{
		LOG_ERROR("Failed to load IQ data: %s\n", strerror(errno));
		return -1;
	}

	if ( exp )
		dsa_channel_calc_exp(&dsa_evt, reps);

	// try mapping once, first time through
	if ( dsa_main_map(reps) )
	{
		LOG_ERROR("DMA mapping failed: %s\n", strerror(errno));
		return -1;
	}

	// set timeout: if user's specified a value use that even if it's wrong. 
	// otherwise calculate from data sizes + margin, minimum 1 sec.
	if ( !(timeout = dsa_opt_timeout) )
		timeout = dsa_channel_timeout(&dsa_evt, 4);
	if ( timeout < 100 )
		timeout = 100;
	dsa_ioctl_set_timeout(timeout);

	if ( reps > 1 && (dsa_evt.rx[0] || dsa_evt.rx[1]) )
		LOG_WARN("Specified %lu reps applies to TX only; RX will run once\n", reps);

	if ( reps )
	{
		// New FIFO controls: Reset only
		if ( dsa_adi_new )
			for ( dev = 0; dev < 2; dev++ ) 
			{ 
				// RX side
				dsa_ioctl_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_RSTN, 0);
				dsa_ioctl_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_RSTN,
				                        ADI_NEW_RX_RSTN);

				// TX side
				dsa_ioctl_adi_new_write(dev, ADI_NEW_TX, ADI_NEW_RX_REG_RSTN, 0);
				dsa_ioctl_adi_new_write(dev, ADI_NEW_TX, ADI_NEW_RX_REG_RSTN,
				                        ADI_NEW_RX_RSTN);
			} 

		// Old FIFO controls: Reset and stop all FIFO controls
		else
			for ( dev = 0; dev < 2; dev++ )
			{
				dsa_ioctl_adi_old_set_ctrl(dev, DSM_LVDS_CTRL_RESET);
				dsa_ioctl_adi_old_set_ctrl(dev, DSM_LVDS_CTRL_STOP);
			}

		// New FIFO controls: Setup channels based on transfer setup
		if ( dsa_adi_new )
			for ( dev = 0; dev < 2; dev++ ) 
			{ 
				// this is a bit of a hack: dev here is 0 for AD1, 1 for AD2, but 
				// dsa_channel_ctrl()'s dev param is a bitmap of DC_DEV_AD1 (=1) and
				// DC_DEV_AD2 (=2).  this should be replaced by a conversion macro
				unsigned long  mask = dsa_channel_ctrl(&dsa_evt, dev + 1, 0);
				unsigned long  reg;

				// RX channel 1 parameters - minimal setup for now
				reg = ADI_NEW_RX_FORMAT_ENABLE;
				if ( mask & DSM_NEW_CTRL_RX1 )
					reg |= ADI_NEW_RX_ENABLE;
				dsa_ioctl_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(0), reg);
				dsa_ioctl_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(1), reg);

				// RX channel 2 parameters - minimal setup for now
				reg = ADI_NEW_RX_FORMAT_ENABLE;
				if ( mask & DSM_NEW_CTRL_RX2 )
					reg |= ADI_NEW_RX_ENABLE;
				dsa_ioctl_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(2), reg);
				dsa_ioctl_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(3), reg);

				// RX control of T1R1 vs T2R2 - maybe this should always be T2R2...
				dsa_ioctl_adi_new_read(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CNTRL, &reg);
				switch ( mask & (DSM_NEW_CTRL_RX1 | DSM_NEW_CTRL_RX2) )
				{
					case DSM_NEW_CTRL_RX1:
					case DSM_NEW_CTRL_RX2:
						reg |= ADI_NEW_RX_R1_MODE;
						break;

					default:
					case DSM_NEW_CTRL_RX1 | DSM_NEW_CTRL_RX2:
						reg &= !ADI_NEW_RX_R1_MODE;
						break;
				}
				dsa_ioctl_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CNTRL, reg);

				// TX channel parameters - minimal setup for now
				if ( dsa_evt.tx[dev] )
				{
					reg  = ADI_NEW_TX_DATA_SEL(ADI_NEW_TX_DATA_SEL_DMA);
					reg |= ADI_NEW_TX_DATA_FORMAT;
					reg &= ~ADI_NEW_TX_R1_MODE;
					dsa_ioctl_adi_new_write(dev, ADI_NEW_TX, ADI_NEW_TX_REG_CNTRL_2, reg);

					switch ( mask & (DSM_NEW_CTRL_TX1 | DSM_NEW_CTRL_TX2) )
					{
						case DSM_NEW_CTRL_TX1:
						case DSM_NEW_CTRL_TX2:
							reg = ADI_NEW_TX_TO_RATE(1);
							break;

						default:
						case DSM_NEW_CTRL_TX1 | DSM_NEW_CTRL_TX2:
							reg = ADI_NEW_TX_TO_RATE(3);
							break;
					}
					dsa_ioctl_adi_new_write(dev, ADI_NEW_TX, ADI_NEW_TX_REG_RATECNTRL, reg);
				}
			} 

		// Old FIFO controls: Program FIFO counters with expected buffer size
		else
			for ( dev = 0; dev < 2; dev++ )
			{
				if ( dsa_evt.tx[dev] )
					dsa_ioctl_adi_old_set_tx_cnt(dev, dsa_evt.tx[dev]->len, reps);
			
				if ( dsa_evt.rx[dev] )
					dsa_ioctl_adi_old_set_rx_cnt(dev, dsa_evt.rx[dev]->len, reps);
			}
	}

	if ( ctrl && !dsa_adi_new ) 
	{
		printf("Before run:\n");
		int r;
		for ( r = 0; r < 4; r++ )
		{
			usleep(25000);
			for ( dev = 0; dev < 2; dev++ )
			{
				unsigned long val;

				ret = dsa_ioctl_adi_old_get_ctrl(dev, &val);
				printf("ADI_G_CTRL  [%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));

				ret = dsa_ioctl_adi_old_get_tx_cnt(dev, &val);
				printf("ADI_G_TX_CNT[%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));

				ret = dsa_ioctl_adi_old_get_rx_cnt(dev, &val);
				printf("ADI_G_RX_CNT[%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));
			}
		}
	}

	// Show FIFO numbers before transfer
	if ( fifo )
	{
		struct dsm_fifo_counts fb;

		dsa_ioctl_adi_old_get_fifo_cnt(&fb);
		dsa_main_show_fifos(&fb);
	}

	// Trigger DMA and block until complete
	LOG_INFO("Triggering DMA...\n");
	errno = 0;
	if ( !dsa_ioctl_trigger() && !stats )
		LOG_INFO("DMA triggered\n");


	// Show FIFO numbers before transfer
	if ( fifo )
	{
		struct dsm_fifo_counts fb;

		dsa_ioctl_adi_old_get_fifo_cnt(&fb);
		dsa_main_show_fifos(&fb);
	}

	// For TX transfers, check 
	if ( exp && !dsa_adi_new ) 
		for ( dev = 0; dev < 2; dev++ )
		{
			if ( !dsa_evt.tx[dev] )
				continue;

			dsa_ioctl_adi_old_get_sum(dev, sum);
			dsa_ioctl_adi_old_get_last(dev, last);

			printf("Last wrd: %08lx%08lx\n", last[0], last[1]);

			u64   = sum[0];
			u64 <<= 32;
			u64  |= sum[1];

			printf("Sink sum: %016llx (%08lx.%08lx)\n", u64, sum[0],  sum[1]);
			printf("Expected: %016llx\n", dsa_evt.tx[dev]->exp);

			if ( u64 > dsa_evt.tx[dev]->exp )
				printf("Larger  : %016llx\n", u64 - dsa_evt.tx[dev]->exp);
			else if ( u64 < dsa_evt.tx[dev]->exp )
				printf("Smaller : %016llx\n", dsa_evt.tx[dev]->exp - u64);
		}

	if ( ctrl && !dsa_adi_new ) 
	{
		for ( dev = 0; dev < 2; dev++ )
		{
			unsigned long val;
			printf("After run:\n");

			ret = dsa_ioctl_adi_old_get_ctrl(dev, &val);
			printf("ADI_G_CTRL  [%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));

			ret = dsa_ioctl_adi_old_get_tx_cnt(dev, &val);
			printf("ADI_G_TX_CNT[%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));

			ret = dsa_ioctl_adi_old_get_rx_cnt(dev, &val);
			printf("ADI_G_RX_CNT[%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));
		}
	}

	if ( stats )
	{
		struct dsm_user_stats  sb;

		dsa_ioctl_get_stats(&sb);

		if ( dsa_evt.tx[0] )  dsa_main_show_stats(&sb.adi1.tx, "AD1 TX");
		if ( dsa_evt.rx[0] )  dsa_main_show_stats(&sb.adi1.rx, "AD1 RX");
		if ( dsa_evt.tx[1] )  dsa_main_show_stats(&sb.adi2.tx, "AD2 TX");
		if ( dsa_evt.rx[1] )  dsa_main_show_stats(&sb.adi2.rx, "AD2 RX");
	}


	// UTP function
	if ( utp )
	{
		uint8_t *RxPacket;
		uint8_t  temp1, temp2;
		size_t   BYTES_TO_RX;
		int      Index;

		for ( dev = 0; dev < 2; dev++ )
			if ( dsa_evt.rx[dev] )
			{
				RxPacket     = (uint8_t *)dsa_evt.rx[dev]->smp;
				BYTES_TO_RX  = dsa_evt.rx[dev]->len * sizeof(struct dsa_sample_pair);
				BYTES_TO_RX -= 16;

				for (Index = 0; Index< BYTES_TO_RX; Index = Index+8){
					temp1 = RxPacket[Index+8] & 0x3F;
					temp2 = RxPacket[Index+10] & 0x3F;
					RxPacket[Index] = (RxPacket[Index] & 0xC0) + temp1;
					RxPacket[Index+2] = (RxPacket[Index+2] & 0xC0) + temp2;
				}
			}
	}

	if ( dsa_main_unmap() )
		LOG_ERROR("DMA unmapping failed: %s\n", strerror(errno));

	// save sink buffers 
	if ( dsa_channel_save(&dsa_evt) < 0 )
		LOG_ERROR("Failed to save IQ data: %s\n", strerror(errno));

	return 0;
}

