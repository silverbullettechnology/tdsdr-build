/** Numeric/string mapping
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
#include <string.h>

#include <sbt_common/log.h>
#include <sbt_common/descript.h>


LOG_MODULE_STATIC("lib_descript", LOG_LEVEL_WARN);


/** Search a map for a label
 *
 *  \param descript Map to search
 *  \param label    Label to search for
 *
 *  \return value on match, -1 on failure
 */
int descript_value_for_label (struct descript *descript, const char *label)
{
	ENTER("descript %p, label %s", descript, label);

	while ( descript->label )
		if ( !strcmp(descript->label, label) )
			RETURN_ERRNO_VALUE(0, "%d", descript->value);
		else
			descript++;

	RETURN_ERRNO_VALUE(ENOENT, "%d", -1);
}


/** Search a map for a value
 *
 *  \param descript Map to search
 *  \param value    Value to search for
 *
 *  \return label on match, NULL on failure
 */
const char *descript_label_for_value (struct descript *descript, int value)
{
	ENTER("descript %p, value %d", descript, value);

	while ( descript->label )
		if ( descript->value == value )
			RETURN_ERRNO_VALUE(0, "%s", descript->label);
		else
			descript++;

	RETURN_ERRNO_VALUE(ENOENT, "%p", NULL);
}


/** Dump a separated list of labels to the logfile
 *
 *  \param descript Map to dump
 *  \param level    Debug level to use
 *  \param sep      Separator string
 *  \param file     Caller source file
 *  \param line     Caller source line
 *
 *  \return label on match, NULL on failure
 */
void __descript_dump_labels (struct descript *map, int level, const char *sep,
                             const char *file, int line)
{
	ENTER("map %p, level %d, sep %s, file %s, line %d", map, level, sep, file, line);

	while ( map->label )
	{
		log_printf(level, module_level, file, line, "%s", map->label);
		map++;
		if ( map->label )
			log_printf(level, module_level, file, line, "%s", sep);
	}

	RETURN;
}


#ifdef UNIT_TEST
#include <stdio.h>
static struct descript descript[] = 
{
	{ "one", 1 },
	{ "two", 2 },
	{ NULL }
};

int main (int argc, char **argv)
{
	printf ("1 -> %s, want one\n",    descript_label_for_value(descript, 1));
	printf ("2 -> %s, want two\n",    descript_label_for_value(descript, 2));
	printf ("3 -> %s, want NULL\n",   descript_label_for_value(descript, 3));
	printf ("'one' -> %d, want 1\n",  descript_value_for_label(descript, "one"));
	printf ("'two' -> %d, want 2\n",  descript_value_for_label(descript, "two"));
	printf ("'xxx' -> %d, want -1\n", descript_value_for_label(descript, "xxx"));
	return 0;
}
#endif


/* Ends    : src/lib/descript.c */
