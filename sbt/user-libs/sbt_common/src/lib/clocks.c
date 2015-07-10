/** High-res clocks used by timer code
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "sbt_common/log.h"
#include "sbt_common/clocks.h"


clocks_t get_clocks (void)
{
	clocks_t         ret = 0;
	struct timespec  ts;

	if ( !clock_gettime(CLOCK_MONOTONIC, &ts) )
	{
		ret  = ts.tv_sec;
		ret *= 1000000000;
		ret += ts.tv_nsec;
	}

	return ret;
}

int clocks_to_us (clocks_t clocks)
{
	clocks /= 1000;
	return clocks;
}

int clocks_to_ms (clocks_t clocks)
{
	clocks /= 1000000;
	return clocks;
}

int clocks_to_s (clocks_t clocks)
{
	clocks /= 1000000000;
	return clocks;
}

clocks_t us_to_clocks (int us)
{
	clocks_t ret = us;
	ret *= 1000;
	return ret;
}

clocks_t ms_to_clocks (int ms)
{
	clocks_t ret = ms;
	ret *= 1000000;
	return ret;
}

clocks_t s_to_clocks (int s)
{
	clocks_t ret = s;
	ret *= 1000000000;
	return ret;
}

void clocks_to_tv (struct timeval *tv, clocks_t clocks)
{
	clocks_t tmp = clocks;
	tmp /= 1000000000;
	tv->tv_sec = tmp;

	clocks %= 1000000000;
	tv->tv_usec = clocks / 1000;
}

clocks_t tv_to_clocks (struct timeval *tv)
{
	clocks_t ret;
	ret  = (tv->tv_sec * 1000000000);
	ret += (tv->tv_usec * 1000);

	return ret;
}


/* Ends    : src/lib/clocks.c */
