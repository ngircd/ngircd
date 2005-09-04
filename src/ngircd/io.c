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

static char UNUSED id[] = "$Id: io.c,v 1.11 2005/09/04 13:38:59 fw Exp $";

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


typedef struct {
 void (*callback)(int, short);
 int fd;
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
static bool io_event_new_epoll(int fd, short what);
static bool io_event_change_epoll(int fd, short what);
static int io_dispatch_epoll(struct timeval *tv);
#endif

#ifdef IO_USE_KQUEUE
#include <sys/types.h>
#include <sys/event.h>
static array io_evcache;
static int io_masterfd;

static int io_dispatch_kqueue(struct timeval *tv);
static bool io_event_add_kqueue(int, short);
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
	i = (io_event *) array_get(&io_events, sizeof(io_event), fd);
	assert(i);

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
	if (eventsize && !array_alloc(&io_events, sizeof(io_event), eventsize))
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
	if (Conf_MaxConnections >= FD_SETSIZE) {
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

	i = (io_event *) array_alloc(&io_events, sizeof(io_event), fd);
	if (!i) {
		Log(LOG_WARNING,
		    "array_alloc failed: could not allocate space for %d io_event structures",
		    fd);
		return false;
	}

	i->fd = fd;
	i->callback = cbfunc;
#ifdef IO_USE_EPOLL
	i->what = what;
	return io_event_new_epoll(fd, what);
#endif
#ifdef IO_USE_KQUEUE
	i->what = what;
	return io_event_add_kqueue(fd, what);
#endif
#ifdef IO_USE_SELECT
	i->what = 0;
	return io_event_add(fd, what);
#endif
}


#ifdef IO_USE_EPOLL
static bool
io_event_new_epoll(int fd, short what)
{
	struct epoll_event ev = { 0, {0} };
	ev.data.fd = fd;

	if (what & IO_WANTREAD)
		ev.events = EPOLLIN | EPOLLPRI;
	if (what & IO_WANTWRITE)
		ev.events |= EPOLLOUT;

	return epoll_ctl(io_masterfd, EPOLL_CTL_ADD, fd, &ev) == 0;
}


static bool
io_event_change_epoll(int fd, short what)
{
	struct epoll_event ev = { 0, {0} };
	ev.data.fd = fd;

	if (what & IO_WANTREAD)
		ev.events = EPOLLIN | EPOLLPRI;
	if (what & IO_WANTWRITE)
		ev.events |= EPOLLOUT;

	return epoll_ctl(io_masterfd, EPOLL_CTL_MOD, fd, &ev) == 0;
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

	assert(events);

	ret = kevent(io_masterfd, events, len, NULL, 0, NULL) == 0;
	if (ret)
		array_trunc(&io_evcache);
	return ret;
}


static bool
io_event_add_kqueue(int fd, short what)
{
	struct kevent kev;
	short filter = 0;
	size_t len = array_length(&io_evcache, sizeof kev);

	if (what & IO_WANTREAD)
		filter = EVFILT_READ;

	if (what & IO_WANTWRITE)
		filter |= EVFILT_WRITE;

	if (len >= 100) {
		(void)io_event_kqueue_commit_cache();
	}

	EV_SET(&kev, fd, filter, EV_ADD | EV_ENABLE, 0, 0, NULL);
	return array_catb(&io_evcache, (char*) &kev, sizeof (kev));
}
#endif


bool
io_event_add(int fd, short what)
{
	io_event *i = io_event_get(fd);

	assert(i);

	if (!i)
		return false;
	if (i->what == what)
		return true;
#ifdef DEBUG
	Log(LOG_DEBUG, "io_event_add(): fd %d (arg: %d), what %d.", i->fd, fd, what);
#endif

	i->what |= what;

#ifdef IO_USE_EPOLL
	return io_event_change_epoll(fd, i->what);
#endif

#ifdef IO_USE_KQUEUE
	return io_event_add_kqueue(fd, what);
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
	io_event *i = io_event_get(fd);
	if (i) {
		memset(i, 0, sizeof(io_event));
		i->fd = -1;
	}
#ifdef IO_USE_SELECT
	FD_CLR(fd, &writers);
	FD_CLR(fd, &readers);

	if (fd == select_maxfd)
		select_maxfd--;
#endif
#ifdef IO_USE_KQUEUE
	if (array_length(&io_evcache, sizeof (struct kevent)))	/* pending data in cache? */
		io_event_kqueue_commit_cache();
#endif
#ifdef IO_USE_EPOLL
	epoll_ctl(io_masterfd, EPOLL_CTL_DEL, fd, NULL);
#endif
	return close(fd) == 0;	/* kqueue will remove fd from all sets automatically */
}


bool
io_event_del(int fd, short what)
{
#ifdef IO_USE_KQUEUE
	struct kevent kev;
	short filter = 0;
#endif
	io_event *i = io_event_get(fd);
#ifdef DEBUG
	Log(LOG_DEBUG, "io_event_del(): trying to delete eventtype %d on fd %d", what, fd);
#endif
	assert(i);
	if (!i)
		return false;

	i->what &= ~what;

#ifdef IO_USE_EPOLL
	return io_event_change_epoll(fd, i->what);
#endif

#ifdef IO_USE_KQUEUE
	if (what & IO_WANTREAD)
		filter = EVFILT_READ;

	if (what & IO_WANTWRITE)
		filter |= EVFILT_WRITE;

	EV_SET(&kev, fd, filter, EV_DELETE, 0, 0, NULL);
	return kevent(io_masterfd, &kev, 1, NULL, 0, NULL) == 0;
#endif

#ifdef IO_USE_SELECT
	if (what & IO_WANTWRITE)
		FD_CLR(i->fd, &writers);

	if (what & IO_WANTREAD)
		FD_CLR(i->fd, &readers);

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
	short type;
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
			assert(newevents);
#endif

		ret = kevent(io_masterfd, newevents, newevents_len, kev,
			     100, &ts);
		if ((newevents_len>0) && ret != -1)
			array_trunc(&io_evcache);

		total += ret;
		if (ret <= 0)
			return total;

		for (i = 0; i < ret; i++) {
			type = 0;
			if (kev[i].flags & EV_EOF)
				type = IO_ERROR;

			if (kev[i].filter & EV_ERROR)
				type = IO_ERROR;

			if (kev[i].filter & EVFILT_READ)
				type |= IO_WANTREAD;

			if (kev[i].filter & EVFILT_WRITE)
				type |= IO_WANTWRITE;

			io_docallback(kev[i].ident, type);
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
#ifdef DEBUG
	Log(LOG_DEBUG, "doing callback for fd %d, what %d", fd, what);
#endif
	i = io_event_get(fd);
	assert(i);

	if (i->callback)	/* callback might be 0 if previous callback function called io_close on this fd */
		i->callback(fd, (what & IO_ERROR) ? i->what : what);
	/* if error indicator is set, we return the event(s) the app asked for */
}
