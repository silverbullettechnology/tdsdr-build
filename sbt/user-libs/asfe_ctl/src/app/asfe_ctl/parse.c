/** \file      src/app/asfe_ctl/parse.c
 *  \brief     Scalar value parsing code
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#include <asfe_ctl.h>
#include <config.h>

#include "parse.h"
#include "scalar.h"
#include "struct.h"


#define SEP_CHARS ",;"


/******** Scalars ********/


int parse_int (int *val, size_t size, int argc, const char **argv, int idx)
{
	const char *arg, *nxt;
	int        *end = (int *)((char *)val + size);
	long        tmp = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = argv[idx];
	while ( val < end )
	{
		errno = 0;
		tmp = strtol(arg, NULL, 0);
		if ( errno )
		{
			fprintf(stderr, "%s: '%s' is not a valid int\n", argv[0], arg);
			return -1;
		}
		if ( tmp < INT_MIN || tmp > INT_MAX )
		{
			errno = ERANGE;
			fprintf(stderr, "%s: '%s' is out of range (%d-%d)\n", argv[0], arg,
			        INT_MIN, INT_MAX);
			return -1;
		}
		*val++ = tmp;

		if ( (nxt = strchr(arg, ',')) )
			arg = nxt + 1;
		else
			break;
	}

	return end - val;
}


int parse_BOOL (BOOL *val, size_t size, int argc, const char **argv, int idx)
{
	const char    *arg, *nxt;
	BOOL          *end = (BOOL *)((char *)val + size);
	unsigned long  tmp = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = argv[idx];
	while ( val < end )
	{
		switch ( tolower(*arg) )
		{
			case 'y':
			case 't':
				tmp = 1;
				break;

			case 'n':
			case 'f':
				tmp = 0;
				break;

			default:
				errno = 0;
				tmp = strtoul(arg, NULL, 0);
				if ( errno )
				{
					fprintf(stderr, "%s: '%s' is not TRUE or YES, FALSE or NO, or a valid "
					        "number\n", argv[0], arg);
					return -1;
				}
				break;
		}
		*val++ = tmp > 0;

		if ( (nxt = strchr(arg, ',')) )
			arg = nxt + 1;
		else
			break;
	}

	return end - val;
}


int parse_UINT8 (UINT8 *val, size_t size,
                 int argc, const char **argv, int idx)
{
	const char    *arg, *nxt;
	UINT8         *end = (UINT8 *)((char *)val + size);
	unsigned long  tmp = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = argv[idx];
	while ( val < end )
	{
		errno = 0;
		tmp = strtoul(arg, NULL, 0);
		if ( errno )
		{
			fprintf(stderr, "%s: '%s' is not a valid UINT8\n", argv[0], arg);
			return -1;
		}
		if ( tmp > 0xFF )
		{
			errno = ERANGE;
			fprintf(stderr, "%s: '%s' is out of range (0-255)\n", argv[0], arg);
			return -1;
		}
		*val++ = tmp & 0xFF;

		if ( (nxt = strchr(arg, ',')) )
			arg = nxt + 1;
		else
			break;
	}

	return end - val;
}


int parse_UINT16 (UINT16 *val, size_t size,
                  int argc, const char **argv, int idx)
{
	const char    *arg, *nxt;
	UINT16        *end = (UINT16 *)((char *)val + size);
	unsigned long  tmp = 0;
	unsigned char  invalid = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = argv[idx];
	while ( val < end )
	{
		errno = 0;
		//check arg for validity
		// Trim leading space
		while(isspace(*arg)) arg++;

		if ((strstr(arg, "0X") == 0) &&
		    (strstr(arg, "0x") == 0)){
			if (strspn(arg, "0123456789") > 0)
			    invalid = 0;
			else
			    invalid = 1;
		}else{
		    if (strspn(arg, "0123456789abcdefABCDEFxX") > 0)
			invalid = 0;
		    else
			invalid = 1;
		}
		
		tmp = strtoul(arg, NULL, 0);
		if ( errno || invalid )
		{
			fprintf(stderr, "%s: '%s' is not a valid UINT16\n", argv[0], arg);
			return -1;
		}
		if ( tmp > 65535 )
		{
			errno = ERANGE;
			fprintf(stderr, "%s: '%s' is out of range (0-65535)\n", argv[0], arg);
			return -1;
		}
		*val++ = tmp & 0xFFFF;

		if ( (nxt = strchr(arg, ',')) )
			arg = nxt + 1;
		else
			break;
	}

	return end - val;
}


int parse_SINT16 (SINT16 *val, size_t size,
                  int argc, const char **argv, int idx)
{
	const char  *arg, *nxt;
	SINT16      *end = (SINT16 *)((char *)val + size);
	signed long  tmp = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = argv[idx];
	while ( val < end )
	{
		errno = 0;
		tmp = strtol(arg, NULL, 0);
		if ( errno )
		{
			fprintf(stderr, "%s: '%s' is not a valid SINT16\n", argv[0], arg);
			return -1;
		}
		if ( tmp < -32768 || tmp > 32767 )
		{
			errno = ERANGE;
			fprintf(stderr, "%s: '%s' is out of range (-32768-32767)\n", argv[0], arg);
			return -1;
		}
		*val++ = tmp;

		if ( (nxt = strchr(arg, ',')) )
			arg = nxt + 1;
		else
			break;
	}

	return end - val;
}


