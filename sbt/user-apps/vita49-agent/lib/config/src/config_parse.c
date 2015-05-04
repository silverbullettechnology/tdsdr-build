/*
 * File    : config_parse.c
 * Descript: Config-file parser
 *
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

#ifdef NDEBUG
#define dprintf(...) do{ }while(0)
#else
#define dprintf(fmt, ...) \
	do{ \
		fprintf(stderr, "%s[%d]: " fmt, __func__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#endif

static int strmatch (const char *candidate, const char *criterion);
int config_parse_path (struct config_context *ctx, struct config_state *state, const char *path);

int config_parse_file (struct config_context *ctx, struct config_state *state, const char *file)
{
	struct config_buffer *buff;
	FILE                 *fp;
	char                 *p, *o, *t;
	config_func_t         function = NULL;
	int                   ret = 0;

	if ( !(buff = config_buffer_alloc (1024, 1024)) )
		return -1;

	if ( !(fp = fopen(file, "r")) )
	{
		config_buffer_free(buff);
		return -1;
	}

	if ( ctx->default_section )
		snprintf (state->section, sizeof(state->section), "%s", ctx->default_section);
	if ( ctx->default_function )
		ctx->default_function (state->section, NULL, state->section, file, 0, ctx->data);
	function = ctx->default_function;

	while ( config_buffer_read (buff, fp, CONFIG_BUFFER_READ_OPT_DEFAULT) >= 0 )
	{
		p = buff->buff;
		dprintf ("%d: Read: '%s'\n", buff->line, buff->buff);
	
		if ( !*p )
			continue;

		dprintf ("\n%d: Parse: '%s'\n", buff->line, buff->buff);

		/* section switching if it starts with [ */
		if ( *p == '[' )
		{
			struct config_section_map *mp = NULL;

			/* strip leading space, then find the end of the section name and strip trailing space */
			for ( p++; isspace(*p); p++ ) ;
			for ( o = p; *o && *o != ']'; o++ ) ;
			while ( o > p && isspace(*(o - 1)) )
				o--;
			*o = '\0';

			/* When finishing a section, both "tag" and "val" will be NULL.
			 * Log a message for debugging */
			if ( strlen(state->section) )
			{
				dprintf ("Leave Section [%s]\n", state->section);
				if ( function
				 && (ret = function(state->section, NULL, NULL, file, buff->line, ctx->data)) )
					goto out;

				function = NULL;
				state->section[0] = '\0';
			}

			/* Set state to the new section, and find a matching function */
			snprintf (state->section, sizeof(state->section), "%s", p);
			dprintf ("Enter Section [%s]\n", state->section);
			if ( ctx->section_map )
				for ( mp = ctx->section_map; mp->pattern; mp++ )
					if ( strmatch(p, mp->pattern) )
					{
						dprintf ("Match pattern %s\n", mp->pattern);
						function = mp->function;
						break;
					} 

			/* didn't match any section; use the catchall if defined, log a warning otherwise */
			if ( !mp || !mp->pattern )
			{
				if ( ctx->catchall_function )
					function = ctx->catchall_function;
				else
					dprintf ("%s[%d]: section [%s] not recognized\n", file, buff->line, p);
			}

			/* When starting a new section, "tag" will be NULL and "val" will be the map pattern which matched the section name.
			 * For catchall, val will be the string "*" which is functionally equivalent.  */
			if ( function )
			{
				const char *val = mp && mp->pattern ? mp->pattern : "*";
				if ( (ret = function(state->section, NULL, val, file, buff->line, ctx->data)) )
					goto out;
			}
			continue;
		}

		/* get the tag for this line */
		for ( t = p; *p && *p != '='; p++ )
			*p = tolower(*p);
		if ( !*p )
		{
			dprintf ("%s[%d]: tag '%s' with no value\n", file, buff->line, t);
			if ( function
			 && (ret = function (state->section, t, p, file, buff->line, ctx->data)) )
				goto out;
		}

		/* strip trailing whitespace in tag and leading in value */
		for ( o = p; o > t && isspace(*(o - 1)); o--) ;
		if ( *p )
			p++;
		*o = '\0';

		if ( *p )
		{
			while ( *p && isspace(*p) )
				p++;
		}

		/* While reading lines within a section, "tag" will contain the word left of the equals sign, "val" the line after.
		 */
		dprintf ("%s[%d]: tag '%s' has value '%s'\n", file, buff->line, t, p);
		if ( function
		 && (ret = function (state->section, t, p, file, buff->line, ctx->data)) )
			goto out;
	}

	dprintf ("Leave Section [%s]\n", state->section);
	if ( function )
		ret = function(state->section, NULL, NULL, file, buff->line, ctx->data);

