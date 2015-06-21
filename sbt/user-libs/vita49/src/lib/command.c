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
#include <string.h>

#include <sbt_common/log.h>
#include <sbt_common/mbuf.h>
#include <sbt_common/uuid.h>
#include <sbt_common/mqueue.h>
#include <sbt_common/growlist.h>

#include <vita49/resource.h>
#include <vita49/types.h>
#include <vita49/common.h>
#include <vita49/command.h>


LOG_MODULE_STATIC("vita49_command", LOG_LEVEL_INFO);


/******* *******/


/** Reset a command struct 
 */
void v49_command_reset (struct v49_command *cmd)
{
	ENTER("cmd %p", cmd);
	if ( !cmd )
		RETURN_ERRNO(EFAULT);

	cmd->role      = V49_CMD_ROLE_MAX;
	cmd->request   = V49_CMD_RES_MAX;
	cmd->result    = V49_CMD_REQ_MAX;
	cmd->indicator = 0;

	memset(&cmd->cid, 0, sizeof(cmd->cid));

	cmd->priority = V49_CMD_DEF_PRIORITY;

	cmd->rid_list = NULL;
	cmd->res_info = NULL;

	cmd->sid_assign = 0;

	cmd->tstamp_int = 0;

	RETURN_ERRNO(0);
}


#define FAIL(err) do{ loc = __LINE__; ret = (err); goto fail; }while(0)


/** Parse a VITA49 packet into the command struct
 * 
 *  \note The data is merged into the command struct, to handle paginated messages.  The
 *        caller is responsible for resetting the struct before the first packet
 */