int parse_UINT32 (UINT32 *val, size_t size,
                  int argc, const char **argv, int idx)
{
	const char    *arg, *nxt;
	UINT32        *end = (UINT32 *)((char *)val + size);
	unsigned long  tmp = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = argv[idx];
	while ( val < end )
	{
		errno = 0;
		tmp = strtoul(arg, NULL, 0);
		if ( errno )
		{
			fprintf(stderr, "%s: '%s' is not a valid UINT32\n", argv[0], arg);
			return -1;
		}
		*val++ = tmp;

		if ( (nxt = strchr(arg, ',')) )
			arg = nxt + 1;
		else
			break;
	}

	return end - val;
}


int parse_FLOAT (FLOAT *val, size_t size,
                 int argc, const char **argv, int idx)
{
	const char  *arg, *nxt;
	FLOAT       *end = (FLOAT *)((char *)val + size);
	FLOAT        tmp = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = argv[idx];
	while ( val < end )
	{
		if ( sscanf(arg, "%g", &tmp) != 1 )
		{
			fprintf(stderr, "%s: '%s' is not a valid FLOAT\n", argv[0], arg);
			return -1;
		}
		*val++ = tmp;

		if ( (nxt = strchr(arg, ',')) )
			arg = nxt + 1;
		else
			break;
	}

	return end - val;
}


int parse_DOUBLE (DOUBLE *val, size_t size,
                  int argc, const char **argv, int idx)
{
	const char  *arg, *nxt;
	DOUBLE      *end = (DOUBLE *)((char *)val + size);
	DOUBLE       tmp = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = argv[idx];
	while ( val < end )
	{
		if ( sscanf(arg, "%lg", &tmp) != 1 )
		{
			fprintf(stderr, "%s: '%s' is not a valid DOUBLE\n", argv[0], arg);
			return -1;
		}
		*val++ = tmp;

		if ( (nxt = strchr(arg, ',')) )
			arg = nxt + 1;
		else
			break;
	}

	return end - val;
}





/******** Structures ********/


struct parse_state
{
	void                    *val;
	const struct struct_map *map;
};


int parse_field (void *dst, const struct struct_map *map, const char *src)
{
	char  *dat = (char *)dst + map->offs;;

	switch ( map->type )
	{
		case ST_BOOL:
			return parse_BOOL((BOOL *)dat, sizeof(BOOL), 1, &src, 0);

		case ST_UINT8:
			return parse_UINT8((UINT8 *)dat, sizeof(UINT8), 1, &src, 0);

		case ST_UINT16:
			return parse_UINT16((UINT16 *)dat, sizeof(UINT16), 1, &src, 0);

		case ST_UINT32:
			return parse_UINT32((UINT32 *)dat, sizeof(UINT32), 1, &src, 0);

		case ST_DOUBLE:
			return parse_DOUBLE((DOUBLE *)dat, sizeof(DOUBLE), 1, &src, 0);

		default:
			fprintf(stderr, "parse_field(): don't know how to handle type %d\n",
			        map->type);
			errno = EINVAL;
			return -1;
	}
}


static int parse_func (const char *section, const char *tag, const char *val,
                       const char *file, int line, void *data)
{
	struct parse_state      *state = (struct parse_state *)data;
	const struct struct_map *map   = state->map;

	if ( !tag || !val )
		return 0;

	while ( map->type < ST_MAX )
		if ( !strcasecmp(tag, map->name) )
			return parse_field(state->val, map, val);
		else
			map++;

	fprintf(stderr, "%s[%d]: '%s' not recognized\n", file, line, tag);
	return -1;
}


int parse_struct (const struct struct_map *map, void *val, size_t size,
                  int argc, const char **argv, int idx)
{
	struct config_context cc;
	struct config_state   cs;
	struct parse_state    ps;
	struct stat           sb;
	char                 *buf;
	char                 *tp;
	char                 *sp;
	int                   ret = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	memset(&cc, 0, sizeof(cc));
	memset(&cs, 0, sizeof(cs));
	memset(&ps, 0, sizeof(ps));
	cc.default_function  = parse_func;
	cc.catchall_function = parse_func;
	cs.function          = parse_func;
	cc.data              = &ps;
	ps.val               = val;
	ps.map               = map;

	// If the value is a valid filename, parse it using libconfig
	// TODO: path search
	if ( !stat(argv[idx], &sb) )
		return config_parse_file(&cc, &cs, argv[idx]);

	// if not a filename, then a list of values.  Make a local copy since strtok_r()
	// changes the buffer contents while tokenizing
	if ( !(buf = strdup(argv[idx])) )
		return -1;

	// if list of values contains = the use libconfig and the same callback function
	// allow partial initialization, the remainder of the struct will be 0
	if ( strchr(buf, '=') )
	{
		if ( (tp = strtok_r(buf, SEP_CHARS, &sp)) )
			do
				ret = config_parse_buff(&cc, &cs, "argv", idx, tp);
			while ( !ret && (tp = strtok_r(NULL, SEP_CHARS, &sp)) );

		free(buf);
		return ret;
	}

	// if the list contains bare values start with the first field and iterate through the
	// field map.  allow partial initialization, the remainder of the struct will be 0
	if ( (tp = strtok_r(buf, SEP_CHARS, &sp)) )
		do
			ret = parse_field(val, map++, tp);
		while ( !ret && (tp = strtok_r(NULL, SEP_CHARS, &sp)) );

	free(buf);
	return ret;
}



// Output-only for the moment

