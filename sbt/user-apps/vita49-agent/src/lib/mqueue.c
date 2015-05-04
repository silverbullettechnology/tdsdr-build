/** Queue of message buffers
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
#include <limits.h>

#include <lib/genlist.h>
#include <lib/mqueue.h>
#include <lib/mbuf.h>
#include <lib/log.h>

 
LOG_MODULE_STATIC("lib_mbuf", LOG_LEVEL_WARN);


/** Allocate a queue head object on the heap
 *
 *  Caller should free with mqueue_free()
 *
 *  \param  size  Maximum number of mbufs allowed, or 0 for no limit
 *
 *  \return  Pointer to allocated mqueue object
 */
struct mqueue *mqueue_alloc (int size)
{
	ENTER("size %d", size);

	struct mqueue *mq = malloc(sizeof(struct mqueue));
	if ( !mq )
		RETURN_VALUE("%p", NULL);

	mqueue_init(mq, size);

	RETURN_ERRNO_VALUE(0, "%p", mq);
}


/** Init a queue head object
 *
 *  \note  This is called internally by mqueue_alloc().  It's provided for initializing
 *         static queue objects.
 *
 *  \param  mq    Queue to initialize
 *  \param  size  Maximum number of mbufs allowed, or 0 for no limit
 */
void mqueue_init (struct mqueue *mq, int size)
{
	ENTER("mq %p, size %d", mq, size);
	if ( !mq )
		RETURN_ERRNO(EFAULT);

	if ( size < 0 )
		size = 0;

	memset(mq, 0, sizeof(struct mqueue));
	mq->size = size;

	RETURN_ERRNO(0);
}


/** Cleanup a queue head object
 *
 *  \note  This is called internally by mqueue_free().  It's provided for cleaning up
 *         static queue objects.
 */
void mqueue_done (struct mqueue *mq)
{
	ENTER("mq %p", mq);
	if ( !mq )
		RETURN_ERRNO(EFAULT);

	mqueue_flush(mq);

	RETURN;
}


/** Free a queue head object
 *
 *  \note  This internally calls mqueue_done() before freeing the object.
 */
void mqueue_free (struct mqueue *mq)
{
	ENTER("mq %p", mq);
	if ( !mq )
		RETURN_ERRNO(EFAULT);

	mqueue_done(mq);
	free(mq);

	RETURN;
}


/** Detach and deref all mbufs in the queue
 *
 *  \note  mqueue_flush() dereferences each mbuf, which will free it unless some other
 *         party holds a reference.
 */
