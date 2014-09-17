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

#include <ad9361.h>

#include "dsa_main.h"
#include "dsa_ioctl_dsm.h"
#include "dsa_ioctl_fifo_dev.h"
#include "dsa_command.h"
#include "dsa_common.h"

#include "log.h"
LOG_MODULE_STATIC("main", LOG_LEVEL_INFO);

#define LIB_DIR "/usr/lib/ad9361"

const char *dsa_argv0;
int         dsa_dsm_dev = -1;
int         dsa_fifo_dev = -1;
int         dsa_adi_new = 0;
int         dsa_dsxx = 0;

size_t      dsa_opt_len      = 1000000; // 1MS default
unsigned    dsa_opt_timeout  = 0; // auto-calculated now
const char *dsa_opt_dsm_dev  = DEF_DSM_DEVICE;
const char *dsa_opt_fifo_dev = DEF_FIFO_DEVICE;
long        dsa_opt_adjust   = 0;
size_t      dsa_total_words  = 0;

char *opt_lib_dir   = NULL;
char  env_data_path[PATH_MAX];

struct format *dsa_opt_format = NULL;
struct dsa_channel_event dsa_evt;

struct dsm_limits     dsa_dsm_limits;
struct dsm_chan_list *dsa_dsm_channels = NULL;
unsigned long         dsa_dsm_rx_channels[2] = { 0, 2 };
unsigned long         dsa_dsm_tx_channels[2] = { 1, 3 };


static const char *dsa_main_dev_name (int slot)
{
	int  dev = dsa_dsm_channels->list[slot].private >> 28 ? DC_DEV_AD2 : DC_DEV_AD1;
	dev |= (dsa_dsm_channels->list[slot].flags & DSM_CHAN_DIR_TX) ? DC_DIR_TX : DC_DIR_RX;

	switch ( dev )
	{
		case DC_DEV_AD1 | DC_DIR_TX: return dsa_dsxx ? "DSNK1" : "AD1 TX";
		case DC_DEV_AD1 | DC_DIR_RX: return dsa_dsxx ? "DSRC1" : "AD1 RX";
		case DC_DEV_AD2 | DC_DIR_TX: return dsa_dsxx ? "DSNK2" : "AD2 TX";
		case DC_DEV_AD2 | DC_DIR_RX: return dsa_dsxx ? "DSRC2" : "AD2 RX";
	}

	return "???";
}

void dsa_main_show_stats (const struct dsm_xfer_stats *st, int slot)
{
	printf("%s stats:\n", dsa_main_dev_name(slot));
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

void dsa_main_show_fifos (const struct fd_fifo_counts *buff)
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
		     mask & FD_TARGT_ADI1 ? "adi1 " : "",
		     mask & FD_TARGT_ADI2 ? "adi2 " : "",
		     mask & FD_TARGT_NEW  ? "new: " : "",
		     mask & FD_TARGT_DSX0 ? "dsx0 " : "",
		     mask & FD_TARGT_DSX1 ? "dsx1 " : "");

	return buff;
}


int dsa_main_map (int reps)
{
	struct dsm_xfer_buff  xfer;
	int                   ret;
	int                   dev;

	if ( dsa_dsm_dev < 0 )
		return -1;

	errno = 0;
	for ( dev = 0; dev < 2; dev++ )
	{
		if ( dsa_evt.tx[dev] )
		{
			xfer.addr = (unsigned long)dsa_evt.tx[dev]->smp;
			xfer.size = dsa_evt.tx[dev]->len;
			xfer.reps = reps;
			if ( (ret = dsa_ioctl_dsm_map_chan(dsa_dsm_tx_channels[dev], 1, 1, &xfer)) )
				goto error;
		}

		if ( dsa_evt.rx[dev] )
		{
			xfer.addr = (unsigned long)dsa_evt.rx[dev]->smp;
			xfer.size = dsa_evt.rx[dev]->len;
			xfer.reps = reps;
			if ( (ret = dsa_ioctl_dsm_map_chan(dsa_dsm_rx_channels[dev], 0, 1, &xfer)) )
				goto error;
		}
	}

	return 0;

error:
	return -1;
}


int dsa_main_unmap (void)
{
	if ( dsa_dsm_dev < 0 )
		return -1;

	return dsa_ioctl_dsm_unmap();
}


void dsa_main_dev_close (void)
{
	if ( dsa_dsm_dev >= 0 )
		close(dsa_dsm_dev);
	dsa_dsm_dev = -1;

	if ( dsa_fifo_dev >= 0 )
		close(dsa_fifo_dev);
	dsa_fifo_dev = -1;
}


