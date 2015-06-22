/** \file      include/app/asfe_ctl/main.h
 *  \brief     Application global externs
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
#ifndef _INCLUDE_APP_MAIN_H_
#define _INCLUDE_APP_MAIN_H_


extern int   opt_dev_num;
extern char *opt_dev_node;
extern int   opt_soft_fail;
extern char *opt_lib_dir;

extern char  env_script_path[];


void stop (const char *fmt, ...);

typedef void (* script_hint_f) (const char *hint);

int script (FILE *fp, script_hint_f hint_func);
int interact (FILE *fp);

int execute (char *line);

int command (int argc, char **argv);

int dev_reopen (int num, int reinit);

#endif // _INCLUDE_APP_MAIN_H_
