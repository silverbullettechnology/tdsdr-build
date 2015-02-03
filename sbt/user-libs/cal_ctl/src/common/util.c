/** \file      src/app/cal_ctl/util.c
 *  \brief     Utility functions
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
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>


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
	FILE *fp = fopen(path, "w");
	if ( !fp )
		return -1;

	va_list ap;
	va_start(ap, fmt);
	errno = 0;
	int ret = vfprintf(fp, fmt, ap);
	int err = errno;
	va_end(ap);

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
	FILE *fp = fopen(path, "r");
	if ( !fp )
		return -1;

	va_list ap;
	va_start(ap, fmt);
	errno = 0;
	int ret = vfscanf(fp, fmt, ap);
	int err = errno;
	va_end(ap);

	fclose(fp);

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
