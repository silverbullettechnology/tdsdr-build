/** \file      include/common/util.h
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
#ifndef _INCLUDE_COMMON_UTIL_H_
#define _INCLUDE_COMMON_UTIL_H_
#include <stdlib.h>
#include <stdarg.h>


/** Format and write a string to a /proc or /sys entry
 *
 *  \param path Full path to /proc or /sys entry
 *  \param fmt  Format string for vfprintf()
 *  \param ...  Variable argument list
 *
 *  \return <0 on error, number of characters written on success
 */
int proc_printf (const char *path, const char *fmt, ...);


/** Write a string into a /proc or /sys entry from the caller's buffer
 *
 *  \param path Full path to /proc or /sys entry
 *  \param dst  Buffer to write from
 *  \param len  Number of bytes to write
 *
 *  \return <0 on error, number of bytes read on success
 */
int proc_write (const char *path, const void *src, size_t len);


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
int proc_read (const char *path, void *dst, size_t max);


/** Read and parse a string from a /proc or /sys entry
 *
 *  \param path Full path to /proc or /sys entry
 *  \param fmt  Format string for vscanf()
 *  \param ...  Variable argument list
 *
 *  \return <0 on error, number of fields parsed on success
 */
int proc_scanf (const char *path, const char *fmt, ...);


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
char *path_match (char *dst, size_t max, const char *search, const char *leaf);


#endif // _INCLUDE_COMMON_UTIL_H_
