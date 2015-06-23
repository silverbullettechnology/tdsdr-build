/** start request sequence
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
#include <sbt_common/mbuf.h>

#include <v49_message/common.h>
#include <v49_message/command.h>

#include <v49_client/socket.h>
#include <v49_client/sequence.h>
#include <v49_client/expect.h>


LOG_MODULE_STATIC("seq_start", LOG_LEVEL_DEBUG);


static struct v49_common req_start = 
{
	.sid  = V49_CMD_RSVD_SID,
	.type = V49_TYPE_COMMAND,
	.command = 
	{
		.role      = V49_CMD_ROLE_REQUEST,
		.request   = V49_CMD_REQ_START,
	}
};


static int exp_start (struct mbuf *mbuf, void *data)
{
	static struct v49_common  rsp;
	int                       ret;

	ENTER("mbuf %p, data %p", mbuf, data);

	if ( (ret = expect_common(&rsp, mbuf, &req_start, data)) < 1 )
		RETURN_VALUE("%d", ret);

	LOG_DEBUG("start success\n");
	RETURN_ERRNO_VALUE(0, "%d", 1);
}

struct expect_map map_start[] = 
{
	{ exp_start, NULL },
	{ NULL }
};


int seq_start (struct socket *sock, uint32_t stream_id,
               v49_command_tstamp_int_t tstamp_int, time_t tsi, size_t tsf)
{
	struct mbuf *mbuf;
	int          ret;

	ENTER("sock %p, sid %u, tstamp_int %s, tsi %d, tsf %zu",
	      sock, stream_id, v49_command_tstamp_int(tstamp_int), tsi, tsf);

	req_start.sid     = stream_id;
	req_start.ts_int  = tsi;
	req_start.ts_frac = tsf;

	req_start.command.tstamp_int = tstamp_int;
	req_start.command.indicator |= (1 << V49_CMD_IND_BIT_TSTAMP_INT);

	LOG_DEBUG("start: SID %u, tstamp_int %s, TSI %d, TSF %zu\n",
	          req_start.sid, v49_command_tstamp_int(req_start.command.tstamp_int),
	          req_start.ts_int, req_start.ts_frac);

	if ( !(mbuf = mbuf_alloc(DEFAULT_MBUF_SIZE, 0)) )
	{
		LOG_ERROR("mbuf_alloc() failed: %s\n", strerror(errno));
		RETURN_VALUE("%d", -1);
	}
	mbuf_beg_set(mbuf, DEFAULT_MBUF_HEAD);

	if ( (ret = v49_format(&req_start, mbuf, NULL)) != V49_OK_COMPLETE )
	{
		LOG_ERROR("v49_format(start) failed: %s\n", v49_return_desc(ret));
		mbuf_deref(mbuf);
	}
	expect_send(sock, mbuf);

	ret = expect_loop(sock, map_start, s_to_clocks(5));
	RETURN_ERRNO_VALUE(0, "%d", ret);
}

