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


double a[LEN], b[LEN];


int load (double *dst, int len)
{
	if ( load_random(dst, len * sizeof(double)) )
		return -1;

	int i;
	for ( i = 0; i < len; i++ )
		while ( fpclassify(dst[i]) != FP_NORMAL )
			if ( load_random(&dst[i], sizeof(double))  )
				break;

	return 0;
}


int main (int argc, char **argv)
{
	double  r;
	int     x, y;

	if ( load(a, LEN) || load(b, LEN) )
		return 1;

	fprintf(stderr, "%s[%d]: %d^2 doubles for FPU exercise\n", argv[0], getpid(), LEN);

	while ( 1 )
		for ( y = 0; y < LEN; y++ )
			for ( x = 0; x < LEN; x++ )
			{
				r  = a[x] + b[y];
				r /= b[x];
				r *= a[y];
			}

	return 0;
}
