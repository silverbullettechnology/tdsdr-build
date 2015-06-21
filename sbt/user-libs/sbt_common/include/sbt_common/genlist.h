/** Generic doubly-linked-list
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
#ifndef INCLUDE_LIB_GENLIST_H
#define INCLUDE_LIB_GENLIST_H
#include <string.h>

#include <sbt_common/gen.h>
#include <sbt_common/log.h>

/* Generic list head/tail struct */
struct genlist
{
	void *head;
	void *tail;
	void *walk;
};
#define GENLIST_INIT { .head = NULL, .tail = NULL, .walk = NULL }

 
/** Allocate a node 
 *
 *  Caller must free the object with genlist_free() 
 *
 *  \param size Size in bytes of client object
 *
 *  \return The user portion of the allocated object
 */
void *genlist_alloc (int size);


/** Allocate a genlist node and memcpy an object into it
 *
 *  \param src Source string
 */
static inline char *genlist_memdup (const char *src, int size)
{
	char *temp;
	if ( (temp = genlist_alloc(size)) )
		memcpy(temp, src, size);
	return temp;
}

/** Allocate a genlist node and strcpy a string into it
 *
 *  \param src Source string
 */
static inline char *genlist_strdup (const char *src)
{
	return genlist_memdup(src, strlen(src) + 1);
}


/** Returns nonzero if the list is empty
 *
 *  \param list List to be tested
 */
static inline int genlist_empty (struct genlist *list)
{
	return list->head == NULL;
}


/** Free a node allocated with genlist_alloc
 *
 *  \param node Node to be freed
 */
void genlist_free (void *node);


/** Iterate through a list, genlist_free()ing each node
 *
 *  \param list List to be destroyed
 */
void genlist_destroy (struct genlist *list);


/** Reset the internal cursor
 *
 *  Iterative genlist functions use a persistent cursor in the genlist struct to allow
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
void genlist_reset (struct genlist *list);


/** Insert a node as a list head
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to insert into
 *  \param node Node to be inserted as head
 */
void genlist_prepend (struct genlist *list, void *node);


/** Append a node as a list tail
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to append onto
 *  \param node Node to be appended
 */
void genlist_append (struct genlist *list, void *node);


/** Detach and return the node at list head
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to detach from
 *
 *  \return Node at head of list, or NULL if list is empty
 */
void *genlist_head (struct genlist *list);


/** Detach and return the node at list tail
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to detach from
 *
 *  \return Node at tail of list, or NULL if list is empty
 */
void *genlist_tail (struct genlist *list);


/** Insert a node into an ordered list
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to insert into
 *  \param comp Compare function
 *  \param uniq If nonzero, do not allow duplicates
 *  \param node Node to be inserted
 */
void genlist_insert (struct genlist *list, gen_comp_fn comp, int uniq, void *node);


/** Insert a node into an ordered list, enforcing unique elements
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to append onto
 *  \param comp Compare function
 *  \param node Node to be appended
 */
static inline void genlist_insert_uniq (struct genlist *list, gen_comp_fn comp, void *node)
{
	genlist_insert(list, comp, 1, node);
}


/** Insert a node into an ordered list, allowing duplicates
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to append onto
 *  \param comp Compare function
 *  \param node Node to be appended
 */
static inline void genlist_insert_dupe (struct genlist *list, gen_comp_fn comp, void *node)
{
	genlist_insert(list, comp, 0, node);
}


/** Remove a node from a list
 *
 *  \note The node is not freed, caller must do that afterwards with genlist_free()
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to remove node from
 *  \param node Node to be removed
 */
void genlist_remove (struct genlist *list, void *node);


/** Remove a node from a list and free it
 *
 *  \param list List to remove node from
 *  \param node Node to remove and free
 */
static inline void genlist_remove_and_free (struct genlist *list, void *node)
{
	genlist_remove (list, node);
	genlist_free   (node);
}


/** Sort a list according to a comparison function
 *
 *  \param list List to sort
 *  \param comp Compare function
 *  \param uniq Strip duplicates 
 */
void genlist_sort (struct genlist *list, gen_comp_fn comp, int uniq);


/** Search through a list by value
 *
 *  Iterate through the list; if a compare function is supplied, call it for each node,
 *  returning the next node for which comp returned 0.  If no function is supplied (comp
 *  passed as NULL) find whether node is in the list.
 *
 *  \note the cursor is persistent across searches and iterations; see genlist_reset(),
 *  so the caller can match multiple nodes with a custom compare function
 *
 *  \param list List to search
 *  \param comp Compare function, or NULL for pointer compare
 *  \param node Node to search for
 *
 *  \return Node matching the passed node according to comp
 */
void *genlist_search (struct genlist *list, gen_comp_fn comp, void *node);


/** Search through a list by pointer
 *
 *  Iterate through the list; return node if it's in the list, NULL otherwise
 *
 *  \note Resets the cursor; see genlist_reset()
 *
 *  \param list List to search
 *  \param node Node to search for
 *
 *  \return Node matching the passed node according to comp
 */
static inline void *genlist_contains (struct genlist *list, void *node)
{
	genlist_reset(list);
	return genlist_search(list, NULL, node);
}


/** Return the next node in the list, post-incrementing the cursor 
 *
 *  Note: the cursor is persistent across searches and iterations; see genlist_reset()
 *
 *  \param list List to return nodes from
 *
 *  \return Next node in the list
 */
void *genlist_next (struct genlist *list);


#endif /* INCLUDE_LIB_GENLIST_H */
/* Ends */