int dsa_main_dev_reopen (unsigned long *mask)
{
	dsa_main_dev_close();

	if ( (dsa_dsm_dev = open(dsa_opt_dsm_dev, O_RDWR)) < 0 )
	{
		LOG_DEBUG("open(%s): %s\n", dsa_opt_dsm_dev, strerror(errno));
		return -1;
	}

	if ( dsa_ioctl_dsm_limits(&dsa_dsm_limits) )
		stop("DSM_IOCG_LIMITS");
	
	LOG_INFO("DSM reports %lu total DMA channels\n", dsa_dsm_limits.channels);
	LOG_INFO("DSM reports %lu words (%luMB) aggregate DMA limit\n",
	         dsa_dsm_limits.total_words, dsa_dsm_limits.total_words >> 17);

	free(dsa_dsm_channels);
	if ( !(dsa_dsm_channels = dsa_ioctl_dsm_channels()) )
		stop("DSM_IOCG_CHANNELS");

	int i;
	printf("dsa_ioctl_dsm_channels() gave %d channels:\n", dsa_dsm_channels->size);
	for ( i = 0; i < dsa_dsm_channels->size; i++ )
	{
		LOG_DEBUG("%2d: private:%08lx flags:%08lx width:%02u align:%02u "
		          "device:%s driver:%s\n", i,
		          dsa_dsm_channels->list[i].private,
		          dsa_dsm_channels->list[i].flags,
		          (1 << (dsa_dsm_channels->list[i].width + 3)),
		          (1 << (dsa_dsm_channels->list[i].align + 3)),
		          dsa_dsm_channels->list[i].device,
		          dsa_dsm_channels->list[i].driver);

		switch ( dsa_dsm_channels->list[i].flags & DSM_CHAN_DIR_MASK )
		{
			case DSM_CHAN_DIR_RX:
				switch ( dsa_dsm_channels->list[i].private & 0xF0000000 )
				{
					case 0x00000000:
						dsa_dsm_rx_channels[0] = i;
						printf("    using for RX1\n");
						break;

					case 0x10000000:
						dsa_dsm_rx_channels[1] = i;
						printf("    using for RX2\n");
						break;
				}
				break;

			case DSM_CHAN_DIR_TX:
				switch ( dsa_dsm_channels->list[i].private & 0xF0000000 )
				{
					case 0x00000000:
						dsa_dsm_tx_channels[0] = i;
						printf("    using for TX1\n");
						break;

					case 0x10000000:
						dsa_dsm_tx_channels[1] = i;
						printf("    using for TX2\n");
						break;
				}
				break;
		}
	}

	if ( (dsa_fifo_dev = open(dsa_opt_fifo_dev, O_RDWR)) < 0 )
	{
		LOG_DEBUG("open(%s): %s\n", dsa_opt_fifo_dev, strerror(errno));
		dsa_main_dev_close();
		return -1;
	}

	if ( dsa_ioctl_fifo_dev_target_list(mask) )
		stop("FD_IOCG_TARGET_LIST");

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
#ifdef BOARD_REV
		d += snprintf(d, e - d, "%s%s/%s/%s", d > dst ? ":" : "",
		              *root_walk, leaf, BOARD_REV);
#endif
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
	       "  # dma_streamer_app  ad12t12 802.16d-64q-cp16.iqw  -s 1000\n\n"
	       "- The above could be shortened to:\n"
	       "  # dma_streamer_app  adt 802.16d-64q-cp16.iqw  1k\n\n"
	       "- The WiMAX signal above could be transmitted until a key is pressed:\n"
	       "  # dma_streamer_app  adt 802.16d-64q-cp16.iqw  cont\n\n"
	       "- Alternatively, the signal could be transmitted each time a key is pressed:\n"
	       "  # dma_streamer_app  adt 802.16d-64q-cp16.iqw  press\n\n"
	       "- Each press can also trigger a repeated burst of transmit events:\n"
	       "  # dma_streamer_app  adt 802.16d-64q-cp16.iqw  press 100\n\n"
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

	// Now that we're using the new API directly, init the HAL
//	if ( ad9361_hal_linux_init() < 0 )
//		stop("failed to init HAL");
//	if ( ad9361_hal_linux_attach() < 0 )
//		stop("failed to attach HAL");

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

	unsigned long  mask;
	if ( dsa_main_dev_reopen(&mask) < 0 )
		stop("failed to open device nodes");
	dsa_adi_new = mask & FD_TARGT_NEW;
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
	dsa_channel_event_dump(&dsa_evt);

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

