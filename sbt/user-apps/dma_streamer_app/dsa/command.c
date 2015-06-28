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
#include <ctype.h>
#include <errno.h>

#include <dma_streamer_mod.h>
#include <fifo_dev.h>
#include <pipe_dev.h>
#include <dsm.h>
#include <dsa_util.h>

#include "dsa/main.h"
#include "dsa/sample.h"
#include "dsa/channel.h"
#include "common/common.h"

#ifdef DSA_USE_PMON
#include "pmon/pmon.h"
#endif // DSA_USE_PMON

#include "common/log.h"
LOG_MODULE_STATIC("command", LOG_LEVEL_INFO);


void dsa_command_options_usage (void)
{
	printf("\nGlobal options: [-qv] [-D mod:lvl] [-s bytes[K|M]] [-S samples[K|M]]\n"
	       "                [-f format] [-t timeout] [-p priority] [-m mode]\n"
	       "Where:\n"
	       "-q          Quiet messages: warnings and errors only\n"
	       "-v          Verbose messages: enable debugging\n"
	       "-D mod:lvl  Set debugging level for module\n"
	       "-s bytes    Set buffer size in bytes, add K/B for KB/MB\n"
	       "-S samples  Set buffer size in samples, add K/M for kilo-samples/mega-samples\n"
	       "-f format   Set data format for sample data\n"
	       "-t timeout  Set timeout in jiffies\n"
	       "-p priority Priority for pipe-dev access request\n"
	       "-m mode     DMA mode select for new ADI FIFOs (0-15, default 4)\n"
	       "\n");
}

