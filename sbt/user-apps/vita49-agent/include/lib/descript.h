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
#ifndef INCLUDE_LIB_DESCRIPT_H
#define INCLUDE_LIB_DESCRIPT_H


/** Defines a mapping between a numeric and string value */
struct descript
{
	const char *label;
	int         value;
};


/** Search a map for a label
 *
 *  \param descript Map to search
 *  \param label    Label to search for
 *
 *  \return value on match, -1 on failure
 */
int descript_value_for_label (struct descript *descript, const char *label);


/** Search a map for a value
 *
 *  \param descript Map to search
 *  \param value    Value to search for
 *
 *  \return label on match, NULL on failure
 */
const char *descript_label_for_value (struct descript *descript, int value);


/** Dump a separated list of labels to the logfile
 *
 *  \param map    Map to dump
 *  \param level  Debug level to use
 *  \param sep    Separator string
 *  \param file   Caller source file
 *  \param line   Caller source line
 */
#define descript_dump_labels(m, l, s) __descript_dump_labels(m, l, s, __FILE__, __LINE__)
void __descript_dump_labels (struct descript *map, int level, const char *sep,
                             const char *file, int line);


#endif /* INCLUDE_LIB_DESCRIPT_H */
/* Ends */
