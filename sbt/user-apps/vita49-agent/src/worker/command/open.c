/** Open command implementation
 *
 *  \author    Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
 *  \version   v0.0
 *  \copyright Copyright 2014,2015 Silver Bullet Technology
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
 *  vim:ts=4:noexpandtab
 */
#include <unistd.h>

#include <sbt_common/log.h>

#include <fifo_dev.h>
#include <pipe_dev.h>

#include <dsa_util.h>
#include <v49_message/common.h>
#include <v49_message/command.h>

#include <worker/worker.h>


LOG_MODULE_STATIC("worker_command_open", LOG_LEVEL_WARN);


static void worker_command_open_rx (int ident)
{
	unsigned long  reg;
	int            adi = DC_DEV_MASK_TO_IDX(ident & (DC_DEV_AD1|DC_DEV_AD2));

	// reset blocks in the PL
	fifo_adi_new_write(adi,            ADI_NEW_RX, ADI_NEW_RX_REG_RSTN, 0);
	pipe_vita49_pack_set_ctrl(adi,     PD_VITA49_PACK_CTRL_RESET);
	pipe_vita49_trig_adc_set_ctrl(adi, PD_VITA49_TRIG_CTRL_RESET);
	pipe_adi2axis_set_ctrl(adi,        PD_ADI2AXIS_CTRL_RESET);
	pipe_swrite_pack_set_cmd(adi,      PD_SWRITE_PACK_CMD_RESET);
	usleep(1000);

	fifo_adi_new_write(adi,            ADI_NEW_RX, ADI_NEW_RX_REG_RSTN, ADI_NEW_RX_RSTN);
	pipe_vita49_pack_set_ctrl(adi,     0);
	pipe_vita49_trig_adc_set_ctrl(adi, 0);
	pipe_adi2axis_set_ctrl(adi,        0);

	// V49 packer setup
	pipe_vita49_pack_set_streamid(adi, worker_sid);
	pipe_vita49_pack_set_pkt_size(adi, 64); // 64 32-bit words -> 256 bytes
	pipe_vita49_pack_set_trailer(adi,  0xaaaaaaaa); // trailer for alignment
	pipe_vita49_pack_set_ctrl(adi,     PD_VITA49_PACK_CTRL_ENABLE |
	                                   PD_VITA49_PACK_CTRL_TRAILER);

	// SWRITE packer setup 
	pipe_swrite_pack_set_srcdest(adi, worker_tuser);
	pipe_swrite_pack_set_addr(adi,    worker_sid); // sets SWRITE addr, matched on RX side
	pipe_swrite_pack_set_cmd(adi,     PD_SWRITE_PACK_CMD_START);

	// Set adc_sw_dest switch to 1,0 for ADI -> SRIO
	pipe_routing_reg_get_adc_sw_dest(&reg);
	switch ( adi )
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
	fifo_adi_new_write(adi, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(0), reg);
	fifo_adi_new_write(adi, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(1), reg);
	fifo_adi_new_write(adi, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(2), reg);
	fifo_adi_new_write(adi, ADI_NEW_RX, ADI_NEW_RX_REG_CHAN_CNTRL(3), reg);

	// Always using T2R2 for now
	fifo_adi_new_read(adi, ADI_NEW_RX, ADI_NEW_RX_REG_CNTRL, &reg);
	reg &= ~ADI_NEW_RX_R1_MODE;
	fifo_adi_new_write(adi, ADI_NEW_RX, ADI_NEW_RX_REG_CNTRL, reg);
}