int v49_command_parse (struct v49_common *v49, struct mbuf *mbuf)
{
	struct v49_command *cmd = &v49->command;;
	uint32_t            u32;
	uint32_t            len;
	uint32_t            paging = 0x00010001;
//	uint64_t            u64;
	int                 loc = -1;
	int                 ret = V49_OK_COMPLETE;

	ENTER("v49 %p, mbuf %p", v49, mbuf);
	if ( !v49 || !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

LOG_DEBUG("%s: start avail %d\n", __func__, mbuf_get_avail(mbuf));

	// Request/Response Field
	if ( mbuf_get_be32(mbuf, &u32) != sizeof(u32) )
		FAIL(V49_ERR_SHORT);

	cmd->role    = (u32 & 0xC0000000) >> 30;
	cmd->result  = (u32 & 0x00FF0000) >> 16;
	cmd->request = (u32 & 0x000000FF);
	LOG_DEBUG("  req/rsp 0x%08x: role %s, request %s, response %s\n", u32,
	          v49_command_role(cmd->role),
	          v49_command_request(cmd->request),
	          v49_command_result(cmd->result));

	if ( cmd->role    >= V49_CMD_ROLE_MAX ) FAIL(V49_ERR_CMD_RANGE);
	if ( cmd->result  >= V49_CMD_RES_MAX  ) FAIL(V49_ERR_CMD_RANGE);
	if ( cmd->request >= V49_CMD_REQ_MAX  ) FAIL(V49_ERR_CMD_RANGE);

	// Parameter Indicator
	if ( mbuf_get_be32(mbuf, &cmd->indicator) != sizeof(cmd->indicator) )
		FAIL(V49_ERR_SHORT);

	LOG_DEBUG("  indicator %s\n", v49_command_indicator(cmd->indicator));

	// Check indicator for Message Paging
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_PAGING) )
	{
		if ( mbuf_get_be32(mbuf, &paging) != sizeof(paging) )
			FAIL(V49_ERR_SHORT);

		LOG_DEBUG("  page %d/%d\n", paging & 0xFFFF, paging >> 16);

		// not the last page yet: success value should indicate more expected
		if ( (paging & 0xFFFF) < (paging >> 16) )
			ret = V49_ERR_PAGING;
	}

	// Check indicator for Client Identifier
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_CID) )
	{
		if ( mbuf_get_mem(mbuf, &cmd->cid, sizeof(cmd->cid)) != sizeof(cmd->cid) )
			FAIL(V49_ERR_SHORT);

		LOG_DEBUG("  cid %s\n", uuid_to_str(&cmd->cid));
	}

	// Check indicator for Request Priority
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_PRIORITY) )
	{
		if ( mbuf_get_be32(mbuf, &cmd->priority) != sizeof(cmd->priority) )
			FAIL(V49_ERR_SHORT);

		LOG_DEBUG("  priority 0x%08x\n", cmd->priority);
	}

	// Check indicator for Resource Identifier List
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_RID_LIST) )
	{
		uuid_t *rid;

		if ( !cmd->rid_list && !(cmd->rid_list = growlist_alloc(4, 4)) )
			FAIL(V49_ERR_MALLOC);

		// get list length in words and convert to bytes
		if ( mbuf_get_be32(mbuf, &len) != sizeof(len) )
			FAIL(V49_ERR_SHORT);

		// check against size of UUIDs (4 words) and that enough words in message
		len <<= 2;
		if ( (len & 0x0000000F) || mbuf_get_avail(mbuf) < len )
			FAIL(V49_ERR_LIST_SIZE);

		// read UUIDs and merge into list
		LOG_DEBUG("  rid_list %u UUIDs\n", len >> 4);
		for ( ; len; len -= sizeof(*rid) )
		{
			if ( !(rid = malloc(sizeof(*rid))) )
				FAIL(V49_ERR_MALLOC);

			if ( mbuf_get_mem(mbuf, rid, sizeof(*rid)) != sizeof(*rid) )
			{
				free(rid);
				FAIL(V49_ERR_SHORT);
			}

			if ( growlist_search(cmd->rid_list, uuid_cmp, rid) )
			{
				free(rid);
				continue;
			}

			if ( growlist_append(cmd->rid_list, rid) < 0 )
			{
				free(rid);
				FAIL(V49_ERR_MALLOC);
			}

			LOG_DEBUG("  rid %s\n", uuid_to_str(rid));
		}
	}

	// Check indicator for Resource Information List
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_RES_INFO) )
	{
		struct resource_info  *res;

		if ( !cmd->res_info && !(cmd->res_info = growlist_alloc(4, 4)) )
			FAIL(V49_ERR_MALLOC);

		// get list length in words and convert to bytes
		if ( mbuf_get_be32(mbuf, &len) != sizeof(len) )
			FAIL(V49_ERR_SHORT);

		// check against size of record (12 words) and that enough words in message
		len <<= 2;
		if ( (len % sizeof(*res)) || mbuf_get_avail(mbuf) < len )
			FAIL(V49_ERR_LIST_SIZE);

		// read UUIDs and merge into list
		LOG_DEBUG("  res_info %u records\n", len / sizeof(uuid_t));
		for ( ; len; len -= sizeof(*res) )
		{
			if ( !(res = calloc(1, sizeof(*res))) )
				FAIL(V49_ERR_MALLOC);

			if ( mbuf_get_mem(mbuf, &res->uuid, sizeof(res->uuid)) != sizeof(res->uuid) )
			{
				free(res);
				FAIL(V49_ERR_SHORT);
			}
			if ( mbuf_get_mem(mbuf, &res->name, 16) < 16 )
			{
				free(res);
				FAIL(V49_ERR_SHORT);
			}
			if ( mbuf_get_be32(mbuf, &u32) != sizeof(u32) )
			{
				free(res);
				FAIL(V49_ERR_SHORT);
			}
			res->txch    = (u32 & 0xF0000000) >> 28;
			res->rxch    = (u32 & 0x0F000000) >> 24;
			res->rate_q8 = (u32 & 0x00FFFFFF);

			if ( mbuf_get_be32(mbuf, &u32) != sizeof(u32) )
			{
				free(res);
				FAIL(V49_ERR_SHORT);
			}
			res->min = (u32 & 0xFFFF0000) >> 16;
			res->max = (u32 & 0x0000FFFF);

			// TBD: two words for future expansion
			if ( mbuf_cur_adj(mbuf, sizeof(uint32_t) * 2) < 0 )
			{
				free(res);
				FAIL(V49_ERR_SHORT);
			}

			if ( growlist_search(cmd->res_info, uuid_cmp, res) )
			{
				free(res);
				continue;
			}

			if ( growlist_append(cmd->res_info, res) < 0 )
			{
				free(res);
				FAIL(V49_ERR_MALLOC);
			}

			if ( module_level >= LOG_LEVEL_DEBUG )
				resource_dump(LOG_LEVEL_DEBUG, "  res ", res);
		}
	}

	// Check indicator for Stream Identifier Assignment
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_SID_ASSIGN) )
	{
		if ( mbuf_get_be32(mbuf, &cmd->sid_assign) != sizeof(cmd->sid_assign) )
			FAIL(V49_ERR_SHORT);

		LOG_DEBUG("  sid_assign 0x%08x\n", cmd->sid_assign);
	}

	// Check indicator for Timestamp Interpretation
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_TSTAMP_INT) )
	{
		if ( mbuf_get_be32(mbuf, &u32) != sizeof(u32) )
			FAIL(V49_ERR_SHORT);

		LOG_DEBUG("  tstamp_int %s\n", v49_command_tstamp_int(u32));
		switch ( u32 )
		{
			case TSTAMP_INT_IMMEDIATE:
			case TSTAMP_INT_ABSOLUTE:
			case TSTAMP_INT_RELATIVE:
				cmd->tstamp_int = u32;
				break;

			default:
				FAIL(V49_ERR_TSTAMP_INT);
		}
	}

	RETURN_ERRNO_VALUE(0, "%d", ret);

