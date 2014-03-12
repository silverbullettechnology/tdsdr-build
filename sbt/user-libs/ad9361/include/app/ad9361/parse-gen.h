/** \file      include/app/ad9361/parse-gen.h
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
#ifndef _INCLUDE_APP_AD9361_PARSE_GEN_H_
#define _INCLUDE_APP_AD9361_PARSE_GEN_H_
#include <stdlib.h>

#include <ad9361.h>

#include "struct-gen.h"


int parse_int       (int       *val, size_t size, int argc, const char **argv, int idx);
int parse_BOOL      (BOOL      *val, size_t size, int argc, const char **argv, int idx);
int parse_uint8_t   (uint8_t   *val, size_t size, int argc, const char **argv, int idx);
int parse_uint16_t  (uint16_t  *val, size_t size, int argc, const char **argv, int idx);
int parse_int16_t   (int16_t   *val, size_t size, int argc, const char **argv, int idx);
int parse_uint32_t  (uint32_t  *val, size_t size, int argc, const char **argv, int idx);
int parse_uint64_t  (uint64_t  *val, size_t size, int argc, const char **argv, int idx);

int parse_enum (int *val, size_t size, const struct ad9361_enum_map *map, 
                int argc, const char **argv, int idx);

int parse_struct (const struct struct_map *map, void *val, size_t size,
                  int argc, const char **argv, int idx);

#endif /* _INCLUDE_APP_AD9361_PARSE_GEN_H_ */
