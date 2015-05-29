/** \file      dsa_sample.h
 *  \brief     definition of sample data types
 *  \copyright Copyright 2013,2014 Silver Bullet Technology
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
 * vim:ts=4:noexpandtab
 */
#ifndef _INCLUDE_DSA_SAMPLE_H_
#define _INCLUDE_DSA_SAMPLE_H_
#include <stdint.h>


// a single complex sample for one channel
struct dsa_sample
{
	uint16_t  i;
	uint16_t  q;
} __attribute__((packed));


// a pair of simultaneous samples from channels 1 and 2 on the same chip
struct dsa_sample_pair
{
	struct dsa_sample ch[2];
} __attribute__((packed));


#endif // _INCLUDE_DSA_SAMPLE_H_
