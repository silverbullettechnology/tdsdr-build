/** Release request sequence
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

#include <common/default.h>
#include <common/control/local.h>
#include <common/vita49/common.h>
#include <common/vita49/command.h>

#include <tool/control/sequence.h>
#include <tool/control/expect.h>


LOG_MODULE_STATIC("control_sequence_release", LOG_LEVEL_INFO);


static struct v49_common release_req = 
{
	.sid  = V49_CMD_RSVD_SID,
	.type = V49_TYPE_COMMAND,
	.command = 
	{
		.role      = V49_CMD_ROLE_REQUEST,
		.request   = V49_CMD_REQ_RELEASE,
	}
};


static int release_expect (struct mbuf *mbuf, void *data)
{
	static struct v49_common  rsp;
	int                       ret;

	ENTER("mbuf %p, data %p", mbuf, data);

	if ( (ret = expect_common(&rsp, mbuf, &release_req, data)) < 1 )
		RETURN_VALUE("%d", ret);

	if ( module_level >= LOG_LEVEL_DEBUG )
		v49_dump(LOG_LEVEL_DEBUG, &rsp);

	LOG_INFO("RELEASE success\n");
	RETURN_ERRNO_VALUE(0, "%d", 1);
}

struct expect_map release_map[] = 
{
	{ release_expect, NULL },
	{ NULL }
};


static int sequence_release (int argc, char **argv)
{
	struct mbuf *mbuf;
	int          ret;

	ENTER("argc %d, argv [%s,...]", argc, argv[0]);

	if ( !argv[1] || !(release_req.sid = strtoul(argv[1], NULL, 0)) )
	{
		LOG_ERROR("Command '%s' requires a SID\n", argv[0]);
		RETURN_VALUE("%d", -1);
	}
	LOG_DEBUG("SID %u\n", release_req.sid);

	if ( !(mbuf = mbuf_alloc(DEFAULT_MBUF_SIZE, 0)) )
	{
		LOG_ERROR("mbuf_alloc() failed: %s\n", strerror(errno));
		RETURN_VALUE("%d", -1);
	}
	mbuf_beg_set(mbuf, DEFAULT_MBUF_HEAD);

	if ( (ret = v49_format(&release_req, mbuf, NULL)) != V49_OK_COMPLETE )
	{
		LOG_ERROR("v49_format(release_req) failed: %s\n", v49_return_desc(ret));
		mbuf_deref(mbuf);
	}
	expect_send(mbuf);

	ret = expect_loop(release_map, s_to_clocks(5));
	RETURN_ERRNO_VALUE(0, "%d", ret);
}
SEQUENCE_MAP("release", sequence_release, "Release request");

