/** \file      dsm.v
 *  \brief     Shared-library exports and version control
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
dsm_0.1 {
	global:
		/* dsm.c */
		dsm_limits;
		dsm_channels;
		dsm_close;
		dsm_reopen;
		dsm_user_alloc;
		dsm_user_free;
		dsm_map_user_array;
		dsm_map_user;
		dsm_cleanup;
		dsm_set_timeout;
		dsm_oneshot_start;
		dsm_oneshot_wait;
		dsm_get_stats;
		dsm_cyclic_stop;
		dsm_cyclic_start;
	local:
		*;
};
