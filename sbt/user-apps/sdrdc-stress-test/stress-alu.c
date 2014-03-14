/** \file      stress-fpu.c
 *  \brief     CPU stress test for FPU operations
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
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "common.h"


#define LEN 512


long a[LEN], b[LEN];


int load (long *dst, int len)
{
	if ( load_random(dst, len * sizeof(long)) )
		return -1;

	int i;
	for ( i = 0; i < len; i++ )
		while ( !dst[i] )
			if ( load_random(&dst[i], sizeof(long))  )
				break;

	return 0;
}


int main (int argc, char **argv)
{
	long long  ll;
	int        x, y;

	if ( load(a, LEN) || load(b, LEN) )
		return 1;

	fprintf(stderr, "%s[%d]: %d^2 longs for ALU exercise\n", argv[0], getpid(), LEN);

	while ( 1 )
		for ( y = 0; y < LEN; y++ )
			for ( x = 0; x < LEN; x++ )
			{
				ll  = a[x] + b[y];
				ll *= a[y];
				ll /= b[x];
			}

	return 0;
}
