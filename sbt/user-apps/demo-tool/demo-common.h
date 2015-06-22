/** Demo tool
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
#ifndef _INCLUDE_ST_TERM_H_
#define _INCLUDE_ST_TERM_H_
#include <sbt_common/uuid.h>
#include <v49_client/socket.h>


// st_term.c
int st_term_setup (void);
int st_term_cleanup (void);

// access.c
int seq_access (struct socket *sock, uuid_t *cid, uuid_t *uuid, uint32_t *stream_id);

// release.c
int seq_release (struct socket *sock, uuid_t *cid, uint32_t stream_id);

// enum.c
int seq_enum (struct socket *sock, uuid_t *cid, struct resource_info *dest, char *name);



#endif // _INCLUDE_ST_TERM_H_
