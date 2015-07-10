/** Access request sequence
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


LOG_MODULE_STATIC("seq_access", LOG_LEVEL_DEBUG);


struct growlist  rid_access = GROWLIST_INIT;

static struct v49_common req_access = 
{
	.sid  = V49_CMD_RSVD_SID,
	.type = V49_TYPE_COMMAND,
	.command = 
	{
		.role      = V49_CMD_ROLE_REQUEST,
		.request   = V49_CMD_REQ_ACCESS,
		.indicator = 0,
	}
};

static uint32_t sid_assign = 0;

static int exp_access (struct mbuf *mbuf, void *data)
{
	static struct v49_common  rsp;
	int                       ret;

	ENTER("mbuf %p, data %p", mbuf, data);

	if ( (ret = expect_common(&rsp, mbuf, &req_access, data)) < 1 )
		RETURN_VALUE("%d", ret);

	if ( !(rsp.command.indicator & (1 << V49_CMD_IND_BIT_SID_ASSIGN)) )
	{
		LOG_ERROR("Successful ACCESS req needs a SID assigned\n");
		RETURN_ERRNO_VALUE(0, "%d", -1);
	}

	LOG_DEBUG("SID assigned: %u\n", rsp.command.sid_assign);
	sid_assign = rsp.command.sid_assign;
	RETURN_ERRNO_VALUE(0, "%d", 1);
}

struct expect_map map_access[] = 
{
	{ exp_access, NULL },
	{ NULL }
};

int seq_access (struct socket *sock, uuid_t *cid, uuid_t *uuid, uint32_t *stream_id)
{
	struct mbuf *mbuf;
	int          ret;

	ENTER("sock %p, uuid %s", sock, uuid_to_str(uuid));

	if ( growlist_append(&rid_access, uuid) < 0 )
	{
		LOG_ERROR("growlist_append() failed: %s\n", strerror(errno));
		ret = -1;
		goto out;
	}
	req_access.command.rid_list   = &rid_access;
	req_access.command.indicator |= 1 << V49_CMD_IND_BIT_RID_LIST;

	if ( !(mbuf = mbuf_alloc(DEFAULT_MBUF_SIZE, 0)) )
	{
		LOG_ERROR("mbuf_alloc() failed: %s\n", strerror(errno));
		RETURN_VALUE("%d", -1);
	}

	if ( cid )
	{
		memcpy(&req_access.command.cid, cid, sizeof(uuid_t));
		LOG_DEBUG("Access: CID %s\n", uuid_to_str(&req_access.command.cid));
		req_access.command.indicator |= (1 << V49_CMD_IND_BIT_CID);
	}

	mbuf_beg_set(mbuf, DEFAULT_MBUF_HEAD);
	if ( (ret = v49_format(&req_access, mbuf, NULL)) != V49_OK_COMPLETE )
	{
		LOG_ERROR("v49_format(access) failed: %s\n", v49_return_desc(ret));
		mbuf_deref(mbuf);
		RETURN_VALUE("%d", -1);
	}
	expect_send(sock, mbuf);

	if ( (ret = expect_loop(sock, map_access, s_to_clocks(5))) > 0 )
		*stream_id = sid_assign;

out:
	growlist_done(&rid_access, NULL, NULL);
	RETURN_ERRNO_VALUE(0, "%d", ret);
}


