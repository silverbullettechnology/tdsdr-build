/** Demo tool - terminal routines
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
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>


static struct termios tio_old;

int st_term_setup (void)
{
	int ret;

	// set stdin nonblocking
	if ( (ret = fcntl(0, F_GETFL))< 0 )
	{
		perror("fcntl(0, F_GETFL)");
		return ret;
	}

	if ( (ret = fcntl(0, F_SETFL, ret | O_NONBLOCK)) < 0 )
	{
		perror("fcntl(0, F_SETFL)");
		return ret;
	}
	
	// no termios stuff needed for non-terminals
	if ( !isatty(0) )
		return 0;

	if ( (ret = tcgetattr(0, &tio_old)) )
	{
		perror("tcgetattr(0, &tio_old)");
		return ret;
	}

	struct termios tio_new = tio_old;
	tio_new.c_lflag &= ~(ECHO|ICANON);
	if ( (ret = tcsetattr(0, TCSANOW, &tio_new)) )
		perror("tcsetattr(0, TCSANOW, &tio_new)");

	return ret;
}


int st_term_cleanup (void)
{
	int ret;

	if ( (ret = fcntl(0, F_GETFL))< 0 )
	{
		perror("fcntl(0, F_GETFL)");
		return ret;
	}

	if ( (ret = fcntl(0, F_SETFL, ret & ~O_NONBLOCK)) < 0 )
	{
		perror("fcntl(0, F_SETFL)");
		return ret;
	}

	if ( !isatty(0) )
		return 0;

	if ( (ret = tcsetattr(0, TCSANOW, &tio_old)) )
		perror("tcsetattr(0, TCSANOW, &tio_old)");

	return ret;
}


