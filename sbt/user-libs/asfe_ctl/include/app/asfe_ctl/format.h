/** \file      include/app/asfe_ctl/format.h
 *  \brief     Scalar format functions
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
#ifndef _INCLUDE_APP_ASFE_CTL_FORMAT_H_
#define _INCLUDE_APP_ASFE_CTL_FORMAT_H_
#include <stdlib.h>

#include <asfe_ctl.h>


// Scalar formatters
void format_UINT8        (const UINT8        *val, const char *name, int num);
void format_UINT16       (const UINT16       *val, const char *name, int num);
void format_FLOAT        (const FLOAT        *val, const char *name, int num);
void format_DOUBLE       (const DOUBLE       *val, const char *name, int num);


#endif /* _INCLUDE_APP_ASFE_CTL_FORMAT_H_ */
