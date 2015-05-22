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
#include <arpa/inet.h>

#include <lib/log.h>
#include <lib/mbuf.h>
#include <lib/mqueue.h>
#include <lib/growlist.h>

#include <common/vita49/common.h>
#include <common/vita49/command.h>
#include <common/vita49/context.h>
#include <common/vita49/control.h>


LOG_MODULE_STATIC("vita49_common", LOG_LEVEL_DEBUG);


/** Reset a command struct 
 */
void v49_reset (struct v49_common *v49)
{
	ENTER("v49 %p", v49);
	if ( !v49 )
		RETURN_ERRNO(EFAULT);

	memset(v49, 0, sizeof(struct v49_common));
	v49->type = V49_TYPE_MAX;

	RETURN_ERRNO(0);
}


#define FAIL(err) do{ loc = __LINE__; ret = (err); goto fail; }while(0)


/** Parse and validate the header of a VITA-49 packet
 * 
 *  \param  v49   v49_common struct to fill in
 *  \param  mbuf  Message buffer
 *
 *  \return V49_OK_COMPLETE on success, V49_ERR_* on validation failure, <0 on other
 *          errors
 */
int v49_common_parse (struct v49_common *v49, struct mbuf *mbuf)
{
	uint32_t  u32;
	int       len;
	int       loc = -1;
	int       ret = 0;

	ENTER("v49 %p, mbuf %p", v49, mbuf);
	if ( !v49 || !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	// Get overall packet length before starting, get first word without endian conversion
	// before checking for local tool magic
	len = mbuf_get_avail(mbuf);
	LOG_DEBUG("%d bytes available\n", len);
	if ( mbuf_get_n32(mbuf, &u32) != sizeof(u32) )
		FAIL(V49_ERR_SHORT);
	
	// Check for messages from control tool - rewind if it matches
	if ( u32 == V49_CTRL_MAGIC )
	{
		mbuf_cur_adj(mbuf, -4);
		RETURN_ERRNO_VALUE(0, "%d", V49_OK_CONTROL);
	}

	// Not the local control too: convert big-endian for V49 processing and continue
	v49->hdr = ntohl(u32);
	v49_dump_hdr(LOG_LEVEL_DEBUG, v49->hdr);

	switch ( v49->hdr & V49_HDR_PKT_TYPE )
	{
		case 0x40000000: v49->type = V49_TYPE_CONTEXT; break;
		case 0x50000000: v49->type = V49_TYPE_COMMAND; break;

		default:
			FAIL(V49_ERR_HEADER_TYPE);
	}
	if ( ! (v49->hdr & V49_HDR_HAS_CID) )
		FAIL(V49_ERR_HEADER_CID);

	LOG_DEBUG("hdr len %u, pkt len %d\n", ((v49->hdr & 0x0000FFFF) << 2), len);
	if ( ((v49->hdr & 0x0000FFFF) << 2) > len )
		FAIL(V49_ERR_HEADER_SIZE);

	// Stream ID
	if ( mbuf_get_be32(mbuf, &v49->sid) != sizeof(v49->sid) )
		FAIL(V49_ERR_SHORT);

	LOG_DEBUG("  sid %08x\n", v49->sid);

	// Class ID - OUI
	if ( mbuf_get_be32(mbuf, &u32) != sizeof(u32) )
		FAIL(V49_ERR_SHORT);

	LOG_DEBUG("  cid oui %08x\n", u32);

	if ( u32 != V49_SBT_OUI )
		FAIL(V49_ERR_CLASS_ID);

	// Class ID - Information Class Code & Packet Class Code
	if ( mbuf_get_be32(mbuf, &u32) != sizeof(u32) )
		FAIL(V49_ERR_SHORT);

	LOG_DEBUG("  cid xcc %08x\n", u32);

	if ( u32 != V49_SBT_XCC )
		FAIL(V49_ERR_CLASS_ID);

	// TSI - Integer Timestamp presence / interpretation
	if ( (v49->hdr & V49_HDR_TSI) )
	{
		if ( mbuf_get_be32(mbuf, &v49->ts_int) != sizeof(v49->ts_int) )
			FAIL(V49_ERR_SHORT);

		LOG_DEBUG("  ts_int  %u\n", v49->ts_int);
	}

	// TSF - Fractional Timestamp presence / interpretation
	switch ( v49->hdr & V49_HDR_TSF )
	{
		// No Fractional Timestamp
		case 0x00000000:
			break;

		// Sample Count Timestamp
		case 0x00100000:
			if ( mbuf_cur_adj(mbuf, sizeof(uint32_t)) < 0 )
				FAIL(V49_ERR_SHORT);

	     	if ( mbuf_get_be32(mbuf, &v49->ts_frac) != sizeof(v49->ts_frac) )
				FAIL(V49_ERR_SHORT);

			LOG_DEBUG("  ts_frac %u\n", v49->ts_frac);
			break;

		// Fractional timestamps not supported
		case 0x00200000:
		case 0x00300000:
			FAIL(V49_ERR_TSF_FRAC);
	}

	RETURN_ERRNO_VALUE(0, "%d", V49_OK_COMPLETE);

fail:
	LOG_ERROR("%s[%d]: %s:\n", __func__, loc, v49_return_desc(ret));
	mbuf_dump(mbuf);
	RETURN_VALUE("%d", ret);
}


/** Parse and validate a VITA-49 packet
 *
 *  Internally calls v49_common_parse() for the common header fields, and if that passes
 *  calls the correct parser for the subtype.
 * 
 *  \param  v49   v49_common struct to fill in
 *  \param  mbuf  Message buffer
 *
 *  \return V49_OK_COMPLETE or V49_OK_PAGING on success, V49_ERR_* on validation failure,
 *          <0 on other errors
 */
int v49_parse (struct v49_common *v49, struct mbuf *mbuf)
{
	int ret;

	ENTER("v49 %p, mbuf %p", v49, mbuf);
	if ( !v49 || !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	errno = 0;
	ret = v49_common_parse(v49, mbuf);
	if ( V49_RET_ERROR(ret) )
		RETURN_VALUE("%d", ret);
		
	// Common parsing done, go on to subtype
	errno = 0;
	switch ( v49->type )
	{
		case V49_TYPE_CONTEXT:
			ret = v49_context_parse(v49, mbuf);
			break;

		case V49_TYPE_COMMAND:
			ret = v49_command_parse(v49, mbuf);
			break;

		default:
			RETURN_VALUE("%d", V49_ERR_HEADER_TYPE);
	}

	RETURN_VALUE("%d", ret);
}




/** Format a command struct into VITA49 packets
 * 
 *  \note The caller should provide the MTU for the connection being used, which will be
 *        used to allocate mbufs and to paginate if necessary.
 */
int v49_format (struct v49_common *v49, struct mbuf *mbuf, struct mqueue *dest)
{
	struct mqueue  temp;
	uint32_t       hdr;
	size_t         max;
//	int            beg;
	int            len;
	int            page = 1;
	int            loc = -1;
	int            ret = V49_OK_COMPLETE;

	ENTER("v49 %p, mbuf %p, dest %p", v49, mbuf, dest);
	if ( !v49 || !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	// temporary: mbuf must not be enqueued
	if ( mbuf->ext )
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);
	
	// setup before first packet
	max = mbuf->max;
//	beg = mbuf->beg;
	mqueue_init(&temp, 0);

	// setup header field
	switch ( v49->type )
	{
		case V49_TYPE_CONTEXT:
			hdr = 0x40000000;
			break;

		case V49_TYPE_COMMAND:
			hdr = 0x50000000;
			v49->command.page = 0;
			break;

		default:
			FAIL(V49_ERR_HEADER_TYPE);
	}
	hdr |= V49_HDR_HAS_CID | V49_HDR_TSM;

	// TSI / TSF - Integer / Fractional Timestamp
	hdr |= v49->hdr & V49_HDR_TSI;
	switch ( v49->hdr & 0x00300000 )
	{
		// No Fractional Timestamp or Sample Count Timestamp: OK
		case 0x00000000:
		case 0x00100000:
			hdr |= v49->hdr & V49_HDR_TSF;
			break;

		// Fractional timestamps not supported
		case 0x00200000:
		case 0x00300000:
			FAIL(V49_ERR_TSF_FRAC);
	}

	// loop through packets as long as subtype returns _PAGING - always format the header
	// and fixed fields first, enqueue in a temp queue, and alloc the next 
	do
	{
		// mbuf re/set
		mbuf_cur_set_beg(mbuf);
		mbuf_end_set_beg(mbuf);

		// Common header fields: header is left to last so skip 4
		hdr &= ~V49_HDR_PKT_COUNT;
		hdr |= (v49->count++ << 16) & V49_HDR_PKT_COUNT;
		if ( mbuf_cur_adj(mbuf, sizeof(uint32_t)) < 0 )
			FAIL(V49_ERR_SPACE);

		// Stream ID
		if ( mbuf_set_be32(mbuf, v49->sid) < sizeof(uint32_t) )
			FAIL(V49_ERR_SPACE);

		// Class ID - OUI
		if ( mbuf_set_be32(mbuf, V49_SBT_OUI) < sizeof(uint32_t) )
			FAIL(V49_ERR_SPACE);

		// Class ID - Information Class Code & Packet Class Code
		if ( mbuf_set_be32(mbuf, V49_SBT_XCC) < sizeof(uint32_t) )
			FAIL(V49_ERR_SPACE);

		// TSI - Integer Timestamp presence / interpretation
		if ( (hdr & V49_HDR_TSI) && mbuf_set_be32(mbuf, v49->ts_int) < sizeof(uint32_t) )
			FAIL(V49_ERR_SPACE);

		// TSF - Fractional Timestamp presence / interpretation
		if ( hdr & V49_HDR_TSF )
		{
			if ( mbuf_set_be32(mbuf, 0) < sizeof(uint32_t) )
				FAIL(V49_ERR_SPACE);

			if ( mbuf_set_be32(mbuf, v49->ts_frac) < sizeof(uint32_t) )
				FAIL(V49_ERR_SPACE);
		}

		// Common formatting done, go on to subtype.  If it failed, abort
		errno = 0;
		switch ( v49->type )
		{
			case V49_TYPE_CONTEXT: ret = v49_context_format(v49, mbuf); break;
			case V49_TYPE_COMMAND: ret = v49_command_format(v49, mbuf); break;
			default:
				FAIL(V49_ERR_HEADER_TYPE);
		}
		if ( V49_RET_ERROR(ret) )
			FAIL(ret);

		// Pagination not supported currently - allocating new mbuf in here means passing
		// in the correct sizeof the user section, which differs between daemon and worker
		if ( ret == V49_OK_PAGING )
			FAIL(V49_ERR_PAGING);

		if ( (len = mbuf_length(mbuf)) < 0 )
			FAIL(V49_ERR_SPACE);
		if ( (len & 0x03) || len >= max )
			FAIL(V49_ERR_WORD_LEN);

		// Seek back and write the header length now that we know it
		hdr |= len >> 2;
		mbuf_cur_set_beg(mbuf);
		if ( mbuf_set_be32(mbuf, hdr) < sizeof(uint32_t) )
			FAIL(V49_ERR_SPACE);

		// enqueue in temporary queue 
		mqueue_enqueue(&temp, mbuf);

#if 0
		// if paginating, alloc new one and loop
		if ( ret == V49_OK_PAGING )
		{
			if ( !(mbuf = mbuf_alloc(max, sizeof(struct message))) )
				FAIL(V49_ERR_MALLOC);

			page++;
			mbuf_beg_set(mbuf, beg);
		}
#endif
	}
	while ( ret == V49_OK_PAGING );

	// if paginated, the number of pages should match the queue length
	if ( page > 1 && page != mqueue_used(&temp) )
		FAIL(V49_ERR_PAGING);

	// if paginated, we need dest set. otherwise it's optional
	if ( page > 1 && !dest )
		FAIL(V49_ERR_PAGING);

	// transfer from temp queue to destination
	if ( !dest )
		mqueue_dequeue(&temp);
	else
		while ( (mbuf = mqueue_dequeue(&temp)) )
			if ( mqueue_enqueue(dest, mbuf) < 0 )
				FAIL(V49_ERR_ENQUEUE);

	RETURN_ERRNO_VALUE(0, "%d", V49_OK_COMPLETE);

fail:
	LOG_ERROR("%s[%d]: %s\n", __func__, loc, v49_return_desc(ret));

	if ( mbuf )
		mbuf_deref(mbuf);
	while ( (mbuf = mqueue_dequeue(&temp)) )
		mbuf_deref(mbuf);

	RETURN_VALUE("%d", ret);
}


const char *v49_type (int type)
{
	switch ( type )
	{
		case V49_TYPE_COMMAND: return "COMMAND";
		case V49_TYPE_CONTEXT: return "CONTEXT";
	}

	static char buff[32];
	snprintf(buff, sizeof(buff), "Unknown(0x%08x)", type);
	return buff;
}

/** Return a descriptive string for a return value
 *
 *  \param err Return value from v49_parse or v49_format
 *
 *  \return Constant string suitable for error messages
 */
const char *v49_return_desc (int err)
{
	switch ( V49_RET_ERROR(err) )
	{
		case V49_RET_SUCCESS:
			switch ( err )
			{
				case V49_OK_COMPLETE: return "Completed message";
				case V49_OK_PAGING:   return "Completed page, call with next";
			}
			break;

		case V49_RET_COMMON:
			switch ( err )
			{
				case V49_ERR_MALLOC:      return "Memory allocation";
				case V49_ERR_SHORT:       return "Packet too short";
				case V49_ERR_SPACE:       return "Out of space";
				case V49_ERR_HEADER_TYPE: return "Packet header: bad type";
				case V49_ERR_HEADER_CID:  return "Packet header: bad CID field";
				case V49_ERR_HEADER_SIZE: return "Packet header: bad size";
				case V49_ERR_CLASS_ID:    return "Class ID: bad values";
				case V49_ERR_TSF_FRAC:    return "Fractional TSF: bad type";
			}
			break;

		case V49_RET_COMMAND:
			return v49_command_return_desc(err);

		case V49_RET_CONTEXT:
			return v49_context_return_desc(err);
	}

	static char buff[32];
	snprintf(buff, sizeof(buff), "Unknown(0x%08x)", err);
	return buff;
}



void v49_dump_hdr (int level, uint32_t hdr)
{
	LOG_MESSAGE(level, "hdr %08x { type:%u C:%u TSM:%u TSI:%u TSF:%u count:%u size:%u }\n",
	            hdr,
	            (hdr & V49_HDR_PKT_TYPE)  >> 28,
	            (hdr & V49_HDR_HAS_CID)   >> 27,
	            (hdr & V49_HDR_TSM)       >> 26,
	            (hdr & V49_HDR_TSI)       >> 22,
	            (hdr & V49_HDR_TSF)       >> 20,
	            (hdr & V49_HDR_PKT_COUNT) >> 16,
	            (hdr & V49_HDR_PKT_SIZE));
}

void v49_dump (int level, struct v49_common *v49)
{
	v49_dump_hdr(level, v49->hdr);
	LOG_MESSAGE(level, "sid %08x\n", v49->sid);

	LOG_MESSAGE(level, "ts_int %u\n", v49->ts_int);
	LOG_MESSAGE(level, "ts_int %u\n", v49->ts_frac);

	LOG_MESSAGE(level, "count %u\n", v49->count);

	LOG_MESSAGE(level, "type %s\n", v49_type(v49->type));
	switch ( v49->type )
	{
		case V49_TYPE_CONTEXT:
			v49_context_dump(level, &v49->context);
			break;

		case V49_TYPE_COMMAND:
			v49_command_dump(level, &v49->command);
			break;

		default:
			break;
	}
}


/* Ends */
