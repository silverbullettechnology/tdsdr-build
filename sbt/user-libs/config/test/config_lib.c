/*
 * File    : test/config_lib.c
 * Descript: System test for config lib
 * Version : 3.0
 * Author  : Morgan Hughes (morgan.hughes@silver-bullet-tech.com)
 * Copying : Copyright (C) 1998,1999,2000,2010,2011 Morgan Hughes
 *           All rights reserved.  For more info contact the address above.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <glob.h>
#include <errno.h>

#include "config.h"


static int func (const char *sect, const char *tag, const char *val, const char *file, int line, void *data)
{
	printf ("func (section '%s', tag '%s', val '%s', file '%s', line %d, data %s)\n",
	        sect ? sect : "(null)",
	        tag  ? tag  : "(null)",
	        val  ? val  : "(null)",
	        file ? file : "(null)",
			line,
			data ? (char *)data : "(null)");
	return 0;
}

static struct config_section_map map[] = 
{
	{ "exact", func, (void *)"exact-data"   },
	{ "pre*",  func, (void *)"prefix-data"  },
	{ "*post", func, (void *)"postfix-data" },
	{ "*out*", func, (void *)"outer-data"   },
	{ "in*in", func, (void *)"inner-data"   },
};

static struct config_context ctx = 
{
	.default_section   =  "default-section",
	.default_function  =  func,
	.section_map       =  map,
	.catchall_function =  func,
	.data              =  "default-data",
};

static void proc (const char *path)
{
	int ret;
	
	printf ("start processing '%s'\n", path);
	ret = config_parse (&ctx, path);
	printf ("done processing '%s' with ret %d\n", path, ret);
}

int main (int argc, char **argv)
{
	int i;
	setbuf (stdout, NULL);
	for ( i = 1; i < argc; i++ )
		proc (argv[i]);

	return 0;
}


/* Ends    : test/config_lib.c */
