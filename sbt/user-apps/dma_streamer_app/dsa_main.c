/** \file      dsa_main.c
 *  \brief     application main
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "dsa_main.h"
#include "dsa_ioctl.h"
#include "dsa_command.h"
#include "dsa_common.h"

#include "log.h"
LOG_MODULE_STATIC("main", LOG_LEVEL_INFO);

#define LIB_DIR "/usr/lib/ad9361"
#if defined(BOARD_REV_CUT1)
#define BOARD_REV "cut1"
#elif defined(BOARD_REV_CUT2)
#define BOARD_REV "cut2"
#else
#define BOARD_REV "sim"
#endif

#define EXPORT_FILE "/tmp/.ad9361.channels"

const char *dsa_argv0;
int         dsa_dev = -1;
int         dsa_adi_new = 0;

size_t      dsa_opt_len      = 1000000; // 1MS default
unsigned    dsa_opt_timeout  = 0; // auto-calculated now
const char *dsa_opt_device   = DEF_DEVICE;

char *opt_lib_dir   = NULL;
char  env_data_path[PATH_MAX];

unsigned char  dsa_active_channels[4];

struct format *dsa_opt_format = NULL;
struct dsa_channel_event dsa_evt;


void dsa_main_show_stats (const struct dsm_xfer_stats *st, const char *dir)
{
	printf("%s stats:\n", dir);
	if ( !st->bytes )
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

	printf("  completes: %lu\n",       st->completes);
	printf("  errors   : %lu\n",       st->errors);
	printf("  timeouts : %lu\n",       st->timeouts);
}

void dsa_main_show_fifos (const struct dsm_fifo_counts *buff)
{
	printf("  RX 1: %08lx/%08lx\n", buff->rx_1_ins, buff->rx_1_ext);
	printf("  RX 2: %08lx/%08lx\n", buff->rx_2_ins, buff->rx_2_ext);
	printf("  TX 1: %08lx/%08lx\n", buff->tx_1_ins, buff->tx_1_ext);
	printf("  TX 2: %08lx/%08lx\n", buff->tx_2_ins, buff->tx_2_ext);
}


static char *target_desc (int mask)
{
	static char buff[64];

	snprintf(buff, sizeof(buff), "{ %s%s%s%s%s}",
		     mask & DSM_TARGT_ADI1 ? "adi1 " : "",
		     mask & DSM_TARGT_ADI2 ? "adi2 " : "",
		     mask & DSM_TARGT_NEW  ? "new: " : "",
		     mask & DSM_TARGT_DSXX ? "dsrc " : "",
		     mask & DSM_TARGT_DSXX ? "dsnk " : "");

	return buff;
}


int dsa_main_map (int reps)
{
	// pass to kernelspace and prepare DMA
	struct dsm_user_buffs  buffs;

	if ( dsa_dev < 0 )
		return -1;

	memset (&buffs, 0, sizeof(struct dsm_user_buffs));

	if ( dsa_evt.tx[0] )
	{
		buffs.adi1.tx.addr = (unsigned long)dsa_evt.tx[0]->smp;
		buffs.adi1.tx.size = dsa_evt.tx[0]->len * DSM_BUS_WIDTH;
		buffs.adi1.tx.words = dsa_evt.tx[0]->len * reps;
	}

	if ( dsa_evt.tx[1] )
	{
		buffs.adi2.tx.addr = (unsigned long)dsa_evt.tx[1]->smp;
		buffs.adi2.tx.size = dsa_evt.tx[1]->len * DSM_BUS_WIDTH;
		buffs.adi2.tx.words = dsa_evt.tx[1]->len * reps;
	}

	if ( dsa_evt.rx[0] )
	{
		buffs.adi1.rx.addr = (unsigned long)dsa_evt.rx[0]->smp;
		buffs.adi1.rx.size = dsa_evt.rx[0]->len * DSM_BUS_WIDTH;
		buffs.adi1.rx.words = dsa_evt.rx[0]->len;
	}

	if ( dsa_evt.rx[1] )
	{
		buffs.adi2.rx.addr = (unsigned long)dsa_evt.rx[1]->smp;
		buffs.adi2.rx.size = dsa_evt.rx[1]->len * DSM_BUS_WIDTH;
		buffs.adi2.rx.words = dsa_evt.rx[1]->len;
	}

	// old: calculate FIFO control register ctrl value based on channel config
	// new: calculate channel/TX/RX enable ctrl value based on channel config
	if ( dsa_evt.tx[0] || dsa_evt.rx[0] )
		buffs.adi1.ctrl = dsa_channel_ctrl(&dsa_evt, DC_DEV_AD1, !dsa_adi_new);
	if ( dsa_evt.tx[1] || dsa_evt.rx[1] )
		buffs.adi2.ctrl = dsa_channel_ctrl(&dsa_evt, DC_DEV_AD2, !dsa_adi_new);

	return dsa_ioctl_map(&buffs);
}


int dsa_main_unmap (void)
{
	if ( dsa_dev < 0 )
		return -1;

	return dsa_ioctl_unmap();
}


void dsa_main_dev_close (void)
{
	if ( dsa_dev < 0 )
		return;

	close(dsa_dev);
	dsa_dev = -1;
}


int dsa_main_dev_reopen (unsigned long *mask)
{
	dsa_main_dev_close();

	if ( (dsa_dev = open(dsa_opt_device, O_RDWR)) < 0 )
	{
		LOG_DEBUG("open(%s): %s\n", dsa_opt_device, strerror(errno));
		return -1;
	}

	if ( dsa_ioctl_target_list(mask) )
		stop("DSM_IOCG_TARGET_LIST");

	LOG_DEBUG("Supported targets: %s\n", target_desc(*mask));
	if ( !*mask )
	{
		LOG_ERROR("No supported targets in this build; reconfigure your PetaLinux with the path to\n"
		       "your hardware project's BSP include directory, containing xparameters.h.  The\n"
		       "petalinux-config-apps path is:\n"
		       "  User Modules ->\n"
		       "    dma_streamer_mod -> \n"
		       "      Path to Xilinx-generated BSP containing xparameters.h\n"
		       "or search for USER_MODULES_DMA_STREAMER_MOD_BSP\n");

		dsa_main_dev_close();
		return -1;
	}

	return 0;
}



static void path_setup (char *dst, size_t max, const char *leaf)
{
	char  *d = dst;
	char  *e = dst + max;
	char  *root_list[] = { "/media/card", opt_lib_dir, NULL };
	char **root_walk = root_list;

	while ( *root_walk && d < e )
	{
		d += snprintf(d, e - d, "%s%s/%s/%s", d > dst ? ":" : "",
		              *root_walk, leaf, BOARD_REV);
		d += snprintf(d, e - d, ":%s/%s", *root_walk, leaf);
		root_walk++;
	}
}

static void dsa_main_header (void)
{
	printf("\nThe dma_streamer_app is used to perform sample-data transfers to and from the\n"
	       "AD9361 chips on the SDRDC board.  The command-line has three distinct sections:\n"
	       "- global options, which set defaults for the application\n"
	       "- buffer options, which setup DMA buffers and load/save data\n"
	       "- trigger options, which control the actual transfers\n\n");
}

static void dsa_main_formats (void)
{
	printf("The following formats are compiled in.  Some formats are output-only, and not\n"
	       "used for loading sample data.\n");
	dsa_format_list(stdout);
	printf("\n");
}

static void dsa_main_footer (void)
{
	printf("Complete example command lines:\n"
	       "- Allocate buffers for TX through both AD1 and AD2, load both TX1 and TX2\n"
	       "  channels with data from a WiMAX signal in IQW format, then play 1000 reps\n"
	       "  with stats displayed afterwards:\n"
	       "  # dma_streamer_app  ad12t12 /media/card/wimax.iqw  -s 1000\n\n"
	       "- The above could be shortened to:\n"
	       "  # dma_streamer_app  adt /media/card/wimax.iqw  1000\n\n"
	       "- Allocate buffers for RX on both AD1 and AD2, sample 1 million samples, and\n"
	       "  write out to four separate files:\n"
	       "  # dma_streamer_app -S 1M \\\n"
	       "  >   ad1r1 /media/card/ad1r1.iqw \\\n"
	       "  >   ad1r2 /media/card/ad1r2.iqw \\\n"
	       "  >   ad2r1 /media/card/ad2r1.iqw \\\n"
	       "  >   ad2r2 /media/card/ad2r2.iqw\n\n"
	       "- Using printf-style substitutions as described under Buffer options, the above\n"
	       "  could be shortened to:\n"
	       "  # dma_streamer_app  -S 1M  adr /media/card/ad%%ar%%c.iqw\n");
}

static void import_active_channels (void)
{
	int  fd;
	int  ret;
	int  dev;
	int  dir;

	// before importing, auto-probe the channel config - starts ad9361 looking at ad1,
	// then switch to ad2, then writes out the channel status on exit
	system("/usr/bin/ad9361 device ad2 2>/dev/null");

	memset(dsa_active_channels, 0, sizeof(dsa_active_channels));
	if ( (fd = open(EXPORT_FILE, O_RDONLY)) < 0 )
		return;

	if ( (ret = read(fd, dsa_active_channels, sizeof(dsa_active_channels))) < 0 )
	{
		close(fd);
		return;
	}
	close(fd);

	if ( ret < sizeof(dsa_active_channels) )
		memset(dsa_active_channels + ret, 0, sizeof(dsa_active_channels) - ret);

	LOG_DEBUG("Active channels from %02x %02x %02x %02x:\n", 
	          dsa_active_channels[0], dsa_active_channels[1], 
	          dsa_active_channels[2], dsa_active_channels[3]);

	for ( dev = 0; dev < 4; dev += 2 )
		for ( dir = 0; dir < 2; dir++ )
		{
			unsigned char ch = dsa_active_channels[dev + dir];
			LOG_DEBUG("  %s %s:%s%s%s\n",
			          dev ? "AD2" : "AD1",
			          dir ? "RX"  : "TX",
			          ch & 0x80 ? " CH2" : "",
			          ch & 0x40 ? " CH1" : "",
			          ch & 0xC0 ? "" : " none");
		}
}

int main(int argc, char *argv[])
{
	int  ret;

	log_dupe(stdout);
	setbuf(stdout, NULL);
	if ( (dsa_argv0 = strrchr(argv[0], '/')) )
		dsa_argv0++;
	else
		dsa_argv0 = argv[0];

	char *p;
	if ( ! opt_lib_dir )
		opt_lib_dir = (p = getenv("AD9361_LIB_DIR")) ? p : LIB_DIR;

	if ( (p = getenv("AD9361_DATA_PATH")) )
		snprintf(env_data_path, sizeof(env_data_path), "%s", p);
	else
		path_setup(env_data_path, sizeof(env_data_path), "data");

	LOG_DEBUG("initial argc %d, argv[]:\n", argc);
	int i;
	for ( i = 0; i <= argc; i++ )
		LOG_DEBUG("  argv[%d]: '%s'\n", i, argv[i]);


	// set global options - returns 0 on success, -1 on global options fail, -2 if '?'
	// given, so print all the help
	if ( (ret = dsa_command_options(argc, argv)) < 0 )
	{
		dsa_main_header();
		dsa_command_options_usage();
		dsa_main_formats();
		if ( ret < -1 )
		{
			dsa_command_setup_usage();
			dsa_command_trigger_usage();
		}
		dsa_main_footer();
		return 1;
	}
	import_active_channels();

	unsigned long  mask;
	if ( dsa_main_dev_reopen(&mask) < 0 )
		stop("failed to open: %s", dsa_opt_device);
	dsa_adi_new = mask & DSM_TARGT_NEW;
	LOG_INFO("Using %s ADI access\n", dsa_adi_new ? "new" : "old");

	// iterative buffer setup of new dsa_channel_event struct
	int ofs = optind;
	while ( ofs < argc && argv[ofs] )
	{
		ret = dsa_command_setup(-1, argc - ofs, argv + ofs);
		if ( ret < 0 )
		{
			dsa_main_header();
			dsa_command_setup_usage();
			dsa_main_formats();
			return 1;
		}
		else if ( ret < 1 )
			break;

		ofs += ret;
	}

	// if no buffers were setup, there's nothing to do.  stop and give full usage.
	if ( !dsa_evt.tx[0] && !dsa_evt.tx[1] && !dsa_evt.rx[0] && !dsa_evt.rx[1] )
	{
		dsa_channel_cleanup(&dsa_evt); // probably redundant but doesn't hurt
		dsa_main_dev_close();
		dsa_main_header();
		dsa_command_options_usage();
		dsa_main_formats();
		dsa_command_setup_usage();
		dsa_command_trigger_usage();
		dsa_main_footer();
		return 1;
	}

	ofs--;
	if ( dsa_command_trigger(argc - ofs, argv + ofs) < 0 )
	{
		dsa_main_header();
		dsa_command_trigger_usage();
	}
	dsa_channel_event_dump(&dsa_evt);

	dsa_channel_cleanup(&dsa_evt);

	LOG_DEBUG("Close device...\n");
	dsa_main_dev_close();
	return 0;
}