fail:
	LOG_ERROR("%s[%d]: %s\n", __func__, loc, v49_return_desc(ret));
	mbuf_dump(mbuf);
	return ret;
}


/** Format a v49_command struct into VITA49 packets
 * 
 *  \note The caller should provide the MTU for the connection being used, which will be
 *        used to allocate mbufs and to paginate if necessary.
 */
int v49_command_format (struct v49_common *v49, struct mbuf *mbuf)
{
	struct v49_command *cmd = &v49->command;;
	uint32_t            u32;
	uint32_t            paging = 0x00010001;
	int                 loc = -1;
	int                 ret = V49_OK_COMPLETE;

	ENTER("v49 %p, mbuf %p", v49, mbuf);
	if ( !v49 || !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	// Request/Response Field
	u32  = (cmd->role    << 30) & 0xC0000000;
	u32 |= (cmd->result  << 16) & 0x00FF0000;
	u32 |=  cmd->request        & 0x000000FF;
	if ( mbuf_set_be32(mbuf, u32) != sizeof(u32) )
		FAIL(V49_ERR_SPACE);

	// Mute unsupported indicators and write - pagination not implemented yet
	if ( cmd->indicator & ~V49_COMMAND_IND_BIT_MASK )
	{
		LOG_DEBUG("%s: muted indicators: %s\n", __func__,
		          v49_command_indicator(cmd->indicator & ~V49_COMMAND_IND_BIT_MASK));
		cmd->indicator &= V49_COMMAND_IND_BIT_MASK;
	}
	cmd->indicator &= ~(1 << V49_CMD_IND_BIT_PAGING);

	LOG_DEBUG("  indicator %s\n", v49_command_indicator(cmd->indicator));
	if ( mbuf_set_be32(mbuf, cmd->indicator) != sizeof(cmd->indicator) )
		FAIL(V49_ERR_SPACE);

	// Check indicator for Message Paging
	if ( !(cmd->indicator & (1 << V49_CMD_IND_BIT_PAGING)) )
		LOG_DEBUG("  skip Paging\n");
	else if ( mbuf_set_be32(mbuf, paging) != sizeof(paging) )
		FAIL(V49_ERR_SPACE);
	else
		LOG_DEBUG("  write paging %08x\n", paging);

	// Check indicator for Client Identifier
	if ( !(cmd->indicator & (1 << V49_CMD_IND_BIT_CID)) )
		LOG_DEBUG("  skip CID\n");
	else if ( mbuf_set_mem(mbuf, &cmd->cid, sizeof(uuid_t)) != sizeof(uuid_t) )
		FAIL(V49_ERR_SPACE);
	else
		LOG_DEBUG("  write CID %s\n", uuid_to_str(&cmd->cid));

	// Check indicator for Request Priority
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_PRIORITY) )
		if ( mbuf_set_be32(mbuf, cmd->priority) != sizeof(cmd->priority) )
			FAIL(V49_ERR_SPACE);

	// Check indicator for Resource Identifier List
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_RID_LIST) )
	{
		uuid_t *rid;

		// length of list is required if bit set, use 0 if NULL or empty
		u32  = cmd->rid_list ? growlist_used(cmd->rid_list) : 0;
		u32 *= sizeof(uuid_t) / sizeof(uint32_t);
		if ( mbuf_set_be32(mbuf, u32) != sizeof(u32) )
			FAIL(V49_ERR_SPACE);

		// write list entries if applicable
		if ( u32 )
		{
			growlist_reset(cmd->rid_list);
			while ( (rid = growlist_next(cmd->rid_list)) )
			{
				LOG_DEBUG("  rid %s\n", uuid_to_str(rid));
				if ( mbuf_set_mem(mbuf, rid, sizeof(uuid_t)) != sizeof(uuid_t) )
					FAIL(V49_ERR_SPACE);
			}
		}
	}

	// Check indicator for Resource Information List
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_RES_INFO) )
	{
		struct resource_info  *res;

		// length of list is required if bit set, use 0 if NULL or empty
		u32  = cmd->res_info ? growlist_used(cmd->res_info) : 0;
		u32 *= V49_SIZEOF_RESOURCE_INFO / sizeof(uint32_t);
		if ( mbuf_set_be32(mbuf, u32) != sizeof(u32) )
			FAIL(V49_ERR_SPACE);

		// write list entries if applicable
		if ( u32 )
		{
			growlist_reset(cmd->res_info);
			while ( (res = growlist_next(cmd->res_info)) )
			{
				if ( module_level >= LOG_LEVEL_DEBUG )
					resource_dump(LOG_LEVEL_DEBUG, "  res ", res);

				// Resource Identifier
				if ( mbuf_set_mem(mbuf, &res->uuid, sizeof(res->uuid)) != sizeof(res->uuid) )
					FAIL(V49_ERR_SPACE);

				// Resource Name
				if ( mbuf_set_mem(mbuf, &res->name, 16) < 16 )
					FAIL(V49_ERR_SPACE);

				// Resource Capabilities: TX Ch, RX Ch, Maximum Sample Rate 
				u32  = (res->txch << 28) & 0xF0000000;
				u32 |= (res->rxch << 24) & 0x0F000000;
				u32 |=  res->rate_q8     & 0x00FFFFFF;
				if ( mbuf_set_be32(mbuf, u32) != sizeof(u32) )
					FAIL(V49_ERR_SPACE);

				// Resource Capabilities: Minimum Frequency, Maximum Frequency
				u32  = (res->min << 16) & 0xFFFF0000;
				u32 |=  res->max        & 0x0000FFFF;
				if ( mbuf_set_be32(mbuf, u32) != sizeof(u32) )
					FAIL(V49_ERR_SPACE);

				// TBD: two words for future expansion
				if ( mbuf_set_be32(mbuf, 0) != sizeof(uint32_t) )
					FAIL(V49_ERR_SPACE);
				if ( mbuf_set_be32(mbuf, 0) != sizeof(uint32_t) )
					FAIL(V49_ERR_SPACE);
			}
		}
	}

	// Check indicator for Stream Identifier Assignment
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_SID_ASSIGN) )
	{
		LOG_DEBUG("  sid_assign 0x%08x\n", cmd->sid_assign);
		if ( mbuf_set_be32(mbuf, cmd->sid_assign) != sizeof(cmd->sid_assign) )
			FAIL(V49_ERR_SPACE);
	}

	// Check indicator for Timestamp Interpretation
	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_TSTAMP_INT) )
	{
		u32 = cmd->tstamp_int;
		LOG_DEBUG("  tstamp_int 0x%08x\n", u32);
		if ( mbuf_set_be32(mbuf, u32) != sizeof(u32) )
			FAIL(V49_ERR_SPACE);
	}

	RETURN_ERRNO_VALUE(0, "%d", ret);

