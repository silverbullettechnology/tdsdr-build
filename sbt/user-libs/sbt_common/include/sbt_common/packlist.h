/** Packed list - a packed list of objects in a single free()able block
 *
 *  A packlist is a convenient way to iteratively add objects (strings, structs, etc) to a
 *  variable-length list, represented as a single char ** with a NULL pointer to terminate
 *  it, then the object data concatenated.  The packlist is allocated with malloc() and
 *  freed with free().
 *
 *  A packlist is suitable for data which doesn't change in its lifetime, such as argv
 *  lists for exec()ed proccesses or results of discovery scans.
 *
 *  A packlist is created in two passes: first to calculate the size requirements then a
 *  second to pack the data into the allocated space.  That the data to be stored must be
 *  identical for both passes.  A temporary structure holds the state necessary to
 *  assemble the packlist; once complete the state man be be discarded, or reset and
 *  reused.
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
#ifndef _INCLUDE_SBT_COMMON_PACKLIST_H_
#define _INCLUDE_SBT_COMMON_PACKLIST_H_


/** packlist structure is used by the functions below while assembling the packlist; once
 *  all the objects have been added it can be discarded or reset and reused */
struct packlist
{
	int    cnt;
	int    len;
	char **buf;
	char  *dst;
};


/** Add an object's size to a packlist's size requirements
 *
 *  For binary objects, the len arg should be a size > 0 in bytes.
 *  If len == 0, the corresponding object pointer will be NULL.
 *  If len < 0, strlen() will be used on obj to calculate len, +1 for NUL
 *
 *  \param pl  Packlist struct
 *  \param obj String pointer, if len < 0 
 *  \param len Object size
 */
void packlist_size (struct packlist *pl, const char *obj, int len);


/** Allocate a packlist
 *
 *  Attempt to malloc() a block big enough to store the objects 
 *  If len == 0, the corresponding object pointer will be NULL.
 *  If len < 0, strlen() will be used on obj to calculate len, +1 for NUL
 *
 *  \param pl  Packlist struct
 */
char **packlist_alloc (struct packlist *pl);


/** Add an object's data to a packlist's allocated data
 *
 *  For binary objects, the len arg should be a size > 0 in bytes.
 *  If len == 0, the corresponding object pointer will be NULL.
 *  If len < 0, strlen() will be used on obj to calculate len, +1 for NUL
 *
 *  \param pl  Packlist struct
 *  \param obj String pointer, if len < 0 
 *  \param len Object size
 */
void packlist_data (struct packlist *pl, const char *obj, int len);


#endif /* _INCLUDE_SBT_COMMON_PACKLIST_H_ */
/* Ends */
