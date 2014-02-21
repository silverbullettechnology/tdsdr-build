/** \file      include/app/asfe_ctl/struct.h
 *  \brief     Structure map code
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
#ifndef _INCLUDE_APP_ASFE_CTL_STRUCT_H_
#define _INCLUDE_APP_ASFE_CTL_STRUCT_H_


#include "scalar.h"


/** Describes one scalar field in a structure, setup in an iterable array for all fields
 *  exposed to user.  name is the field name and should be consistent with the field name
 *  for searching code.  type is used to select a rendering function.  units is optional,
 *  for dimensionless values.  size and offs are used to cast a typed pointer to the
 *  correct field. 
 */
struct struct_map
{
	scalar_type_t  type;
	const char    *name;
	size_t         size;
	size_t         offs;
};

size_t struct_count (struct struct_map *map);


#endif /* _INCLUDE_APP_ASFE_CTL_STRUCT_H_ */
