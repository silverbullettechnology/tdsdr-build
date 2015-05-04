/** High-res clock type
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
#ifndef INCLUDE_LIB_CLOCKS_H
#define INCLUDE_LIB_CLOCKS_H
#include <sys/time.h>
#include <stdint.h>

typedef  unsigned long long  clocks_t;


clocks_t get_clocks(void);


static inline void clock_rate_init (void) { }

int clocks_to_us (clocks_t clocks);
int clocks_to_ms (clocks_t clocks);
int clocks_to_s  (clocks_t clocks);
clocks_t us_to_clocks (int us);
clocks_t ms_to_clocks (int ms);
clocks_t s_to_clocks  (int sec);

void     clocks_to_tv (struct timeval *tv, clocks_t clocks);
clocks_t tv_to_clocks (struct timeval *tv);


#endif /* INCLUDE_LIB_CLOCKS_H */
/* Ends */
