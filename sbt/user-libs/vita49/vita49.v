/** \file      vita49.v
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
vita49_0.1 {
	global:
		/* src/lib/command.c */
		v49_command_reset;
		v49_command_parse;
		v49_command_format;
		v49_command_role;
		v49_command_result;
		v49_command_request;
		v49_command_indicator;
		v49_command_tstamp_int;
		v49_command_return_desc;
		v49_command_dump;
		/* src/lib/common.c */
		v49_reset;
		v49_common_parse;
		v49_parse;
		v49_format;
		v49_return_desc;
		v49_type;
		v49_dump_hdr;
		v49_dump;
		/* src/lib/context.c */
		v49_context_reset;
		v49_context_parse;
		v49_context_format;
		v49_context_indicator;
		v49_context_return_desc;
		v49_context_dump;
		/* src/lib/control.c */
		v49_control_parse;
		v49_control_format_shutdown;
		v49_control_format_start;
		v49_control_format_stop;
		v49_control_format_list;
		/* src/lib/resource.c */
		resource_dump;
	local:
		*;
};