out:
	config_buffer_free(buff);
	fclose (fp);
	return ret;
}


int config_parse_path (struct config_context *ctx, struct config_state *state, const char *path)
{
	struct stat st;
	glob_t      gl;
	int         i;
	int         ret = 0;
	char       *tmp = NULL;

	/* if path exists and is stat-able, check for file or directory */
	if ( !stat(path, &st) )
	{
		if ( S_ISREG(st.st_mode) )	/* regular file, parse it and return */
		{	
			ret = config_parse_file (ctx, state, path);
			return ret;
		}
		else if ( S_ISDIR(st.st_mode) )	/* directory: format a pattern out of it and use glob() */
		{
			if ( !(tmp = malloc(i = strlen(path) + 3)) )
				return -1;
			snprintf (tmp, i, "%s/*", path);
			path = tmp;
		}
		else /* anything else: invalid */
		{
			errno = EINVAL;
			return -1;
		}
	}

	/* if it's a valid shell glob, process each matching file */
	if ( glob(path, GLOB_ERR, NULL, &gl) == 0 )
	{
		for ( i = 0; !ret && i < gl.gl_pathc; i++ )
			if ( !stat(gl.gl_pathv[i], &st) && S_ISREG(st.st_mode) )
				ret = config_parse_file (ctx, state, gl.gl_pathv[i]);
		globfree (&gl);
		free (tmp);
		return ret;
	}

	free (tmp);
	errno = 0;
	return 0;
}

int config_parse (struct config_context *ctx, const char *path)
{
	int                  ret;
	struct config_state *state;
	
	state = calloc (1, sizeof(struct config_state));
	if ( !state )
		return -1;

	ret = config_parse_path (ctx, state, path);

	free (state);
	return ret;
}

static int strmatch (const char *candidate, const char *criterion)
{
	char        cand[160],
	            crit[160],
	           *d;
	const char *p, *q, *s;
	int         ret = 1;

	// no *'s in criterion, so it's a simple match
	if ( (p = strchr(criterion, '*')) == NULL )
	{
		p = candidate; q = criterion;
		while ( *p && *q && tolower(*p) == tolower(*q) )
		{
			p++; 
			q++;
		}

		if ( *p || *q )     // loop exits on EOS or mismatch
			ret = 0;
	}

	// Single *
	else if ( (q = strchr(p+1, '*')) == NULL )
	{ 
		// Leading *
		if ( p == criterion )
		{
			s = candidate; d = cand;
			while (*s)
				*d++ = tolower(*s++);
			*d++ = '$';
			*d = '\0';
		 
			s = criterion + 1; d = crit;
			while (*s)
				*d++ = tolower(*s++);
			*d++ = '$';
			*d   = '\0';

			ret = (strstr(cand, crit) != NULL);
		}

		// Middle *
		else if ( p+1 )
		{
			s = candidate; d = cand;
			*d++ = '^';
			while (*s)
				*d++ = tolower(*s++);
			*d = '\0';

			s = criterion; d = crit;
			*d++ = '^';
			while (*s && *s != '*')
				*d++ = tolower(*s++);
			*d = '\0';

			if ( strstr(cand, crit) == NULL)
				ret = 0;
			else
			{
				s = candidate; d = cand;
				while (*s)
					*d++ = tolower(*s++);
				*d++ = '$';
				*d = '\0';

				s = p + 1; d = crit;
				while (*s)
					*d++ = tolower(*s++);
				*d++ = '$';
				*d   = '\0';

				ret = (strstr(cand, crit) != NULL); 
			}
		}

		// Trailing *
		else 
		{
			s = candidate; d = cand;
			*d++ = '^';
			while (*s)
				*d++ = tolower(*s++);
			*d = '\0';
		 
			s = criterion; d = crit;
			*d++ = '^';
			while (*s && *s != '*')
				*d++ = tolower(*s++);
			*d = '\0';

			ret = (strstr(cand, crit) != NULL); 
		} 
	}

	// Two *'s in criterion, this is basically a stristr on whatever is
	// between them.  Everything after the second * is ignored.
	else
	{
		s = p+1; d = crit;
		while (*s && s != q)
			*d++ = tolower(*s++);
		*d = '\0';

		s = candidate; d = cand;
		while (*s)
			*d++ = tolower(*s++);
		*d = '\0';

		ret = (strstr(cand, crit) != NULL);
	}

	return (ret);
}


#ifdef UNIT_TEST
#include <getopt.h>

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
#endif
