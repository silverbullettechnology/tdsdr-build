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

#include <dsa_util.h>
#include <v49_message/common.h>
#include <v49_message/command.h>

#include <worker/worker.h>


LOG_MODULE_STATIC("worker_command_stop", LOG_LEVEL_WARN);


/** stop command
 *
 *  \param req Request from client
 *  \param rsp Response to client
 */
	
void worker_command_stop (struct v49_common *req, struct v49_common *rsp)
{
	ENTER("req %p, rsp %p", req, rsp);

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

	worker_tsi = req->ts_int;
	worker_tsf = req->ts_frac;
	LOG_DEBUG("Stop: TSI %d, TSF %zu\n", (int)worker_tsi, worker_tsf);
	rsp->command.result = V49_CMD_RES_SUCCESS;

	RETURN_ERRNO(0);
}

