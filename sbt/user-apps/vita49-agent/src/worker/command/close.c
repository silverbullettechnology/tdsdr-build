/** Close command implementation
 *
 *  \author    Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
 *  \version   v0.0
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


LOG_MODULE_STATIC("worker_command_close", LOG_LEVEL_WARN);


static void worker_command_close_rx (int ident)
{
	LOG_INFO("CLOSE RX: requested %s -> adi %d\n", dsa_util_chan_desc(ident), worker_adi);

	// reset and shut-down pipeline
	fifo_adi_new_write(worker_adi,            ADI_NEW_RX, ADI_NEW_RX_REG_RSTN, 0);
	pipe_vita49_pack_set_ctrl(worker_adi,     PD_VITA49_PACK_CTRL_RESET);
	pipe_vita49_trig_adc_set_ctrl(worker_adi, PD_VITA49_TRIG_CTRL_RESET);
	pipe_adi2axis_set_ctrl(worker_adi,        PD_ADI2AXIS_CTRL_RESET);
	pipe_swrite_pack_set_cmd(worker_adi,      PD_SWRITE_PACK_CMD_RESET);
	usleep(1000);

	fifo_adi_new_write(worker_adi,            ADI_NEW_RX, ADI_NEW_RX_REG_RSTN, ADI_NEW_RX_RSTN);
	pipe_vita49_pack_set_ctrl(worker_adi,     0);
	pipe_vita49_trig_adc_set_ctrl(worker_adi, 0);
	pipe_adi2axis_set_ctrl(worker_adi,        0);
}

static void worker_command_close_tx (int ident)
{
	LOG_INFO("CLOSE TX: requested %s -> adi %d\n", dsa_util_chan_desc(ident), worker_adi);

	// reset and shut-down pipeline
	fifo_adi_new_write(worker_adi,            ADI_NEW_TX, ADI_NEW_TX_REG_RSTN, 0);
	pipe_vita49_unpack_set_ctrl(worker_adi,   PD_VITA49_UNPACK_CTRL_RESET);
	pipe_vita49_trig_dac_set_ctrl(worker_adi, PD_VITA49_TRIG_CTRL_RESET);
	pipe_swrite_unpack_set_cmd(PD_SWRITE_UNPACK_CMD_RESET);
	usleep(1000);

	fifo_adi_new_write(worker_adi,            ADI_NEW_TX, ADI_NEW_TX_REG_RSTN, ADI_NEW_TX_RSTN);
	pipe_vita49_unpack_set_ctrl(worker_adi,   0);
	pipe_vita49_assem_set_cmd(worker_adi,     0);
	pipe_vita49_trig_dac_set_ctrl(worker_adi, 0);
}


/** Close command
 *
 *  \param req Request from client
 *  \param rsp Response to client
 */
	
void worker_command_close (struct v49_common *req, struct v49_common *rsp)
{
	ENTER("req %p, rsp %p", req, rsp);

	switch ( (worker_res.dc_bits & (DC_DIR_RX|DC_DIR_TX)) )
	{
		case DC_DIR_RX:
			worker_command_close_rx(worker_res.dc_bits);
			rsp->command.result = V49_CMD_RES_SUCCESS;
			RETURN_ERRNO(0);

		case DC_DIR_TX:
			worker_command_close_tx(worker_res.dc_bits);
			rsp->command.result = V49_CMD_RES_SUCCESS;
			RETURN_ERRNO(0);

		default:
			fifo_access_release(~0);
			pipe_access_release(~0);
			rsp->command.result = V49_CMD_RES_INVAL;
			RETURN_ERRNO(EINVAL);
	}

	pipe_access_release(~0);
	fifo_access_release(~0);
	RETURN_ERRNO(0);
}

