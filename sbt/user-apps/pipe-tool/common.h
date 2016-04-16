/** \file      dsa_common.h
 *  \brief     interfaces for various utility functions
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
#ifndef _INCLUDE_DSA_COMMON_H_
#define _INCLUDE_DSA_COMMON_H_
#include <stdarg.h>
#include <stdint.h>


struct srio_header
{
	uint32_t  tuser;
	uint32_t  padding;
	uint32_t  hello[2];
};

struct vita_header
{
	uint32_t  hdr;
	uint32_t  sid;
	uint32_t  tsf1;
	uint32_t  tsf2;
};

#define SRIO_HEAD  sizeof(struct srio_header)
#define VITA_HEAD  sizeof(struct vita_header)
#define VITA_TAIL  sizeof(uint32_t)


void stop (const char *fmt, ...);
size_t size_bin (const char *str);
size_t size_dec (const char *str);
uint64_t dsnk_sum (void *buff, size_t size, int dsxx);
int terminal_pause (void);


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


/** Return a power of 10^e
 */
unsigned long pow10_32 (unsigned e);

void hexdump_line (const unsigned char *ptr, const unsigned char *org, int len);
void hexdump_buff (const void *buf, int len);


#endif // _INCLUDE_DSA_COMMON_H_
