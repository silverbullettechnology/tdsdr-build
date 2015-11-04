/** Stop command implementation
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
#include <format.h>

#include <dsa_util.h>
#include <v49_message/common.h>
#include <v49_message/command.h>

#include <worker/worker.h>


LOG_MODULE_STATIC("worker_command_stop", LOG_LEVEL_WARN);


static struct format_options fmt_opts =
{
	.channels = 3,
	.single   = sizeof(uint16_t) * 2,
	.sample   = sizeof(uint16_t) * 4,
	.bits     = 16,
	.head     = VITA_HEAD,
};


/** stop command
 *
 *  \param req Request from client
 *  \param rsp Response to client
 */
	
void worker_command_stop (struct v49_common *req, struct v49_common *rsp)
{
	ENTER("req %p, rsp %p", req, rsp);

	LOG_INFO("STOP: ReQuEsTeD %s -> adi %d\n",
	         dsa_util_chan_desc(worker_res.dc_bits), worker_adi);

	if ( !(req->command.indicator & (1 << V49_CMD_IND_BIT_TSTAMP_INT)) )
	{
		LOG_ERROR("Stop: tstamp_int field is required\n");
		rsp->command.result = V49_CMD_RES_INVAL;
		RETURN_ERRNO(EINVAL);
	}
	if ( req->command.tstamp_int != TSTAMP_INT_RELATIVE )
	{
		LOG_ERROR("Stop: tstamp_int field must be TSTAMP_INT_RELATIVE\n");
		rsp->command.result = V49_CMD_RES_INVAL;
		RETURN_ERRNO(EINVAL);
	}

	fmt_opts.data   = worker_opt_body - fmt_opts.head;
	fmt_opts.packet = worker_opt_body;

	worker_tsi = req->ts_int;
	worker_tsf = req->ts_frac;
	worker_len = worker_tsf * fmt_opts.sample;
	worker_pkt = format_num_packets_from_data(&fmt_opts, worker_len);
	LOG_DEBUG("Stop: TSI %d, TSF %zu -> len %zu, %zu pkts\n",
	          (int)worker_tsi, worker_tsf, worker_len, worker_pkt);
	rsp->command.result = V49_CMD_RES_SUCCESS;

	RETURN_ERRNO(0);
}

