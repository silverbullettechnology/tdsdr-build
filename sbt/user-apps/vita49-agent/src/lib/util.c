/** Miscellaneous utilities
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
#include <limits.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "lib/util.h"
#include "lib/log.h"


/** Set minimum log level for this module
 *
 *  \param level LOG_LEVEL_* constant 
 */
static int module_level = 0;
void lib_util_set_module_level (int level)
{
	module_level = level;
}


static char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

char *macfmt (uint8_t *mac)
{
	ENTER("mac %02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	static char buff[13];
	int i = 0;

	while ( i < 12 )
	{
		buff[i++] = hex[*mac >> 4];
		buff[i++] = hex[*mac++ & 0x0f];
	}

	buff[i] = '\0';
	RETURN_VALUE("%s", buff);
}


int strmatch (const char *cantidate, const char *criterion)
{
	char        cant[160],
	            crit[160],
	           *d;
	const char *p, *q, *s;
	int         ret = 1;

	// no *'s in criterion, so it's a simple match
	if ( (p = strchr(criterion, '*')) == NULL )
	{
		p = cantidate; q = criterion;
		while ( *p && *q && tolower(*p) == tolower(*q) )
		{
			p++; 
			q++;
		}

		if ( *p || *q )     // loop exits on EOS or mismatch
			ret = 0;
	}

	// Single *
	else if ( (q = strchr(p+1, '*')) == NULL )
	{ 
		// Leading *
		if ( p == criterion )
		{
			s = cantidate; d = cant;
			while (*s)
				*d++ = tolower(*s++);
			*d++ = '$';
			*d = '\0';
		 
			s = criterion + 1; d = crit;
			while (*s)
				*d++ = tolower(*s++);
			*d++ = '$';
			*d   = '\0';

			ret = (strstr(cant, crit) != NULL);
		}

		// Middle *
		else if ( p+1 )
		{
			s = cantidate; d = cant;
			*d++ = '^';
			while (*s)
				*d++ = tolower(*s++);
			*d = '\0';

			s = criterion; d = crit;
			*d++ = '^';
			while (*s && *s != '*')
				*d++ = tolower(*s++);
			*d = '\0';

			if ( strstr(cant, crit) == NULL)
				ret = 0;
			else
			{
				s = cantidate; d = cant;
				while (*s)
					*d++ = tolower(*s++);
				*d++ = '$';
				*d = '\0';

				s = p + 1; d = crit;
				while (*s)
					*d++ = tolower(*s++);
				*d++ = '$';
				*d   = '\0';

				ret = (strstr(cant, crit) != NULL); 
			}
		}

		// Trailing *
		else 
		{
			s = cantidate; d = cant;
			*d++ = '^';
			while (*s)
				*d++ = tolower(*s++);
			*d = '\0';
		 
			s = criterion; d = crit;
			*d++ = '^';
			while (*s && *s != '*')
				*d++ = tolower(*s++);
			*d = '\0';

			ret = (strstr(cant, crit) != NULL); 
		} 
	}

	// Two *'s in criterion, this is basically a stristr on whatever is
	// between them.  Everything after the second * is ignored.
	else
	{
		s = p+1; d = crit;
		while (*s && s != q)
			*d++ = tolower(*s++);
		*d = '\0';

		s = cantidate; d = cant;
		while (*s)
			*d++ = tolower(*s++);
		*d = '\0';

		ret = (strstr(cant, crit) != NULL);
	}

	return (ret);
}


long binval (const char *value)
{
	long  t;

	while ( *value && isspace(*value) )
		value++;

	switch ( tolower(*value) )
	{
		// True or Yes 
		case 't':
		case 'y':
			t = 1;
			break;

		// False or No
		case 'f':
		case 'n':
			t = 0; break;

		// On or Off, depending on 2nd char
		case 'o':
			t = tolower(value[1]) == 'n' ? 1 : 0;
			break;

		// Default, parse as numeric 
		default:
			t = strtol(value, NULL, 0);
			break; 
	}

	return (t);
}


/** Return a power of 10^e
 */
unsigned long pow10_32 (unsigned e)
{
	switch ( e )
	{
		case  0: return 1;
		case  1: return 10;
		case  2: return 100;
		case  3: return 1000;
		case  4: return 10000;
		case  5: return 100000;
		case  6: return 1000000;
		case  7: return 10000000;
		case  8: return 100000000;
		default: return 1000000000;
	}
}

unsigned long long pow10_64 (unsigned e)
{
	if ( e < 10 )
		return pow10_32(e);

	unsigned long long p = 1;
	while ( e-- > 0 )
		p *= 10;
	return p;
}


unsigned long dec_to_u_fix (const char *s, char d, unsigned e)
{
	while ( *s && isspace(*s) )
		s++;

	// convert integer part first, stop if no decimal char
	unsigned long v = strtoul(s, (char **)&s, 10);
	if ( *s != d )
		return v;

	// check for decimal digit
	s++;
	if ( !isdigit(*s) )
		return v;

	// want an int, have decimals, round
	if ( !e )
		return v + (*s >= '5' ? 1 : 0);

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


signed long dec_to_s_fix (const char *s, char d, unsigned e)
{
	while ( *s && isspace(*s) )
		s++;

	if ( *s == '-' )
		return 0 - dec_to_u_fix(s + 1, d, e);
	
	return dec_to_u_fix(s, d, e);
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
	ENTER("dst %p, max %zu, search %s, leaf %s", dst, max, search, leaf);
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
		if ( !stat(path, &s) )
			RETURN_ERRNO_VALUE(0, "%s", dst);
	}

	RETURN_ERRNO_VALUE(ENOENT, "%p", NULL);
}


/** Write a formatted string to a /proc or /sys entry
 *
 *  \param path Full path to /proc or /sys entry
 *  \param fmt  Format string for vfprintf()
 *  \param ...  Variable argument list
 *
 *  \return <0 on error, number of characters written on success
 */
int proc_write (const char *path, const char *fmt, ...)
{
	ENTER("path %s, fmt %s, ... ", path, fmt);

	FILE *fp = fopen(path, "w");
	if ( !fp )
		RETURN_VALUE("%d", -1);

	va_list ap;
	va_start(ap, fmt);
	errno = 0;
	int ret = vfprintf(fp, fmt, ap);
	int err = errno;
	va_end(ap);

	fclose(fp);

	RETURN_ERRNO_VALUE(err, "%d", ret);
}


/** Set and clear file descriptor / status flags 
 *
 *  \param fd      Full path to /proc or /sys entry
 *  \param fl_clr  Bits to clear with F_SETFL
 *  \param fl_set  Bits to set with F_SETFL
 *  \param fd_clr  Bits to clear with F_SETFD
 *  \param fd_set  Bits to set with F_SETFD
 *
 *  \return <0 on error, 0 on success
 */
int set_fd_bits (int fd, int fl_clr, int fl_set, int fd_clr, int fd_set)
{
	ENTER("fd %d", fd);
	int flags;

	if ( (flags = fcntl(fd, F_GETFL, 0)) < 0 )
		RETURN_VALUE("%d", -1);

	flags &= ~fl_clr;
	flags |=  fl_set;
	if ( fcntl(fd, F_SETFL, flags) < 0 )
		RETURN_VALUE("%d", -1);

	if ( (flags = fcntl(fd, F_GETFD, 0)) < 0 )
		RETURN_VALUE("%d", -1);

	flags &= ~fd_clr;
	flags |=  fd_set;
	if ( fcntl(fd, F_SETFD, flags) < 0 )
		RETURN_VALUE("%d", -1);

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** sprintf() a string into a big enough dynamically-allocated buffer
 *
 *  \note  this makes two passes through vsnprintf(), first to calculate the size
 *         required, second to actually format the string into the buffer.
 *  \note  the buffer returned is allocated with malloc(), the caller must free() it.
 *
 *  \param  fmt  Format string for vsnprintf()
 *  \param  ...  Variable-length argument list
 *
 *  \return Formatted string pointer on success, NULL on failure
 */
char *str_dup_sprintf (const char *fmt, ...)
{
	va_list  ap;
	char    *ret;
	char     buf[4];
	int      len;

	va_start(ap, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if ( len < 0 )
		return NULL;

	len++;
	if ( (ret = malloc(len)) )
	{
		va_start(ap, fmt);
		len = vsnprintf(ret, len, fmt, ap);
		va_end(ap);
	}

	return ret;
}



#ifdef UNIT_TEST
int main (int argc, char **argv)
{
	log_dupe(stdout);
	module_level = LOG_LEVEL_TRACE;

	char *ret;
	for ( argv++; *argv; argv++ )
		if ( (ret = path_match(NULL, 0, getenv("PATH"), *argv)) )
			printf ("%s: match: %s\n", *argv, ret);
		else
			printf ("%s: fail\n", *argv);

	return 0;
}
#endif

/* Ends    : src/lib/util.c */
