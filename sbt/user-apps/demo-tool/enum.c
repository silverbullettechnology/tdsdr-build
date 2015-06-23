/** Enumeration sequence
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

#include <v49_message/resource.h>
#include <v49_message/common.h>
#include <v49_message/command.h>

#include <v49_client/socket.h>
#include <v49_client/sequence.h>
#include <v49_client/expect.h>


LOG_MODULE_STATIC("seq_enum", LOG_LEVEL_INFO);


struct growlist  rid_enum = GROWLIST_INIT;
struct growlist *res_enum;

static struct v49_common req_enum = 
{
	.sid  = V49_CMD_RSVD_SID,
	.type = V49_TYPE_COMMAND,
	.command = 
	{
		.role      = V49_CMD_ROLE_REQUEST,
		.request   = V49_CMD_REQ_ENUM,
		.indicator = 0,
	}
};


static int exp_enum (struct mbuf *mbuf, void *data)
{
	struct v49_common  rsp;
	int                ret;

	ENTER("mbuf %p, data %p", mbuf, data);

	if ( (ret = expect_common(&rsp, mbuf, &req_enum, data)) < 1 )
		RETURN_VALUE("%d", ret);

	if ( module_level >= LOG_LEVEL_DEBUG )
		v49_dump(LOG_LEVEL_DEBUG, &rsp);

	if ( !(rsp.command.indicator & (1 << V49_CMD_IND_BIT_RES_INFO)) ||
	     !rsp.command.res_info )
	{
		LOG_ERROR("Successful DISCO req needs a Resource list returned\n");
		RETURN_ERRNO_VALUE(0, "%d", -1);
	}

	// save it for enum_seq, free rid_list if allocated
	res_enum = rsp.command.res_info;
	if ( rsp.command.rid_list )
	{
		growlist_done(rsp.command.rid_list, gen_free, NULL);
		free(rsp.command.rid_list);
	}

	RETURN_ERRNO_VALUE(0, "%d", 1);
}

struct expect_map map_enum[] = 
{
	{ exp_enum, NULL },
	{ NULL }
};


int seq_enum (struct socket *sock, uuid_t *cid, struct resource_info *dest, char *name)
{
	struct resource_info *res;
	struct mbuf          *mbuf;
	uuid_t                buf;
	int                   bad;
	int                   ret;

	ENTER("sock %p, res %p, name %s", sock, res, name);

	// if it's a valid UUID, put that in the rid_list and ask only for that
	// otherwise we should get all the resources on that SDR node
	memset(&buf, 0xFF, sizeof(buf));
	if ( !(bad = uuid_from_str(&buf, name)) )
	{
		if ( growlist_append(&rid_enum, &buf) < 0 )
		{
			LOG_ERROR("growlist_append() failed: %s\n", strerror(errno));
			ret = -1;
			goto out;
		}

		req_enum.command.rid_list   = &rid_enum;
		req_enum.command.indicator |= 1 << V49_CMD_IND_BIT_RID_LIST;
	}

	if ( cid )
	{
		memcpy(&req_enum.command.cid, cid, sizeof(uuid_t));
		LOG_DEBUG("Access: CID %s\n", uuid_to_str(&req_enum.command.cid));
		req_enum.command.indicator |= (1 << V49_CMD_IND_BIT_CID);
	}

	if ( !(mbuf = mbuf_alloc(DEFAULT_MBUF_SIZE, 0)) )
	{
		LOG_ERROR("mbuf_alloc() failed: %s\n", strerror(errno));
		RETURN_VALUE("%d", -1);
	}

	mbuf_beg_set(mbuf, DEFAULT_MBUF_HEAD);
	if ( (ret = v49_format(&req_enum, mbuf, NULL)) != V49_OK_COMPLETE )
	{
		LOG_ERROR("v49_format(req_enum) failed: %s\n", v49_return_desc(ret));
		mbuf_deref(mbuf);
		RETURN_VALUE("%d", -1);
	}
	expect_send(sock, mbuf);

	if ( (ret = expect_loop(sock, map_enum, s_to_clocks(5))) < 1 )
		goto out;
	
	// search by UUID
	growlist_reset(res_enum);
	if ( (res = growlist_search(res_enum, uuid_cmp, &buf)) )
	{
		resource_dump(LOG_LEVEL_INFO, "Matched by UUID: ", res);
		memcpy(dest, res, sizeof(struct resource_info));
		goto out;
	}
	
	// search by name
	growlist_reset(res_enum);
	while ( (res = growlist_next(res_enum)) )
		if ( !strcmp(res->name, name) )
		{
			resource_dump(LOG_LEVEL_INFO, "Matched by name: ", res);
			memcpy(dest, res, sizeof(struct resource_info));
			goto out;
		}
		else
			LOG_DEBUG("Not matched: '%s'\n", res->name);

	LOG_ERROR("Enum for '%s' failed: not in list returned\n", name);
	ret = -1;

out:
	growlist_done(&rid_enum, NULL, NULL);
	growlist_done(res_enum, gen_free, NULL);
	free(res_enum);
	RETURN_ERRNO_VALUE(0, "%d", ret);
}


