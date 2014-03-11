/** \file      include/app/ad9361/format-gen.h
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
#ifndef _INCLUDE_APP_AD9361_FORMAT_GEN_H_
#define _INCLUDE_APP_AD9361_FORMAT_GEN_H_
#include <stdlib.h>

#include <ad9361.h>


// Scalar formatters
void format_int          (const int       *val, const char *name, int num);
void format_uint8_t      (const uint8_t   *val, const char *name, int num);
void format_uint16_t     (const uint16_t  *val, const char *name, int num);
void format_int32_t      (const int32_t   *val, const char *name, int num);
void format_uint32_t     (const uint32_t  *val, const char *name, int num);
void format_uint64_t     (const uint64_t  *val, const char *name, int num);

// Structure formatters
void format_struct (const struct struct_map *map, const void *val, const char *name, int num);


#endif /* _INCLUDE_APP_AD9361_FORMAT_GEN_H_ */
