/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * I/O abstraction interface.
 * Copyright (c) 2005 Florian Westphal (westphal@foo.fh-furtwangen.de)
 *
 */

#include "portab.h"

static char UNUSED id[] = "$Id: io.c,v 1.15 2006/07/12 19:27:12 fw Exp $";

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "array.h"
#include "io.h"
#include "log.h"

/* Enables extra debug messages in event add/delete/callback code. */
/* #define DEBUG_IO */

typedef struct {
 void (*callback)(int, short);
 short what;
} io_event;

#define INIT_IOEVENT    { NULL, -1, 0, NULL }
#define IO_ERROR        4

#ifdef HAVE_EPOLL_CREATE
#define IO_USE_EPOLL    1
#else
# ifdef HAVE_KQUEUE
#define IO_USE_KQUEUE   1
# else
#define IO_USE_SELECT   1
#endif
#endif

static bool library_initialized;

#ifdef IO_USE_EPOLL
#include <sys/epoll.h>

static int io_masterfd;
static bool io_event_change_epoll(int fd, short what, const int action);
static int io_dispatch_epoll(struct timeval *tv);
#endif

#ifdef IO_USE_KQUEUE
#include <sys/types.h>
#include <sys/event.h>
static array io_evcache;
static int io_masterfd;

static int io_dispatch_kqueue(struct timeval *tv);
static bool io_event_change_kqueue(int, short, const int action);
#endif

#ifdef IO_USE_SELECT
#include "defines.h"	/* for conn.h */
#include "conn.h"	/* for CONN_IDX (needed by resolve.h) */
#include "resolve.h"	/* for RES_STAT (needed by conf.h) */
#include "conf.h"	/* for Conf_MaxConnections */

static fd_set readers;
static fd_set writers;
static int select_maxfd;		/* the select() interface sucks badly */
static int io_dispatch_select(struct timeval *tv);
#endif

static array io_events;

static void io_docallback PARAMS((int fd, short what));

static io_event *
io_event_get(int fd)
{
	io_event *i;

	assert(fd >= 0);

	i = (io_event *) array_get(&io_events, sizeof(io_event), (size_t) fd);

	assert(i != NULL);

	return i;
}


bool
io_library_init(unsigned int eventsize)
{
	bool ret;
#ifdef IO_USE_EPOLL
	int ecreate_hint = (int)eventsize;
	if (ecreate_hint <= 0)
		ecreate_hint = 128;
#endif
	if (library_initialized)
		return true;

#ifdef IO_USE_SELECT
#ifdef FD_SETSIZE
	if (eventsize >= FD_SETSIZE)
		eventsize = FD_SETSIZE - 1;
#endif
#endif
	if ((eventsize > 0) && !array_alloc(&io_events, sizeof(io_event), (size_t)eventsize))
		eventsize = 0;

#ifdef IO_USE_EPOLL
	io_masterfd = epoll_create(ecreate_hint);
	Log(LOG_INFO,
	    "IO subsystem: epoll (hint size %d, initial maxfd %u, masterfd %d).",
	    ecreate_hint, eventsize, io_masterfd);
	ret = io_masterfd >= 0;
	if (ret) library_initialized = true;

	return ret;
#endif
#ifdef IO_USE_SELECT
	Log(LOG_INFO, "IO subsystem: select (initial maxfd %u).",
	    eventsize);
	FD_ZERO(&readers);
	FD_ZERO(&writers);
#ifdef FD_SETSIZE
	if (Conf_MaxConnections >= (int)FD_SETSIZE) {
		Log(LOG_WARNING,
		    "MaxConnections (%d) exceeds limit (%u), changed MaxConnections to %u.",
		    Conf_MaxConnections, FD_SETSIZE, FD_SETSIZE - 1);

		Conf_MaxConnections = FD_SETSIZE - 1;
	}
#else
	Log(LOG_WARNING,
	    "FD_SETSIZE undefined, don't know how many descriptors select() can handle on your platform ...");
#endif /* FD_SETSIZE */
	library_initialized = true;
	return true;
#endif /* SELECT */
#ifdef IO_USE_KQUEUE
	io_masterfd = kqueue();

	Log(LOG_INFO,
	    "IO subsystem: kqueue (initial maxfd %u, masterfd %d)",
	    eventsize, io_masterfd);
	ret = io_masterfd >= 0;
	if (ret) library_initialized = true;

	return ret;
#endif
}


