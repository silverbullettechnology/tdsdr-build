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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <lib/log.h>
#include <lib/genlist.h>


LOG_MODULE_STATIC("lib_genlist", LOG_LEVEL_WARN);


/* List accounting structure, hidden from caller.  Allocated objects are prefixed with a
 * node struct and the */
struct node
{
	void *next;
	void *prev;
};


#define node_from_user(user) (((struct node *)user) - 1)
#define user_from_node(node) ((void *)(node + 1))


/** Allocate a node 
 *
 *  Caller must free the object with genlist_free() 
 *
 *  \param size Size in bytes of client object
 *
 *  \return The user portion of the allocated object
 */
void *genlist_alloc (int size)
{
	ENTER("size %d", size);
	struct node *node = malloc(sizeof(struct node) + size);
	if ( !node )
		RETURN_VALUE("%p", NULL);

	node->next = NULL;
	node->prev = NULL;

	RETURN_ERRNO_VALUE(0, "%p", user_from_node(node));
}


/** Free a node allocated with genlist_alloc
 *
 *  \param node Node to be freed
 */
void genlist_free (void *node)
{
	ENTER("node %p", node);
	if ( !node )
		RETURN_ERRNO(EFAULT);

	errno = 0;
	free(node_from_user(node));

	RETURN;
}


/** Iterate through a list, genlist_free()ing each node
 *
 *  \param list List to be destroyed
 */
void genlist_destroy (struct genlist *list)
{
	ENTER("list %p", list);
	if ( !list )
		RETURN_ERRNO(EFAULT);

	/* Reset cursor */
	list->walk = list->head;

	void *curr;
	errno = 0;
	while ( list->walk )
	{
		curr = list->walk;
		list->walk = node_from_user(curr)->next;
		genlist_free(curr);
	}

	list->head = NULL;
	list->tail = NULL;
	list->walk = NULL;

	RETURN;
}


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
void genlist_reset (struct genlist *list)
{
	ENTER("list %p", list);
	if ( !list )
		RETURN_ERRNO(EFAULT);

	/* Reset cursor */
	list->walk = list->head;

	RETURN;
}


/** Insert a node as a list head
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to prepend onto
 *  \param node Node to be inserted as head
 */
void genlist_prepend (struct genlist *list, void *node)
{
	ENTER("list %p, node %p", list, node);
	if ( !list || !node )
		RETURN_ERRNO(EFAULT);

	if ( node_from_user(node)->next || node_from_user(node)->prev )
		LOG_WARN("node with next/prev will leak memory\n");

	node_from_user(node)->next = list->head;
	node_from_user(node)->prev = NULL;
	if ( list->head )
		node_from_user(list->head)->prev = node;
	list->head = node;
	if ( !list->tail )
		list->tail = list->head;

	/* Reset cursor */
	list->walk = list->head;

	RETURN;
}


/** Append a node as a list tail
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to append onto
 *  \param node Node to be appended
 */
void genlist_append (struct genlist *list, void *node)
{
	ENTER("list %p, node %p", list, node);
	if ( !list || !node )
		RETURN_ERRNO(EFAULT);

	if ( node_from_user(node)->next || node_from_user(node)->prev )
		LOG_WARN("node with next/prev will leak memory\n");

	node_from_user(node)->prev = list->tail;
	node_from_user(node)->next = NULL;
	if ( list->tail )
		node_from_user(list->tail)->next = node;
	list->tail = node;
	if ( !list->head )
		list->head = list->tail;

	/* Reset cursor */
	list->walk = list->head;

	RETURN;
}


/** Detach and return the node at list head
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to detach from
 *
 *  \return Node at head of list, or NULL if list is empty
 */
void *genlist_head (struct genlist *list)
{
	ENTER("list %p", list);
	if ( !list )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	if ( genlist_empty(list) )
		RETURN_ERRNO_VALUE(ENOENT, "%p", NULL);

	void *node = list->head;
	genlist_remove(list, node);

	RETURN_ERRNO_VALUE(0, "%p", node);
}


/** Detach and return the node at list tail
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to detach from
 *
 *  \return Node at tail of list, or NULL if list is empty
 */