fail:
	LOG_ERROR("%s[%d]: %s\n", __func__, loc, v49_return_desc(ret));
	return ret;
}



/******* Debug / descript code *******/

const char *v49_command_role (int role)
{
	switch ( role )
	{
		case V49_CMD_ROLE_REQUEST:  return "REQUEST";
		case V49_CMD_ROLE_RESULT:   return "RESULT";
		case V49_CMD_ROLE_NOTIFY:   return "NOTIFY";
		case V49_CMD_ROLE_INTERNAL: return "INTERNAL";
	}

	static char  buff[32];
	snprintf(buff, sizeof(buff), "Unknown:%d", role);
	return buff;
}
const char *v49_command_result (int result)
{
	switch ( result )
	{
		case V49_CMD_RES_SUCCESS: return "SUCCESS";
		case V49_CMD_RES_UNSPEC:  return "UNSPEC";
		case V49_CMD_RES_INVAL:   return "INVAL";
		case V49_CMD_RES_NOENT:   return "NOENT"; 
		case V49_CMD_RES_ALLOC:   return "ALLOC"; 
	}

	static char  buff[32];
	snprintf(buff, sizeof(buff), "Unknown:%d", result);
	return buff;
}
const char *v49_command_request (int request)
{
	switch ( request )
	{
		case V49_CMD_REQ_DISCO:   return "DISCO";
		case V49_CMD_REQ_ENUM:    return "ENUM";
		case V49_CMD_REQ_ACCESS:  return "ACCESS";
		case V49_CMD_REQ_OPEN:    return "OPEN";
		case V49_CMD_REQ_CONFIG:  return "CONFIG";
		case V49_CMD_REQ_START:   return "START";
		case V49_CMD_REQ_STOP:    return "STOP";
		case V49_CMD_REQ_CLOSE:   return "CLOSE";
		case V49_CMD_REQ_RELEASE: return "RELEASE";
	}

	static char  buff[64];
	snprintf(buff, sizeof(buff), "Unknown:%d", request);
	return buff;
}
const char *v49_command_indicator (int ind)
{
	static char  buff[256];

	snprintf(buff, sizeof(buff), "0x%08x: { %s%s%s%s%s%s%s}", ind,
	          ind & (1 << V49_CMD_IND_BIT_PAGING    ) ? "PAGING "     : "",
	          ind & (1 << V49_CMD_IND_BIT_CID       ) ? "CID "        : "",
	          ind & (1 << V49_CMD_IND_BIT_PRIORITY  ) ? "PRIORITY "   : "",
	          ind & (1 << V49_CMD_IND_BIT_RID_LIST  ) ? "RID_LIST "   : "",
	          ind & (1 << V49_CMD_IND_BIT_RES_INFO  ) ? "RES_INFO "   : "",
	          ind & (1 << V49_CMD_IND_BIT_SID_ASSIGN) ? "SID_ASSIGN " : "",
	          ind & (1 << V49_CMD_IND_BIT_TSTAMP_INT) ? "TSTAMP_INT " : "");

	return buff;
}
const char *v49_command_tstamp_int (int tstamp_int)
{
	switch ( tstamp_int )
	{
		case TSTAMP_INT_IMMEDIATE: return "Immediate";
		case TSTAMP_INT_ABSOLUTE:  return "Absolute";
		case TSTAMP_INT_RELATIVE:  return "Relative";
	}

	static char  buff[64];
	snprintf(buff, sizeof(buff), "Unknown:%d", tstamp_int);
	return buff;
}