void
io_library_shutdown(void)
{
#ifdef IO_USE_SELECT
	FD_ZERO(&readers);
	FD_ZERO(&writers);
#else
	close(io_masterfd);	/* kqueue, epoll */
	io_masterfd = -1;
#endif
#ifdef IO_USE_KQUEUE
	array_free(&io_evcache);
#endif
	library_initialized = false;
}


bool
io_event_setcb(int fd, void (*cbfunc) (int, short))
{
	io_event *i = io_event_get(fd);
	if (!i)
		return false;

	i->callback = cbfunc;
	return true;
}


bool
io_event_create(int fd, short what, void (*cbfunc) (int, short))
{
	bool ret;
	io_event *i;

	assert(fd >= 0);

#ifdef IO_USE_SELECT
#ifdef FD_SETSIZE
	if (fd >= FD_SETSIZE) {
		Log(LOG_ERR,
		    "fd %d exceeds FD_SETSIZE (%u) (select can't handle more file descriptors)",
		    fd, FD_SETSIZE);
		return false;
	}
#endif				/* FD_SETSIZE */
#endif				/* IO_USE_SELECT */

	i = (io_event *) array_alloc(&io_events, sizeof(io_event), (size_t) fd);
	if (!i) {
		Log(LOG_WARNING,
		    "array_alloc failed: could not allocate space for %d io_event structures",
		    fd);
		return false;
	}

	i->callback = cbfunc;
	i->what = 0;
#ifdef IO_USE_EPOLL
	ret = io_event_change_epoll(fd, what, EPOLL_CTL_ADD);
#endif
#ifdef IO_USE_KQUEUE
	ret = io_event_change_kqueue(fd, what, EV_ADD|EV_ENABLE);
#endif
#ifdef IO_USE_SELECT
	ret = io_event_add(fd, what);
#endif
	if (ret) i->what = what;
	return ret;
}


#ifdef IO_USE_EPOLL
static bool
io_event_change_epoll(int fd, short what, const int action)
{
	struct epoll_event ev = { 0, {0} };
	ev.data.fd = fd;

	if (what & IO_WANTREAD)
		ev.events = EPOLLIN | EPOLLPRI;
	if (what & IO_WANTWRITE)
		ev.events |= EPOLLOUT;

	return epoll_ctl(io_masterfd, action, fd, &ev) == 0;
}
#endif

#ifdef IO_USE_KQUEUE
static bool
io_event_kqueue_commit_cache(void)
{
	struct kevent *events;
	bool ret;
	int len = (int) array_length(&io_evcache, sizeof (struct kevent));
 
	if (!len) /* nothing to do */
		return true;

	assert(len>0);

	if (len < 0) {
		array_free(&io_evcache);
		return false;
	}

	events = array_start(&io_evcache);

	assert(events != NULL);

	ret = kevent(io_masterfd, events, len, NULL, 0, NULL) == 0;
	if (ret)
		array_trunc(&io_evcache);
	return ret;
}


static bool
io_event_change_kqueue(int fd, short what, const int action)
{
	struct kevent kev;
	bool ret = true;

	if (what & IO_WANTREAD) {
		EV_SET(&kev, fd, EVFILT_READ, action, 0, 0, 0);
		ret = array_catb(&io_evcache, (char*) &kev, sizeof (kev));
		if (!ret)
			ret = kevent(io_masterfd, &kev,1, NULL, 0, NULL) == 0;
	}	

	if (ret && (what & IO_WANTWRITE)) {
		EV_SET(&kev, fd, EVFILT_WRITE, action, 0, 0, 0);
		ret = array_catb(&io_evcache, (char*) &kev, sizeof (kev));
		if (!ret)
			ret = kevent(io_masterfd, &kev, 1, NULL, 0, NULL) == 0;
	}

	if (array_length(&io_evcache, sizeof kev) >= 100)
		io_event_kqueue_commit_cache();
	return ret;
}
#endif


