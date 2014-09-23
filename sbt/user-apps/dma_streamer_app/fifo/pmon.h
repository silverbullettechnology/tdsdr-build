/** \file      fifo/pmon.h
 *  \brief     interface declarations for register access to performance monitor
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
 *           Copyright 2013(c) Analog Devices, Inc.
 *
 *           All rights reserved.
 *
 *           Redistribution and use in source and binary forms, with or without
 *           modification, are permitted provided that the following conditions are met:
 *            - Redistributions of source code must retain the above copyright
 *              notice, this list of conditions and the following disclaimer.
 *            - Redistributions in binary form must reproduce the above copyright
 *              notice, this list of conditions and the following disclaimer in
 *              the documentation and/or other materials provided with the
 *              distribution.
 *            - Neither the name of Analog Devices, Inc. nor the names of its
 *              contributors may be used to endorse or promote products derived
 *              from this software without specific prior written permission.
 *            - The use of this software may or may not infringe the patent rights
 *              of one or more patent holders.  This license does not release you
 *              from the requirement that you obtain separate licenses from these
 *              patent holders to use this software.
 *            - Use of the software either in source or binary form, must be run
 *              on or directly connected to an Analog Devices Inc. component.
 *
 *           THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 *           IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 *           MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *           IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 *           INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *           LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 *           SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *           CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *           OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *           OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _INCLUDE_FIFO_PMON_H_
#define _INCLUDE_FIFO_PMON_H_


int fifo_pmon_read  (unsigned long ofs, unsigned long *val);
int fifo_pmon_write (unsigned long ofs, unsigned long  val);


#endif // _INCLUDE_FIFO_PMON_H_
