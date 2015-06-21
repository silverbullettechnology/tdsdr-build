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
#ifndef INCLUDE_COMMON_VITA49_COMMON_H
#define INCLUDE_COMMON_VITA49_COMMON_H
#include <stdint.h>

#include <sbt_common/mbuf.h>
#include <sbt_common/mqueue.h>
#include <sbt_common/growlist.h>

#include <vita49/types.h>
#include <vita49/command.h>
#include <vita49/context.h>


// Class Identifier values
#define V49_SBT_OUI  0x00112233  // IEEE Organizationally Unique Identifier (TBD)
#define V49_SBT_XCC  0x00010001  // Information Class Code & Packet Class Code

// Packet Header fields
#define V49_HDR_PKT_TYPE     0xF0000000
#define V49_HDR_HAS_CID      0x08000000
#define V49_HDR_TSM          0x01000000
#define V49_HDR_TSI          0x00C00000
#define V49_HDR_TSF          0x00300000
#define V49_HDR_PKT_COUNT    0x000F0000
#define V49_HDR_PKT_SIZE     0x0000FFFF

// Parser error-status test and ranges
#define V49_RET_ERROR(ret)   ((ret) & 0xFFFF0000)
#define V49_RET_SUCCESS      0x00000000
#define V49_RET_COMMON       0x00010000
#define V49_RET_COMMAND      0x00020000
#define V49_RET_CONTEXT      0x00030000
#define V49_RET_CONTROL      0x00040000

// Parser success values
#define V49_OK_COMPLETE      (V49_RET_SUCCESS | 0x0000)
#define V49_OK_PAGING        (V49_RET_SUCCESS | 0x0001)
#define V49_OK_CONTROL       (V49_RET_SUCCESS | 0x0002)

// Common parser errors
#define V49_ERR_MALLOC       (V49_RET_COMMON | 0x0001)
#define V49_ERR_SHORT        (V49_RET_COMMON | 0x0002)
#define V49_ERR_SPACE        (V49_RET_COMMON | 0x0003)
#define V49_ERR_HEADER_TYPE  (V49_RET_COMMON | 0x0004)
#define V49_ERR_HEADER_CID   (V49_RET_COMMON | 0x0005)
#define V49_ERR_HEADER_SIZE  (V49_RET_COMMON | 0x0006)
#define V49_ERR_CLASS_ID     (V49_RET_COMMON | 0x0007)
#define V49_ERR_TSF_FRAC     (V49_RET_COMMON | 0x0008)
#define V49_ERR_WORD_LEN     (V49_RET_COMMON | 0x0009)
#define V49_ERR_PAGING       (V49_RET_COMMON | 0x0009)
#define V49_ERR_ENQUEUE      (V49_RET_COMMON | 0x000a)


struct v49_common
{
	// Header and Stream Identifier
	uint32_t  hdr;
	uint32_t  sid;

	// Timestamp fields
	uint32_t  ts_int;
	uint32_t  ts_frac;

	v49_type_t  type;
	uint8_t     count;

	union
	{
		struct v49_command  command;
		struct v49_context  context;
	};
};


/** Reset a v49 struct
 */
void v49_reset (struct v49_common *v49);


/** Parse and validate the header of a VITA-49 packet
 * 
 *  \param  v49   v49_common struct to fill in
 *  \param  mbuf  Message buffer
 *
 *  \return V49_OK_COMPLETE on success, V49_ERR_* on validation failure, <0 on other
 *          errors
 */
int v49_common_parse (struct v49_common *v49, struct mbuf *mbuf);

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
int v49_parse (struct v49_common *v49, struct mbuf *mbuf);

/** Format a v49 struct into VITA49 packets
 * 
 *  \note The caller should provide the MTU for the connection being used, which will be
 *        used to allocate mbufs and to paginate if necessary.
 */
int v49_format (struct v49_common *v49, struct mbuf *mbuf, struct mqueue *mqueue);

/** Return a descriptive string for a return value
 *
 *  \param err Return value from v49_parse or v49_format
 *
 *  \return Constant string suitable for error messages
 */
const char *v49_return_desc (int err);

const char *v49_type (int type);

void v49_dump_hdr (int level, uint32_t hdr);
void v49_dump     (int level, struct v49_common *v49);



#endif /* INCLUDE_COMMON_VITA49_COMMON_H */
/* Ends */
