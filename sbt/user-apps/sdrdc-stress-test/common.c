/** \file      common.c
 *  \brief     Utility code
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
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


int load_random (void *dst, size_t len)
{
	char  *d = (char *)dst;
	int    r = open("/dev/urandom", O_RDONLY);
	int    l = len;
	int    s;

	if ( r < 0 )
	{
		perror("/dev/urandom");
		return r;
	}

	while ( l > 0 )
	{
		if ( (s = read(r, d, l)) < 0 )
		{
			perror("read");
			close(r);
			return s;
		}

		d += s;
		l -= s;
	}

	close(r);
	return 0;
}


void stop (const char *fmt, ...)
{
	int      err = errno;
	va_list  ap;

	va_start(ap, fmt); 
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, ": %s\n", strerror(err));
	exit(1);
}

