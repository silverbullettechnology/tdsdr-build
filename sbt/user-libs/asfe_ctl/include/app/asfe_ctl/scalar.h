/** \file      include/app/asfe_ctl/scalar.h
 *  \brief     Scalar types
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
#ifndef _INCLUDE_APP_ASFE_CTL_SCALAR_H_
#define _INCLUDE_APP_ASFE_CTL_SCALAR_H_


typedef enum
{
	ST_BOOL,
	ST_UINT8,
	ST_UINT16,
	ST_UINT32,
	ST_DOUBLE,
	ST_MAX
}
scalar_type_t;

struct scalar_type
{
	const char *name;
};


#endif /* _INCLUDE_APP_ASFE_CTL_SCALAR_H_ */
