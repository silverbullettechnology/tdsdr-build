/** Worker interface to a child process
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

#include <lib/log.h>
#include <lib/mbuf.h>
#include <lib/mqueue.h>
#include <lib/child.h>
#include <daemon/worker.h>
#include <daemon/message.h>


LOG_MODULE_STATIC("worker_child", LOG_LEVEL_WARN);


#define  WC_PERIOD   60
#define  WC_COUNT     5


struct worker_child_priv
{
	struct worker  worker;
	struct child   child;
};


static struct worker *worker_child_alloc (void)
{
	ENTER("");
	struct worker_child_priv *priv = malloc(sizeof(struct worker_child_priv));
	if ( !priv )
		RETURN_VALUE("%p", NULL);

	memset(priv, 0, sizeof(struct worker_child_priv));
	priv->child.write = -1;
	priv->child.read  = -1;
	priv->child.pid   = -1;

	// temporary
	priv->child.argv = child_argv("bin/test-stub", NULL);

	RETURN_ERRNO_VALUE(0, "%p", (struct worker *)priv);
}

static int worker_child_config (const char *section, const char *tag, const char *val,
                                const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);

	errno = 0;
	int ret = worker_config_common(section, tag, val, file, line, (struct worker *)data);
	RETURN_VALUE("%d", ret);
}

static void worker_child_exit (struct worker *worker)
{
	struct worker_child_priv *priv = (struct worker_child_priv *)worker;

	if ( priv->child.ret < 0 )
	{
		LOG_ERROR("%s: pid %d killed by signal\n", worker->name,
		          priv->child.pid);
		worker->state = WS_READY;
	}
	else if ( priv->child.ret > 0 )
	{
		LOG_ERROR("%s: pid %d exited with %d\n", worker->name,
		          priv->child.pid, priv->child.ret);
		worker->state = WS_READY;
	}
	else
	{
		LOG_INFO("%s: pid %d exited with 0\n", worker->name, priv->child.pid);
		worker->state = WS_STOP;
	}

	child_close(&priv->child, 1);
	priv->child.pid = -1;
}

static worker_state_t worker_child_state_get (struct worker *worker)
{
	ENTER("worker %p", worker);
	if ( !worker )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	struct worker_child_priv *priv = (struct worker_child_priv *)worker;

	switch ( worker->state )
	{
		case WS_CONFIG:
			if ( !priv->child.argv )
			{
				LOG_DEBUG("%s: waiting for argv\n", worker->name);
				RETURN_ERRNO_VALUE(0, "%d", worker->state);
			}

			LOG_DEBUG("%s: CONFIG done, move to IDLE\n", worker->name);
			worker->state = WS_READY;
			// fall-through

		case WS_READY:
			// check auto-start if starts counter 0
			if ( !worker->starts && !(worker->flags & WF_AUTO_START) )
			{
				LOG_DEBUG("%s: waiting for manual START\n", worker->name);
				RETURN_ERRNO_VALUE(0, "%d", worker->state);
			}
			if ( worker->starts )
			{
				if ( !priv->child.ret && !(worker->flags & WF_RESTART_CLEAN) )
				{
					LOG_DEBUG("%s: waiting for manual (re)START\n", worker->name);
					RETURN_ERRNO_VALUE(0, "%d", worker->state);
				}
				if ( priv->child.ret && !(worker->flags & WF_RESTART_CLEAN) )
				{
					LOG_DEBUG("%s: waiting for manual (re)START\n", worker->name);
					RETURN_ERRNO_VALUE(0, "%d", worker->state);
				}
			}
			LOG_DEBUG("%s: READY, move to START\n", worker->name);
			worker->state = WS_START;
			// fall-through

		case WS_START:
			if ( priv->child.pid > -1 || priv->child.read > -1 || priv->child.write > -1 )
			{
				LOG_ERROR("%s: WS_START with pid %d, read %d, write %d?!\n", 
				          worker_name(worker), priv->child.pid, priv->child.read,
				          priv->child.write);
				worker->state = WS_STOP;
				RETURN_ERRNO_VALUE(0, "%d", worker->state);
			}
			// start with rate-limit: may go to _LIMIT or _ERROR, or _RUN on success
			if ( worker_limit(worker, WC_PERIOD, WC_COUNT) )
			{
				LOG_DEBUG("%s: rate-limited, waiting...\n", worker->name);
				worker->state = WS_LIMIT;
				RETURN_ERRNO_VALUE(0, "%d", worker->state);
			}
			worker->starts++;
			if ( child_spawn(&priv->child, 1) < 0 )
			{
				LOG_ERROR("%s: child_spawn(): %s\n", worker->name, strerror(errno));
				worker->state = WS_READY;
				RETURN_VALUE("%d", worker->state);
			}
			worker->state = WS_NORMAL;
			// fall-through

		case WS_NORMAL:
			switch ( child_check(&priv->child) )
			{
				// normal operation: trace debug only
				case CHILD_STAT_READY:
					LOG_TRACE("%s: pid %d ready, no action taken.\n", worker->name,
					          priv->child.pid);
					RETURN_ERRNO_VALUE(0, "%d", worker->state);

				// child exited during run: cleanup and go to _STOP or _ERROR
				case CHILD_STAT_EXIT:
					worker_child_exit(worker);
					RETURN_ERRNO_VALUE(0, "%d", worker->state);

				// pid was valid but waitpid() failed: go to _ERROR
				case CHILD_STAT_ERROR:
					LOG_ERROR("%s: child_check(%d): %s\n", worker->name,
					          priv->child.pid, strerror(errno));
					worker->state = WS_READY;
					RETURN_VALUE("%d", worker->state);

				// pid went missing during run? go to _ERROR
				case CHILD_STAT_NONE:
					LOG_ERROR("%s: lost child pid unexpectedly, stop\n", worker->name);
					worker->state = WS_READY;
					RETURN_VALUE("%d", worker->state);
			}

		case WS_STOP:
			switch ( child_check(&priv->child) )
			{
				// running when _STOP requested: close pipes and stay in _STOP to catch
				// exit code 
				case CHILD_STAT_READY:
					LOG_INFO("%s: STOP: pid %d running, close pipes.\n", worker->name,
					          priv->child.pid);
					child_close(&priv->child, 1);
					RETURN_ERRNO_VALUE(0, "%d", worker->state);

				// pid was valid but waitpid() failed: go to _ERROR
				case CHILD_STAT_ERROR:
					LOG_ERROR("%s: STOP: child_check(%d): %s\n", worker->name,
					          priv->child.pid, strerror(errno));
					worker->state = WS_READY;
					RETURN_VALUE("%d", worker->state);

				// exited since last check: cleanup and go to _STOP or _ERROR
				case CHILD_STAT_EXIT:
					worker_child_exit(worker);
					// fall-through

				// no PID: restart or stay in _STOP depending on policy
				case CHILD_STAT_NONE:
					worker->state = WS_READY;
					RETURN_ERRNO_VALUE(0, "%d", worker->state);
			}

		case WS_LIMIT:
			if ( worker_limit(worker, WC_PERIOD, WC_COUNT) )
			{
				LOG_DEBUG("%s: rate-limited, waiting...\n", worker->name);
				RETURN_ERRNO_VALUE(0, "%d", worker->state);
			}
			LOG_INFO("%s: rate-limit done, move to START\n", worker->name);
			worker->state = WS_START;
			break;

		default:
			LOG_DEBUG("Weird state %d (%s)\n", worker->state,
			          worker_state_desc(worker->state));
			RETURN_ERRNO_VALUE(ENOSYS, "%d", -1);
	}

	RETURN_ERRNO_VALUE(0, "%d", worker->state);
}

static void worker_child_fd_set (struct worker *worker,
                                 fd_set *rfds, fd_set *wfds, int *mfda)
{
	ENTER("worker %p, rfds %p, wfds %p, mfda %d", worker, rfds, wfds, *mfda);
	if ( !worker )
		RETURN_ERRNO(EFAULT);

	struct worker_child_priv *priv = (struct worker_child_priv *)worker;

	// no action if child is not currently running
	if ( priv->child.pid < 0 )
		RETURN_ERRNO(ECHILD);

	if ( rfds && priv->child.read > -1 )
	{
		FD_SET(priv->child.read, rfds);
		if ( priv->child.read > *mfda )
			*mfda = priv->child.read;
	}

	if ( wfds && mqueue_used(&worker->send) && priv->child.write > -1 )
	{
		FD_SET(priv->child.write, wfds);
		if ( priv->child.write > *mfda )
			*mfda = priv->child.write;
	}

	RETURN_ERRNO(0);
}

static int worker_child_fd_is_set (struct worker *worker, fd_set *rfds, fd_set *wfds)
{
	ENTER("worker %p, rfds %p, wfds %p", worker, rfds, wfds);
	if ( !worker )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	struct worker_child_priv *priv = (struct worker_child_priv *)worker;

	int ret = 0;
	if ( rfds && FD_ISSET(priv->child.read, rfds) )
		ret++;
	else if ( wfds && FD_ISSET(priv->child.write, wfds) )
		ret++;

	RETURN_ERRNO_VALUE(0, "%d", ret);
}

static int worker_child_read (struct worker *worker, fd_set *rfds)
{
	ENTER("worker %p, rfds %p", worker, rfds);
	if ( !worker )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	struct worker_child_priv *priv = (struct worker_child_priv *)worker;
	struct message           *user;
	struct mbuf              *mbuf;
	int                       len;

	// Receive into failsafe buffer if mbuf_alloc failed
	if ( !(mbuf = mbuf_alloc(DAEMON_MBUF_SIZE, sizeof(struct message))) )
	{
		char  buff[128];
		LOG_WARN("%s: mbuf_alloc() failed, message dropped\n", worker_name(worker));

		errno = 0;
		while ( (len = read(priv->child.read, buff, sizeof(buff))) > 0 )
			LOG_HEXDUMP(buff, len);

		RETURN_VALUE("%d", 0);
	}
	mbuf_beg_set(mbuf, DAEMON_MBUF_HEAD);
	user = mbuf_user(mbuf);
	user->worker = worker;

	// error handling
	if ( (len = mbuf_read(mbuf, priv->child.read, 4096)) < 0 )
	{
		// treat these as idle
		switch ( errno )
		{
			case EINTR:
			case EAGAIN:
			case EPIPE:
				LOG_DEBUG("%s: ignore read() %s\n", worker_name(worker), strerror(errno));
				worker_child_exit(worker);
				mbuf_deref(mbuf);
				RETURN_VALUE("%d", 0);

			default:
				LOG_ERROR("%s: read() failed: %s\n", worker_name(worker), strerror(errno));
				RETURN_VALUE("%d", len);
		}
	}

	// closed pipe after select said read OK?
	if ( len == 0 && errno == 0 )
	{
		LOG_WARN("%s: Worker child pid %d closed pipe??\n",
		         worker_name(worker), priv->child.pid);

		worker_child_exit(worker);
		mbuf_deref(mbuf);
		RETURN_VALUE("%d", len);
	}

	// message to handle
	LOG_DEBUG("%s: Worker child pid %d sent %d bytes:\n",
	          worker_name(worker), priv->child.pid, len); 

	mbuf_dump(mbuf);
	if ( mqueue_enqueue(&worker->recv, mbuf) < 0 )
	{
		LOG_ERROR("%s: enqueue failed: %s\n", worker_name(worker), strerror(errno));
		RETURN_VALUE("%d", -1);
	}

	RETURN_ERRNO_VALUE(0, "%d", 0);
}

static int worker_child_write (struct worker *worker, fd_set *wfds)
{
	ENTER("worker %p, wfds %p", worker, wfds);
	if ( !worker )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	struct worker_child_priv *priv = (struct worker_child_priv *)worker;
//	struct message           *user;
	struct mbuf              *mbuf;
	int                       ret;

	if ( !(mbuf = mqueue_dequeue(&worker->send)) )
		RETURN_ERRNO_VALUE(0, "%d", 0);

	// try to transmit
	mbuf_cur_set_beg(mbuf);
	errno = 0;
	if ( (ret = mbuf_write(mbuf, priv->child.write, 4096)) < 0 )
	{
		// treat these as idle
		switch ( errno )
		{
			case EINTR:
			case EAGAIN:
			case EPIPE:
				LOG_DEBUG("%s: ignore write(): %s\n", worker_name(worker), strerror(errno));
				break;

			default:
				LOG_ERROR("%s: worker child pid %d write failed: %s: dropped\n",
				          worker_name(worker), priv->child.pid, strerror(errno));
		}
	}
	else
		LOG_DEBUG("%s: wrote %d bytes to child pid %d\n", 
		          worker_name(worker), ret, priv->child.pid);

	mbuf_deref(mbuf);
	RETURN_VALUE("%d", 0);
}

static int worker_child_close (struct worker *worker)
{
	ENTER("worker %p", worker);
	if ( !worker )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	struct worker_child_priv *priv = (struct worker_child_priv *)worker;

	child_close(&priv->child, 1);

	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);
}

static void worker_child_free (struct worker *worker)
{
	ENTER("worker %p", worker);
	if ( !worker )
		RETURN_ERRNO(EFAULT);

	struct worker_child_priv *priv = (struct worker_child_priv *)worker;

	free(priv->child.argv);
	free(priv);

	RETURN_ERRNO(0);
}


/** Operation pointers. */
static struct worker_ops worker_child_ops =
{
	alloc_fn:      worker_child_alloc,
	config_fn:     worker_child_config,
	state_get_fn:  worker_child_state_get,
	fd_set_fn:     worker_child_fd_set,
	fd_is_set_fn:  worker_child_fd_is_set,
	read_fn:       worker_child_read,
	write_fn:      worker_child_write,
	close_fn:      worker_child_close,
	free_fn:       worker_child_free,
};
WORKER_CLASS(child, worker_child_ops);


/* Ends    : src/daemon/worker/child.c */

