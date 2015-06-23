/** Generic list primitives
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
#ifndef _INCLUDE_SBT_COMMON_GEN_H_
#define _INCLUDE_SBT_COMMON_GEN_H_
#include <stdlib.h>
#include <string.h>


/** Typedef for compare function for list ordering, strcmp() compatible */
typedef int (* gen_comp_fn) (const void *a, const void *b);

static inline int gen_strcmp (const void *a, const void *b)
{
	return strcmp ((const char *)a, (const char *)b);
}


/** Typedef for iterator function */
typedef int (* gen_iter_fn) (void *node, void *data);


/** Convenience destructor iterator */
static inline int gen_free (void *node, void *data)
{
	free(node);
	return 0;
}
 

#endif /* _INCLUDE_SBT_COMMON_GEN_H_ */
/* Ends */
