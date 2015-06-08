/** \file      private.h
 *  \brief     I/O formatters - private definitions
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
#ifndef _PRIVATE_H_
#define _PRIVATE_H_
#include <stdio.h>


typedef long (* format_size_fn)   (FILE *fp, int chan);
typedef int  (* format_read_fn)   (FILE *fp, void *buff, size_t size, int chan, int lsh);
typedef int  (* format_write_fn)  (FILE *fp, void *buff, size_t size, int chan);


struct format_class
{
	const char      *name;
	const char      *exts;
	format_size_fn   size;
	format_read_fn   read;
	format_write_fn  write;
};


extern FILE *format_debug;



#endif // _PRIVATE_H_
