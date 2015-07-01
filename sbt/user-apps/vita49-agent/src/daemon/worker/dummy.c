/** Dummy worker
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
#include <sys/select.h>

#include <sbt_common/log.h>
#include <sbt_common/mbuf.h>
#include <sbt_common/mqueue.h>

#include <v49_message/resource.h>

#include <daemon/worker.h>


LOG_MODULE_STATIC("worker_dummy", LOG_LEVEL_WARN);


static struct worker *worker_dummy_alloc (void)
{
	ENTER("");
	struct worker *worker = malloc(sizeof(struct worker));
	if ( !worker )
		RETURN_VALUE("%p", NULL);

	memset (worker, 0, sizeof(struct worker));

	RETURN_ERRNO_VALUE(0, "%p", worker);
}

static int worker_dummy_config (const char *section, const char *tag, const char *val,
                                const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);

	errno = 0;
	int ret = worker_config_common(section, tag, val, file, line, (struct worker *)data);
	RETURN_VALUE("%d", ret);
}

static void worker_dummy_fd_set (struct worker *worker,
                                 fd_set *rfds, fd_set *wfds, int *nfds)
{
	ENTER("worker %p, rfds %p, wfds %p, nfds %d", worker, rfds, wfds, *nfds);
	RETURN_ERRNO(ENOSYS);
}

static int worker_dummy_fd_is_set (struct worker *worker, fd_set *rfds, fd_set *wfds)
{
	ENTER("worker %p, rfds %p, wfds %p", worker, rfds, wfds);
	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);
}

static int worker_dummy_read (struct worker *worker, fd_set *rfds)
{
	ENTER("worker %p, rfds %p", worker, rfds);
	RETURN_ERRNO_VALUE(0, "%d", 0);
}

static int worker_dummy_write (struct worker *worker, fd_set *wfds)
{
	ENTER("worker %p, wfds %p", worker, wfds);
	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);
}

static void worker_dummy_process (struct worker *worker)
{
	ENTER("worker %p", worker);

	// discard enqueued mbufs
	struct mbuf *mbuf;
	while ( (mbuf = mqueue_dequeue(&worker->send)) )
		mbuf_deref(mbuf);

	RETURN_ERRNO(0);
}

static int worker_dummy_close (struct worker *worker)
{
	ENTER("worker %p", worker);
	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);
}

static void worker_dummy_free (struct worker *worker)
{
	ENTER("worker %p", worker);
	free(worker);
	RETURN_ERRNO(0);
}


/** Operation pointers. */
static struct worker_ops worker_dummy_ops =
{
	alloc_fn:      worker_dummy_alloc,
	config_fn:     worker_dummy_config,
	fd_set_fn:     worker_dummy_fd_set,
	fd_is_set_fn:  worker_dummy_fd_is_set,
	read_fn:       worker_dummy_read,
	write_fn:      worker_dummy_write,
	process_fn:    worker_dummy_process,
	close_fn:      worker_dummy_close,
	free_fn:       worker_dummy_free,
};
WORKER_CLASS(dummy, worker_dummy_ops);


/* Ends    : src/worker/dummy.c */
