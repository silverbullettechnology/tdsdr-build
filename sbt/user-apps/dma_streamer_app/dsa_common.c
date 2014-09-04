/** \file      dsa_common.c
 *  \brief     implementation of various utility functions
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
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#include "log.h"
LOG_MODULE_STATIC("common", LOG_LEVEL_INFO);


void stop (const char *fmt, ...)
{
	int      err = errno;
	va_list  ap;
	char     buff[256];
	char    *p = buff;
	char    *e = buff + sizeof(buff);

	va_start(ap, fmt); 
	p += vsnprintf(p, e - p, fmt, ap);
	va_end(ap);

	if ( err && p < e )
		p += snprintf(p, e - p, ": %s", strerror(err));

	if ( p < e )
		p += snprintf(p, e - p, "\n");

	LOG_FOCUS("%s", buff);
	exit(1);
}

size_t size_bin (const char *str)
{
	char          *ptr;
	unsigned long  ret;

	errno = 0;
	ret = strtoul(str, &ptr, 0);
	if ( errno )
		return 0;

	switch ( tolower(*ptr) )
	{
		case 'm': ret <<= 10; // fall-through to k
		case 'k': ret <<= 10;
	}

	return ret;
}

size_t size_dec (const char *str)
{
	char          *ptr;
	unsigned long  ret;

	errno = 0;
	ret = strtoul(str, &ptr, 0);
	if ( errno )
		return 0;

	switch ( tolower(*ptr) )
	{
		case 'm': ret *= 1000; // fall-through to k
		case 'k': ret *= 1000;
	}

	return ret;
}

uint64_t dsnk_sum (void *buff, size_t size, int dsxx)
{
	uint64_t *end = (uint64_t *)(buff + size);
	uint64_t *ptr = (uint64_t *)buff;
	uint64_t  sum = 0;
	int       cry;

	while ( ptr < end )
	{
		// some PL versions rotate the sum before each words is added
		if ( dsxx )
		{
			cry   = sum & 0x0000000000000001;
			sum >>= 1;
			sum  &= 0x7FFFFFFFFFFFFFFF;
			if ( cry )
				sum |= 0x8000000000000000;
		}
		sum += *ptr++;
	}

	return sum;
}

int posix_getopt (int argc, char **argv, const char *optstring)
{
	static int first = 1;
	int        ret;

	if ( first )
	{
		if ( putenv("POSIXLY_CORRECT=1") < 0 )
			stop("putenv(POSIXLY_CORRECT=1)");
		first = 0;
	}

	if ( (ret = getopt(argc, argv, optstring)) < 0 )
	{
		unsetenv("POSIXLY_CORRECT");
		first = 1;
	}

	return ret;
}


/** Find a leaf filename in a colon-separated path list and return a full path
 *
 *  This tries a leaf filename against each element in a colon-separated path string,
 *  producing a full path and stat() it.  If the stat indicates the file exists, the full
 *  path is returned.  If the leaf file doesn't exist in any of the path components,
 *  returns NULL and sets errno to ENOENT.
 *
 *  If search is NULL or empty, the current directory is searched.
 *
 *  If dst is not given, a static buffer of PATH_MAX bytes is used and returned.
 *
 *  \param  dst     Destination buffer, if NULL a static buffer is used
 *  \param  max     Size of destination buffer
 *  \param  search  Path list to search
 *  \param  leaf    Leaf filename to search
 *
 *  \return Full pathname to existing file, or NULL if not matched.
 */
char *path_match (char *dst, size_t max, const char *search, const char *leaf)
{
	// use a static buffer if the caller didn't pass one
	static char path[PATH_MAX];
	if ( !dst )
	{
		dst = path;
		max = sizeof(path);
	}

	// if search unset use current directory
	if ( !search || !*search )
		search = ".";

	// traditional ':' separated search path: for each component 
	struct stat  s;
	const char  *cur = search;
	const char  *nxt;
	char        *end = dst + max;
	char        *col;
	char        *cat;
	while ( cur && *cur )
	{
		// copy remaining search into working buffer, set r to either position of next :
		// char or to end of string.  q ends up either NULL at eos, or the next p to try
		cat = dst + snprintf(dst, max, "%s", cur);
		nxt = cur + (cat - dst);
		if ( (col = strchr(dst, ':')) )
		{
			nxt = cur + (col - dst) + 1;
			cat = col;
		}
		cur = nxt;

		// append the leaf filename at r to get full path; if it exists return it
		snprintf(cat, end - cat, "/%s", leaf);
		if ( !stat(dst, &s) )
		{
			errno = 0;
			return dst;
		}
	}

	errno = ENOENT;
	return NULL;
}

int terminal_pause (void)
{
	struct timeval  tv;
	struct termios  tio_old;
	struct termios  tio_new;
	fd_set          fds;
	int             ret;
	char            buff[4096];

	// swich to raw mode
	tcgetattr(STDIN_FILENO, &tio_old);
	tio_new = tio_old;
	cfmakeraw(&tio_new);
	tcsetattr(STDIN_FILENO, TCSANOW, &tio_new);

	while ( 1 )
	{
		tv.tv_sec  = 5;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		if ( (ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv)) > 0 )
		{
			read(0, buff, sizeof(buff));
			break;
		}
	}

	// restore settings
	tcsetattr(STDIN_FILENO, TCSANOW, &tio_old);
	return buff[0];
}


