/** \file      dsa_command.h
 *  \brief     interfaces for DSA commands
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
#ifndef _INCLUDE_DSA_COMMAND_H_
#define _INCLUDE_DSA_COMMAND_H_

int dsa_command_options (int argc, char **argv);
int dsa_command_setup   (int sxx, int argc, char **argv);
int dsa_command_trigger (int argc, char **argv);

void dsa_command_options_usage (void);
void dsa_command_setup_usage   (void);
void dsa_command_trigger_usage (void);

#endif // _INCLUDE_DSA_COMMAND_H_
