/** Sequence types
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
#ifndef SRC_TOOL_CONTROL_SEQUENCE_H
#define SRC_TOOL_CONTROL_SEQUENCE_H


typedef int (* sequence_fn) (int argc, char **argv);


struct sequence_map
{
	const char  *name;
	sequence_fn  func;
	const char  *desc;
};


#define SEQUENCE_MAP(name,func,desc) \
	static struct sequence_map _sequence_map \
	     __attribute__((unused,used,section("sequence_map"),aligned(sizeof(void *)))) \
	     = { name, func, desc };


struct sequence_map *sequence_find (const char *name);

void sequence_list (int level);


#endif // SRC_TOOL_CONTROL_SEQUENCE_H