void mqueue_flush (struct mqueue *mq)
{
	ENTER("mq %p", mq);
	if ( !mq )
		RETURN_ERRNO(EFAULT);

	struct mbuf *mb;
	while ( (mb = mqueue_dequeue(mq)) )
		mbuf_deref(mb);

	RETURN_ERRNO(0);
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
 *  \param  mq  Queue to enqueue the mbuf in
 *  \param  mb  Message buffer to enqueue
 * 
 *  \return  Number of messages in the queue after successful enqueue, or <1 on error
 */
int mqueue_enqueue (struct mqueue *mq, struct mbuf *mb)
{
	ENTER("mq %p, mb %p", mq, mb);
	if ( !mq || !mb )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	// check mbuf not already enqueued somewhere
	if ( mb->ext )
	{
		LOG_ERROR("%s(mq %p, mb %p): ext %p, refusing to double-enqueue\n", __func__,
		          mq, mb, mb->ext);
		RETURN_ERRNO_VALUE(EBUSY, "%d", -1);
	}

	if ( mq->size && mq->used >= mq->size )
	{
		LOG_ERROR("%s(mq %p, mb %p): used %d of size %d, queue full\n", __func__,
		          mq, mb, mq->used, mq->size);
		RETURN_ERRNO_VALUE(ENOSPC, "%d", -1);
	}

	mb->ext = mq;
	genlist_append(&mq->list, mb);
	mq->used++;

	RETURN_ERRNO_VALUE(0, "%d", mq->used);
}


/** Dequeue a message buffer from a queue
 *
 *  \note  mqueue_dequeue() implictly transfers the caller's reference to the mbuf to the
 *         caller.  
 *
 *  \param  mq  Queue to dequeue the mbuf from
 * 
 *  \return  Message buffer pointer, or NULL on error
 */
struct mbuf *mqueue_dequeue (struct mqueue *mq)
{
	ENTER("mq %p", mq);
	if ( !mq )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	errno = 0;
	struct mbuf *mb = genlist_head(&mq->list);

	if ( mb )
	{
		mb->ext = NULL;
		mq->used--;
	}

	RETURN_VALUE("%p", mb);
}


#ifdef UNIT_TEST
const char *ord[] = { "first", "second", "third", "fourth" };
int main (int argc, char **argv)
{
	module_level = LOG_LEVEL_DEBUG;
	log_set_module_level("lib_mbuf", module_level);
	log_dupe (stderr);
	setbuf   (stderr, NULL);
	setbuf   (stdout, NULL);

	printf("mqueue_alloc() test: ");
	struct mqueue *mq = mqueue_alloc(3);
	if ( !mq )
	{
		printf("fail: %s\n", strerror(errno));
		return 1;
	}
	printf("full mqueue_used %d, mqueue_room %d: pass\n",
	       mqueue_used(mq), mqueue_room(mq));

	// note, depends on the queue size limit of 3
	printf("mqueue_enqueue() test:\n");
	struct mbuf *mb;
	int          i;
	int          ret;
	for ( i = 0; i < 3; i++ )
	{
		if ( !(mb = mbuf_alloc(16, 0)) )
		{
			printf("fail: mbuf_alloc: %s\n", strerror(errno));
			return 1;
		}

		if ( (ret = mbuf_printf(mb, 16, "%s mbuf", ord[i])) < 0 )
		{
			printf("fail: mbuf_printf: %d, %s\n", ret, strerror(errno));
			return 1;
		}

		if ( (ret = mqueue_enqueue(mq, mb)) < 0 )
		{
			printf("fail: mqueue_enqueue: %d, %s\n", ret, strerror(errno));
			return 1;
		}

		printf(" %d: %p(%s) enqueued\n", i, mb, mb->buf);
	}
	printf("full mqueue_used %d, mqueue_room %d: pass\n",
	       mqueue_used(mq), mqueue_room(mq));

	// note, depends on previous test leaving mb at the last allocated mbuf
	printf("double-enqueue() test: ");
	if ( (ret = mqueue_enqueue(mq, mb)) > -1 || errno != EBUSY )
	{
		printf("fail: return should be -1, got %d, errno should be %d, got %d (%s)\n", 
		       ret, EBUSY, errno, strerror(errno));
		return 1;
	}
	printf("pass\n");

	// note, depends previous test leaving i == 4
	printf("queue size limit test: ");
	if ( !(mb = mbuf_alloc(128, 0)) )
	{
		printf("fail: mbuf_alloc: %s\n", strerror(errno));
		return 1;
	}
	if ( (ret = mbuf_printf(mb, 16, "%s mbuf", ord[i])) < 0 )
	{
		printf("fail: mbuf_printf: %d, %s\n", ret, strerror(errno));
		return 1;
	}
	if ( mqueue_enqueue(mq, mb) > -1 || errno != ENOSPC )
	{
		printf("fail: return should be -1, got %d, errno should be %d, got %d (%s)\n", 
		       ret, ENOSPC, errno, strerror(errno));
		return 1;
	}
	mbuf_deref(mb);
	printf("pass\n");

	// depends on previous tests
	printf("dequeue buffers test:\n");
	for ( i = 0; i < 3; i++ )
	{
		if ( !(mb = mqueue_dequeue(mq)) )
		{
			printf("fail: mqueue_dequeue: %s\n", strerror(errno));
			return 1;
		}

		printf(" %d: %p(%s) dequeued\n", i, mb, mb->buf);
		mbuf_deref(mb);
	}
	printf("full mqueue_used %d, mqueue_room %d: pass\n",
	       mqueue_used(mq), mqueue_room(mq));

	// depends on previous tests
	printf("queue empty test: ");
	if ( (mb = mqueue_dequeue(mq)) )
	{
		printf(" %d: %p(%s) dequeued, should be NULL\n", i, mb, mb->buf);
		return 1;
	}
	printf("pass\n");

	// free test
	printf("free queue test: ");
	errno = 0;
	mqueue_free(mq);
	if ( errno )
	{
		printf("fail: after free errno should be 0, got %d (%s)\n", 
		       errno, strerror(errno));
		return 1;
	}
	printf("pass\n");

	return 0;
}
#endif


/* Ends    : src/lib/mqueue.c */
