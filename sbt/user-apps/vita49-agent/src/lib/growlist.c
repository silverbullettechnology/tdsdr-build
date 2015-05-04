/** Growable list - a variable-size list of object pointers
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <lib/log.h>
#include <lib/gen.h>
#include <lib/growlist.h>


LOG_MODULE_STATIC("lib_growlist", LOG_LEVEL_WARN);


/** Allocate and initialize growlist
 *
 *  \param init Initial size of pointer array
 *  \param grow Increment size of pointer array
 *
 *  \return The initialized growlist, or NULL on error
 */
struct growlist *growlist_alloc (size_t size, size_t grow)
{
	ENTER("size %zu, grow %zu", size, grow);
	struct growlist *list = malloc(sizeof(struct growlist));
	if ( !list )
		RETURN_VALUE("%p", NULL);

	growlist_init(list, size, grow);

	RETURN_ERRNO_VALUE(0, "%p", list);
}


/** Initialize a growlist
 *
 *  \param list Growlist to initialize
 *  \param init Initial size of pointer array
 *  \param grow Increment size of pointer array
 *
 *  \return 0 on success, <0 on error
 */
int growlist_init (struct growlist *list, size_t size, size_t grow)
{
	ENTER("list %p, size %zu, grow %zu", list, size, grow);
	if ( !list )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	/* allocate array if nonzero initial size */
	list->base = NULL;
	if ( size && !(list->base = malloc(size * sizeof(void *))) )
		RETURN_VALUE("%d", -1);

	if ( size )
		memset (list->base, 0, size * sizeof(void *));

	list->size = size;
	list->grow = grow;
	list->used = 0;
	list->curs = 0;

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Make a copy of a a growlist 
 *
 *  \param dst  Growlist to copy to
 *  \param src  Growlist to copy from
 *
 *  \return 0 on success, <0 on error
 */
int growlist_copy (struct growlist *dst, struct growlist *src)
{
	ENTER("dst %p, src %p", dst, src);
	if ( !dst || !src )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	/* allocate array if nonzero src size */
	dst->size = src->size;
	dst->grow = src->grow;
	dst->used = src->used;
	dst->curs = src->curs;
	dst->base = NULL;
	if ( src->size && !(dst->base = malloc(src->size * sizeof(void *))) )
		RETURN_VALUE("%d", -1);

	if ( src->size )
		memcpy (dst->base, src->base, src->size * sizeof(void *));

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Grow a growlist 
 *
 *  If size is >0, it gives a target size to grow the list to.  If this is smaller than 
 *  the current size, an error is returned.  If this is equal to the current size, no 
 *  action is taken.
 *
 *  If size is 0, the list is grown by the step size list->grow.
 *
 *  \param list Growlist to grow
 *  \param size Optional target size
 *
 *  \return 0 on success, <0 on error
 */
int growlist_grow (struct growlist *list, size_t size)
{
	ENTER("list %p, size %zu", list, size);
	if ( !list )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	/* target size not given: grow by step size */
	if ( size <= 0 )
		size = list->size + list->grow;

	/* Check for too small or equal size */
	if ( size < list->size )
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);
	if ( size == list->size )
		RETURN_ERRNO_VALUE(0, "%d", 0);

	/* allocate new pointer array */
	void **temp = malloc(size * sizeof(void *));
	if ( !temp )
		RETURN_VALUE("%d", -1);
	
	/* Copy existing entries if present */
	if ( list->base )
		memcpy (temp, list->base, list->used * sizeof(void *));

	/* Initialize new entries to NULL, free old list and replace with new */
	memset (temp + list->used, 0, (size - list->used) * sizeof(void *));
	free (list->base);
	list->base = temp;
	list->size = size;
	list->curs = 0;

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Finalize a growlist
 *
 *  If func is give, call func for over each node in the growlist, passing the node and
 *  supplied context pointer.  Then free the pointer list and reset the growlist struct.
 *
 *  \note This does not free the passed growlist, which may be statically allocated or a
 *  member in some other structure.  It is used to "empty" a growlist properly, so it can
 *  be freed without leaking, or reused.
 *
 *  \param list Growlist to finalize
 *  \param func Destructor function 
 *  \param data Context for func()
 */
void growlist_done (struct growlist *list, gen_iter_fn func, void *data)
{
	ENTER("list %p, func %p, data %p", list, func, data);
	if ( !list )
		RETURN_ERRNO(EFAULT);

	/* Iterate optional destructor function */
	if ( func )
		for ( list->curs = 0; list->curs < list->used; list->curs++ )
			func(list->base[list->curs], data);

	/* Free pointer list and reset struct, POSIX says free(NULL) is safe... */
	free (list->base);
	list->base = NULL;
	list->size = 0;
	list->used = 0;
	list->curs = 0;

	RETURN_ERRNO(0);
}


/** Empty a growlist
 *
 *  This is a low-cost way to empty an allocated growlist of its items without any free
 *  or alloc calls.
 *
 *  \param list Growlist to empty
 */
void growlist_empty (struct growlist *list)
{
	ENTER("list %p", list);
	if ( !list )
		RETURN_ERRNO(EFAULT);

	/* Reset used count and cursor, zero pointer list */
	list->used = 0;
	list->curs = 0;
	if ( list->base && list->size )
		memset (list->base, 0, list->size * sizeof(void *));

	RETURN_ERRNO(0);
}


/** Swap the contents of two growlists
 *
 *  \param a Growlist to copy b to
 *  \param b Growlist to copy a to
 */
void growlist_swap (struct growlist *a, struct growlist *b)
{
	ENTER("a %p, b %p", a, b);
	if ( !a || !b )
		RETURN_ERRNO(EFAULT);

	struct growlist t;
	memcpy (&t, a,  sizeof(struct growlist));
	memcpy (a,  b,  sizeof(struct growlist));
	memcpy (b,  &t, sizeof(struct growlist));

	RETURN_ERRNO(EFAULT);
}


/** Reset the internal cursor
 *
 *  Iterative growlist functions use a persistent cursor in the growlist struct to allow
 *  restartable iteration.  The caller can, for example, iterate through 5 nodes at a
 *  time, or use search with a compare function which matches multiple nodes.
 *
 *  Functions which modify the list (adding or removing nodes) will reset the cursor to 
 *  the list head.  If the list is emptied, the cursor is set to NULL, and iterative 
 *  functions will have no effect.
 *
 *  This function resets the cursor 
 *
 *  \param list List to reset
 */
void growlist_reset (struct growlist *list)
{
	ENTER("list %p", list);
	if ( !list )
		RETURN_ERRNO(EFAULT);

	list->curs = 0;

	RETURN_ERRNO(0);
}


/* Check a growlist and grow it if necessary */
static int growlist_check (struct growlist *list, size_t grow)
{
	ENTER("list %p, grow %zu", list, grow);
	if ( !list )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	if ( grow <= 0 )
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);

	/* list has enough room to hold new items: no action necessary */
	if ( list->used + grow <= list->size )
		RETURN_ERRNO_VALUE(0, "%d", 0);

	/* grow by at least the step size */
	if ( grow < list->grow )
		grow = list->grow;
	
	/* attempt to grow list */
	if ( growlist_grow(list, list->size + grow) )
		RETURN_VALUE("%d", -1);

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Append a node as a list tail
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to append onto
 *  \param node Node to be appended
 *
 *  \return 0 on success, <0 on error
 */
int growlist_append (struct growlist *list, void *node)
{
	ENTER("list %p, node %p", list, node);
	if ( !list || !node )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( growlist_check(list, 1) )
		RETURN_VALUE("%d", -1);

	list->base[list->used] = node;
	list->used++;
	list->curs = 0;

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Pop an item off a growlist
 *
 *  A growlist can be cheaply used as a stack, since it keeps an index of where its last
 *  item is.  The pop operation removes the last item pushed and returns it; if the list
 *  is empty NULL is returned
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to append onto
 *  \param list List to pop from
 *
 *  \return Node pointer popped off the list, or NULL if list is empty
 */
void *growlist_pop (struct growlist *list)
{
	ENTER("list %p", list);
	if ( !list )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	/* early-out: can't pop from an empty list */
	if ( !list->used )
		RETURN_ERRNO_VALUE(ENOENT, "%p", NULL);

	/* detaching the last node is easy */
	list->used--;
	void *node = list->base[list->used];
	list->base[list->used] = NULL;
	list->curs = 0;

	RETURN_ERRNO_VALUE(EFAULT, "%p", node);
}


/** Insert a node into an ordered list
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to insert into
 *  \param comp Compare function
 *  \param uniq If nonzero, do not allow duplicates
 *  \param node Node to be inserted
 *
 *  \return 0 on success, <0 on failure
 */
int growlist_insert (struct growlist *list, gen_comp_fn comp, int uniq, void *node)
{
	ENTER("list %p, comp %p, uniq %d, node %p", list, comp, uniq, node);
	if ( !list || !comp || !node )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	/* early-out: if the list is empty, or there's at least one node and the new one is
	 * greater than the last existing, then just append */
	if ( !list->used || comp(node, list->base[list->used - 1]) >= 0 )
	{
		int ret = growlist_append(list, node);
		RETURN_VALUE("%d", ret);
	}

	/* Otherwise we have to search the (ordered) list for the correct insert point */
	size_t insert;
	for ( insert = 0; insert < list->used; insert++ )
		if ( comp(node, list->base[insert]) <= 0 )
			break;

	/* verify duplicate if uniq */
	if ( uniq && comp(node, list->base[list->curs]) == 0 )
		RETURN_ERRNO_VALUE(0, "%d", 0);
	
	/* grow the list */
	if ( growlist_check(list, 1) )
		RETURN_VALUE("%d", -1);

	/* shift the list towards the end */
	size_t last;
	for ( last = list->used; last > insert; last-- )
		list->base[last] = list->base[last - 1];

	/* insert new node and fix up pointers */
	list->base[insert] = node;
	list->used++;
	list->curs = 0;

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Remove a node from a list
 *
 *  \note The node is not freed, caller must do that separately
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to remove node from
 *  \param node Node to be removed
 */
void growlist_remove (struct growlist *list, void *node)
{
	ENTER("list %p, node %p", list, node);
	if ( !list || !node )
		RETURN_ERRNO(EFAULT);

	/* early-out: can't remove from an empty list */
	if ( !list->used )
		RETURN_ERRNO(ENOENT);

	/* early-out: if it's the last element no shift required */
	if ( list->base[list->used - 1] == node )
	{
		list->base[list->used - 1] = NULL;
		list->used--;
		list->curs = 0;
		RETURN_ERRNO(0);
	}

	/* Otherwise have to search: if we find it, decrement the used counter and shift the
	 * rest of the array down, and return success */
	for ( list->curs = 0; list->curs < list->used; list->curs++ )
		if ( list->base[list->curs] == node )
		{
			list->used--;
			for ( ; list->curs < list->used; list->curs++ )
				list->base[list->curs] = list->base[list->curs + 1];
			list->base[list->used] = NULL;
			list->curs = 0;
			RETURN_ERRNO(0);
		}

	/* Not found: return ENOENT */
	RETURN_ERRNO(ENOENT);
}


/** Sort a list according to a comparison function
 *
 *  \param list List to sort
 *  \param comp Compare function
 */
void growlist_sort (struct growlist *list, gen_comp_fn comp)
{
	/* From qsort(3): The actual arguments to this function are "pointers to pointers to
	 * void", but compare arguments are "pointers to var", hence the following cast plus
	 * dereference */
	int shim (const void *a, const void *b)
	{
		return comp (*(void * const *)a, *(void * const *)b);
	}

	ENTER("list %p, comp %p", list, comp);
	if ( !list || !comp )
		RETURN_ERRNO(EFAULT);

	if ( list->used < 2 )
		RETURN_ERRNO(0);

	/* qsort the list using a shim around the passed compare function */
	qsort (list->base, list->used - 1, sizeof(void *), shim);

	RETURN_ERRNO(0);
}


/** Sort a list according to a comparison function
 *
 *  \param list List to sort
 *  \param comp Compare function
 */
void growlist_uniq (struct growlist *list, gen_comp_fn comp, gen_iter_fn dest, void *data)
{

	/* Iterate optional destructor function */
	if ( dest )
		for ( list->curs = 0; list->curs < list->used; list->curs++ )
			dest(list->base[list->curs], data);
}


/** Search through a list by value
 *
 *  Iterate through the list; if a compare function is supplied, call it for each node,
 *  returning the next node for which comp returned 0.  If no function is supplied (comp
 *  passed as NULL) find whether node is in the list.
 *
 *  \note the cursor is persistent across searches and iterations; see growlist_reset(),
 *  so the caller can match multiple nodes with a custom compare function
 *
 *  \param list List to search
 *  \param comp Compare function, or NULL for pointer compare
 *  \param node Node to search for
 *
 *  \return Node matching the passed node according to comp
 */
void *growlist_search (struct growlist *list, gen_comp_fn comp, const void *node)
{
	ENTER("list %p, comp %p, node %p", list, comp, node);
	if ( !list || !node )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	if ( !list->used || list->curs >= list->used )
		RETURN_ERRNO_VALUE(0, "%p", NULL);

	void *curr;
	while ( list->curs < list->used )
	{
		curr = list->base[list->curs++];
		if ( comp && !comp(curr, node) )
			RETURN_ERRNO_VALUE(0, "%p", curr);
		else if ( !comp && curr == node )
			RETURN_ERRNO_VALUE(0, "%p", curr);
	}

	RETURN_ERRNO_VALUE(ENOENT, "%p", NULL);
}


/** Return the next node in the list, post-incrementing the cursor 
 *
 *  Note: the cursor is persistent across searches and iterations; see growlist_reset()
 *
 *  \param list List to return nodes from
 *
 *  \return Next node in the list
 */
void *growlist_next (struct growlist *list)
{
	ENTER("list %p", list);
	if ( !list )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	if ( !list->used || list->curs >= list->used )
		RETURN_ERRNO_VALUE(0, "%p", NULL);

	void *node = list->base[list->curs++];
	RETURN_ERRNO_VALUE(0, "%p", node);
}


#ifdef UNIT_TEST
#include <stdio.h>
void dump_list (struct growlist *list)
{
	growlist_reset(list);
	printf ("{");
	char *c;
	while ( (c = growlist_next(list)) )
		printf (" '%s'", c);
	printf (" }\n");
}

int destroy (void *node, void *data)
{
	free(node);
	return 0;
}

int main (int argc, char **argv)
{
	module_level = LOG_LEVEL_DEBUG;
	log_dupe (stderr);
	setbuf   (stderr, NULL);

	struct growlist  l = GROWLIST_INIT;
	char           *c;

	printf ("Simple in-order append: ");
	growlist_append(&l, strdup("222"));
	growlist_append(&l, strdup("555"));
	growlist_append(&l, strdup("888"));
	dump_list(&l);

	printf ("Ordered-insert using strcmp: ");
	growlist_insert(&l, gen_strcmp, 0, strdup("111"));
	growlist_insert(&l, gen_strcmp, 0, strdup("444"));
	growlist_insert(&l, gen_strcmp, 0, strdup("999"));
	dump_list(&l);

	printf ("Get-next iterate: ");
	dump_list(&l);

	printf ("Random-order search and delete: ");
	growlist_reset(&l);
	if ( (c = growlist_search(&l, gen_strcmp, "444")) )
	{
		growlist_remove(&l, c);
		free(c);
	}
	growlist_reset(&l);
	if ( (c = growlist_search(&l, gen_strcmp, "888")) )
	{
		growlist_remove(&l, c);
		free(c);
	}
	growlist_reset(&l);
	if ( (c = growlist_search(&l, gen_strcmp, "222")) )
	{
		growlist_remove(&l, c);
		free(c);
	}
	growlist_reset(&l);
	if ( (c = growlist_search(&l, gen_strcmp, "666")) )
	{
		growlist_remove(&l, c);
		free(c);
	}
	dump_list(&l);

	printf ("Destroy:\n");
	growlist_done(&l, destroy, NULL);

	return 0;
}
#endif


/* Ends    : child.c */
