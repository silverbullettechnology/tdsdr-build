/** Discovery sequence
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
#include <lib/log.h>
#include <lib/mbuf.h>

#include <common/default.h>
#include <common/control/local.h>
#include <common/vita49/common.h>
#include <common/vita49/command.h>

#include <tool/control/sequence.h>
#include <tool/control/expect.h>


LOG_MODULE_STATIC("control_sequence_disco", LOG_LEVEL_TRACE);

struct growlist  disco_rid_list = GROWLIST_INIT;

static struct v49_common disco_req = 
{
	.sid  = 0, // TODO: symbol for reserved sid
	.type = V49_TYPE_COMMAND,
	.command = 
	{
		.role      = V49_CMD_ROLE_REQUEST,
		.request   = V49_CMD_REQ_DISCO,
		.indicator = (1 << V49_CMD_IND_BIT_CID),
	}
};


static int disco_expect (struct mbuf *mbuf, void *data)
{
	static struct v49_common  v49;
	int                       ret;

	ENTER("mbuf %p, data %p", mbuf, data);

	v49_reset(&v49);
	if ( (ret = v49_parse(&v49, mbuf)) < 0 )
	{
		LOG_ERROR("Parse error: %s\n", strerror(errno));
		RETURN_VALUE("%d", -1);
	}

	if ( V49_RET_ERROR(ret) )
	{
		LOG_ERROR("Parse/validation fail: %s\n", v49_return_desc(ret));
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	if ( v49.type != V49_TYPE_COMMAND )
		RETURN_ERRNO_VALUE(0, "%d", 0);

	if ( v49.command.role    != V49_CMD_ROLE_RESULT ) RETURN_ERRNO_VALUE(0, "%d", 0);
	if ( v49.command.request != V49_CMD_REQ_DISCO   ) RETURN_ERRNO_VALUE(0, "%d", 0);
	if ( v49.command.result  != V49_CMD_RES_SUCCESS ) RETURN_ERRNO_VALUE(0, "%d", 0);
	if ( uuid_cmp(&v49.command.cid, &disco_req.command.cid) )
	{
		LOG_DEBUG("req cid %s\n", uuid_to_str(&disco_req.command.cid));
		LOG_DEBUG("rsp cid %s\n", uuid_to_str(&v49.command.cid));
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	v49_dump(LOG_LEVEL_INFO, &v49);
	RETURN_ERRNO_VALUE(0, "%d", 1);
}

struct expect_map disco_map[] = 
{
	{ disco_expect, NULL },
	{ NULL }
};


static int sequence_disco (int argc, char **argv)
{
	struct mbuf *mbuf;
	uuid_t      *ptr;
	uuid_t       buf;
	int          ret;

	ENTER("argc %d, argv [%s,...]", argc, argv[0]);

	for ( argv++; *argv; argv++ )
	{
		if ( (ret = uuid_from_str(&buf, *argv)) )
		{
			LOG_ERROR("UUID '%s' malformed: %d\n", *argv, ret);
			goto out;
		}
		LOG_DEBUG("UUID '%s' -> %s\n", *argv, uuid_to_str(&buf));
		if ( !(ptr = malloc(sizeof(*ptr))) )
		{
			LOG_ERROR("malloc failed: %s\n", strerror(errno));
			ret = -1;
			goto out;
		}
		memcpy(ptr, &buf, sizeof(*ptr));
		if ( growlist_append(&disco_rid_list, ptr) < 0 )
		{
			LOG_ERROR("growlist_append() failed: %s\n", strerror(errno));
			ret = -1;
			goto out;
		}
	}
	if ( growlist_used(&disco_rid_list) )
	{
		disco_req.command.rid_list   = &disco_rid_list;
		disco_req.command.indicator |= 1 << V49_CMD_IND_BIT_RID_LIST;
	}

	if ( !(mbuf = mbuf_alloc(DEFAULT_MBUF_SIZE, 0)) )
	{
		LOG_ERROR("mbuf_alloc() failed: %s\n", strerror(errno));
		RETURN_VALUE("%d", -1);
	}
	mbuf_beg_set(mbuf, DEFAULT_MBUF_HEAD);

	uuid_random(&disco_req.command.cid);
	LOG_DEBUG("Disco: CID %s\n", uuid_to_str(&disco_req.command.cid));
	if ( (ret = v49_format(&disco_req, mbuf, NULL)) != V49_OK_COMPLETE )
	{
		LOG_ERROR("v49_format(disco_req) failed: %s\n", v49_return_desc(ret));
		mbuf_deref(mbuf);
		RETURN_VALUE("%d", -1);
	}
	expect_send(mbuf);

	ret = expect_loop(disco_map, s_to_clocks(5));

out:
	RETURN_ERRNO_VALUE(0, "%d", ret);
}
SEQUENCE_MAP("disco", sequence_disco, "Discover resources");

