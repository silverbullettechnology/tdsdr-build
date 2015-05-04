/** Packed list - a packed list of objects in a single free()able block
 *
 *  A packlist is a convenient way to iteratively add objects (strings, structs, etc) to a
 *  variable-length list, represented as a single char ** with a NULL pointer to terminate
 *  it, then the object data concatenated.  The packlist is allocated with malloc() and
 *  freed with free(). 
 *
 *  A packlist is suitable for data which doesn't change in its lifetime, such as argv
 *  lists for exec()ed proccesses or results of discovery scans. 
 *
 *  A packlist is created in two passes: first to calculate the size requirements then a
 *  second to pack the data into the allocated space.  That the data to be stored must be
 *  identical for both passes.  A temporary structure holds the state necessary to
 *  assemble the packlist; once complete the state man be be discarded, or reset and
 *  reused.
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <lib/log.h>
#include <lib/packlist.h>


LOG_MODULE_STATIC("lib_packlist", LOG_LEVEL_WARN);


/** Add an object's size to a packlist's size requirements
 *
 *  For binary objects, the len arg should be a size > 0 in bytes.
 *  If len == 0, the corresponding object pointer will be NULL.
 *  If len < 0, strlen() will be used on obj to calculate len, +1 for NUL
 *
 *  \param pl  Packlist struct
 *  \param obj String pointer, if len < 0 
 *  \param len Object size
 */
void packlist_size (struct packlist *pl, const char *obj, int len)
{
	if ( len < 0 )
		len = obj ? strlen(obj) + 1 : 0;

	pl->len += len;
	pl->cnt++;
}


/** Allocate a packlist
 *
 *  Attempt to malloc() a block big enough to store the objects 
 *  If len == 0, the corresponding object pointer will be NULL.
 *  If len < 0, strlen() will be used on obj to calculate len, +1 for NUL
 *
 *  \param pl  Packlist struct
 *
 *  \return Pointer to the allocated space
 */
char **packlist_alloc (struct packlist *pl)
{
	pl->buf = malloc((sizeof(char *) * pl->cnt) + pl->len);
	if ( !pl->buf )
		return NULL;

	memset (pl->buf, 0, (sizeof(char *) * pl->cnt) + pl->len);
	pl->dst = (char *)pl->buf + sizeof(char *) * pl->cnt;
	return pl->buf;
}


/** Add an object's data to a packlist's allocated data
 *
 *  For binary objects, the len arg should be a size > 0 in bytes.
 *  If len == 0, the corresponding object pointer will be NULL.
 *  If len < 0, strlen() will be used on obj to calculate len, +1 for NUL
 *
 *  \param pl  Packlist struct
 *  \param obj String pointer, if len < 0 
 *  \param len Object size
 */
void packlist_data (struct packlist *pl, const char *obj, int len)
{
	*(pl->buf++) = len ? pl->dst : NULL;

	if ( len < 0 )
	{
		while ( *obj )
			*(pl->dst++) = *obj++;
		*(pl->dst++) = '\0';
	}
	else if ( len > 0 )
	{
		memcpy(pl->dst, obj, len);
		pl->dst += len;
	}
}


#ifdef UNIT_TEST
#include <stdio.h>

void dump (char **list, const char *msg)
{
	printf ("%s: {", msg);
	while ( *list )
		printf (" \"%s\",", *list++);
	printf (" NULL }\n");
}

int main (int argc, char **argv)
{
	struct packlist   pl;
	char            **walk = argv;

	setbuf(stdout, NULL);
	memset(&pl, 0, sizeof(struct packlist));
	dump(argv, "argv");

	// first pass: sizing - do/while intentional so we size and store the NULL at EOL
	while (*walk)
		packlist_size(&pl, *walk++, -1);
	packlist_size(&pl, NULL, 0);

	// allocate buffer
	char **list = packlist_alloc(&pl);
	if ( !list )
	{
		printf ("packlist_alloc() failed: %s\n", strerror(errno));
		printf ("state: { cnt %d, len %d } -> size %lu\n",
		        pl.cnt, pl.len, ((sizeof(char *) * pl.cnt) + pl.len));
		return 1;
	}

	// second pass: storing - do/while intentional so we size and store the NULL at EOL
	walk = argv;
	while (*walk)
		packlist_data(&pl, *walk++, -1);
	packlist_size(&pl, NULL, 0);

	dump(list, "list");
	free(list);
	return 0;
}
#endif


/* Ends    : child.c */
