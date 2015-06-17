/** \file      pipe-recv.c
 *  \brief     Sample pipeline receive tool - ADI to SRIO
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

#include <sd_user.h>
#include <fifo_dev.h>
#include <pipe_dev.h>

#include "common.h"

uint16_t    opt_remote   = 2;
size_t      opt_data     = 2048;
unsigned    opt_timeout  = 1500;
uint32_t    opt_adi      = 0;
uint32_t    opt_sid      = 0;
FILE       *opt_debug    = NULL;


static void usage (void)
{
	printf("Usage: srio-recv [-v] [-R dest] [-s bytes] [-t timeout] adi stream-id\n"
	       "Where:\n"
	       "-R dest     SRIO destination address (default 2)\n"
	       "-v          Verbose/debugging enable\n"
	       "-s bytes    Set payload size in bytes\n"
	       "-t timeout  Set timeout in jiffies\n"
	       "\n"
	       "adi is required value, and be 0 or 1 for the ADI chain to use (T2R2 mode only,\n"
	       "this will be aligned with DSA syntax in future.\n"
	       "stream-id is a required value, and must match the expected stream-id on the\n"
	       "receiver side, if that receiver implements stream-ID filtering.\n"
		   "\n");
}


int main (int argc, char **argv)
{
//	unsigned long          routing;
	unsigned long          tuser;
	unsigned long          reg;
//	void                  *buff;
	int                    srio_dev;
	int                    ret = 0;
	int                    opt;
//	int                    idx;

	while ( (opt = getopt(argc, argv, "?hvs:t:R:")) > -1 )
		switch ( opt )
		{
			case 'v':
				opt_debug = stderr;
				break;

			case 'R': opt_remote  = strtoul(optarg, NULL, 0); break;
			case 's': opt_data    = strtoul(optarg, NULL, 0); break;
			case 't': opt_timeout = strtoul(optarg, NULL, 0); break;

			default:
				usage();
				return 1;
		}

	printf("optind %d, argc %d\n", optind, argc);
	if ( (argc - optind) < 2 )
	{
		usage();
		return 1;
	}
	if ( (opt_adi = strtoul(argv[optind], NULL, 0)) > 1 )
	{
		usage();
		return 1;
	}
	opt_sid = strtoul(argv[optind + 1], NULL, 0);
	opt_remote &= 0xFFFF;
	printf("ADI %u, SID 0x%x, remote 0x%04x\n", opt_adi, opt_sid, opt_remote);


	// open SRIO dev, get local address
	if ( (srio_dev = open("/dev/" SD_USER_DEV_NODE, O_RDWR|O_NONBLOCK)) < 0 )
	{
		printf("SRIO dev open(%s): %s\n", "/dev/" SD_USER_DEV_NODE, strerror(errno));
		return 1;
	}
	if ( ioctl(srio_dev, SD_USER_IOCG_LOC_DEV_ID, &tuser) )
	{
		printf("SD_USER_IOCG_LOC_DEV_ID: %s\n", strerror(errno));
		ret = 1;
		goto exit_srio;
	}
	if ( !tuser || tuser >= 0xFFFF )
	{
		printf("Local Device-ID invalid, wait for discovery to complete\n");
		ret = 1;
		goto exit_srio;
	}


	// open and access for FIFO dev
	if ( fifo_dev_reopen("/dev/" FD_DRIVER_NODE) )
	{
		printf("fifo_dev_reopen(%s): %s\n", "/dev/" PD_DRIVER_NODE, strerror(errno));
		goto exit_srio;
	}
	if ( fifo_access_request(opt_adi ? FD_ACCESS_AD2_RX : FD_ACCESS_AD1_RX) )
	{
		printf("fifo_access_request(FD_ACCESS_AD%d_RX): %s\n",
		       opt_adi + 1, strerror(errno));
		goto exit_fifo;
	}


	// open and access for pipe-dev
	if ( pipe_dev_reopen("/dev/" PD_DRIVER_NODE) )
	{
		printf("pipe_dev_reopen(%s): %s\n", "/dev/" PD_DRIVER_NODE, strerror(errno));
		ret = 1;
		goto exit_fifo;
	}
	if ( pipe_access_request(opt_adi ? PD_ACCESS_AD2_RX : PD_ACCESS_AD1_RX) )
	{
		printf("pipe_access_request(PD_ACCESS_AD%d_RX): %s\n",
		       opt_adi + 1, strerror(errno));
		goto exit_pipe;
	}

	// clamp timeout 
	if ( opt_timeout < 1 )
		opt_timeout = 1;
	else if ( opt_timeout > 60 )
		opt_timeout = 60;


	// reset blocks in the PL
	fifo_adi_new_write(opt_adi,            ADI_NEW_RX, ADI_NEW_RX_REG_RSTN, 0);
	pipe_vita49_pack_set_ctrl(opt_adi,     PD_VITA49_PACK_CTRL_RESET);
	pipe_vita49_trig_adc_set_ctrl(opt_adi, PD_VITA49_TRIG_CTRL_RESET);
	pipe_adi2axis_set_ctrl(opt_adi,        PD_ADI2AXIS_CTRL_RESET);
	pipe_swrite_pack_set_cmd(opt_adi,      PD_SWRITE_PACK_CMD_RESET);
	usleep(1000);

	fifo_adi_new_write(opt_adi,            ADI_NEW_RX, ADI_NEW_RX_REG_RSTN, ADI_NEW_RX_RSTN);
	pipe_vita49_pack_set_ctrl(opt_adi,     0);
	pipe_vita49_trig_adc_set_ctrl(opt_adi, 0);
	pipe_adi2axis_set_ctrl(opt_adi,        0);

	// V49 packer setup
	pipe_vita49_pack_set_streamid(opt_adi, opt_sid);
	pipe_vita49_pack_set_pkt_size(opt_adi, 64); // 64 32-bit words -> 256 bytes
	pipe_vita49_pack_set_trailer(opt_adi,  0xaaaaaaaa); // trailer for alignment
	pipe_vita49_pack_set_ctrl(opt_adi,     PD_VITA49_PACK_CTRL_ENABLE |
	                                       PD_VITA49_PACK_CTRL_TRAILER);

	// SWRITE packer setup 
	tuser <<= 16;
	tuser  |= opt_remote;
	printf("tuser word: 0x%08x\n", tuser);
	pipe_swrite_pack_set_srcdest(opt_adi, tuser);
	pipe_swrite_pack_set_addr(opt_adi,    opt_sid); // sets SWRITE addr, matched on RX side
	pipe_swrite_pack_set_cmd(opt_adi,     PD_SWRITE_PACK_CMD_START);

	// Set adc_sw_dest switch to 1,0 for ADI -> SRIO
	pipe_routing_reg_get_adc_sw_dest(&reg);
	switch ( opt_adi )
	{
		case 0:
			reg |=  PD_ROUTING_REG_ADC_SW_DEST_0;
			reg &= ~PD_ROUTING_REG_DMA_LOOPBACK_0;
			break;

		case 1:
			reg |=  PD_ROUTING_REG_ADC_SW_DEST_1;
			reg &= ~PD_ROUTING_REG_DMA_LOOPBACK_1;
			break;
	}
	pipe_routing_reg_set_adc_sw_dest(reg);

	// FIFO setup - RX channel 1/2 parameters - minimal setup for now
	reg = ADI_NEW_RX_FORMAT_ENABLE | ADI_NEW_RX_ENABLE;
	fifo_adi_new_write(opt_adi, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(0), reg);
	fifo_adi_new_write(opt_adi, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(1), reg);
	fifo_adi_new_write(opt_adi, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(2), reg);
	fifo_adi_new_write(opt_adi, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(3), reg);

	// Always using T2R2 for now
	fifo_adi_new_read(opt_adi, ADI_NEW_RX, ADI_NEW_RX_REG_CNTRL, &reg);
	reg &= ~ADI_NEW_RX_R1_MODE;
	fifo_adi_new_write(opt_adi, ADI_NEW_RX, ADI_NEW_RX_REG_CNTRL, reg);


	// using legacy fixed-length mode
	pipe_adi2axis_set_bytes(opt_adi, opt_data);
	pipe_adi2axis_set_ctrl(opt_adi,  PD_ADI2AXIS_CTRL_LEGACY);

	// manual trigger
	pipe_vita49_trig_adc_set_ctrl(opt_adi, PD_VITA49_TRIG_CTRL_PASSTHRU);

	// wait for complete or timeout
	do
	{
		pipe_adi2axis_get_stat(opt_adi, &reg);
		printf("%4u: %08x\n", opt_timeout--, reg);
		sleep(1);
	}
	while ( opt_timeout && !(reg & PD_ADI2AXIS_STAT_COMPLETE) );

	if ( opt_timeout )
		printf("Completed normally with timeout %u\n", opt_timeout);
	else
		printf("Timed out\n");

	// reset and shut-down pipeline
	fifo_adi_new_write(opt_adi,            ADI_NEW_RX, ADI_NEW_RX_REG_RSTN, 0);
	pipe_vita49_pack_set_ctrl(opt_adi,     PD_VITA49_PACK_CTRL_RESET);
	pipe_vita49_trig_adc_set_ctrl(opt_adi, PD_VITA49_TRIG_CTRL_RESET);
	pipe_adi2axis_set_ctrl(opt_adi,        PD_ADI2AXIS_CTRL_RESET);
	pipe_swrite_pack_set_cmd(opt_adi,      PD_SWRITE_PACK_CMD_RESET);
	usleep(1000);

	fifo_adi_new_write(opt_adi,            ADI_NEW_RX, ADI_NEW_RX_REG_RSTN, ADI_NEW_RX_RSTN);
	pipe_vita49_pack_set_ctrl(opt_adi,     0);
	pipe_vita49_trig_adc_set_ctrl(opt_adi, 0);
	pipe_adi2axis_set_ctrl(opt_adi,        0);


exit_pipe:
	pipe_access_release(~0);
	pipe_dev_close();

exit_fifo:
	fifo_access_release(~0);
	fifo_dev_close();

exit_srio:
	close(srio_dev);

	return ret;
}

