/** Daemon manager message handler
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
#include <lib/mqueue.h>
#include <lib/util.h>

#include <common/default.h>
#include <common/control/local.h>
#include <common/vita49/types.h>
#include <common/vita49/common.h>
#include <common/vita49/command.h>

#include <daemon/resource.h>
#include <daemon/worker.h>
#include <daemon/control.h>
#include <daemon/manager.h>
#include <daemon/message.h>


LOG_MODULE_IMPORT(daemon_manager_module_level);
#define module_level daemon_manager_module_level


static unsigned sid_assign;


void daemon_manager_command_recv (struct v49_common *req_v49, struct message *req_user,
                                  struct worker *worker)
{
	struct v49_common     rsp_v49;
	struct resource_info *res;
	struct growlist       list = GROWLIST_INIT;
	struct mbuf          *mbuf;
	uuid_t               *rid;
	int                   ret;

	ENTER("req_v49 %p, worker %p", req_v49, worker);

	LOG_DEBUG("%s: got message for worker %s:\n", __func__, worker_name(worker));
	v49_dump(LOG_LEVEL_DEBUG, req_v49);

	if ( !(mbuf = mbuf_alloc(DEFAULT_MBUF_SIZE, sizeof(struct message))) )
	{
		LOG_ERROR("mbuf_alloc() failed: %s\n", strerror(errno));
		RETURN;
	}
	mbuf_beg_set(mbuf, DEFAULT_MBUF_HEAD);
	memcpy(mbuf_user(mbuf), req_user, sizeof(*req_user));

	v49_reset(&rsp_v49);
	rsp_v49.type = V49_TYPE_COMMAND;
	rsp_v49.command.request = req_v49->command.request;

	if ( req_v49->command.indicator & (1 << V49_CMD_IND_BIT_CID) )
	{
		LOG_DEBUG("Proxy CID from req to rsp: %s\n", uuid_to_str(&req_v49->command.cid));
		rsp_v49.command.indicator |= 1 << V49_CMD_IND_BIT_CID;
		memcpy(&rsp_v49.command.cid, &req_v49->command.cid, sizeof(uuid_t));
	}

	switch ( req_v49->command.request )
	{
		case V49_CMD_REQ_DISCO:
			rsp_v49.command.role     = V49_CMD_ROLE_RESULT;
			rsp_v49.command.result   = V49_CMD_RES_SUCCESS;
			if ( req_v49->command.rid_list )
			{
				ret = growlist_intersect(&list, &resource_list,
				                         req_v49->command.rid_list, uuid_cmp);
				if ( ret < 0 )
					LOG_ERROR("growlist_intersect() failed\n");
				rsp_v49.command.rid_list = &list;

				LOG_DEBUG("Resource list %d, filter list %d entries: result list %d entries:\n",
				          growlist_used(&resource_list),
				          growlist_used(req_v49->command.rid_list),
				          growlist_used(&list));
				growlist_reset(&list);
				while ( (rid = growlist_next(&list)) )
					LOG_DEBUG("  %s\n", uuid_to_str(rid));
			}
			else
				rsp_v49.command.rid_list = &resource_list;
			rsp_v49.command.indicator |= (1 << V49_CMD_IND_BIT_RID_LIST);
			break;

		case V49_CMD_REQ_ENUM:
			rsp_v49.command.role     = V49_CMD_ROLE_RESULT;
			rsp_v49.command.result   = V49_CMD_RES_SUCCESS;
			if ( req_v49->command.rid_list )
			{
				ret = growlist_intersect(&list, &resource_list,
				                         req_v49->command.rid_list, uuid_cmp);
				if ( ret < 0 )
					LOG_ERROR("growlist_intersect() failed\n");
				rsp_v49.command.res_info = &list;

				LOG_DEBUG("Resource list %d, filter list %d entries: result list %d entries:\n",
				          growlist_used(&resource_list),
				          growlist_used(req_v49->command.rid_list),
				          growlist_used(&list));
				growlist_reset(&list);
				while ( (rid = growlist_next(&list)) )
					LOG_DEBUG("  %s\n", uuid_to_str(rid));
			}
			else
				rsp_v49.command.res_info = &resource_list;
			rsp_v49.command.indicator |= (1 << V49_CMD_IND_BIT_RES_INFO);
			break;

		case V49_CMD_REQ_ACCESS:
			// duplicate request: return a duplicate sid_assign
			if ( worker )
			{
				LOG_WARN("ACCESS: dup req for worker %s: re-assign SID %u\n",
				         worker_name(worker), worker->sid);
				rsp_v49.command.role       = V49_CMD_ROLE_RESULT;
				rsp_v49.command.result     = V49_CMD_RES_SUCCESS;
				rsp_v49.command.indicator |= (1 << V49_CMD_IND_BIT_SID_ASSIGN);
				rsp_v49.command.sid_assign = worker->sid;
				break;
			}

			// client must specify exactly one RID
			if ( !req_v49->command.rid_list ||
			     growlist_used(req_v49->command.rid_list) != 1 )
			{
				LOG_ERROR("ACCESS: client must specify exactly 1 RID\n");
				rsp_v49.command.role   = V49_CMD_ROLE_RESULT;
				rsp_v49.command.result = V49_CMD_RES_INVAL;
				break;
			}
			growlist_reset(req_v49->command.rid_list);
			rid = growlist_next(req_v49->command.rid_list);
			LOG_DEBUG("ACCESS: req for RID %s\n", uuid_to_str(rid));

			// check requested RID exists
			growlist_reset(&resource_list);
			if ( !(res = growlist_search(&resource_list, uuid_cmp, rid)) )
			{
				LOG_ERROR("ACCESS: client RID %s not found: %s\n",
				          uuid_to_str(rid), strerror(errno));
				rsp_v49.command.role   = V49_CMD_ROLE_RESULT;
				rsp_v49.command.result = V49_CMD_RES_NOENT;
				break;
			}
			LOG_DEBUG("ACCESS: req for RID %s:\n", uuid_to_str(rid));
			resource_dump(LOG_LEVEL_DEBUG, "Resource", res);

			// can't assign the reserved value; may need a smaller max here
			if ( sid_assign == V49_CMD_RSVD_SID )
				sid_assign++;

			// create the new worker struct
			if ( !(worker = worker_alloc(DEF_WORKER_CLASS, sid_assign++)) )
			{
				LOG_ERROR("worker_alloc() failed: %s\n", strerror(errno));
				rsp_v49.command.role   = V49_CMD_ROLE_RESULT;
				rsp_v49.command.result = V49_CMD_RES_ALLOC;
				break;
			}

			worker->control = req_user->control;
			worker->socket  = req_user->socket;
			worker->name    = str_dup_sprintf("SID:%u", worker->sid);
			worker->res     = res;
			memcpy(&worker->rid, rid, sizeof(worker->rid));
			if ( req_v49->command.indicator & (1 << V49_CMD_IND_BIT_CID) )
				memcpy(&worker->cid, &req_v49->command.cid, sizeof(worker->cid));
			else
				memset(&worker->cid, 0, sizeof(worker->cid));

			// return success and assign SID
			rsp_v49.command.role       = V49_CMD_ROLE_RESULT;
			rsp_v49.command.result     = V49_CMD_RES_SUCCESS;
			rsp_v49.command.indicator |= (1 << V49_CMD_IND_BIT_SID_ASSIGN);
			rsp_v49.command.sid_assign = worker->sid;
			LOG_DEBUG("ACCESS: granted: RID %s SID %u\n", uuid_to_str(rid), worker->sid);
			break;


		// side-effect: mark the worker as shutting down, so we don't respawn it
		case V49_CMD_REQ_RELEASE:
			if ( !worker )
			{
				rsp_v49.command.role   = V49_CMD_ROLE_RESULT;
				rsp_v49.command.result = V49_CMD_RES_NOENT;
				LOG_ERROR("RELEASE: message for nonexistent worker?\n");
				break;
			}

			LOG_DEBUG("RELEASE: expecting a shutdown, disable restart\n");
			worker->flags &= ~(WF_RESTART_CLEAN|WF_RESTART_ERROR);
			mbuf_deref(mbuf);
			growlist_done(&list, NULL, NULL);
			RETURN_ERRNO(0);


		default:
			LOG_ERROR("Command not (yet) handled: %s\n",
			          v49_command_request(req_v49->command.request));
			mbuf_deref(mbuf);
			RETURN_ERRNO(EBADMSG);
	}

	LOG_DEBUG("%s: format reply:\n", __func__);
	v49_dump(LOG_LEVEL_DEBUG, &rsp_v49);

	if ( (ret = v49_format(&rsp_v49, mbuf, NULL)) != V49_OK_COMPLETE )
	{
		LOG_ERROR("v49_format: %s\n", v49_return_desc(ret));
		mbuf_deref(mbuf);
		growlist_done(&list, NULL, NULL);
		RETURN_ERRNO(0);
	}

	daemon_northbound(mbuf);
	growlist_done(&list, NULL, NULL);
	RETURN_ERRNO(0);
}

/* Ends    : src/daemon/worker/manager/command.c */
