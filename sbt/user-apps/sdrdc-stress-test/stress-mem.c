/** \file      stress-mem.c
 *  \brief     Memory stress test
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "common.h"

// each buffer 1MByte
#define BUF_BITS 20
#define BUF_SIZE (1 << BUF_BITS)

// 5 bits -> 32 buffers
#define LEN_BITS 5
#define LEN_SIZE (1 << LEN_BITS)
#define LEN_MASK (LEN_SIZE - 1)

#define SEQ_LEN  256

void *buff[LEN_SIZE];
int   seq[SEQ_LEN];


int buff_alloc (void **buff, long page, long size)
{
	if ( posix_memalign(buff, page, size) )
	{
		perror("posix_memalign");
		return -1;
	}

	if ( mlock(*buff, size) )
	{
		perror("mlock");
		free(*buff);
		return -1;
	}

	return 0;
}


void buff_free (void *buff, long size)
{
	munlock(buff, size);
	free(buff);
}


int main (int argc, char **argv)
{
	long  page = sysconf(_SC_PAGESIZE);
	int   x, y, z = 0;

	if ( load_random(seq, sizeof(seq)) )
	{
		perror("load_random");
		return 1;
	}
	for ( x = 0; x < SEQ_LEN; x++ )
		while ( (seq[x] & LEN_MASK) == ((seq[x] >> LEN_BITS) & LEN_MASK) )
			if ( load_random(&seq[x], sizeof(int)) )
			{
				perror("load_random");
				return 1;
			}

	for ( x = 0; x < LEN_SIZE; x++ )
		if ( buff_alloc(&buff[x], page, BUF_SIZE) )
		{
			perror("buff_alloc");
			return 1;
		}
		else if ( load_random(buff[x], BUF_SIZE) )
		{
			perror("load_random");
			return 1;
		}
	fprintf(stderr, "%s[%d]: allocated %d buffers of size %d for memcpy\n",
	        argv[0], getpid(), LEN_SIZE, BUF_SIZE);

	while ( 1 )
	{
		if ( z >= SEQ_LEN )
			z = 0;
		x = seq[z] & LEN_MASK;
		y = (seq[z] >> LEN_BITS) & LEN_MASK;
		z++;
		assert(x != y);

		memcpy(buff[x], buff[y], BUF_SIZE);
	}

	return 0;
}