int dsa_command_options (int argc, char **argv)
{
	char *ptr;
	int   opt;
	while ( (opt = getopt(argc, argv, "+?hqvs:S:f:t:D:A:p:m:")) > -1 )
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
					LOG_ERROR("Invalid buffer size '%s' - minimum %u, maximum %lu,\n"
					          "and must be a multiple of %u\n",
					          optarg, DSM_BUS_WIDTH, 
					          dsm_limits.total_words * DSM_BUS_WIDTH,
					          DSM_BUS_WIDTH);
					return -1;
				}
				dsa_opt_len /= DSM_BUS_WIDTH;
				break;

			case 'S':
				dsa_opt_len = size_dec(optarg);
				if ( dsa_opt_len < 1 )
				{
					LOG_ERROR("Invalid buffer size '%s' - minimum %u, maximum %lu\n",
					          optarg, 1, dsm_limits.total_words);
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

			case 'f':
				if ( !(dsa_opt_format = format_class_find(optarg)) )
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

			// priority may be a 32-bit unsigned, or a decimal fixed-point
			case 'p':
				dsa_opt_priority = strtoul(optarg, &ptr, 0);
				if ( *ptr && *ptr == '.' )
				{
					ptr++;
					char *end;
					uint64_t  frac = strtoull(ptr, &end, 0);
					if ( (end - ptr) > 5 )
						return -1;
					frac <<= 16;
					frac /= pow10_32(end - ptr);
					if ( frac & ~0xFFFF )
						LOG_WARN("optarg '%s' -> int %lu, frac '%s' -> %08llx?\n",
						         optarg, dsa_opt_priority, ptr, frac);

					dsa_opt_priority <<= 16;
					dsa_opt_priority  |= (frac & 0xFFFF);
				}
				LOG_DEBUG("optarg '%s' -> prio %08lx -> %lu.%04lu\n",
				          optarg, dsa_opt_priority, dsa_opt_priority >> 16,
				          ((dsa_opt_priority & 0xFFFF) * 10000) >> 16);
				break;

			case 'm':
				errno = 0;
				dsa_adi_mode = strtoul(optarg, NULL, 0);
				if ( errno || dsa_adi_mode > 15 )
					return -1;
				LOG_INFO("DMA mode set to %d\n", dsa_adi_mode);
				break;

			case '?':
			case 'h':
				return -2;

			default:
				return -1;
		}
	}

	if ( !dsa_opt_format )
		dsa_opt_format = format_class_find(DEF_FORMAT);
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
	struct format_class *fmt   = NULL;
	size_t               len   = 0;
	int                  paint = 1;
	int                  ret;
	int                  ident;

	LOG_DEBUG("dsa_parse_channel(argc %d, argv):\n", argc);
	for ( ret = 0; ret <= argc; ret++ )
		LOG_DEBUG("  argv[%d]: '%s'\n", ret, argv[ret]);

	LOG_DEBUG("  argv[0] '%s' ", argv[0]);
	if ( (ident = dsa_util_spec_parse(argv[0])) < 1 )
	{
		if ( ident < 0 )
			LOG_ERROR("%s is malformed\n", argv[0]);
		else
			LOG_DEBUG("%s is not a channel-desc\n", argv[0]);
		return ident;
	}
	LOG_DEBUG("  argv[0] '%s' -> %s\n", argv[0], dsa_util_chan_desc(ident));

	// check the requested buffer mask is supported by the active channels in the
	// ADI9361(s)
	if ( (ret = dsa_channel_check_active(ident)) )
		return ret;

	// TODO: reserve required channels in pipe-dev if open

	// sxx < 0 means to use the ident flags instead, when used on the command-line
	// sxx = 0 means no setup of a source/sink allowed, for dmaBuffer
	if ( sxx < 0 )
		sxx = ident;

	optind = 1;
	while ( (ret = getopt(argc, argv, "+zZs:S:f:")) > -1 )
	{
		switch ( ret )
		{
			case 'z': paint = 1; break;
			case 'Z': paint = 0; break;

			case 's':
				if ( (len = size_bin(optarg)) < DSM_BUS_WIDTH || len % DSM_BUS_WIDTH )
				{
					LOG_ERROR("Invalid buffer size '%s' - minimum %u, maximum %lu,\n"
					          "and must be a multiple of %u\n",
					          optarg, DSM_BUS_WIDTH, 
					          dsm_limits.total_words * DSM_BUS_WIDTH,
					          DSM_BUS_WIDTH);
					return -1;
				}
				len /= DSM_BUS_WIDTH;
				break;

			case 'S':
				if ( (len = size_dec(optarg)) < 1 )
				{
					LOG_ERROR("Invalid buffer size '%s' - minimum %u, maximum %lu\n",
					          optarg, 1, dsm_limits.total_words);
					return -1;
				}
				break;

			case 'f':
				if ( !(fmt = format_class_find(optarg)) )
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
        len, paint, fmt ? format_class_name(fmt) : "(none)",
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

	// for either direction check against the max size - strictly speaking this should be
	// against the appropriate DMA channel's limit, but presently it's the same value as
	// the total_words limit
	if ( len > dsm_limits.total_words )
	{
		LOG_ERROR("Invalid buffer size, maximum %lu samples\n",
		          dsm_limits.total_words);
		return -1;
	}

	// add to total and test that too
	dsa_total_words += len;
	if ( dsa_total_words > dsm_limits.total_words )
	{
		LOG_ERROR("Total buffer sizes too large, maximum aggregate %lu samples\n",
		          dsm_limits.total_words);
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
			if ( !(fmt = format_class_find(argv[optind])) )
			{
				LOG_ERROR("Format '%s' not known\n", argv[optind]);
				return -1;
			}
		}
		// fallback: guess from filename
		else if ( !fmt && (fmt = format_class_guess(argv[optind])) )
		{
			p = argv[optind];
			LOG_DEBUG("Format '%s' guessed from filename '%s'\n", 
			          format_class_name(fmt), argv[optind]);
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

	// request access to stack
	if ( dsa_adi_new && dsa_pipe_dev )
		if ( (ret = dsa_channel_access_request(ident, dsa_opt_priority)) )
			return ret;

LOG_DEBUG("dsa_parse_channel(): return %d\n", optind);
	return optind;
}


void dsa_command_trigger_usage (void)
{
	printf("\nTrigger options: [-sSMefucp] [reps|once|cont|press [reps]]\n"
	       "Where:\n"
	       "-s  Show statistics for DMA transfers after completion (default)\n"
	       "-S  Suppress statistics display\n"
	       "-M  Forbid changing ENSM mode: user must setup AD9361s in correct mode\n"
	       "-e  Debugging: compare expected TX checksum with FPGA value\n"
	       "-f  Debugging: show FIFO counters before and after transfer\n"
	       "-u  Debugging: un-transpose RX data after transfer in software\n"
	       "-c  Debugging: debug FIFO control registers before and after transfer\n\n"
#ifdef DSA_USE_PMON
	       "-p  Debugging: collect pmon stats\n\n"
#endif
	       "The positional arguments support several operating modes:\n"
	       "- reps may be a numeric number of repetitions to trigger, once, before cleaning\n"
	       "  up and exiting.  The number may suffixed with K or M for convenience.\n"
	       "- The word \"once\" is equivalent to a reps value of 1, and is the default.\n"
	       "- The word \"cont\" means to repeat continuously until a key is pressed.\n"
	       "- The word \"press\" means to wait until a key is pressed before each trigger.\n"
	       "  After the first trigger, pressing Q, Esc, or arrow/function keys will\n"
	       "  break the loop and exit, any other key will trigger another transfer.\n"
	       "  A repetition count as described above may be given after \"press\" to specify the\n"
	       "  number of repetitions per keypress.\n\n");
}

typedef enum
{
	TRIG_ONCE,
	TRIG_PRESS,
	TRIG_CONT,
}
trigger_t;


static const char *dsa_command_dev_name (int slot)
{
	int  dev = dsm_channels->list[slot].private >> 28 ? DC_DEV_AD2 : DC_DEV_AD1;
	dev |= (dsm_channels->list[slot].flags & DSM_CHAN_DIR_TX) ? DC_DIR_TX : DC_DIR_RX;

	switch ( dev )
	{
		case DC_DEV_AD1 | DC_DIR_TX: return dsa_dsxx ? "DSNK1" : "AD1 TX";
		case DC_DEV_AD1 | DC_DIR_RX: return dsa_dsxx ? "DSRC1" : "AD1 RX";
		case DC_DEV_AD2 | DC_DIR_TX: return dsa_dsxx ? "DSNK2" : "AD2 TX";
		case DC_DEV_AD2 | DC_DIR_RX: return dsa_dsxx ? "DSRC2" : "AD2 RX";
	}

	return "???";
}

void dsa_command_show_stats (const struct dsm_xfer_stats *st, int slot)
{
	printf("%s stats:\n", dsa_command_dev_name(slot));
	if ( !st->starts && !st->errors )
	{
		printf ("  not run\n");
		return;
	}

	unsigned long long usec = st->total.tv_sec;
	usec *= 1000000000;
	usec += st->total.tv_nsec;
	usec /= 1000;

	unsigned long long rate = 0;
	if ( usec > 0 )
		rate = st->bytes / usec;

	printf("  bytes    : %llu\n",      st->bytes);
	printf("  total    : %lu.%09lu\n", st->total.tv_sec, st->total.tv_nsec);
	printf("  rate     : %llu MB/s\n", rate);

	printf("  starts   : %lu\n",       st->starts);
	printf("  completes: %lu\n",       st->completes);
	printf("  errors   : %lu\n",       st->errors);
	printf("  timeouts : %lu\n",       st->timeouts);
}

void dsa_command_show_fifos (const struct fd_fifo_counts *buff)
{
	printf("  RX 1: %08lx/%08lx\n", buff->rx_1_ins, buff->rx_1_ext);
	printf("  RX 2: %08lx/%08lx\n", buff->rx_2_ins, buff->rx_2_ext);
	printf("  TX 1: %08lx/%08lx\n", buff->tx_1_ins, buff->tx_1_ext);
	printf("  TX 2: %08lx/%08lx\n", buff->tx_2_ins, buff->tx_2_ext);
}

static void dsa_command_trigger_start (void)
{
	unsigned long reg;
	int           dev;

	// New FIFO controls
	if ( dsa_adi_new )
	{
		for ( dev = 0; dev < 2; dev++ )
			if ( dsa_evt.tx[dev] )
			{
				fifo_adi_new_read(dev, ADI_NEW_TX, ADI_NEW_TX_REG_CNTRL_1, &reg);
				reg |= ADI_NEW_TX_ENABLE;
				fifo_adi_new_write(dev, ADI_NEW_TX, ADI_NEW_TX_REG_CNTRL_1, reg);
			}
	}

	// Data source/sink module
	else if ( dsa_dsxx )
		for ( dev = 0; dev < 2; dev++ )
		{
			if ( dsa_evt.tx[dev] )
				fifo_dsrc_set_ctrl(dev, FD_DSXX_CTRL_ENABLE);

			if ( dsa_evt.rx[dev] )
				fifo_dsrc_set_ctrl(dev, FD_DSXX_CTRL_ENABLE);
		}

	// Old FIFO controls
	else
		for ( dev = 0; dev < 2; dev++ )
		{
			reg = dsa_channel_ctrl(&dsa_evt, dev, 1);
			if ( dsa_evt.tx[dev] || dsa_evt.rx[dev] )
				fifo_adi_old_set_ctrl(dev, reg);
		}
}

int dsa_command_trigger (int argc, char **argv)
{
	unsigned long long  u64;
	unsigned long       sum[2];
	unsigned long       last[2];
	unsigned long       reps     = 1;
	unsigned long       timeout;
	unsigned long       reg;
	trigger_t           trig = TRIG_ONCE;
	int                 fifo     = 0;
	int                 stats    = 1;
	int                 exp      = 0;
	int                 utp      = 0;
	int                 ctrl     = 0;
	int                 ensm     = 0;
#ifdef DSA_USE_PMON
	int                 pmon     = 0;
#endif // DSA_USE_PMON
	int                 ret;
	int                 dev;

LOG_DEBUG("dsa_command_trigger(argc %d, argv):\n", argc);
for ( ret = 0; ret <= argc; ret++ )
	LOG_DEBUG("  argv[%d]: '%s'\n", ret, argv[ret]);

	//
	optind = 1;
	while ( (ret = getopt(argc, argv, "+fsSMeucp")) > -1 )
		switch ( ret )
		{
			case 'f': fifo  = 1; break;
			case 's': stats = 1; break;
			case 'S': stats = 0; break;
			case 'M': ensm = 1;  break;
			case 'e': exp = 1;   break;
			case 'u': utp = 1;   break;
			case 'c': ctrl = 1;  break;
#ifdef DSA_USE_PMON
			case 'p': pmon = 1; break;
#endif // DSA_USE_PMON

			default:
				return -1;
		}

	errno = 0;
#ifdef DSA_USE_PMON
	if ( pmon && (Init_Pmon() || errno) )
	{
		LOG_ERROR("pmon failed to init, stop\n");
		return -1;
	}
#endif // DSA_USE_PMON

	// figure number of reps from argument
	if ( !argv[optind] || !strcasecmp(argv[optind], "once") )
	{
		trig = TRIG_ONCE;
		LOG_INFO("Set for single trigger...\n");
	}
	else if ( !strcasecmp(argv[optind], "cont") )
	{
		trig = TRIG_CONT;
		if ( dsa_adi_new )
			LOG_INFO("Set continuous triggers...\n");
		else
		{
			LOG_ERROR("Continuous transfer requires new ADI controls\n");
			return -1;
		}
	}
	else if ( !strcasecmp(argv[optind], "press") )
	{
		trig = TRIG_PRESS;
		if ( dsa_adi_new )
			LOG_INFO("Set to trigger per keypress...\n");
		if ( argv[optind + 1] )
			optind++;
	}

	if ( argv[optind] && isdigit(argv[optind][0]) && (reps = size_dec(argv[optind])) < 1 )
	{
		LOG_ERROR("Invalid repetition count '%s'\n", argv[optind]);
		return -1;
	}
	if ( reps > 1 )
		LOG_INFO("Set for %lu repetitions...\n", reps);

	// load source buffers before map - allows buffer sized to input data size
	// pass dsa_adi_new as lsh: new ADI PL enforces a 4-bit right-shift on TX data
	if ( dsa_channel_load(&dsa_evt, dsa_adi_new) < 0 )
	{
		LOG_ERROR("Failed to load IQ data: %s\n", strerror(errno));
		return -1;
	}

	if ( exp )
		dsa_channel_calc_exp(&dsa_evt, reps, dsa_dsxx);

	// mapping for DMA
	for ( dev = 0; dev < 2; dev++ )
	{
		int err = 0;

		if ( dsa_evt.tx[dev] )
		{
			ret = dsm_map_user(dsa_dsm_tx_channels[dev], 1,
			                   dsa_evt.tx[dev]->smp, dsa_evt.tx[dev]->len);
			if ( ret ) 
				err++;
		}

		if ( dsa_evt.rx[dev] )
		{
			ret = dsm_map_user(dsa_dsm_rx_channels[dev], 0,
			                   dsa_evt.rx[dev]->smp, dsa_evt.rx[dev]->len);
			if ( ret ) 
				err++;
		}

		if ( err )
		{
			LOG_ERROR("DMA mapping failed: %s\n", strerror(errno));
			dsm_cleanup();
			return -1;
		}
	}

	// set timeout: if user's specified a value use that even if it's wrong. 
	// otherwise calculate from data sizes + margin, minimum 1 sec.
	if ( !(timeout = dsa_opt_timeout) )
		timeout = dsa_channel_timeout(&dsa_evt);
	if ( timeout < 100 )
		timeout = 100;
	dsm_set_timeout(timeout);

#if 0
	if ( dsa_evt.rx[0] || dsa_evt.rx[1] )
	{
		if ( reps > 1 )
			LOG_WARN("Specified %lu reps applies to TX only; RX will run once\n", reps);
		else if ( !reps )
			LOG_WARN("Specified continuous transfer is TX only; RX will run once\n");
	}
#endif

	// New FIFO controls: Reset only
	if ( dsa_adi_new )
	{
		for ( dev = 0; dev < 2; dev++ ) 
		{
			// RX side
			if ( dsa_evt.rx[dev] )
			{
				fifo_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_RSTN, 0);
				fifo_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_RSTN,
				                        ADI_NEW_RX_RSTN);
			}

			// TX side
			if ( dsa_evt.tx[dev] )
			{
				fifo_adi_new_write(dev, ADI_NEW_TX, ADI_NEW_RX_REG_RSTN, 0);
				fifo_adi_new_write(dev, ADI_NEW_TX, ADI_NEW_RX_REG_RSTN,
				                        ADI_NEW_RX_RSTN);
			}
		} 
	
		// Local mode: switch ADI/VITA49 sample pipeline to passthru mode and switch from
		// SRIO to local DMA.
		if ( dsa_pipe_dev )
		{
			for ( dev = 0; dev < 2; dev++ )
			{
				size_t len = 0;

				if ( dsa_evt.rx[dev] )
				{
					// reset blocks in the PL
					pipe_vita49_pack_set_ctrl(dev,     PD_VITA49_PACK_CTRL_RESET);
					pipe_vita49_trig_adc_set_ctrl(dev, PD_VITA49_TRIG_CTRL_RESET);
					pipe_adi2axis_set_ctrl(dev,        PD_ADI2AXIS_CTRL_RESET);
					pipe_adi_dma_comb_set_cmd(dev,     PD_ADI_DMA_COMB_CMD_RESET);

					// Set adc_sw_dest switch to 0 for device
					pipe_routing_reg_get_adc_sw_dest(&reg);
					if ( reg & (1 << dev) )
					{
						reg &= ~(1 << dev);
						pipe_routing_reg_set_adc_sw_dest(reg);
					}

					// bypass the DMA combiner, no vita49 packets
					pipe_vita49_pack_set_ctrl(dev, PD_VITA49_PACK_CTRL_PASSTHRU);
					pipe_adi_dma_comb_set_cmd(dev, PD_ADI_DMA_COMB_CMD_PASSTHRU);
					len = dsa_evt.rx[dev]->len * DSM_BUS_WIDTH;

					// enable legacy timing - no triggers
					pipe_adi2axis_set_ctrl(dev, PD_ADI2AXIS_CTRL_LEGACY);
				}

				if ( dsa_evt.tx[dev] )
				{
					// reset blocks in the PL
					pipe_vita49_unpack_set_ctrl(dev,   PD_VITA49_UNPACK_CTRL_RESET);
					pipe_vita49_trig_dac_set_ctrl(dev, PD_VITA49_TRIG_CTRL_RESET);
					pipe_adi_dma_split_set_cmd(dev,    PD_ADI_DMA_SPLIT_CMD_RESET);

					// enable passthru
					pipe_vita49_unpack_set_ctrl(dev,   PD_VITA49_UNPACK_CTRL_PASSTHRU);
					pipe_vita49_trig_dac_set_ctrl(dev, PD_VITA49_TRIG_CTRL_PASSTHRU);
					pipe_adi_dma_split_set_cmd(dev,    PD_ADI_DMA_SPLIT_CMD_PASSTHRU);

					if ( dsa_evt.tx[dev]->len * DSM_BUS_WIDTH > len )
						len = dsa_evt.tx[dev]->len * DSM_BUS_WIDTH;
				}

				// for either direction set the max length
				if ( dsa_evt.rx[dev] || dsa_evt.tx[dev] )
				{
					pipe_adi2axis_set_bytes(dev, len);
					pipe_adi2axis_set_ctrl(dev, PD_ADI2AXIS_CTRL_LEGACY);
				}
			}
		}
	}

	// Data source/sink module
	else if ( dsa_dsxx )
		for ( dev = 0; dev < 2; dev++ )
		{
			// RX side
			if ( dsa_evt.rx[dev] )
			{
				fifo_dsrc_set_ctrl(dev, FD_DSXX_CTRL_DISABLE);
				fifo_dsrc_set_ctrl(dev, FD_DSXX_CTRL_RESET);
			}

			// TX side
			if ( dsa_evt.tx[dev] )
			{
				fifo_dsnk_set_ctrl(dev, FD_DSXX_CTRL_DISABLE);
				fifo_dsnk_set_ctrl(dev, FD_DSXX_CTRL_RESET);
			}
		}

	// Old FIFO controls: Reset and stop all FIFO controls
	else
		for ( dev = 0; dev < 2; dev++ )
		{
			fifo_adi_old_set_ctrl(dev, FD_LVDS_CTRL_RESET);
			fifo_adi_old_set_ctrl(dev, FD_LVDS_CTRL_STOP);

			if ( dsa_evt.tx[dev] )
				fifo_adi_old_chksum_reset(dev);
		}

	// New FIFO controls: Setup channels based on transfer setup
	if ( dsa_adi_new )
		for ( dev = 0; dev < 2; dev++ ) 
		{ 
			// RX side
			if ( dsa_evt.rx[dev] )
			{
				// RX channel 1 parameters - minimal setup for now
				reg = ADI_NEW_RX_FORMAT_ENABLE | ADI_NEW_RX_ENABLE;
				fifo_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(0), reg);
				fifo_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(1), reg);

				// RX channel 2 parameters - minimal setup for now
				reg = ADI_NEW_RX_FORMAT_ENABLE | ADI_NEW_RX_ENABLE;
				fifo_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(2), reg);
				fifo_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(3), reg);

				// Always using T2R2 for now - discard the extra samples
				fifo_adi_new_read(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CNTRL, &reg);
				reg &= ~ADI_NEW_RX_R1_MODE;
				fifo_adi_new_write(dev, ADI_NEW_RX, ADI_NEW_RX_REG_CNTRL, reg);
			}

			// TX channel parameters - Always using T2R2
			if ( dsa_evt.tx[dev] )
			{
				// Select DMA source, enable format, disable T1R1 mode
				reg  = ADI_NEW_TX_DATA_SEL(ADI_NEW_TX_DATA_SEL_DMA);
				reg |= ADI_NEW_TX_DATA_FORMAT;
				reg &= ~ADI_NEW_TX_R1_MODE;
				fifo_adi_new_write(dev, ADI_NEW_TX, ADI_NEW_TX_REG_CNTRL_2, reg);

				// Rate 3 for T2R2 mode
				reg = ADI_NEW_TX_TO_RATE(3);
				fifo_adi_new_write(dev, ADI_NEW_TX, ADI_NEW_TX_REG_RATECNTRL, reg);

				// for version 8.xx set DAC_DDS_SEL to 0x04 input data (DMA) without the
				// 4-bit shift - this is a recent 
				fifo_adi_new_read(dev, ADI_NEW_TX, ADI_NEW_TX_REG_PCORE_VER, &reg);
				if ( (reg & 0xFFFF0000) == 0x00080000 )
				{
					int ch;

					printf("Set new ADI v8 DAC_DDS_SEL to %d\n", dsa_adi_mode);
					for ( ch = 0; ch < 4; ch++ )
						fifo_adi_new_write(dev, ADI_NEW_TX,
						                   ADI_NEW_RX_REG_CHAN_DAC_DDS_SEL(ch),
						                   dsa_adi_mode);
				}
			}
		} 

	// Data source/sink module
	else if ( dsa_dsxx )
		for ( dev = 0; dev < 2; dev++ )
		{
			// number of bytes the source should generate
			if ( dsa_evt.rx[dev] )
			{
				unsigned long len = dsa_evt.rx[dev]->len;
				fifo_dsrc_set_bytes(dev, len * DSM_BUS_WIDTH);
				fifo_dsrc_set_reps(dev, reps);
			}

			if ( dsa_evt.tx[dev] )
				fifo_dsnk_set_ctrl(dev, FD_DSXX_CTRL_ENABLE);
		}

	// Old FIFO controls: Program FIFO counters with expected buffer size
	else
		for ( dev = 0; dev < 2; dev++ )
		{
			if ( dsa_evt.tx[dev] )
				fifo_adi_old_set_tx_cnt(dev, dsa_evt.tx[dev]->len, reps);
		
			if ( dsa_evt.rx[dev] )
				fifo_adi_old_set_rx_cnt(dev, dsa_evt.rx[dev]->len, reps);
		}

	if ( ctrl && !dsa_adi_new && !dsa_dsxx )
	{
		printf("Before run:\n");
		int r;
		for ( r = 0; r < 4; r++ )
		{
			usleep(25000);
			for ( dev = 0; dev < 2; dev++ )
			{
				unsigned long val;

				ret = fifo_adi_old_get_ctrl(dev, &val);
				printf("ADI_G_CTRL  [%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));

				ret = fifo_adi_old_get_tx_cnt(dev, &val);
				printf("ADI_G_TX_CNT[%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));

				ret = fifo_adi_old_get_rx_cnt(dev, &val);
				printf("ADI_G_RX_CNT[%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));
			}
		}
	}

	// Show FIFO numbers before transfer
	if ( fifo && !dsa_adi_new && !dsa_dsxx )
	{
		struct fd_fifo_counts fb;

		fifo_adi_old_get_fifo_cnt(&fb);
		dsa_command_show_fifos(&fb);
	}


	// check AD9361s are in correct ENSM mode for TX/RX/FDD, wake from sleep if necessary
	if ( !dsa_dsxx && (ret = dsa_channel_check_and_wake(&dsa_evt, ensm)) )
	{
		if ( ret < 0 )
			LOG_ERROR("API call failed: %s\n", strerror(errno));

		dsm_cleanup();
		return -1;
	}

	if ( dsa_pipe_dev && (dsa_evt.rx[0] || dsa_evt.rx[1]) )
	{
//		time_t now = time(NULL);

		pipe_vita49_clk_set_ctrl(0);
		pipe_vita49_clk_get_tsi(&reg);
		LOG_INFO("TSI is %lu\n", reg);

		if ( reg < 86400 )
		{
			reg = 0x12341234;
			pipe_vita49_clk_set_tsi(reg);
			pipe_vita49_clk_set_ctrl(PD_VITA49_CLK_CTRL_SET_INT);
			LOG_INFO("Set clock to %lu\n", reg);
		}

		pipe_vita49_clk_set_ctrl(PD_VITA49_CLK_CTRL_ENABLE);
		for ( dev = 0; dev < 2; dev++ )
			if ( dsa_evt.rx[dev] )
				pipe_vita49_trig_adc_set_ctrl(dev, PD_VITA49_TRIG_CTRL_PASSTHRU);
	}

#ifdef DSA_USE_PMON
	if ( pmon )
	{
		Reset_Pmon();
		SetPmonAXIS();
		SetPmonAXI4();
	}
#endif // DSA_USE_PMON

	// Trigger DMA and block until complete
	switch ( trig )
	{
		case TRIG_PRESS:
			LOG_INFO("Ready to DMA, press any key to trigger...\n");
			terminal_pause();
			// fallthrough
			while ( 1 )
			{
				int ch;

				LOG_INFO("Triggering DMA...\n");
				errno = 0;
				if ( !dsm_oneshot_start(~0) && !stats )
					LOG_INFO("DMA triggered\n");

				dsa_command_trigger_start();

				if ( !dsm_oneshot_wait(~0) && !stats )
					LOG_INFO("DMA triggered\n");

				if ( stats )
				{
					struct dsm_chan_stats *sb = dsm_get_stats();
					if ( sb )
					{
						int slot;
						for ( slot = 0; slot < dsm_channels->size; slot++ )
							if ( sb->mask & (1 << slot) )
								dsa_command_show_stats(&sb->list[slot], slot);
					}
					else
						LOG_ERROR("Failed to get stats: %s\n", strerror(errno));
					free(sb);
				}
				LOG_INFO("Ready to DMA, press Q or Esc to stop, any other key to trigger...\n");

				ch = toupper(terminal_pause());
				if ( ch == 'Q' || ch == 27 )
					break;
			}
			break;

		case TRIG_ONCE:
			LOG_INFO("Triggering DMA...\n");
			errno = 0;
			if ( !dsm_oneshot_start(~0) && !stats )
				LOG_INFO("DMA triggered\n");

			dsa_command_trigger_start();

			if ( !dsm_oneshot_wait(~0) && !stats )
				LOG_INFO("DMA triggered\n");
			break;

		case TRIG_CONT:
			LOG_INFO("Starting DMA, press any key to stop...\n");
			dsm_cyclic_start(~0);
			dsa_command_trigger_start();

			terminal_pause();

			LOG_INFO("Stopping DMA...\n");
			dsm_cyclic_stop(~0);
			break;
	}

#ifdef DSA_USE_PMON
	if ( pmon )
	{
		GetPmonAXIS();
		GetPmonAXI4();
		Reset_Pmon();
	}
#endif // DSA_USE_PMON

	if ( !ensm && dsa_channel_ensm_wake )
		dsa_channel_sleep();


	// Show FIFO numbers before transfer
	if ( fifo )
	{
		struct fd_fifo_counts fb;

		fifo_adi_old_get_fifo_cnt(&fb);
		dsa_command_show_fifos(&fb);
	}

	// For TX transfers, check 
	if ( exp && !dsa_adi_new )
		for ( dev = 0; dev < 2; dev++ )
		{
			if ( !dsa_evt.tx[dev] )
				continue;

			if ( dsa_dsxx )
				fifo_dsnk_get_sum(dev, sum);

			else
			{
				fifo_adi_old_get_sum(dev, sum);
				fifo_adi_old_get_last(dev, last);
				printf("Last wrd: %08lx%08lx\n", last[0], last[1]);
			}


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


	if ( dsa_dsxx )
		for ( dev = 0; dev < 2; dev++ )
		{
			if ( dsa_evt.rx[dev] )
			{
				fifo_dsrc_get_stat(dev, &reg);	
				printf("DSRC%d: stat %lu { %s%s%s}\n", dev, reg,
				       reg & FD_DSXX_STAT_ENABLED ? "enabled " : "",
				       reg & FD_DSXX_STAT_DONE    ? "done "    : "",
				       reg & FD_DSXX_STAT_ERROR   ? "error "   : "");

				fifo_dsrc_get_sent(dev, &reg);	
				printf ("DSRC%d: %lu bytes sent\n", dev, reg);

				fifo_dsrc_get_reps(dev, &reg);	
				printf ("DSRC%d: %lu reps left\n", dev, reg);

				fifo_dsrc_get_rsent(dev, &reg);	
				printf ("DSRC%d: %lu reps sent\n", dev, reg);

				fifo_dsrc_set_ctrl(dev, FD_DSXX_CTRL_DISABLE);

				uint64_t *ptr = (uint64_t *)dsa_evt.rx[dev]->smp;
				uint64_t  val = 0;
				size_t    idx = 0;
				uint32_t  cnt = 0;

				printf("Analyzing %d words in buffer\n", dsa_evt.rx[dev]->len);
				while ( idx < dsa_evt.rx[dev]->len )
				{
					if ( *ptr != val )
					{
						printf("Idx %zu: want %llx, got %llx instead\n",
						       idx,
						       (unsigned long long)val,
						       (unsigned long long)*ptr);
						val = *ptr;
						cnt++;
					}
					ptr++;
					val++;
					idx++;
				}
				printf("%u deviations\n", cnt);
			}

			if ( dsa_evt.tx[dev] )
			{
				fifo_dsnk_get_stat(dev, &reg);	
				printf("DSNK%d: stat %lu { %s%s%s}\n", dev, reg,
				       reg & FD_DSXX_STAT_ENABLED ? "enabled " : "",
				       reg & FD_DSXX_STAT_DONE    ? "done "    : "",
				       reg & FD_DSXX_STAT_ERROR   ? "error "   : "");

				fifo_dsnk_get_bytes(dev, &reg);	
				printf ("DSNK%d: %lu bytes received\n", dev, reg);

				fifo_dsnk_set_ctrl(dev, FD_DSXX_CTRL_DISABLE);
			}
		}

	if ( ctrl && !dsa_adi_new ) 
	{
		for ( dev = 0; dev < 2; dev++ )
		{
			unsigned long val;
			printf("After run:\n");

			ret = fifo_adi_old_get_ctrl(dev, &val);
			printf("ADI_G_CTRL  [%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));

			ret = fifo_adi_old_get_tx_cnt(dev, &val);
			printf("ADI_G_TX_CNT[%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));

			ret = fifo_adi_old_get_rx_cnt(dev, &val);
			printf("ADI_G_RX_CNT[%d]: %08x: %d: %s\n", dev, val, ret, strerror(errno));
		}
	}

	if ( stats && trig != TRIG_PRESS )
	{
		struct dsm_chan_stats *sb = dsm_get_stats();
		if ( sb )
		{
			int slot;
			for ( slot = 0; slot < dsm_channels->size; slot++ )
				if ( sb->mask & (1 << slot) )
					dsa_command_show_stats(&sb->list[slot], slot);
		}
		else
			LOG_ERROR("Failed to get stats: %s\n", strerror(errno));
		free(sb);
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

	if ( dsm_cleanup() )
		LOG_ERROR("DMA unmapping failed: %s\n", strerror(errno));

	if ( dsa_adi_new && dsa_pipe_dev )
	{
		struct pd_vita49_ts  ts;
		for ( dev = 0; dev < 2; dev++ )
			if ( dsa_evt.rx[dev] )
			{
				pipe_vita49_trig_adc_set_ctrl(dev, 0);

				memset(&ts, 0, sizeof(ts));
				pipe_vita49_clk_read(dev, &ts);
				LOG_FOCUS("AD%d TS after RX: %08lx.%08lx%08lx\n", dev + 1, 
				          ts.tsi, ts.tsf_hi, ts.tsf_lo);

			}
	}

	// save sink buffers 
	if ( dsa_channel_save(&dsa_evt) < 0 )
		LOG_ERROR("Failed to save IQ data: %s\n", strerror(errno));

	return 0;
}

