/** \file      include/app/asfe_ctl/parse.h
 *  \brief     Scalar parse functoins
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
#ifndef _INCLUDE_APP_ASFE_CTL_PARSE_H_
#define _INCLUDE_APP_ASFE_CTL_PARSE_H_
#include <stdlib.h>


#include <asfe_ctl.h>


int parse_int                     (int                     *val, size_t size, int argc, const char **argv, int idx);
int parse_BOOL                    (BOOL                    *val, size_t size, int argc, const char **argv, int idx);
int parse_UINT8                   (UINT8                   *val, size_t size, int argc, const char **argv, int idx);
int parse_UINT16                  (UINT16                  *val, size_t size, int argc, const char **argv, int idx);
int parse_SINT16                  (SINT16                  *val, size_t size, int argc, const char **argv, int idx);
int parse_UINT32                  (UINT32                  *val, size_t size, int argc, const char **argv, int idx);
int parse_FLOAT                   (FLOAT                   *val, size_t size, int argc, const char **argv, int idx);
int parse_DOUBLE                  (DOUBLE                  *val, size_t size, int argc, const char **argv, int idx);


#endif /* _INCLUDE_APP_ASFE_CTL_PARSE_H_ */