bool
io_event_add(int fd, short what)
{
	io_event *i = io_event_get(fd);

	if (!i) return false;
	if (i->what == what) return true;
#ifdef DEBUG_IO
	Log(LOG_DEBUG, "io_event_add(): fd %d (arg: %d), what %d.", i->fd, fd, what);
#endif
	i->what |= what;
#ifdef IO_USE_EPOLL
	return io_event_change_epoll(fd, i->what, EPOLL_CTL_MOD);
#endif

#ifdef IO_USE_KQUEUE
	return io_event_change_kqueue(fd, what, EV_ADD | EV_ENABLE);
#endif

#ifdef IO_USE_SELECT
	if (fd > select_maxfd)
		select_maxfd = fd;

	if (what & IO_WANTREAD)
		FD_SET(fd, &readers);
	if (what & IO_WANTWRITE)
		FD_SET(fd, &writers);

	return true;
#endif
}


bool
io_setnonblock(int fd)
{
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		return false;

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif
	flags |= O_NONBLOCK;

	return fcntl(fd, F_SETFL, flags) == 0;
}


bool
io_close(int fd)
{
	io_event *i;
#ifdef IO_USE_SELECT
	FD_CLR(fd, &writers);
	FD_CLR(fd, &readers);

	if (fd == select_maxfd) {
		while (select_maxfd>0) {
			--select_maxfd; /* find largest fd */  
			i = io_event_get(select_maxfd);
			if (i && i->callback) break;
		}	
	}	
#endif
	i = io_event_get(fd);
#ifdef IO_USE_KQUEUE
	if (array_length(&io_evcache, sizeof (struct kevent)))	/* pending data in cache? */
		io_event_kqueue_commit_cache();

	/* both kqueue and epoll remove fd from all sets automatically on the last close
	 * of the descriptor. since we don't know if this is the last close we'll have
	 * to remove the set explicitly. */
	if (i) {
		io_event_change_kqueue(fd, i->what, EV_DELETE);
		io_event_kqueue_commit_cache();
	}	
#endif
#ifdef IO_USE_EPOLL
	io_event_change_epoll(fd, 0, EPOLL_CTL_DEL);
#endif
	if (i) {
		i->callback = NULL;
		i->what = 0;
	}
	return close(fd) == 0;
}


bool
io_event_del(int fd, short what)
{
	io_event *i = io_event_get(fd);
#ifdef DEBUG_IO
	Log(LOG_DEBUG, "io_event_del(): trying to delete eventtype %d on fd %d", what, fd);
#endif
	if (!i) return false;

	i->what &= ~what;

#ifdef IO_USE_EPOLL
	return io_event_change_epoll(fd, i->what, EPOLL_CTL_MOD);
#endif

#ifdef IO_USE_KQUEUE
	return io_event_change_kqueue(fd, what, EV_DISABLE);
#endif
#ifdef IO_USE_SELECT
	if (what & IO_WANTWRITE)
		FD_CLR(fd, &writers);

	if (what & IO_WANTREAD)
		FD_CLR(fd, &readers);

	return true;
#endif
}


#ifdef IO_USE_SELECT
static int
io_dispatch_select(struct timeval *tv)
{
	fd_set readers_tmp = readers;
	fd_set writers_tmp = writers;
	short what;
	int ret, i;
	int fds_ready;
	ret = select(select_maxfd + 1, &readers_tmp, &writers_tmp, NULL, tv);
	if (ret <= 0)
		return ret;

	fds_ready = ret;

	for (i = 0; i <= select_maxfd; i++) {
		what = 0;
		if (FD_ISSET(i, &readers_tmp)) {
			what = IO_WANTREAD;
			fds_ready--;
		}

		if (FD_ISSET(i, &writers_tmp)) {
			what |= IO_WANTWRITE;
			fds_ready--;
		}
		if (what)
			io_docallback(i, what);
		if (fds_ready <= 0)
			break;
	}

	return ret;
}
#endif


