/** \file      include/app/cal_ctl/map.h
 *  \brief     Command map types and macros
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
#ifndef _INCLUDE_APP_CAL_CTL_MAP_H_
#define _INCLUDE_APP_CAL_CTL_MAP_H_


typedef int (* cal_ctl_handler_f) (int argc, const char **argv);


struct cal_ctl_map_cmd_t
{
	const char       *arg0;
	const char       *name;
	cal_ctl_handler_f  func;
	int               argc;
};


struct cal_ctl_map_arg_t
{
	const char *func;
	int         idx;
	const char *name;
	const char *type;
	const char *desc;
};


struct cal_ctl_map_cmd_t *cal_ctl_map_find (const char *arg0);
int cal_ctl_map_call (struct cal_ctl_map_cmd_t *map, int argc, const char **argv);

void cal_ctl_map_help (const char *name);

#ifdef CAL_CTL_USE_READLINE
char **cal_ctl_map_completion (const char *text, int start, int end);
#endif


/* Define an cal_ctl_map_arg_t entry for the argument, used to auto-generate a help entry */
#define MAP_CMD(arg0,func,argc) \
	static struct cal_ctl_map_cmd_t cal_ctl_map_cmd_ ## arg0 \
	 __attribute__((unused,used,section("cal_ctl_map_cmd"),aligned(sizeof(void *)))) \
	 = { #arg0, #func, func, argc }


/* Call, Check-Return macro: call library function func with 0 or more args.  If the
 * return is not ADIERR_OK, print an error message and return -1 */
#define MAP_LIB_CALL(func, ...) do{              \
	ADI_ERR ret = func(__VA_ARGS__);             \
	if ( ret != ADIERR_OK ) {                    \
		printf("%s: ADIERR_%s\n", argv[0], cal_ctl_err_desc(ret)); \
		return ret;                              \
	} }while(0)

/* Define an cal_ctl_map_arg_t entry for the argument, used to auto-generate a help entry */
#define MAP_ARG_HELP(type,name,idx,desc) \
	static struct cal_ctl_map_arg_t cal_ctl_map_arg_ ## name \
	 __attribute__((unused,used,section("cal_ctl_map_arg"),aligned(sizeof(void *)))) \
	 = { __func__, idx, #name, #type, desc }

/* Define argument to be read from argv[idx].  Specify a default initializer as the
 * variable-length argument */
#define MAP_ARG(type, name, idx, desc)                              \
	type  name;                                                 \
	MAP_ARG_HELP(type,name,idx,desc);                                   \
	if ( parse_ ## type(&name, sizeof(name), argc, argv, idx) ) \
		return -1;
#define MAP_ARR(type, name, idx, len, desc)                         \
	type  name[len];                                            \
	MAP_ARG_HELP(type,name,idx,desc);                                   \
	if ( parse_ ## type(name, sizeof(name), argc, argv, idx) )  \
		return -1;

/* Output a result using the appropriate format_* function for its type */
#define MAP_RESULT(type, name, num) do{ format_ ## type(&name, #name, num); }while(0)


#endif /* _INCLUDE_APP_CAL_CTL_MAP_H_ */
