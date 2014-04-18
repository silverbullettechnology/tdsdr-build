/** \file      stress-nor.c
 *  \brief     Flash memory stress test
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
#include <unistd.h>
#include <fcntl.h>

#include "common.h"

#define DEV "/dev/mtdblock0"

#define BUF_BITS 12
#define BUF_SIZE (1 << BUF_BITS)

char buff[BUF_SIZE];


int main (int argc, char **argv)
{
	int fd = open(DEV, O_RDWR);
	if ( fd < 0 )
		stop("open(%s)", DEV);

	off_t size = lseek(fd, 0, SEEK_END);
	off_t offs = lseek(fd, 0, SEEK_SET);
	if ( size < 0 )
		stop("lseek");

	int ret;
	while ( 1 )
	{
		if ( (ret = read(fd, buff, sizeof(buff))) != sizeof(buff) )
			stop("read");

		offs += sizeof(buff);
		if ( offs >= size )
			offs = lseek(fd, 0, SEEK_SET);
	}

	close(fd);
	return 0;
}

