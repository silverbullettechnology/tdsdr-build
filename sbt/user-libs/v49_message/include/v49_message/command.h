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
#ifndef _INCLUDE_V49_MESSAGE_VITA49_COMMAND_H_
#define _INCLUDE_V49_MESSAGE_VITA49_COMMAND_H_
#include <stdint.h>

#include <sbt_common/uuid.h>
#include <sbt_common/mbuf.h>
#include <sbt_common/mqueue.h>
#include <sbt_common/growlist.h>

#include <v49_message/types.h>

#define V49_CMD_DEF_PRIORITY 0x00010000

// Command parser errors
#define V49_ERR_CMD_RANGE      (V49_RET_COMMAND | 0x0001)
#define V49_ERR_LIST_SIZE      (V49_RET_COMMAND | 0x0002)
#define V49_ERR_TSTAMP_INT     (V49_RET_COMMAND | 0x0003)
#define V49_ERR_TSTAMP_FMT     (V49_RET_COMMAND | 0x0004)

// SID value reserved for manager messages
#define V49_CMD_RSVD_SID       0


typedef enum
{
	V49_CMD_ROLE_REQUEST  = 0x0,  // Request or Command
	V49_CMD_ROLE_RESULT   = 0x1,  // Response / Result
	V49_CMD_ROLE_NOTIFY   = 0x2,  // Notification or Advertisement
	V49_CMD_ROLE_INTERNAL = 0x3,  // Internal message, Tool/Master/Worker

	V49_CMD_ROLE_MAX
}
v49_command_role_t;

typedef enum
{
	V49_CMD_RES_SUCCESS = 0x00,  // Success
	V49_CMD_RES_UNSPEC  = 0x01,  // Unspecified reason
	V49_CMD_RES_INVAL   = 0x02,  // Invalid argument/parameter
	V49_CMD_RES_NOENT   = 0x03,  // Requested ID not found
	V49_CMD_RES_ALLOC   = 0x04,  // Alloc (usually memory) failed
	V49_CMD_RES_ACCESS  = 0x04,  // Access denied

	V49_CMD_RES_MAX
}
v49_command_result_t;

typedef enum
{
	V49_CMD_REQ_DISCO       = 0x00,  // Discovery / Advertisement
	V49_CMD_REQ_ENUM        = 0x01,  // Enumeration
	V49_CMD_REQ_ACCESS      = 0x02,  // Access
	V49_CMD_REQ_OPEN        = 0x03,  // Open/Resume
	V49_CMD_REQ_CONFIG      = 0x04,  // Configure
	V49_CMD_REQ_START       = 0x05,  // Start
	V49_CMD_REQ_STOP        = 0x06,  // Stop
	V49_CMD_REQ_CLOSE       = 0x07,  // Close/Suspend
	V49_CMD_REQ_RELEASE     = 0x08,  // Release
    V49_CMD_REQ_CTX_REP     = 0x09,  // Context Reporting
    V49_CMD_REQ_TSTAMP_CTL  = 0x0A,  // Timestamp Control

	V49_CMD_REQ_MAX
}
v49_command_request_t;

typedef enum
{
	V49_CMD_IND_BIT_PAGING     = 31, // Message Paging
	V49_CMD_IND_BIT_CID        = 30, // Client Identifier
	V49_CMD_IND_BIT_PRIORITY   = 29, // Request Priority
	V49_CMD_IND_BIT_RID_LIST   = 28, // Resource Identifier List
	V49_CMD_IND_BIT_RES_INFO   = 27, // Resource Information List
	V49_CMD_IND_BIT_SID_ASSIGN = 26, // Stream Identifier Assignment
	V49_CMD_IND_BIT_TSTAMP_INT = 25, // Timestamp Interpretation
    V49_CMD_IND_BIT_TSTAMP_FMT = 24, // Timestamp Format
    V49_CMD_IND_BIT_EVENT_PRD  = 23, // Event Period Specifier
    V49_CMD_IND_BIT_CTX_IND    = 22, // Context Indicator Field
}
v49_command_ind_bit_t;

