/** \file      api_types.h
 *  \brief     Public header wrapper
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
#ifndef _INCLUDE_POST_API_TYPES_H_
#define _INCLUDE_POST_API_TYPES_H_

/* Typedefs */
typedef unsigned long  UINT32;
typedef signed   long  SINT32;
typedef unsigned short UINT16;
typedef signed   short SINT16;
typedef unsigned char  UINT8;
typedef signed   char  SINT8;
typedef float          FLOAT;
typedef double         DOUBLE;
typedef unsigned char  BOOL;

typedef enum
{
    ADIERR_OK=0,
    ADIERR_FALSE,
    ADIERR_TRUE,
    ADIERR_INV_PARM,
    ADIERR_NOT_AVAILABLE,
    ADIERR_FAILED
}
ADI_ERR;



#endif /* _INCLUDE_POST_API_TYPES_H_ */
