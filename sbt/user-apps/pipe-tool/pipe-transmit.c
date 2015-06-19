/** \file      pipe-transmit.c
 *  \brief     Sample pipeline transmit tool - SRIO to ADI
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
#include <signal.h>
#include <arpa/inet.h>

#include <sd_user.h>
#include <fifo_dev.h>
#include <pipe_dev.h>

#include "common.h"

uint16_t    opt_remote   = 2;
size_t      opt_data     = 8000000;
size_t      opt_npkts    = 0;
long        opt_timeout  = 1500;
uint32_t    opt_adi      = 0;
uint32_t    opt_sid      = 0;
FILE       *opt_debug    = NULL;


static void usage (void)
{
	printf("Usage: pipe-transmit [-v] [-s bytes] [-t timeout] adi stream-id\n"
	       "Where:\n"
	       "-R dest     SRIO destination address (default 2)\n"
	       "-v          Verbose/debugging enable\n"
	       "-s bytes    Set payload size in bytes (K or M optional)\n"
	       "-S samples  Set payload size in samples (K or M optional)\n"
	       "-t timeout  Set timeout in jiffies\n"
	       "\n"
	       "adi is required value, and be 0 or 1 for the ADI chain to use (T2R2 mode only,\n"
	       "this will be aligned with DSA syntax in future.\n"
	       "stream-id is a required value, and must match the expected stream-id on the\n"
	       "receiver side, if that receiver implements stream-ID filtering.\n"
		   "\n");
}

void handler (int sig)
{
	opt_timeout = -1;
}


int main (int argc, char **argv)
{
	struct pd_vita49_unpack  unpack;
	unsigned long            tuser;
	unsigned long            reg;
	int                      srio_dev;
	int                      ret = 0;
	int                      opt;
	int                      ch;

	while ( (opt = getopt(argc, argv, "?hvs:S:t:")) > -1 )
		switch ( opt )
		{
			case 'v':
				opt_debug = stderr;
				break;

			case 't': opt_timeout = strtoul(optarg, NULL, 0); break;

			case 's':
				opt_data = size_bin(optarg);
				break;

			case 'S':
				opt_data  = size_dec(optarg);
				opt_data *= 8;
				break;

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
	if ( fifo_access_request(opt_adi ? FD_ACCESS_AD2_TX : FD_ACCESS_AD1_TX) )
	{
		printf("fifo_access_request(FD_ACCESS_AD%d_TX): %s\n",
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
	if ( pipe_access_request(opt_adi ? PD_ACCESS_AD2_TX : PD_ACCESS_AD1_TX) )
	{
		printf("pipe_access_request(PD_ACCESS_AD%d_TX): %s\n",
		       opt_adi + 1, strerror(errno));
		goto exit_pipe;
	}

	// clamp timeout 
	if ( opt_timeout < 100 )
		opt_timeout = 100;
	else if ( opt_timeout > 6000 )
		opt_timeout = 6000;

	signal(SIGINT,  handler);
	signal(SIGTERM, handler);

	// using legacy fixed-length mode
	printf("Expecting %zu bytes / %zu samples\n", opt_data, opt_data / 8);
	opt_npkts = opt_data / 232;
	if ( opt_data % 232 )
		opt_npkts++;
	printf("Expecting %zu packets\n", opt_npkts);


	// reset blocks in the PL
	fifo_adi_new_write(opt_adi,            ADI_NEW_TX, ADI_NEW_TX_REG_RSTN, 0);
	pipe_vita49_assem_set_cmd(opt_adi,     PD_VITA49_ASSEM_CMD_RESET);
	pipe_vita49_unpack_set_ctrl(opt_adi,   PD_VITA49_UNPACK_CTRL_RESET);
	pipe_vita49_trig_dac_set_ctrl(opt_adi, PD_VITA49_TRIG_CTRL_RESET);
	pipe_swrite_unpack_set_cmd(PD_SWRITE_UNPACK_CMD_RESET);
	usleep(1000);

	fifo_adi_new_write(opt_adi,            ADI_NEW_TX, ADI_NEW_TX_REG_RSTN, ADI_NEW_TX_RSTN);
	pipe_vita49_assem_set_cmd(opt_adi,     0);
	pipe_vita49_unpack_set_ctrl(opt_adi,   0);
	pipe_vita49_trig_dac_set_ctrl(opt_adi, 0);


	// Set swrite routing to ADI
	pipe_routing_reg_get_adc_sw_dest(&reg);
	reg &= ~PD_ROUTING_REG_SWRITE_MASK;
	reg |=  PD_ROUTING_REG_SWRITE_ADI;
	pipe_routing_reg_set_adc_sw_dest(reg);


	// SWRITE unpacker setup 
	pipe_swrite_unpack_set_addr(opt_adi, opt_sid);


	// V49 unpacker setup
	pipe_vita49_unpack_set_strm_id(opt_adi, opt_sid);
	pipe_vita49_unpack_set_ctrl(opt_adi,    PD_VITA49_UNPACK_CTRL_ENABLE |
	                                        PD_VITA49_UNPACK_CTRL_TRAILER);
	pipe_vita49_unpack_get_ctrl(opt_adi, &reg);
	printf("vita49_unpack.ctrl: 0x%08lx\n",  reg);

	// FIFO setup - Select DMA source, enable format, disable T1R1 mode
	reg  = ADI_NEW_TX_DATA_SEL(ADI_NEW_TX_DATA_SEL_DMA);
	reg |= ADI_NEW_TX_DATA_FORMAT;
	reg &= ~ADI_NEW_TX_R1_MODE;
	fifo_adi_new_write(opt_adi, ADI_NEW_TX, ADI_NEW_TX_REG_CNTRL_2, reg);

	// Rate 3 for T2R2 mode
	reg = ADI_NEW_TX_TO_RATE(3);
	fifo_adi_new_write(opt_adi, ADI_NEW_TX, ADI_NEW_TX_REG_RATECNTRL, reg);

	// for version 8.xx set DAC_DDS_SEL to 0x02 input data (DMA)
	printf("Set new ADI v8 DAC_DDS_SEL to 2\n");
	for ( ch = 0; ch < 4; ch++ )
		fifo_adi_new_write(opt_adi, ADI_NEW_TX, ADI_NEW_RX_REG_CHAN_DAC_DDS_SEL(ch), 0x02);

	// enable TX at ADI FIFO
	fifo_adi_new_read(opt_adi, ADI_NEW_TX, ADI_NEW_TX_REG_CNTRL_1, &reg);
	reg |= ADI_NEW_TX_ENABLE;
	fifo_adi_new_write(opt_adi, ADI_NEW_TX, ADI_NEW_TX_REG_CNTRL_1, reg);

	// manual trigger
	pipe_vita49_trig_dac_set_ctrl(opt_adi, PD_VITA49_TRIG_CTRL_PASSTHRU);
	pipe_vita49_assem_set_cmd(opt_adi,     PD_VITA49_ASSEM_CMD_ENABLE);
	pipe_swrite_unpack_set_cmd(PD_SWRITE_UNPACK_CMD_START);


	// wait for complete or timeout
	do
	{
		usleep(10000);
		pipe_vita49_unpack_get_rcvd(opt_adi, &reg);
//		printf("%04u: %08x\n", opt_timeout, reg);
		opt_timeout--;
	}
	while ( opt_timeout > 0 && reg < opt_npkts );

	switch ( opt_timeout )
	{
		case -2:
		case -1:
			printf("Interrupted\n");
			break;

		case 0:
			printf("Timed out\n");
			break;

		default:
			printf("Completed normally with timeout %d\n", opt_timeout);
			break;
	}

	printf("vita49_unpack regs:\n");
	pipe_vita49_unpack_get_ctrl(opt_adi, &reg);
	printf("  ctrl         : %lu\n",  reg);
	pipe_vita49_unpack_get_stat(opt_adi, &reg);
	printf("  stat         : %lx\n",  reg);
	pipe_vita49_unpack_get_rcvd(opt_adi, &reg);
	printf("  rcvd         : %lu\n",  reg);

	pipe_vita49_assem_get_hdr_err(opt_adi, &reg);
	printf("vita49_assem stats:\n");
	printf("  hdr_err_cnt  : %lu\n",  reg);

	pipe_vita49_unpack_get_counts(opt_adi, &unpack);
	printf("vita49_unpack stats:\n");
	printf("  pkt_rcv_cnt  : %lu\n",  unpack.pkt_rcv_cnt);
	printf("  pkt_drop_cnt : %lu\n",  unpack.pkt_drop_cnt);
	printf("  pkt_size_err : %lu\n",  unpack.pkt_size_err);
	printf("  pkt_type_err : %lu\n",  unpack.pkt_type_err);
	printf("  pkt_order_err: %lu\n",  unpack.pkt_order_err);
	printf("  ts_order_err : %lu\n",  unpack.ts_order_err);
	printf("  strm_id_err  : %lu\n",  unpack.strm_id_err);
	printf("  trailer_err  : %lu\n",  unpack.trailer_err);

	// reset and shut-down pipeline
	fifo_adi_new_write(opt_adi,            ADI_NEW_TX, ADI_NEW_TX_REG_RSTN, 0);
	pipe_vita49_unpack_set_ctrl(opt_adi,   PD_VITA49_UNPACK_CTRL_RESET);
	pipe_vita49_trig_dac_set_ctrl(opt_adi, PD_VITA49_TRIG_CTRL_RESET);
	pipe_swrite_unpack_set_cmd(PD_SWRITE_UNPACK_CMD_RESET);
	usleep(1000);

	fifo_adi_new_write(opt_adi,            ADI_NEW_TX, ADI_NEW_TX_REG_RSTN, ADI_NEW_TX_RSTN);
	pipe_vita49_unpack_set_ctrl(opt_adi,   0);
	pipe_vita49_assem_set_cmd(opt_adi,     0);
	pipe_vita49_trig_dac_set_ctrl(opt_adi, 0);


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

