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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <lib/log.h>
#include <lib/uuid.h>


LOG_MODULE_STATIC("lib_uuid", LOG_LEVEL_DEBUG);


const char *uuid_to_str (const uuid_t *uuid)
{
	const uint8_t *s = uuid->byte;
	static char    b[40];
	char          *d = b;

	d += sprintf(d, "%02x%02x%02x%02x-",        s[0], s[1], s[2], s[3]);
	d += sprintf(d, "%02x%02x-",                s[4], s[5]);
	d += sprintf(d, "%02x%02x-",                s[6], s[7]);
	d += sprintf(d, "%02x%02x-",                s[8], s[9]);
	d += sprintf(d, "%02x%02x%02x%02x%02x%02x", s[10], s[11], s[12], s[13], s[14], s[15]);

	return b;
}


int uuid_from_str (uuid_t *uuid, const char *src)
{
	uint8_t *d = uuid->byte;
	char     b[3] = { 0, 0, 0 };
	long     f = 0x000002a8;
	int      l = 16;

	while ( l && isxdigit(*src) )
	{
		b[0] = *src++;
		if ( !isxdigit(*src) )
			return -1;

		b[1] = *src++;
		*d++ = strtoul(b, NULL, 16);
		l--;

		if ( f & 1 )
		{
			if ( *src != '-' )
				return -4;

			src++;
		}
		f >>= 1;
	}

	if ( l )
		return -2;
	else if ( isxdigit(*src) )
		return -5;

	return 0;
}


void uuid_random (uuid_t *uuid)
{
	int  d = 0;
	int  r = rand();
	int  l = RAND_MAX;

	while ( d < 16 )
	{
		uuid->byte[d++] = r & 0xFF;
		r >>= 8;
		l >>= 8;
		if ( l < 255 )
		{
			r = rand();
			l = RAND_MAX;
		}
	}
}


#ifdef UNIT_TEST
#include <time.h>
char *str[] =
{
	"4cb6f860-107e-42b3-a2bc-cda24cff1b73",
	"4cb6f860-107e-42b3-a2bc-cda24cff1b7300",
	"4cb6f860107e42b3a2bccda24cff1b73",
	"4cb6f86-107e-42b3-a2bc-cda24cff1b73",
	"4cb6f8-107e-42b3-a2bc-cda24cff1b73",
	"-107e-42b3-a2bc-cda24cff1b73",
	"4cb6f860-107-42b3-a2bc-cda24cff1b73",
	"4cb6f860-10-42b3-a2bc-cda24cff1b73",
	"4cb6f860-107e-42b3-a2bc-cda24cff1b7",
	"4cb6f860-107e-42b3-a2bc-cda24cff1b",
	"random-string",
	"",
};

int main (int argc, char **argv)
{
	const char *s;
	uuid_t      u;
	int         t;
	int         r;

	for ( t = 0; t < (sizeof(str) / sizeof(str[0])); t++ )
	{
		memset(&u, 0, sizeof(u));
		r = uuid_from_str(&u, str[t]);
		printf("uuid_from_str(&u, '%s'): %d -> ", str[t], r);
		if ( !!t != !!r )
		{
			printf("fail, expected %d\n", !!r);
			return 1;
		}
		if ( r && t )
		{
			printf("pass\n");
			continue;
		}

		printf("'%s' -> ", s = uuid_to_str(&u));
		if ( strcmp(s, str[t]) )
		{
			printf("fail\n");
			return 1;
		}

		printf("pass\n");
	}

	srand(time(NULL));
	uuid_random(&u);
	printf("uuid_random: '%s'\n", uuid_to_str(&u));

	printf("All tests passed\n");
	return 0;
}
#endif
