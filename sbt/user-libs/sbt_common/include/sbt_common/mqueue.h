/** Queue of mbuf message buffers
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
#ifndef _INCLUDE_SBT_COMMON_MQUEUE_H_
#define _INCLUDE_SBT_COMMON_MQUEUE_H_
#include <limits.h>

#include <sbt_common/genlist.h>
#include <sbt_common/mbuf.h>
#include <sbt_common/log.h>


/** Queue object uses a genlist for head/tail, plus a size/used pair for enforcing max
 *  queue length */
struct mqueue
{
	struct genlist  list;
	int             size;
	int             used;
};
#define MQUEUE_INIT(s) { .list = GENLIST_INIT, .size = (s), .used = 0 }

 
/** Allocate a queue head object on the heap
 *
 *  Caller should free with mqueue_free()
 *
 *  \param  size  Maximum number of mbufs allowed, or 0 for no limit
 *
 *  \return  Pointer to allocated mqueue object
 */
struct mqueue *mqueue_alloc (int size);


/** Init a queue head object
 *
 *  \note  This is called internally by mqueue_alloc().  It's provided for initializing
 *         static queue objects.
 *
 *  \param  mq    Queue to initialize
 *  \param  size  Maximum number of mbufs allowed, or 0 for no limit
 */
void mqueue_init (struct mqueue *mq, int size);


/** Cleanup a queue head object
 *
 *  \note  This is called internally by mqueue_free().  It's provided for cleaning up
 *         static queue objects.
 */
void mqueue_done (struct mqueue *mq);


/** Free a queue head object
 *
 *  \note  This internally calls mqueue_done() before freeing the object.
 */
void mqueue_free (struct mqueue *mq);


/** Detach and deref all mbufs in the queue
 *
 *  \note  mqueue_flush() dereferences each mbuf, which will free it unless some other
 *         party holds a reference.
 */
void mqueue_flush (struct mqueue *mq);


/** Get number of messages currently in a queue
 *
 *  \param  mq  Queue to examine
 * 
 *  \return  Number of messages in the queue
 */
static inline int mqueue_used (struct mqueue *mq)
{
	return mq->used;
}


/** Get number of messages which could be added to a queue
 *
 *  \note  This is only really meaningful for queues which had a size limit set on their
 *         initialization; for queues with zero size, returns INT_MAX
 *
 *  \param  mq  Queue to examine
 * 
 *  \return  Number of messages the queue has room for
 */
static inline int mqueue_room (struct mqueue *mq)
{
	if ( mq->size < 1 )
		return INT_MAX;

	return mq->size - mq->used;
}


/** Enqueue a message buffer in a queue
 *
 *  \note  mqueue_enqueue() implictly transfers the caller's reference to the mbuf to the
 *         destination queue.  If the caller intends to hold onto the mbuf after enqueuing
 *         it the caller should take an additional reference and be prepared to
 *         dereference the mbuf later.
 *
 *  \note  It's an error to enqueue a mbuf which is already in some other queue, as it
 *         will entangle the queues together. 
 *
 *  \param  mq    Queue to enqueue the mbuf in
 *  \param  mbuf  Message buffer to enqueue
 * 
 *  \return  Number of messages in the queue after successful enqueue, or <1 on error
 */
int mqueue_enqueue (struct mqueue *mq, struct mbuf *mbuf);


/** Dequeue a message buffer from a queue
 *
 *  \note  mqueue_dequeue() implictly transfers the caller's reference to the mbuf to the
 *         caller.  
 *
 *  \param  mq  Queue to dequeue the mbuf from
 * 
 *  \return  Message buffer pointer, or NULL on error
 */
struct mbuf *mqueue_dequeue (struct mqueue *mq);


#endif /* _INCLUDE_SBT_COMMON_MQUEUE_H_ */
/* Ends */
