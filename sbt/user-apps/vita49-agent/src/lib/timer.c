/** High-resolution event timers
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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "lib/util.h"
#include "lib/timer.h"
#include "lib/log.h"


LOG_MODULE_STATIC("lib_timer", LOG_LEVEL_WARN);


// Active timer list is kept as a sorted, doubly-linked list.  The next timer 
// due is always the head.  
static struct timer *timer_head = NULL;
static struct timer *timer_tail = NULL;


static inline void learn (struct timer *t, const char *n)
{
	if ( !t || !n || t->name )
		return;
	LOG_DEBUG("learn(t %p -> n %s)\n", t, n);
	t->name = n;
}

static clocks_t t0 = 0;
static inline int due(clocks_t at)
{
	return clocks_to_ms(at - t0);
}

static const inline char *name (struct timer *node)
{
	return node ? node->name : "NULL";
}

static void dump (const char *mesg, struct timer *node)
{
	LOG_DEBUG("%s: { ", mesg);

	int cycles = 10;
	while ( node && cycles-- )
	{
		LOG_DEBUG ("{%s:%s:%d:%s}, ", name(node->prev), name(node), due(node->due), name(node->next));
		node = node->next;
	}

	if ( cycles < 1 )
		LOG_DEBUG("}: circular list!\n");
	else
		LOG_DEBUG("END }\n");
}

static void stop (const char *fmt, ...)
{
	char buff[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buff, sizeof(buff), fmt, ap);
	va_end(ap);

	LOG_ERROR("stop: %s\n", buff);
	module_level = LOG_LEVEL_DEBUG;
	dump("\nstopping, final", timer_head);
	exit(1);
}


////// Upper API

// Init/reset a timer.  
void _timer_init (struct timer *t, const char *n)
{
	ENTER("t %s", name(t));

	memset (t, 0, sizeof(struct timer));

	learn(t, n);
	if ( !t0 )
		t0 = get_clocks();

	RETURN;
}

// Alloc and init a timer
struct timer *timer_alloc (void)
{
	ENTER();
	struct timer *t = malloc (sizeof(struct timer));

	if ( t )
		_timer_init(t, NULL);

	RETURN_ERRNO_VALUE(0, "%p", t);
}

// Return nonzero if a timer is on the live list: 
// - if it's the head or the tail of the list, this is easy
// - if it's in the middle, it must have next and prev
int _timer_is_attached (struct timer *t, const char *n)
{
	learn(t, n);
	ENTER("t %s", name(t));

	if ( t == timer_head )
		RETURN_ERRNO_VALUE (0, "%d", 1);

	if ( t == timer_tail )
		RETURN_ERRNO_VALUE (0, "%d", 2);

	if ( t->next )
		RETURN_ERRNO_VALUE (0, "%d", 3);

	if ( t->prev )
		RETURN_ERRNO_VALUE (0, "%d", 4);

	RETURN_ERRNO_VALUE (0, "%d", 0);
}

static inline void timer_remove (struct timer *t)
{
	ENTER("t %s", name(t));

	LOG_DEBUG ("  remove(t {%s:%s:%d:%s})\n", name(t->prev), name(t), due(t->due), name(t->next));
	if ( t->prev )
	{
		if ( t->prev->next != t )
			stop("t->prev %s ->next %s != t?!", name(t->prev), name(t->prev->next));

		LOG_DEBUG("    t->prev->next %s to %s\n", name(t->prev->next), name(t->next));
		t->prev->next = t->next;
	}
	else if ( t == timer_head )
	{
		LOG_DEBUG("    timer_head %s to %s\n", name(timer_head), name(t->next));
		timer_head = t->next;
	}

	if ( t->next )
	{
		if ( t->next->prev != t )
			stop("t->next %s ->prev %s != t?!", name(t->next), name(t->next->prev));

		LOG_DEBUG ("    t->next->prev %s to %s\n", name(t->next->prev), name(t->prev));
		t->next->prev = t->prev;
	}
	else if ( t == timer_tail )
	{
		LOG_DEBUG("    timer_tail %s to %s\n", name(timer_tail), name(t->prev));
		timer_tail = t->prev;
	}

	t->prev = t->next = NULL;
	RETURN;
}

// Detach a timer from the active list
void _timer_detach (struct timer *t, const char *n)
{
	learn(t, n);
	ENTER("t %s", name(t));

	timer_remove(t);

	RETURN;
}

static inline void timer_insert (struct timer *t)
{
	ENTER("t %s", name(t));

	// empty list: set timer_head and timer_tail
	if ( !timer_head )
	{
		LOG_DEBUG("    timer_head %s / timer_tail %s to %s\n", name(timer_tail), name(timer_head), name(t));
		timer_head = timer_tail = t;
		t->prev = t->next = NULL;
		RETURN;
	}

	// smaller than timer_head, insert before timer_head
	if ( t->due <= timer_head->due )
	{
		if ( timer_head->prev )
			stop("timer_head %s has prev %s?!", name(timer_head), name(timer_head->prev));

		LOG_DEBUG("    t %s due %d <= timer_head %s due %d\n",
		          name(t), due(t->due), name(timer_head), due(timer_head->due));

		t->prev = timer_head->prev; // NULL
		t->next = timer_head;
		timer_head->prev = t;
		timer_head = t;
		RETURN;
	}

	// greater than timer_tail, append after timer_tail
	if ( t->due >= timer_tail->due )
	{
		if ( timer_tail->next )
			stop("timer_tail %s has next %s?!", name(timer_tail), name(timer_tail->next));

		LOG_DEBUG("    t %s due %d >= timer_tail %s due %d\n",
		          name(t), due(t->due), name(timer_tail), due(timer_tail->due));

		t->prev = timer_tail;
		t->next = timer_tail->next; // NULL
		timer_tail->next = t;
		timer_tail = t;
		RETURN;
	}

	LOG_DEBUG("    t %s due %d to insert in middle\n", name(t), due(t->due));
	dump("    list pre-walk", timer_head);

	// walk the list to insert
	int cycles = 10;
	struct timer *walk = timer_head;
	while ( walk->next )
	{
		if ( walk->due > walk->next->due )
			stop("list order fucked up, walk %s due %d > next %s due %d", 
			     name(walk), due(walk->due), name(walk->next), due(walk->next->due));

		LOG_DEBUG("    check: walk %s due %d and walk->next %s due %d: ",
		          name(walk), due(walk->due), name(walk->next), due(walk->next->due));

		if ( t->due >= walk->due && t->due <= walk->next->due )
		{
			LOG_DEBUG("insert here\n");
			t->prev = walk;
			t->next = walk->next;
			LOG_DEBUG("    t->prev to walk %s, t->next to walk->next %s\n", 
			          name(walk), name(walk->next));
			if ( walk->next )
				walk->next->prev = t;
			else
				timer_tail = t;
			walk->next = t;
			dump("    list post->insert", timer_head);
			RETURN;
		}

		if ( cycles-- < 1 )
			stop("circular list during insert");

		LOG_DEBUG("walk on\n");
		walk = walk->next;
	}

	stop("shouldn't get here");
}

// Attach or re-attach a timer to the active list
void _timer_attach (struct timer *t, clocks_t due_in, timer_callback_fn func, void *arg,
                    const char *n)
{
	learn(t, n);
	ENTER("t %s, due_in %llu, func %p, arg %p", name(t), due_in, func, arg);

	clocks_t now = get_clocks();

	// add the delay to the timeofday and update the due date and other fields
	t->due = now + due_in;
	if ( func != t->func ) t->func = func;
	if ( arg  != t->arg  ) t->arg  = arg;

	// don't rely on the caller doing this
	timer_detach(t);

	if ( (timer_tail && !timer_head) && (!timer_tail && timer_head) )
		stop("timer_tail %s timer_head %s mismatch!", name(timer_tail), name(timer_head));

	timer_insert(t);

	RETURN;
}

////// Lower API

// returns a pointer to the next active timer's due date, or TIMER_LIST_EMPTY if the
// list is empty
clocks_t timer_head_due_at (void)
{
	ENTER();
	if ( !timer_head )
		RETURN_ERRNO_VALUE (ENOENT, "TIMER_LIST_EMPTY %llu", TIMER_LIST_EMPTY);

	RETURN_ERRNO_VALUE (0, "%llu", timer_head->due);
}

// returns the difference between the call time and the next
// active timer's due date, or TIMER_LIST_EMPTY if the list is empty
clocks_t timer_head_due_in (void)
{
	ENTER();
	if ( !timer_head )
		RETURN_ERRNO_VALUE (ENOENT, "%llu", TIMER_LIST_EMPTY);

	register clocks_t ret = timer_head->due - get_clocks();

	RETURN_ERRNO_VALUE (0, "%llu", ret);
}

// returns nonzero if the active list is not empty, and the due date of the
// head item has already passed
int timer_head_due_now (void)
{
	ENTER();
	if ( !timer_head )
		RETURN_ERRNO_VALUE (0, "%d", 0);

	register int ret = get_clocks() >= timer_head->due;

	RETURN_ERRNO_VALUE (0, "%d", ret);
}


static inline struct timer *timer_extract (void)
{
	ENTER();

	if ( !timer_tail )
		stop("!timer_tail (timer_head %s), empty", name(timer_head));

	struct timer *t = timer_head;
	LOG_DEBUG("timer_head %s\n", name(timer_head));

	timer_detach(t);

	RETURN_ERRNO_VALUE (0, "%p", t);
}

// upcall the head timer; caller must use the above functions to ensure it's
// the correct time to do so.
void timer_head_call (void)
{
	ENTER();
	if ( !timer_head )
		RETURN;

	// detach the head 
	struct timer *t = timer_extract();

	// safety check on the callback function
	if ( !t->func )
		RETURN;
	
	// stats & profiling
	t->call_count++;
	clocks_t clocks = get_clocks();
	t->call_late = clocks - t->due;

	// upcall
	t->func(t->arg);

	// finish profiling
	t->call_clocks += get_clocks() - clocks;
	RETURN;
}

// check the current time and dispatch any which have come due.  max 
// can be used to limit the number of timers to run; use -1 for no 
// (practical) limit
void timer_check (int max)
{
	ENTER("max %d", max);
	while ( timer_head_due_now() && max-- )
		timer_head_call();
	RETURN;
}


#ifdef UNIT_TEST
struct timer  red    = { .name = "red",    .due = 10 };
struct timer  orange = { .name = "orange", .due = 20 };
struct timer  yellow = { .name = "yellow", .due = 30 };
struct timer  green  = { .name = "green",  .due = 40 };
struct timer  blue   = { .name = "blue",   .due = 50 };
struct timer  indigo = { .name = "indigo", .due = 60 };
struct timer  violet = { .name = "violet", .due = 70 };

struct timer  order  = { .name = "order",  .due = 35 };


int main (int argc, char **argv)
{
	log_dupe(stdout);
	module_level = LOG_LEVEL_DEBUG;

	dump ("init", timer_head);

	t0 = 1;

	LOG_DEBUG("\n");   timer_insert (&blue);    
	LOG_DEBUG("\n");   timer_insert (&yellow);  
	LOG_DEBUG("\n");   timer_insert (&green);   
	LOG_DEBUG("\n");   timer_insert (&violet);  
	LOG_DEBUG("\n");   timer_insert (&indigo);  
	LOG_DEBUG("\n");   timer_insert (&red);     
	LOG_DEBUG("\n");   timer_insert (&orange);  

	srand(1);
	int cycles = 50;
	struct timer *node;

	LOG_DEBUG("\n");
	while ( (node = timer_extract()) && cycles-- )
	{
		if ( cycles > 25 )
		{
			node->due += (rand() % 15) + 5;
			LOG_DEBUG("\nnode %s due to %d\n", name(node), due(node->due));
			timer_insert(node);
		}

		if ( timer_head )
			LOG_DEBUG("timer_head {%s:%s:%d:%s}, ", name(timer_head->prev),
			       name(timer_head), due(timer_head->due), name(timer_head->next));
		else
			LOG_DEBUG("!timer_head?!\n");

		if ( timer_tail )
			LOG_DEBUG("timer_tail {%s:%s:%d:%s}\n", name(timer_tail->prev),
			       name(timer_tail), due(timer_tail->due), name(timer_tail->next));
		else
			LOG_DEBUG("!timer_tail?!\n");

		LOG_DEBUG("\n");
	}

	LOG_DEBUG("\n");   timer_insert (&red);     
	LOG_DEBUG("\n");   timer_insert (&green);   
	LOG_DEBUG("\n");   timer_insert (&yellow);  
	LOG_DEBUG("\n");   timer_insert (&orange);  
	LOG_DEBUG("\n");   timer_insert (&indigo);  
	LOG_DEBUG("\n");   timer_insert (&violet);  
	LOG_DEBUG("\n");   timer_insert (&blue);    

	while ( timer_extract() )
		;

	return 0;
}
#endif


/* Ends    : src/lib/timer.c */