void *genlist_tail (struct genlist *list)
{
	ENTER("list %p", list);
	if ( !list )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	if ( genlist_empty(list) )
		RETURN_ERRNO_VALUE(ENOENT, "%p", NULL);

	void *node = list->tail;
	genlist_remove(list, node);

	RETURN_ERRNO_VALUE(0, "%p", node);
}


/** Insert a node into an ordered list
 *
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to insert into
 *  \param comp Compare function
 *  \param uniq If nonzero, do not allow duplicates
 *  \param node Node to be inserted
 */
void genlist_insert (struct genlist *list, gen_comp_fn comp, int uniq, void *node)
{
	ENTER("list %p, comp %p, uniq %d, node %p", list, comp, uniq, node);
	if ( !list || !comp || !node )
		RETURN_ERRNO(EFAULT);

	/* easy-out for empty list or node <= head */
	if ( !list->head || comp(node, list->head) <= 0 )
	{
		/* uniq checking for duplicates */
		if ( uniq && comp(node, list->head) == 0 )
		{
			genlist_free (node);
			RETURN;
		}

		genlist_prepend (list, node);
		RETURN;
	}

	/* easy-out for empty list or node >= tail */
	if ( !list->tail || comp(node, list->tail) >= 0 )
	{
		/* uniq checking for duplicates */
		if ( uniq && comp(node, list->tail) == 0 )
		{
			genlist_free (node);
			RETURN;
		}

		genlist_append (list, node);
		RETURN;
	}

	/* slow path: head < node < tail: walk list, break when walk <= node. */
	list->walk = node_from_user(list->head)->next;
	while ( list->walk != list->tail )
		if ( comp(node, list->walk) <= 0 )
			break;
		else
			list->walk = node_from_user(list->walk)->next;

	/* uniq checking for duplicates */
	if ( uniq && comp(node, list->walk) == 0 )
	{
		genlist_free (node);
		RETURN;
	}

	/* Insert before list->walk */
	node_from_user(node)->next = list->walk;
	node_from_user(node)->prev = node_from_user(list->walk)->prev;
	node_from_user(node_from_user(list->walk)->prev)->next = node;
	node_from_user(list->walk)->prev = node;

	/* Reset cursor */
	list->walk = list->head;

	RETURN;
}


/** Remove a node from a list
 *
 *  \note The node is not freed, caller must do that afterwards with gen_node_free()
 *  \note The cursor is reset on any change to the contents of a list
 *
 *  \param list List to remove node from
 *  \param node Node to be removed
 */
void genlist_remove (struct genlist *list, void *node)
{
	ENTER("list %p, node %p", list, node);
	if ( !list || !node )
		RETURN_ERRNO(EFAULT);

	/* remove from list struct */
	if ( list->head == node )
		list->head = node_from_user(node)->next;
	if ( list->tail == node )
		list->tail = node_from_user(node)->prev;

	/* remove from next/prev peers */
	if ( node_from_user(node)->prev )
		node_from_user(node_from_user(node)->prev)->next = node_from_user(node)->next;
	if ( node_from_user(node)->next )
		node_from_user(node_from_user(node)->next)->prev = node_from_user(node)->prev;

	/* ensure no dangling references */
	node_from_user(node)->prev = NULL;
	node_from_user(node)->next = NULL;

	/* Reset cursor */
	list->walk = list->head;

	RETURN;
}


/** Sort a list according to a comparison function
 *
 *  \param list List to sort
 *  \param comp Compare function
 *  \param uniq Strip duplicates 
 */
