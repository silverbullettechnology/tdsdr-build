/** close request sequence
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


LOG_MODULE_STATIC("seq_close", LOG_LEVEL_DEBUG);


static struct v49_common req_close = 
{
	.sid  = V49_CMD_RSVD_SID,
	.type = V49_TYPE_COMMAND,
	.command = 
	{
		.role      = V49_CMD_ROLE_REQUEST,
		.request   = V49_CMD_REQ_CLOSE,
	}
};


static int exp_close (struct mbuf *mbuf, void *data)
{
	static struct v49_common  rsp;
	int                       ret;

	ENTER("mbuf %p, data %p", mbuf, data);

	if ( (ret = expect_common(&rsp, mbuf, &req_close, data)) < 1 )
		RETURN_VALUE("%d", ret);

	LOG_DEBUG("close success\n");
	RETURN_ERRNO_VALUE(0, "%d", 1);
}

struct expect_map map_close[] = 
{
	{ exp_close, NULL },
	{ NULL }
};


int seq_close (struct socket *sock, uint32_t stream_id)
{
	struct mbuf *mbuf;
	int          ret;

	ENTER("sock %p, sid %u", sock, stream_id);

	req_close.sid = stream_id;
	LOG_DEBUG("Close: SID %u\n", req_close.sid);

	if ( !(mbuf = mbuf_alloc(DEFAULT_MBUF_SIZE, 0)) )
	{
		LOG_ERROR("mbuf_alloc() failed: %s\n", strerror(errno));
		RETURN_VALUE("%d", -1);
	}
	mbuf_beg_set(mbuf, DEFAULT_MBUF_HEAD);

	if ( (ret = v49_format(&req_close, mbuf, NULL)) != V49_OK_COMPLETE )
	{
		LOG_ERROR("v49_format(close) failed: %s\n", v49_return_desc(ret));
		mbuf_deref(mbuf);
	}
	expect_send(sock, mbuf);

	ret = expect_loop(sock, map_close, s_to_clocks(5));
	RETURN_ERRNO_VALUE(0, "%d", ret);
}