#define V49_COMMAND_IND_BIT_MASK (0 \
	| (1 << V49_CMD_IND_BIT_PAGING)     \
	| (1 << V49_CMD_IND_BIT_CID)        \
	| (1 << V49_CMD_IND_BIT_PRIORITY)   \
	| (1 << V49_CMD_IND_BIT_RID_LIST)   \
	| (1 << V49_CMD_IND_BIT_RES_INFO)   \
	| (1 << V49_CMD_IND_BIT_SID_ASSIGN) \
	| (1 << V49_CMD_IND_BIT_TSTAMP_INT) \
    | (1 << V49_CMD_IND_BIT_TSTAMP_FMT) \
    | (1 << V49_CMD_IND_BIT_EVENT_PRD) \
    | (1 << V49_CMD_IND_BIT_CTX_IND) \
)

#define V49_SIZEOF_RESOURCE_INFO  (48)


typedef enum
{
	TSTAMP_INT_IMMEDIATE = 0x00, // Immediate
	TSTAMP_INT_ABSOLUTE  = 0x01, // Absolute
	TSTAMP_INT_RELATIVE  = 0x02, // Relative (for stop, to start)
}
v49_command_tstamp_int_t;


typedef enum
{
    TSTAMP_FMT_TSI_NONE  = 0x00, // No Integer-seconds Timestamp field included
    TSTAMP_FMT_TSI_UTC   = 0x04, // Coordinated Universal Time (UTC)
    TSTAMP_FMT_TSI_GPS   = 0x08, // GPS time
    TSTAMP_FMT_TSI_OTHER = 0x0C, // Other
}
v49_command_tstamp_fmt_tsi_t;

typedef enum
{
    TSTAMP_FMT_TSF_NONE  = 0x00, // No Fractional-seconds Timestamp field included
    TSTAMP_FMT_TSF_SAMP  = 0x01, // Sample Count Timestamp
    TSTAMP_FMT_TSF_PICO  = 0x02, // Real Time (Picoseconds) Timestamp
    TSTAMP_FMT_TSF_FREE  = 0x03, // Free Running Count Timestamp
}
v49_command_tstamp_fmt_tsf_t;


struct v49_command
{
	// Request / Response field
	v49_command_role_t     role;
	v49_command_request_t  request;
	v49_command_result_t   result;

	// Indicator bitmap
	uint32_t indicator;

	// Message Paging state
	uint32_t page;

	// Client Identifier
	uuid_t  cid;

	// Request Priority
	uint32_t  priority;

	// Resource Identifier List
	struct growlist *rid_list;

	// Resource Information List
	struct growlist *res_info;

	// Stream Identifier Assignment
	uint32_t  sid_assign;

	// Timestamp Interpretation
	v49_command_tstamp_int_t  tstamp_int;

    // Timestamp Format
    uint32_t  tstamp_fmt;

    // Event Period Specifier
    uint32_t  event_prd;

    // Context Indicator Field
    uint32_t  ctx_ind;
};


/** Reset a v49_command struct 
 */
void v49_command_reset (struct v49_command *cmd);

/** Parse a VITA49 packet into the command struct
 * 
 *  \note The data is merged into the command struct, to handle paginated messages.  The
 *        caller is responsible for resetting the struct before the first packet
 */
int v49_command_parse (struct v49_common *v49, struct mbuf *mbuf);

/** Format a v49_command struct into VITA49 packets
 * 
 *  \note The caller should provide the MTU for the connection being used, which will be
 *        used to allocate mbufs and to paginate if necessary.
 */
int v49_command_format (struct v49_common *v49, struct mbuf *mbuf);


const char *v49_command_role        (int role);
const char *v49_command_result      (int result);
const char *v49_command_request     (int request);
const char *v49_command_indicator   (int ind);
const char *v49_command_tstamp_int  (int tstamp_int);
const char *v49_command_tstamp_fmt  (int tstamp_fmt);
const char *v49_command_return_desc (int err);

void v49_command_dump     (int level, struct v49_command *cmd);

#endif /* _INCLUDE_V49_MESSAGE_VITA49_COMMAND_H_ */
/* Ends */