void genlist_sort (struct genlist *list, gen_comp_fn comp, int uniq)
{
	ENTER("list %p, comp %p, uniq %d", list, comp, uniq);
	if ( !list || !comp )
		RETURN_ERRNO(EFAULT);

	/* Take a copy of the source list and reset it, then clear the source list */
	struct genlist  temp = *list;
	genlist_reset(&temp);
	list->head = NULL;
	list->tail = NULL;

	/* Insert back into the source list using the passed comp/uniq */
	void *node;
	while ( (node = genlist_next(&temp)) )
	{
		node_from_user(node)->prev = NULL;
		node_from_user(node)->next = NULL;
		genlist_insert(&temp, comp, uniq, node);
	}

	RETURN;
}


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
void *genlist_search (struct genlist *list, gen_comp_fn comp, void *node)
{
	ENTER("list %p, comp %p, node %p", list, comp, node);
	if ( !list || !comp || !node )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	if ( !list->head )
		RETURN_ERRNO_VALUE(0, "%p", NULL);

	void *curr;
	while ( list->walk )
	{
		curr = list->walk;
		list->walk = node_from_user(curr)->next;
		if ( comp && !comp(curr, node) )
			RETURN_ERRNO_VALUE(0, "%p", curr);
		else if ( !comp && curr == node )
			RETURN_ERRNO_VALUE(0, "%p", curr);
	}

	RETURN_ERRNO_VALUE(ENOENT, "%p", NULL);
}


/** Return the next node in the list, post-incrementing the cursor 
 *
 *  Note: the cursor is persistent across searches and iterations; see genlist_reset()
 *
 *  \param list List to return nodes from
 *
 *  \return value returned by last call of iter
 */
void *genlist_next (struct genlist *list)
{
	ENTER("list %p", list);
	if ( !list )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	if ( !list->head )
		RETURN_ERRNO_VALUE(0, "%p", NULL);

	void *curr = list->walk;
	if ( curr )
		list->walk = node_from_user(curr)->next;

	RETURN_ERRNO_VALUE(0, "%p", curr);
}


#ifdef UNIT_TEST
#include <stdio.h>
void dump_list (struct genlist *list)
{
	printf ("{");
	char *c;
	while ( (c = genlist_next(list)) )
		printf (" '%s'", c);
	printf (" }\n");
}

int main (int argc, char **argv)
{
	module_level = LOG_LEVEL_DEBUG;
	log_dupe (stderr);
	setbuf   (stderr, NULL);

	struct genlist  l = GENLIST_INIT;
	char           *c;
	int             n;

	printf ("Simple in-order prepend: ");
	genlist_prepend(&l, genlist_strdup("444"));
	genlist_prepend(&l, genlist_strdup("222"));
	genlist_prepend(&l, genlist_strdup("111"));
	dump_list(&l);

	printf ("Simple in-order append: ");
	genlist_append(&l, genlist_strdup("666"));
	genlist_append(&l, genlist_strdup("888"));
	genlist_append(&l, genlist_strdup("999"));
	dump_list(&l);

	printf ("Ordered-insert using strcmp: ");
	genlist_insert(&l, gen_strcmp, 0, genlist_strdup("333"));
	genlist_insert(&l, gen_strcmp, 0, genlist_strdup("555"));
	genlist_insert(&l, gen_strcmp, 0, genlist_strdup("777"));
	dump_list(&l);

	printf ("Get-next iterate: ");
	genlist_reset(&l);
	dump_list(&l);

	printf ("Random-order search and delete: ");
	genlist_reset(&l);
	if ( (c = genlist_search(&l, gen_strcmp, "444")) ) genlist_remove_and_free(&l, c);
	if ( (c = genlist_search(&l, gen_strcmp, "888")) ) genlist_remove_and_free(&l, c);
	if ( (c = genlist_search(&l, gen_strcmp, "222")) ) genlist_remove_and_free(&l, c);
	if ( (c = genlist_search(&l, gen_strcmp, "666")) ) genlist_remove_and_free(&l, c);
	dump_list(&l);

	printf ("Head remove 2:");
	for ( n = 0; n < 2; n++ )
	{
		c = genlist_head(&l);
		printf(" '%s'", c);
		genlist_free(c);
	}
	printf (" :");
	dump_list(&l);

	printf ("Tail remove 2:");
	for ( n = 0; n < 2; n++ )
	{
		c = genlist_tail(&l);
		printf(" '%s'", c);
		genlist_free(c);
	}
	printf (" :");
	dump_list(&l);

	printf ("Not empty: %d\n", genlist_empty(&l));
	c = genlist_tail(&l);
	printf ("Discard: '%s' ", c);
	dump_list(&l);
	printf ("Empty now: %d\n", genlist_empty(&l));
	genlist_destroy(&l);

	return 0;
}
#endif


/* Ends    : src/lib/genlist.c */
