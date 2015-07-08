/** 
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
#include <stdint.h>
#include <stddef.h>

#include <sbt_common/log.h>
#include <sbt_common/mbuf.h>
#include <sbt_common/mqueue.h>
#include <sbt_common/growlist.h>

#include <v49_message/types.h>
#include <v49_message/common.h>
#include <v49_message/control.h>


LOG_MODULE_STATIC("vita49_control", LOG_LEVEL_INFO);


struct v49_control *v49_control_parse (struct mbuf *mbuf)
{
	struct v49_control *ctrl;
	int                 size;

	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	// check the length and get the fixed portion
	size = mbuf_get_avail(mbuf);
	ctrl = mbuf_cur_ptr(mbuf);

	// check message size against buffer length
	if ( ctrl->size > size || ctrl->size < offsetof(struct v49_control, args) )
	{
		LOG_ERROR("%s: Bad message length (size %u, have %d)\n",
		          __func__, ctrl->size, size);
		RETURN_ERRNO_VALUE(EBADMSG, "%p", NULL);
	}

	RETURN_ERRNO_VALUE(0, "%p", ctrl);
}


int v49_control_format_shutdown (struct mbuf *mbuf)
{
	struct v49_control *ctrl;
	int                 need;
	int                 have;

	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	need = offsetof(struct v49_control, args);
	if ( (have = mbuf_set_avail(mbuf)) < need )
	{
		LOG_ERROR("%s: Buffer too small (need %d, have %d)\n", __func__, need, have);
		RETURN_ERRNO_VALUE(ENOSPC, "%d", -1);
	}

	ctrl = mbuf_cur_ptr(mbuf);
	ctrl->magic = V49_CTRL_MAGIC;
	ctrl->size  = need;
	ctrl->verb  = V49_CTRL_SHUTDOWN;

	mbuf_cur_adj(mbuf, need);
	mbuf_end_set_cur(mbuf);

	RETURN_ERRNO_VALUE(0, "%d", need);
}


static int v49_control_format_start_stop (struct mbuf *mbuf, uint32_t sid, uint32_t err, 
                                          v49_verb_t verb)
{
	struct v49_control *ctrl;
	int                 need;
	int                 have;

	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	need = offsetof(struct v49_control, args) + sizeof(struct v49_ctrl_start_stop);
	if ( (have = mbuf_set_avail(mbuf)) < need )
	{
		LOG_ERROR("%s: Buffer too small (need %d, have %d)\n", __func__, need, have);
		RETURN_ERRNO_VALUE(ENOSPC, "%d", -1);
	}

	ctrl = mbuf_cur_ptr(mbuf);
	ctrl->magic = V49_CTRL_MAGIC;
	ctrl->size  = need;
	ctrl->verb  = verb;
	ctrl->args.start_stop.sid = sid;
	ctrl->args.start_stop.err = err;

	mbuf_cur_adj(mbuf, need);
	mbuf_end_set_cur(mbuf);

	RETURN_ERRNO_VALUE(0, "%d", need);
}

int v49_control_format_start (struct mbuf *mbuf, uint32_t sid, uint32_t err)
{
	return v49_control_format_start_stop(mbuf, sid, err, V49_CTRL_START);

}


int v49_control_format_stop (struct mbuf *mbuf, uint32_t sid, uint32_t err)
{
	return v49_control_format_start_stop(mbuf, sid, err, V49_CTRL_STOP);
}


int v49_control_format_list (struct mbuf *mbuf, size_t num,
                             struct v49_ctrl_list_item *items)
{
	struct v49_control *ctrl;
	int                 need;
	int                 have;

	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	need = offsetof(struct v49_control, args) +
	       offsetof(struct v49_ctrl_list, items) +
	       num * sizeof(struct v49_ctrl_list_item);
	if ( (have = mbuf_set_avail(mbuf)) < need )
	{
		LOG_ERROR("%s: Buffer too small (need %d, have %d)\n", __func__, need, have);
		RETURN_ERRNO_VALUE(ENOSPC, "%d", -1);
	}

	ctrl = mbuf_cur_ptr(mbuf);
	ctrl->magic = V49_CTRL_MAGIC;
	ctrl->size  = need;
	ctrl->verb  = V49_CTRL_LIST;
	ctrl->args.list.num = num;
	memcpy(ctrl->args.list.items, items, num * sizeof(struct v49_ctrl_list_item));

	mbuf_cur_adj(mbuf, need);
	mbuf_end_set_cur(mbuf);

	RETURN_ERRNO_VALUE(0, "%d", need);
}


/* Ends */