static void worker_command_open_tx (int ident)
{
	unsigned long  reg;
	int            adi = DC_DEV_MASK_TO_IDX(ident & (DC_DEV_AD1|DC_DEV_AD2));
	int            ch;

	// reset blocks in the PL
	fifo_adi_new_write(adi,            ADI_NEW_TX, ADI_NEW_TX_REG_RSTN, 0);
	pipe_vita49_assem_set_cmd(adi,     PD_VITA49_ASSEM_CMD_RESET);
	pipe_vita49_unpack_set_ctrl(adi,   PD_VITA49_UNPACK_CTRL_RESET);
	pipe_vita49_trig_dac_set_ctrl(adi, PD_VITA49_TRIG_CTRL_RESET);
	pipe_swrite_unpack_set_cmd(PD_SWRITE_UNPACK_CMD_RESET);
	usleep(1000);

	fifo_adi_new_write(adi,            ADI_NEW_TX, ADI_NEW_TX_REG_RSTN, ADI_NEW_TX_RSTN);
	pipe_vita49_assem_set_cmd(adi,     0);
	pipe_vita49_unpack_set_ctrl(adi,   0);
	pipe_vita49_trig_dac_set_ctrl(adi, 0);


	// Set swrite routing to ADI
	pipe_routing_reg_get_adc_sw_dest(&reg);
	reg &= ~PD_ROUTING_REG_SWRITE_MASK;
	reg |=  PD_ROUTING_REG_SWRITE_ADI;
	pipe_routing_reg_set_adc_sw_dest(reg);

	// SWRITE unpacker setup 
	pipe_swrite_unpack_set_addr(adi, worker_sid);

	// V49 unpacker setup
	pipe_vita49_unpack_set_strm_id(adi, worker_sid);
	pipe_vita49_unpack_set_ctrl(adi,    PD_VITA49_UNPACK_CTRL_ENABLE |
	                                    PD_VITA49_UNPACK_CTRL_TRAILER);

	// FIFO setup - Select DMA source, enable format, disable T1R1 mode
	reg  = ADI_NEW_TX_DATA_SEL(ADI_NEW_TX_DATA_SEL_DMA);
	reg |= ADI_NEW_TX_DATA_FORMAT;
	reg &= ~ADI_NEW_TX_R1_MODE;
	fifo_adi_new_write(adi, ADI_NEW_TX, ADI_NEW_TX_REG_CNTRL_2, reg);

	// Rate 3 for T2R2 mode
	reg = ADI_NEW_TX_TO_RATE(3);
	fifo_adi_new_write(adi, ADI_NEW_TX, ADI_NEW_TX_REG_RATECNTRL, reg);

	// for version 8.xx set DAC_DDS_SEL to 0x02 input data (DMA)
	printf("Set new ADI v8 DAC_DDS_SEL to 2\n");
	for ( ch = 0; ch < 4; ch++ )
		fifo_adi_new_write(adi, ADI_NEW_TX, ADI_NEW_RX_REG_CHAN_DAC_DDS_SEL(ch), 0x02);

	// enable TX at ADI FIFO
	fifo_adi_new_read(adi, ADI_NEW_TX, ADI_NEW_TX_REG_CNTRL_1, &reg);
	reg |= ADI_NEW_TX_ENABLE;
	fifo_adi_new_write(adi, ADI_NEW_TX, ADI_NEW_TX_REG_CNTRL_1, reg);
}


/** Open command
 *
 *  \param req Request from client
 *  \param rsp Response to client
 */
void worker_command_open (struct v49_common *req, struct v49_common *rsp)
{
	ENTER("req %p, rsp %p", req, rsp);

	// access request first
	if ( fifo_access_request(worker_res.fd_bits) )
	{
		printf("fifo_access_request(0x%x): %s\n", worker_res.fd_bits, strerror(errno));
		rsp->command.result = V49_CMD_RES_ACCESS;
		RETURN_ERRNO(EPERM);
	}
	if ( pipe_access_request(worker_res.fd_bits) )
	{
		printf("pipe_access_request(0x%x): %s\n", worker_res.fd_bits, strerror(errno));
		fifo_access_release(~0);
		rsp->command.result = V49_CMD_RES_ACCESS;
		RETURN_ERRNO(EPERM);
	}

	switch ( (worker_res.dc_bits & (DC_DIR_RX|DC_DIR_TX)) )
	{
		case DC_DIR_RX:
			worker_command_open_rx(worker_res.dc_bits);
			rsp->command.result = V49_CMD_RES_SUCCESS;
			RETURN_ERRNO(0);

		case DC_DIR_TX:
			worker_command_open_tx(worker_res.dc_bits);
			rsp->command.result = V49_CMD_RES_SUCCESS;
			RETURN_ERRNO(0);

		default:
			fifo_access_release(~0);
			pipe_access_release(~0);
			rsp->command.result = V49_CMD_RES_INVAL;
			RETURN_ERRNO(EINVAL);
	}
}

