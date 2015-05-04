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
#ifndef INCLUDE_LIB_UTIL_H
#define INCLUDE_LIB_UTIL_H

#include <stdint.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))
#endif  // ARRAY_SIZE

extern char *argv0;


char *macfmt (uint8_t *mac);

void dump_line (unsigned char *ptr, int len);
void dump_buff (unsigned char *buff, int len);


/* Matches a cantidate string against a search criterion.  The search is
 * case-insensitive, but is sensitive to whitespace.  The following examples
 * demonstrate the pattern matching done:
 *   criterion     matches cantidates
 *   "bob"         "bob" "BOB" "Bob"
 *   "bob*"        "bob" "Bobby" "bobtail"
 *   "*est"        "best" "West"
 *   "*up*"        "dupe" "upper" "glup"
 *   "up*down"     "upsidedown" "up and down"
 *
 * Returns nonzero on a match, 0 otherwise */
int strmatch (const char *cantidate, const char *criterion);

long binval (const char *value);

/** Return a power of 10^e
 */
unsigned long      pow10_32 (unsigned e);
unsigned long long pow10_64 (unsigned e);


unsigned long dec_to_u_fix (const char *s, char d, unsigned e);
signed long dec_to_s_fix (const char *s, char d, unsigned e);


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


/** Write a formatted string to a /proc or /sys entry
 *
 *  \param path Full path to /proc or /sys entry
 *  \param fmt  Format string for vfprintf()
 *  \param ...  Variable argument list
 *
 *  \return <0 on error, number of characters written on success
 */
int proc_write (const char *path, const char *fmt, ...);


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
int set_fd_bits (int fd, int fl_clr, int fl_set, int fd_clr, int fd_set);


#endif /* INCLUDE_LIB_UTIL_H */
/* Ends */
