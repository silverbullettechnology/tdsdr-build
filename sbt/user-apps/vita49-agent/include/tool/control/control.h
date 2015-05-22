/** 
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
#ifndef INCLUDE_TOOL_CONTROL_CONTROL_H
#define INCLUDE_TOOL_CONTROL_CONTROL_H


// main.c
extern char *argv0;
extern char *opt_log;
extern char *opt_config;
extern char *opt_sock_type;
extern char *opt_sock_path;

// config.c
extern struct config_context config_context;


#endif // INCLUDE_TOOL_CONTROL_CONTROL_H
