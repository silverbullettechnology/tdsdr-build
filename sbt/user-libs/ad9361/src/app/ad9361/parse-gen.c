/** \file      src/app/ad9361/parse.c
 *  \brief     
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

#include <ad9361.h>
#include <config.h>

#include "struct-gen.h"
#include "parse-gen.h"
#include "scalar.h"
#include "util.h"


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
	arg = ltrim((char *)argv[idx]);
	while ( val < end )
	{
		errno = 0;
		tmp = strtol(arg, NULL, 0);
		if ( errno || !(isdigit(*arg) || *arg == '-') )
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
	arg = ltrim((char *)argv[idx]);
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
				if ( errno || !isdigit(*arg) )
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


int parse_uint8_t (uint8_t *val, size_t size,
                   int argc, const char **argv, int idx)
{
	const char    *arg, *nxt;
	uint8_t       *end = (uint8_t *)((char *)val + size);
	unsigned long  tmp = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = ltrim((char *)argv[idx]);
	while ( val < end )
	{
		errno = 0;
		tmp = strtoul(arg, NULL, 0);
		if ( errno || !isdigit(*arg) )
		{
			fprintf(stderr, "%s: '%s' is not a valid uint8_t\n", argv[0], arg);
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


int parse_uint16_t (uint16_t *val, size_t size,
                    int argc, const char **argv, int idx)
{
	const char    *arg, *nxt;
	uint16_t      *end = (uint16_t *)((char *)val + size);
	unsigned long  tmp = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = ltrim((char *)argv[idx]);
	while ( val < end )
	{
		errno = 0;
		tmp = strtoul(arg, NULL, 0);
		if ( errno || !isdigit(*arg) )
		{
			fprintf(stderr, "%s: '%s' is not a valid uint16_t\n", argv[0], arg);
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


int parse_uint32_t (uint32_t *val, size_t size,
                    int argc, const char **argv, int idx)
{
	const char    *arg, *nxt;
	uint32_t      *end = (uint32_t *)((char *)val + size);
	unsigned long  tmp = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = ltrim((char *)argv[idx]);
	while ( val < end )
	{
		errno = 0;
		tmp = strtoul(arg, NULL, 0);
		if ( errno || !isdigit(*arg) )
		{
			fprintf(stderr, "%s: '%s' is not a valid uint32_t\n", argv[0], arg);
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


int parse_uint64_t (uint64_t *val, size_t size,
                    int argc, const char **argv, int idx)
{
	const char    *arg, *nxt;
	uint64_t      *end = (uint64_t *)((char *)val + size);
	uint64_t       tmp = 0;

	if ( idx >= argc )
		return -1;

	memset(val, 0, size);
	arg = ltrim((char *)argv[idx]);
	while ( val < end )
	{
		errno = 0;
		tmp = strtoull(arg, NULL, 0);
		if ( errno || !isdigit(*arg) )
		{
			fprintf(stderr, "%s: '%s' is not a valid uint64_t\n", argv[0], arg);
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


int parse_enum (int *val, size_t size, const struct ad9361_enum_map *map, 
                int argc, const char **argv, int idx)
{
	if ( idx >= argc )
		return -1;

	memset(val, 0, size);

	// first try conversion as number, if it exists in the list keep it
	char *arg = ltrim((char *)argv[idx]);
	if ( *arg && isdigit(*arg) )
	{
		errno = 0;
		*val = strtol(arg, NULL, 0);
		if ( !errno && ad9361_enum_get_string(map, *val) )
			return 0;
	}

	// second try lookup as a string, if that passes keep it
	*val = ad9361_enum_get_value(map, arg);
	if ( !errno )
		return 0;

	// failing that output error with legal values
	fprintf(stderr, "%s: '%s' is not a valid enum\n", argv[0], argv[idx]);
	fprintf(stderr, "Values: %d:%s", map->value, map->string);
	for ( map++; map->string; map++ )
		fprintf(stderr, ", %d:%s", map->value, map->string);
	fprintf(stderr, "\n");
	return -1;
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
			return parse_uint8_t((uint8_t *)dat, sizeof(uint8_t), 1, &src, 0);

		case ST_UINT16:
			return parse_uint16_t((uint16_t *)dat, sizeof(uint16_t), 1, &src, 0);

		case ST_UINT32:
			return parse_uint32_t((uint32_t *)dat, sizeof(uint32_t), 1, &src, 0);

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


