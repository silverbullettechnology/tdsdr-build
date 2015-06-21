/** 
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
#ifndef INCLUDE_COMMON_VITA49_CONTROL_H
#define INCLUDE_COMMON_VITA49_CONTROL_H


#define V49_CTRL_MAGIC  0x93d156CF


typedef enum
{
	V49_CTRL_SHUTDOWN, 
	V49_CTRL_START,
	V49_CTRL_STOP,
	V49_CTRL_LIST,

	V49_CTRL_MAX
}
v49_verb_t;


struct v49_ctrl_start_stop
{
	uint32_t  sid;
	uint32_t  err;
};

struct v49_ctrl_list_item
{
	uint32_t  sid;
};

struct v49_ctrl_list
{
	uint32_t                   num;
	struct v49_ctrl_list_item  items[];
};


struct v49_control
{
	// Magic value, chosen so v49_common_parse() can separate it easily
	uint32_t  magic;

	// Number of bytes in message, min offsetof(struct v49_control, args)
	uint32_t  size;

	// Selector for union below
	v49_verb_t  verb;

	union
	{
		struct v49_ctrl_start_stop  start_stop;
		struct v49_ctrl_list        list;

	}
	args;
};


struct v49_control *v49_control_parse (struct mbuf *mbuf);

int v49_control_format_shutdown (struct mbuf *mbuf);
int v49_control_format_start    (struct mbuf *mbuf, uint32_t sid, uint32_t err);
int v49_control_format_stop     (struct mbuf *mbuf, uint32_t sid, uint32_t err);
int v49_control_format_list     (struct mbuf *mbuf, size_t num,
                                 struct v49_ctrl_list_item *items);


#endif // INCLUDE_COMMON_VITA49_CONTROL_H