const char *v49_command_return_desc (int err)
{
	switch ( err )
	{
		case V49_ERR_CMD_RANGE:  return "Request/Response field: bad range";
		case V49_ERR_LIST_SIZE:  return "Packet too short for list";
		case V49_ERR_TSTAMP_INT: return "Timestamp Interp: bad type";
	}

	static char buff[32];
	snprintf(buff, sizeof(buff), "Unknown:0x%08x", err);
	return buff;
}

void v49_command_dump (int level, struct v49_command *cmd)
{
	LOG_MESSAGE(level, "command:\n");
	LOG_MESSAGE(level, "role      : %s\n",     v49_command_role(cmd->role));
	LOG_MESSAGE(level, "request   : %s\n",     v49_command_request(cmd->request));
	LOG_MESSAGE(level, "result    : %s\n",     v49_command_result(cmd->result));
	LOG_MESSAGE(level, "indicator : %s\n",     v49_command_indicator(cmd->indicator));

	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_PAGING) )
		LOG_MESSAGE(level, "page      : 0x%08x\n", cmd->page);

	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_CID) )
		LOG_MESSAGE(level, "cid       : %s\n", uuid_to_str(&cmd->cid));

	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_PRIORITY) )
		LOG_MESSAGE(level, "priority  : %u.%5u\n", cmd->priority >> 16,
		            (100000 * (cmd->priority & 0xFFFF)) >> 16);

	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_RID_LIST) )
	{
		if ( !cmd->rid_list )
			LOG_MESSAGE(level, "rid_list  : NULL\n");
		else
		{
			uuid_t *rid;

			LOG_MESSAGE(level, "rid_list  : %zu:\n", growlist_used(cmd->rid_list));
			growlist_reset(cmd->rid_list);
			while ( (rid = growlist_next(cmd->rid_list)) )
				LOG_MESSAGE(level, "  %s\n", uuid_to_str(rid));
		}
	}

	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_RES_INFO) )
	{
		if ( !cmd->res_info )
			LOG_MESSAGE(level, "res_info  : NULL\n");
		else
		{
			struct resource_info  *res;

			LOG_MESSAGE(level, "res_info  : %zu:\n", growlist_used(cmd->res_info));
			growlist_reset(cmd->res_info);
			while ( (res = growlist_next(cmd->res_info)) )
				resource_dump(level, "  ", res);
		}
	}

	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_SID_ASSIGN) )
		LOG_MESSAGE(level, "sid_assign: 0x%08x\n", cmd->sid_assign);

	if ( cmd->indicator & (1 << V49_CMD_IND_BIT_TSTAMP_INT) )
		LOG_MESSAGE(level, "tstamp_int: %s\n", v49_command_tstamp_int(cmd->tstamp_int));
}


/* Ends */
