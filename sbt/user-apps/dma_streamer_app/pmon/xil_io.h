/** \file      xil_io.h
 *  \brief     Compatibility shim for Xilinx driver code
 *
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
#ifndef _XIL_IO_H_
#define _XIL_IO_H_
#include "xil_types.h"

u32  Xil_In32  (u32 addr);
void Xil_Out32 (u32 addr, u32 data);

#endif // _XIL_IO_H_
