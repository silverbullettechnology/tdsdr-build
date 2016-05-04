/** tstamp_control command implementation
 *
 *  \author    Larry Gitlitz <larry.gitlitz@silver-bullet-tech.com>
 *  \version   v0.0
 *  \copyright Copyright 2014-2016 Silver Bullet Technology
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


LOG_MODULE_STATIC("worker_command_tstamp_control", LOG_LEVEL_WARN);

/** tstamp_control command
 *
 *  \param req Request from client
 *  \param rsp Response to client
 */
	
void worker_command_tstamp_control (struct v49_common *req, struct v49_common *rsp)
{
	ENTER("req %p, rsp %p", req, rsp);

	if ( req->command.indicator & (1 << V49_CMD_IND_BIT_TSTAMP_FMT) )
	{
        if ( req->command.tstamp_fmt != ( TSTAMP_FMT_TSF_SAMP | TSTAMP_FMT_TSI_NONE ) )
        {
            LOG_ERROR("Timestamp Control: TSF:Sample, TSI:None Supported\n");
            rsp->command.result = V49_CMD_RES_INVAL;
            RETURN_ERRNO(EINVAL);
        }
        else
        {
            LOG_INFO("TIMESTAMP CONTROL: tstamp format 0x%x -> adi %d\n", req->command.tstamp_fmt, worker_adi);
            unsigned long clk_ctrl;
            pipe_vita49_clk_get_ctrl(&clk_ctrl);
            clk_ctrl |= ( PD_VITA49_CLK_CTRL_ENABLE | PD_VITA49_CLK_CTRL_SYNC_EN | PD_VITA49_CLK_CTRL_SYNC_OR );
            LOG_INFO("TIMESTAMP CONTROL: enabling external sync -> clk_ctrl 0x%x\n", (unsigned int)clk_ctrl );
            pipe_vita49_clk_set_ctrl( clk_ctrl );
        }
	}

    if ( req->command.indicator & (1 << V49_CMD_IND_BIT_EVENT_PRD) )
    {
        LOG_INFO("TIMESTAMP CONTROL: event period 0x%x -> adi %d\n", req->command.event_prd, worker_adi);
        worker_tstamp_report_period = req->command.event_prd;
    }

    rsp->command.result = V49_CMD_RES_SUCCESS;
    RETURN_ERRNO(0);
}

