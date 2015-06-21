/** \file      v49_client.v
 *  \brief     Shared-library exports and version control
 *  \copyright Copyright 2015 Silver Bullet Technology
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
v49_client_0.1 {
	global:
		/* src/lib/expect.c */
		expect_send;
		expect_common;
		expect_mbuf;
		expect_loop;
		/* src/lib/sequence.c */
		sequence_find_lib;
		sequence_list_lib;
		/* src/lib/socket.c */
		socket_alloc;
		socket_config_inst;
		socket_config_common;
		socket_enqueue;
		socket_dequeue;
		socket_free;
	local:
		*;
};

