/** \file      private.h
 *  \brief     private declarations for common FIFO controls
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
#ifndef _INCLUDE_FIFO_PRIVATE_H_
#define _INCLUDE_FIFO_PRIVATE_H_
#include <stdio.h>


extern int   fifo_dev_fd;
extern FILE *fifo_dev_err;

#define ERROR(fmt, ...) \
	do{ \
		if ( fifo_dev_err ) \
			fprintf(fifo_dev_err, fmt, ##__VA_ARGS__); \
	} while(0)


#endif /* _INCLUDE_FIFO_PRIVATE_H_ */
