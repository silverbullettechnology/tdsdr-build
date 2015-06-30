/** \file      dsa_util.c
 *  \brief     DSA utilities
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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <fd_main.h>

#include "dsa_util.h"


// Returns a printable static string describing the bits in the passed bitmap
const char *dsa_util_chan_desc (int ident)
{
	static char buff[64];

	snprintf(buff, sizeof(buff), "%02x { %s%s%s%s%s%s}", ident,
	         ident & DC_DEV_AD1 ? "DC_DEV_AD1 " : "",
	         ident & DC_DEV_AD2 ? "DC_DEV_AD2 " : "",
	         ident & DC_DIR_TX  ? "DC_DIR_TX "  : "",
	         ident & DC_DIR_RX  ? "DC_DIR_RX "  : "",
	         ident & DC_CHAN_1  ? "DC_CHAN_1 "  : "",
	         ident & DC_CHAN_2  ? "DC_CHAN_2 "  : "");

	return buff;
}


// Parse and validate a chip/direction/channel into an ident bitmask:
//   AD 12 T 12
//   ^^ ^^ ^ ^^-- Channel: 1 or 2 for single, 12 or omit for both
//   |  |  `----- Direction: T for TX, R for RX, never both
//   |  `-------- ADI device: 1 or 2 for single, 12 or omit for both
//   `----------- "AD" prefix: required
int dsa_util_spec_parse (const char *spec)
{
	const char *ptr = spec + 2;
	int         ret = DC_DEV_AD1|DC_DEV_AD2|DC_CHAN_1|DC_CHAN_2;

	// if the string starts with "AD" in any case it's parsed, otherwise it's passed over
	errno = 0;
	if ( tolower(spec[0]) != 'a' || tolower(spec[1]) != 'd' )
		return 0;

	// skip punctation
	while ( *ptr && !isalnum(*ptr) )
		ptr++;

	// device specifier digits after the "AD" are optional: if present then clear default
	// "both" default and check digits.  "AD12" is legal and equivalent to the default.
	if ( isdigit(*ptr) )
	{
		ret &= ~(DC_DEV_AD1|DC_DEV_AD2);
		while ( isdigit(*ptr) )
		{
			switch ( *ptr )
			{
				case '1':
					ret |= DC_DEV_AD1;
					break;
				case '2':
					ret |= DC_DEV_AD2;
					break;
				default:
					errno = EINVAL;
					return -1;
			}

			// skip punctation
			do
				ptr++;
			while ( *ptr && !isalnum(*ptr) );
		}
	}

	// direction char follows and is mandatory
	switch ( tolower(*ptr) )
	{
		case 't': ret |= DC_DIR_TX; break;
		case 'r': ret |= DC_DIR_RX; break;
		default:
			errno = EINVAL;
			return -1;
	}

	// skip punctation
	do
		ptr++;
	while ( *ptr && !isalnum(*ptr) );

	// channel specifier digits after the "T" or "R" are optional: if present then clear
	// default "both" default and check digits.  "T12" is legal and equivalent to the
	// default.
	if ( isdigit(*ptr) )
	{
		ret &= ~(DC_CHAN_1|DC_CHAN_2);
		while ( isdigit(*ptr) )
		{
			switch ( *ptr )
			{
				case '1':
					ret |= DC_CHAN_1;
					break;
				case '2':
					ret |= DC_CHAN_2;
					break;
				default:
					errno = EINVAL;
					return -1;
			}

			// skip punctation
			do
				ptr++;
			while ( *ptr && !isalnum(*ptr) );
		}
	}

	// catch garbage after direction and/or channel numbers
	if ( *ptr && isalpha(*ptr) )
		return -1;

	return ret;
}


// Maps DC_* ident bits produced in this code onto corresponding FD_ACCESS_* bits for use
// with fifo_dev / pipe_dev modules
unsigned long dsa_util_fd_access (int ident)
{
	unsigned long bits = 0;

	if ( (ident & (DC_DIR_TX|DC_DEV_AD1)) == (DC_DIR_TX|DC_DEV_AD1) )
		bits |= FD_ACCESS_AD1_TX;

	if ( (ident & (DC_DIR_TX|DC_DEV_AD2)) == (DC_DIR_TX|DC_DEV_AD2) )
		bits |= FD_ACCESS_AD2_TX;

	if ( (ident & (DC_DIR_RX|DC_DEV_AD1)) == (DC_DIR_RX|DC_DEV_AD1) )
		bits |= FD_ACCESS_AD1_RX;

	if ( (ident & (DC_DIR_RX|DC_DEV_AD2)) == (DC_DIR_RX|DC_DEV_AD2) )
		bits |= FD_ACCESS_AD2_RX;

	return bits;
}


