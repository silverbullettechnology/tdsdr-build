/** \file      src/common/util.c
 *  \brief     
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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include "util.h"


/** Format and write a string to a /proc or /sys entry
 *
 *  \param path Full path to /proc or /sys entry
 *  \param fmt  Format string for vfprintf()
 *  \param ap   va_list object
 *
 *  \return <0 on error, number of characters written on success
 */
int proc_vprintf (const char *path, const char *fmt, va_list ap)
{
	FILE *fp = fopen(path, "w");
	if ( !fp )
		return -1;

	int ret = vfprintf(fp, fmt, ap);
	int err = errno;

	fclose(fp);

	errno = err;
	return ret;
}


/** Format and write a string to a /proc or /sys entry
 *
 *  \param path Full path to /proc or /sys entry
 *  \param fmt  Format string for vfprintf()
 *  \param ...  Variable argument list
 *
 *  \return <0 on error, number of characters written on success
 */
int proc_printf (const char *path, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = proc_vprintf(path, fmt, ap);
	int err = errno;
	va_end(ap);

	errno = err;
	return ret;
}


/** Write a string into a /proc or /sys entry from the caller's buffer
 *
 *  \param path Full path to /proc or /sys entry
 *  \param dst  Buffer to write from
 *  \param len  Number of bytes to write
 *
 *  \return <0 on error, number of bytes read on success
 */
int proc_write (const char *path, const void *src, size_t len)
{
	FILE *fp = fopen(path, "w");
	if ( !fp )
		return -1;

	int ret = fwrite(src, 1, len, fp);
	int err = errno;

	fclose(fp);

	errno = err;
	return ret;
}


/** Read a string from a /proc or /sys entry into the caller's buffer
 *
 *  At most max-1 bytes will be read, and the buffer will be zeroed before reading.
 *
 *  \param path Full path to /proc or /sys entry
 *  \param dst  Buffer to read into
 *  \param max  Size of buffer in bytes
 *
 *  \return <0 on error, number of bytes read on success
 */
int proc_read (const char *path, void *dst, size_t max)
{
	if ( max < 1 )
	{
		errno = EINVAL;
		return -1;
	}

	FILE *fp = fopen(path, "r");
	if ( !fp )
		return -1;

	memset(dst, 0, max);
	max--;

	int ret = fread(dst, 1, max, fp);
	int err = errno;

	fclose(fp);

	errno = err;
	return ret;
}


/** Read and parse a string from a /proc or /sys entry
 *
 *  \param path Full path to /proc or /sys entry
 *  \param fmt  Format string for vscanf()
 *  \param ...  Variable argument list
 *
 *  \return <0 on error, number of fields parsed on success
 */
int proc_vscanf (const char *path, const char *fmt, va_list ap)
{
	FILE *fp = fopen(path, "r");
	if ( !fp )
		return -1;

	errno = 0;
	int ret = vfscanf(fp, fmt, ap);
	int err = errno;

	fclose(fp);

	errno = err;
	return ret;
}


/** Read and parse a string from a /proc or /sys entry
 *
 *  \param path Full path to /proc or /sys entry
 *  \param fmt  Format string for vscanf()
 *  \param ...  Variable argument list
 *
 *  \return <0 on error, number of fields parsed on success
 */
int proc_scanf (const char *path, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	errno = 0;
	int ret = proc_vscanf(path, fmt, ap);
	int err = errno;
	va_end(ap);

	errno = err;
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


/** Convert an unsigned decimal to a fixed-point long
 *
 *  The d param gives the integer/decimal separator and the e param the number of decimal
 *  digits to convert.  Rounding is performed if more digits are available.  Examples:
 *    dec_to_u_fix("12.345", '.', 3) = 12345  ((12 * 10**3) + 345)
 *    dec_to_u_fix("12.345", '.', 2) = 1235   ((12 * 10**2) + 34 + 1)
 *    dec_to_u_fix("12.345", '.', 1) = 123    ((12 * 10**1) + 3)
 *
 *  \param  s  Source string
 *  \param  d  Decimal character
 *  \param  e  Decimal digits
 *
 *  \return value of s * 10**e
 */
unsigned long dec_to_u_fix (const char *s, char d, unsigned e)
{
	while ( *s && isspace(*s) )
		s++;

	// convert integer part first, stop if no decimal char
	unsigned long v = strtoul(s, (char **)&s, 10);
	if ( *s == d )
		s++;

	// we want a fixed and have decimals
	while ( e-- )
	{
		// avoid integer multiply by 10 for speed - from NAPL
		if ( v )
			v = (v << 3) + (v << 1);
		// use more decimals if available
		if ( isdigit(*s) )
			v += *s++ - '0';
	}

	// more digits than wanted: round
	if ( isdigit(*s) && *s >= '5' )
		v++ ;

	return v;
}


/** Convert a signed decimal to a fixed-point long
 *
 *  \param  s  Source string
 *  \param  d  Decimal character
 *  \param  e  Decimal digits
 *
 *  \return value of s * 10**e
 */
signed long dec_to_s_fix (const char *s, char d, unsigned e)
{
	while ( *s && isspace(*s) )
		s++;

	if ( *s == '-' )
		return 0 - dec_to_u_fix(s + 1, d, e);
	
	return dec_to_u_fix(s, d, e);
}


/** Trim whitespace from both ends of a string
 *
 *  - Reterminates the string after the last non-whitespace character
 *  - Returns a pointer to the first non-whitespace character
 *
 *  \param  s  Source string
 *
 *  \return Pointer into trimmed s
 */
char *trim  (char *s)
{
	return ltrim(rtrim(s));
}

/** Trim whitespace from left end of a string
 *
 *  - Returns a pointer to the first non-whitespace character
 *
 *  \param  s  Source string
 *
 *  \return Pointer into trimmed s
 */
char *ltrim (char *s)
{
	while ( *s && isspace(*s) )
		s++;

	return s;
}

/** Trim whitespace from right end of a string
 *
 *  - Reterminates the string after the last non-whitespace character
 *
 *  \param  s  Source string
 *
 *  \return Trimmed s
 */
char *rtrim (char *s)
{
	char *q;

	for ( q = s; *q; q++ ) ;
    while ( q > s && isspace(*(q - 1)) )
		q--;
	*q = '\0';

	return s;
}

