/** High-res event timer
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
#ifndef _INCLUDE_SBT_COMMON_TIMER_H_
#define _INCLUDE_SBT_COMMON_TIMER_H_

#include "sbt_common/clocks.h"


#define TIMER_LIST_EMPTY 0xFFFFFFFFFFFFFFFFULL


// Timer callbacks take return void and take a single void pointer
typedef void (* timer_callback_fn)(void *);


struct timer
{
	// timer values: callback function & argument, and when it's due
	timer_callback_fn	 func;
	void				*arg;
	clocks_t             due;

	// timer stats: number of times called, accumulated time spent
	long				 call_count;
	clocks_t			 call_clocks;
	clocks_t			 call_late;

	// doubly-linked list pointers
	struct timer		*next;
	struct timer		*prev;

	// for debugging timer lists
	const char          *name;
};


////// Upper API

// Alloc and init a timer
struct timer *timer_alloc (void);

// Init/reset a timer.  
#define timer_init(t) _timer_init(t, #t)
void _timer_init (struct timer *t, const char *n);

// Return nonzero if a timer is on the live list
#define timer_is_attached(t) _timer_is_attached(t, #t)
int _timer_is_attached (struct timer *t, const char *n);

// Detach a timer from the active list
#define timer_detach(t) _timer_detach(t, #t)
void _timer_detach (struct timer *t, const char *n);

// Attach or re-attach a timer to the active list.  
#define timer_attach(t, d, f, a) _timer_attach(t, d, f, a, #t)
void _timer_attach (struct timer *t, clocks_t due_in, timer_callback_fn func, void *arg,
                    const char *n);

////// Lower API

// check the current time and dispatch any which have come due.  max 
// can be used to limit the number of timers to run; use -1 for no 
// (practical) limit
void timer_check (int max);

// returns the next active timer's due date, or TIMER_LIST_EMPTY if the
// list is empty
clocks_t timer_head_due_at (void);

// returns the difference between the call time and the next
// active timer's due date, or TIMER_LIST_EMPTY if the list is empty
clocks_t timer_head_due_in (void);

// returns nonzero if the active list is not empty, and the due date of the
// head item has already passed
int timer_head_due_now (void);

// upcall the head timer; caller must use the above functions to ensure it's
// the correct time to do so.
void timer_head_call (void);

#endif /* _INCLUDE_SBT_COMMON_TIMER_H_ */
/* Ends */
