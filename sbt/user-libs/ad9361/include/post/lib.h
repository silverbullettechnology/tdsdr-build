/** \file      include/post/lib.h
 *  \brief     Stub wrapper
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
#ifndef _INCLUDE_POST_LIB_H_
#define _INCLUDE_POST_LIB_H_

#define GPIO_TXNRX_pin   0
#define GPIO_Enable_pin  1
#define GPIO_Resetn_pin  2


extern unsigned ad9361_legacy_dev;


struct ad9361_enum_map
{
	const char *string;
	int         value;
};
int         ad9361_enum_get_value  (const struct ad9361_enum_map *map, const char *string);
const char *ad9361_enum_get_string (const struct ad9361_enum_map *map, int value);


#endif /* _INCLUDE_POST_LIB_H_ */
