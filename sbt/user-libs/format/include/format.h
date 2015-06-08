/** \file      format.h
 *  \brief     I/O formatters - public definitions
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
#ifndef _FORMAT_H_
#define _FORMAT_H_
#include <stdio.h>


void format_debug_setup (FILE *fp);


struct format_class;

void                 format_class_list  (FILE *fp);
struct format_class *format_class_find  (const char *name);
struct format_class *format_class_guess (const char *name);
const char          *format_class_name  (struct format_class *fmt);


int format_size  (struct format_class *fmt, FILE *fp, int chan);
int format_read  (struct format_class *fmt, FILE *fp, void *buff, size_t size, int chan, int lsh);
int format_write (struct format_class *fmt, FILE *fp, void *buff, size_t size, int chan);


void hexdump_line (FILE *fp, const unsigned char *ptr, int len);
void hexdump_buff (FILE *fp, const void *buf, int len);
void smpdump (FILE *fp, void *buff, size_t size);


#endif /* _FORMAT_H_ */
