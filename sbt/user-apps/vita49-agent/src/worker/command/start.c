/** Start command implementation
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
#include <sbt_common/log.h>

#include <fifo_dev.h>
#include <pipe_dev.h>

#include <dsa_util.h>
#include <v49_message/common.h>
#include <v49_message/command.h>

#include <worker/worker.h>


LOG_MODULE_STATIC("worker_command_start", LOG_LEVEL_WARN);


static void worker_command_start_rx (int ident)
{
	int  adi = DC_DEV_MASK_TO_IDX(ident & (DC_DEV_AD1|DC_DEV_AD2));

	// using legacy fixed-length mode
	printf("Expecting to sample %zu samples\n", worker_tsf);
	pipe_adi2axis_set_bytes(adi, worker_tsf * 8);
	pipe_adi2axis_set_ctrl(adi,  PD_ADI2AXIS_CTRL_LEGACY);

	// manual trigger
	pipe_vita49_trig_adc_set_ctrl(adi, PD_VITA49_TRIG_CTRL_PASSTHRU);
}


static void worker_command_start_tx (int ident)
{
	int  adi = DC_DEV_MASK_TO_IDX(ident & (DC_DEV_AD1|DC_DEV_AD2));

	// manual trigger
	pipe_vita49_trig_dac_set_ctrl(adi, PD_VITA49_TRIG_CTRL_PASSTHRU);
	pipe_vita49_assem_set_cmd(adi,     PD_VITA49_ASSEM_CMD_ENABLE);
	pipe_swrite_unpack_set_cmd(PD_SWRITE_UNPACK_CMD_START);
}


/** start command
 *
 *  \param req Request from client
 *  \param rsp Response to client
 */
	
void worker_command_start (struct v49_common *req, struct v49_common *rsp)
{
	ENTER("req %p, rsp %p", req, rsp);

	if ( !worker_tsf )
	{
		LOG_ERROR("Start: no length was set\n");
		rsp->command.result = V49_CMD_RES_INVAL;
		RETURN_ERRNO(EINVAL);
	}

	switch ( (worker_res.dc_bits & (DC_DIR_RX|DC_DIR_TX)) )
	{
		case DC_DIR_RX:
			worker_command_start_rx(worker_res.dc_bits);
			rsp->command.result = V49_CMD_RES_SUCCESS;
			RETURN_ERRNO(0);

		case DC_DIR_TX:
			worker_command_start_tx(worker_res.dc_bits);
			rsp->command.result = V49_CMD_RES_SUCCESS;
			RETURN_ERRNO(0);

		default:
			rsp->command.result = V49_CMD_RES_INVAL;
			RETURN_ERRNO(EINVAL);
	}
}

