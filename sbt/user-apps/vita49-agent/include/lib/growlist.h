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
#ifndef INCLUDE_LIB_GROWLIST_H
#define INCLUDE_LIB_GROWLIST_H
#include <lib/gen.h>

struct growlist
{
	size_t   size;
	size_t   grow;
	size_t   used;
	size_t   curs;
	void   **base;
};
#define GROWLIST_INIT { .size = 0, .used = 0, .grow = 4, .curs = 0, .base = NULL }


/** Return the allocated current size of a growlist
 *
 *  \param list List to return the size of
 *
 *  \return The size of the growlist
 */
static inline size_t growlist_size (struct growlist *list)
{
	return list->size;
}


/** Return the number of objects in a growlist
 *
 *  \param list List to return the count of objects in
 *
 *  \return The number of objects in the growlist
 */
static inline size_t growlist_used (struct growlist *list)
{
	return list->used;
}


/** Allocate and initialize growlist
 *
 *  \param init Initial size of pointer array
 *  \param grow Increment size of pointer array
 *
 *  \return The initialized growlist, or NULL on error
 */
struct growlist *growlist_alloc (size_t size, size_t grow);


/** Initialize a growlist
 *
 *  \param list Growlist to initialize
 *  \param init Initial size of pointer array
 *  \param grow Increment size of pointer array
 *
 *  \return 0 on success, <0 on error
 */
int growlist_init (struct growlist *list, size_t size, size_t grow);


/** Make a copy of a a growlist 
 *
 *  \param dst  Growlist to copy to
 *  \param src  Growlist to copy from
 *
 *  \return 0 on success, <0 on error
 */
int growlist_copy (struct growlist *dst, struct growlist *src);


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
int growlist_grow (struct growlist *list, size_t size);


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
void growlist_done (struct growlist *list, gen_iter_fn func, void *data);


/** Empty a growlist
 *
 *  \param list Growlist to empty
 */
void growlist_empty (struct growlist *list);


/** Swap the contents of two growlists
 *
 *  \param a Growlist to copy b to
 *  \param b Growlist to copy a to
 */
void growlist_swap (struct growlist *a, struct growlist *b);


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
void growlist_reset (struct growlist *list);


/** Append a node as a list tail
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to append onto
 *  \param node Node to be appended
 *
 *  \return 0 on success, <0 on error
 */
int growlist_append (struct growlist *list, void *node);


/** Push an item onto a growlist
 *
 *  A growlist can be cheaply used as a stack, since it keeps an index of where its last
 *  item is.  The push operation is equivalent to _append, so is just an inline wrapper
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to push into
 *  \param node Node to be pushed
 *
 *  \return 0 on success, <0 on error
 */
static inline int growlist_push (struct growlist *list, void *node)
{
	return growlist_append (list, node);
}


/** Pop an item off a growlist
 *
 *  A growlist can be cheaply used as a stack, since it keeps an index of where its last
 *  item is.  The pop operation removes the last item pushed and returns it; if the list
 *  is empty NULL is returned
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to pop from
 *
 *  \return Node pointer popped off the list, or NULL if list is empty
 */
void *growlist_pop (struct growlist *list);


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
int growlist_insert (struct growlist *list, gen_comp_fn comp, int uniq, void *node);


/** Insert a node into an ordered list, enforcing unique elements
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to append onto
 *  \param comp Compare function
 *  \param node Node to be inserted
 *
 *  \return 0 on success, <0 on failure
 */
static inline int growlist_insert_uniq (struct growlist *list, gen_comp_fn comp, void *node)
{
	return growlist_insert(list, comp, 1, node);
}


/** Insert a node into an ordered list, allowing duplicates
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to append onto
 *  \param comp Compare function
 *  \param node Node to be inserted
 *
 *  \return 0 on success, <0 on failure
 */
static inline int growlist_insert_dupe (struct growlist *list, gen_comp_fn comp, void *node)
{
	return growlist_insert(list, comp, 0, node);
}


/** Remove a node from a list
 *
 *  \note The node is not freed, caller must do that separately
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to remove node from
 *  \param node Node to be removed
 */
void growlist_remove (struct growlist *list, void *node);


/** Sort a list according to a comparison function
 *
 *  \param list List to sort
 *  \param comp Compare function
 */
void growlist_sort (struct growlist *list, gen_comp_fn comp);


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
void *growlist_search (struct growlist *list, gen_comp_fn comp, const void *node);


/** Search through a list by pointer
 *
 *  Iterate through the list; return node if it's in the list, NULL otherwise
 *
 *  \note Resets the cursor; see growlist_reset()
 *
 *  \param list List to search
 *  \param node Node to search for
 *
 *  \return Node matching the passed node according to comp
 */
static inline void *growlist_contains (struct growlist *list, void *node)
{
	growlist_reset(list);
	return growlist_search(list, NULL, node);
}


/** Return the next node in the list, post-incrementing the cursor 
 *
 *  Note: the cursor is persistent across searches and iterations; see growlist_reset()
 *
 *  \param list List to return nodes from
 *
 *  \return Next node in the list
 */
void *growlist_next (struct growlist *list);


#endif /* INCLUDE_LIB_GROWLIST_H */
/* Ends */
