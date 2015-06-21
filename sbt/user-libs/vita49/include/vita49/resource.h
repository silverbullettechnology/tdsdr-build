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
#ifndef INCLUDE_COMMON_RESOURCE_H
#define INCLUDE_COMMON_RESOURCE_H
#include <stdint.h>

#include <sbt_common/uuid.h>
#include <sbt_common/growlist.h>


struct resource_info
{
	uuid_t    uuid;	// must be first, for casts when searching lists
	char      name[20];
	uint8_t   txch;
	uint8_t   rxch;
	uint32_t  rate_q8;
	uint16_t  min;
	uint16_t  max;
};


void resource_dump (int level, const char *msg, struct resource_info *res);


#endif // INCLUDE_COMMON_RESOURCE_H
