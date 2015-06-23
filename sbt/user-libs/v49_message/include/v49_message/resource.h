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
#ifndef _INCLUDE_V49_MESSAGE_RESOURCE_H_
#define _INCLUDE_V49_MESSAGE_RESOURCE_H_
#include <stdint.h>

#include <sbt_common/uuid.h>
//#include <sbt_common/growlist.h>


struct resource_info
{
	uuid_t    uuid;	// must be first, for casts when searching lists
	char      name[20];
	uint8_t   txch;
	uint8_t   rxch;
	uint32_t  rate_q8;
	uint16_t  min;
	uint16_t  max;
	
	unsigned long  fd_bits;  /* Access bits for pipe_dev/fifo_dev */
	int            dc_bits;  /* Dev/dir/channel bits for dsa_util */
};


extern char *resource_config_path;

extern struct growlist  resource_list;


/** Read resource database from file
 *
 *  \param path  Path to resource config file
 *
 *  \return 0 on success, !0 on failure
 */
int resource_config (const char *path);


void resource_dump (int level, const char *msg, struct resource_info *res);


#endif /* _INCLUDE_V49_MESSAGE_RESOURCE_H_ */
