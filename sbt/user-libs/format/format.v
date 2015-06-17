/** \file      format.v
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
format_0.1 {
	global:
		/* src/lib/common.c */
		format_class_find;
		format_class_guess;
		format_class_name;
		format_class_list;
		format_class_read_channels;
		format_class_write_channels;
		format_size;
		format_read;
		format_write;
		format_size_data_from_buff;
		format_size_buff_from_data;
		format_num_packets_from_buff;
		format_num_packets_from_data;
		format_error_setup;
		format_debug_setup;
	local:
		*;
};

