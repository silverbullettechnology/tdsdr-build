/** Generic iterator 
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
#ifndef INCLUDE_LIB_ITER_H
#define INCLUDE_LIB_ITER_H
#include <stdarg.h>


/** Iterator function type, for passing to worker functions */
typedef char * (* iter_str_fn) (void *);

/** Iterator function for getting a char * from a va_list */
static inline char *iter_str_va_list (void *ap)
{
	return va_arg(*((va_list *)ap), char *);
}

/** Iterator function for getting a char * from an argv[] */
static inline char *iter_str_argv (void *argv)
{
	return (*((*((char ***)argv))++));
}


#endif /* INCLUDE_LIB_ITER_H */
/* Ends */
