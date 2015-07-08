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
#ifndef _INCLUDE_SBT_COMMON_UUID_H_
#define _INCLUDE_SBT_COMMON_UUID_H_
#include <stdint.h>
#include <string.h>


typedef struct v49_uuid
{
	uint8_t  byte[16];
}
uuid_t;


static inline int uuid_cmp (const void *a, const void *b)
{
	return memcmp(a, b, sizeof(uuid_t));
}


const char *uuid_to_str (const uuid_t *uuid);


int uuid_from_str (uuid_t *uuid, const char *src);


void uuid_random (uuid_t *uuid);


#endif /* _INCLUDE_SBT_COMMON_UUID_H_ */
/* Ends */