#ifdef IO_USE_EPOLL
static int
io_dispatch_epoll(struct timeval *tv)
{
	time_t sec = tv->tv_sec * 1000;
	int i, total = 0, ret, timeout = tv->tv_usec + sec;
	struct epoll_event epoll_ev[100];
	short type;

	if (timeout < 0)
		timeout = 1000;

	do {
		ret = epoll_wait(io_masterfd, epoll_ev, 100, timeout);
		total += ret;
		if (ret <= 0)
			return total;

		for (i = 0; i < ret; i++) {
			type = 0;
			if (epoll_ev[i].events & (EPOLLERR | EPOLLHUP))
				type = IO_ERROR;

			if (epoll_ev[i].events & (EPOLLIN | EPOLLPRI))
				type |= IO_WANTREAD;

			if (epoll_ev[i].events & EPOLLOUT)
				type |= IO_WANTWRITE;

			io_docallback(epoll_ev[i].data.fd, type);
		}

		timeout = 0;
	} while (ret == 100);

	return total;
}
#endif


#ifdef IO_USE_KQUEUE
static int
io_dispatch_kqueue(struct timeval *tv)
{
	int i, total = 0, ret;
	struct kevent kev[100];
	struct kevent *newevents;
	struct timespec ts;
	int newevents_len;
	ts.tv_sec = tv->tv_sec;
	ts.tv_nsec = tv->tv_usec * 1000;
	
	do {
		newevents_len = (int) array_length(&io_evcache, sizeof (struct kevent));
		newevents = (newevents_len > 0) ? array_start(&io_evcache) : NULL;
		assert(newevents_len >= 0);
		if (newevents_len < 0)
			newevents_len = 0;
#ifdef DEBUG
		if (newevents_len)
			assert(newevents != NULL);
#endif
		ret = kevent(io_masterfd, newevents, newevents_len, kev,
			     100, &ts);
		if ((newevents_len>0) && ret != -1)
			array_trunc(&io_evcache);

		total += ret;
		if (ret <= 0)
			return total;

		for (i = 0; i < ret; i++) {
			if (kev[i].flags & EV_EOF) {
#ifdef DEBUG
				LogDebug("kev.flag has EV_EOF set, setting IO_ERROR",
					kev[i].filter, kev[i].ident);
#endif				
				io_docallback((int)kev[i].ident, IO_ERROR);
				continue;
			}	

			switch (kev[i].filter) {
				case EVFILT_READ:
					io_docallback((int)kev[i].ident, IO_WANTREAD);
					break;
				case EVFILT_WRITE:
					io_docallback((int)kev[i].ident, IO_WANTWRITE);
					break;
				default:
#ifdef DEBUG
					LogDebug("Unknown kev.filter number %d for fd %d",
						kev[i].filter, kev[i].ident); /* Fall through */
#endif
				case EV_ERROR:
					io_docallback((int)kev[i].ident, IO_ERROR);
					break;
			}
		}
		ts.tv_sec = 0;
		ts.tv_nsec = 0;
	} while (ret == 100);

	return total;
}
#endif


int
io_dispatch(struct timeval *tv)
{
#ifdef IO_USE_SELECT
	return io_dispatch_select(tv);
#endif
#ifdef IO_USE_KQUEUE
	return io_dispatch_kqueue(tv);
#endif
#ifdef IO_USE_EPOLL
	return io_dispatch_epoll(tv);
#endif
}


/* call the callback function inside the struct matching fd */
static void
io_docallback(int fd, short what)
{
	io_event *i;
#ifdef DEBUG_IO
	Log(LOG_DEBUG, "doing callback for fd %d, what %d", fd, what);
#endif
	i = io_event_get(fd);

	if (i->callback) {	/* callback might be NULL if a previous callback function 
				   called io_close on this fd */
		i->callback(fd, (what & IO_ERROR) ? i->what : what);
	}	
	/* if error indicator is set, we return the event(s) that were registered */
}
