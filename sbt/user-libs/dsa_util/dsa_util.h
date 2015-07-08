/** \file      dsa_util.h
 *  \brief     DSA utilities
 *  \copyright Copyright 2013,2014,2015 Silver Bullet Technology
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
#ifndef _INCLUDE_DSA_UTIL_H_
#define _INCLUDE_DSA_UTIL_H_


#define DC_DEV_AD1  0x01
#define DC_DEV_AD2  0x02
#define DC_DIR_TX   0x04
#define DC_DIR_RX   0x08
#define DC_CHAN_1   0x10
#define DC_CHAN_2   0x20

#define DC_DEV_IDX_TO_MASK(i)  (DC_DEV_AD1 << (i))
#define DC_DEV_MASK_TO_IDX(i)  ((i) - 1)
#define DC_CHAN_IDX_TO_MASK(i) (DC_CHAN_1  << (i))
#define DC_CHAN_MASK_TO_IDX(i) (((i) >> 4) - 1)


// Returns a printable static string describing the bits in the passed bitmap
const char *dsa_util_chan_desc (int ident);


// Parse and validate a chip/direction/channel into an ident bitmask:
//   AD 12 T 12
//   ^^ ^^ ^ ^^-- Channel: 1 or 2 for single, 12 or omit for both
//   |  |  `----- Direction: T for TX, R for RX, never both
//   |  `-------- ADI device: 1 or 2 for single, 12 or omit for both
//   `----------- "AD" prefix: required
int dsa_util_spec_parse (const char *spec);


// Maps DC_* ident bits produced in this code onto corresponding FD_ACCESS_* bits for use
// with fifo_dev / pipe_dev modules
unsigned long dsa_util_fd_access (int ident);


#endif /* _INCLUDE_DSA_UTIL_H_ */
