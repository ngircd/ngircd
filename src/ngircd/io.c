/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2005-2006 Florian Westphal (westphal@foo.fh-furtwangen.de)
 * Copyright (c)2006-2014 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#include "portab.h"

/**
 * @file
 * I/O abstraction interface.
 */

/* Extra debug messages in event add/delete/callback code: 0=off / 1=on */
#define DEBUG_IO 0

#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "array.h"
#include "io.h"
#include "log.h"

typedef struct {
#ifdef PROTOTYPES
 void (*callback)(int, short);
#else
 void (*callback)();
#endif
 short what;
} io_event;

#define INIT_IOEVENT		{ NULL, -1, 0, NULL }
#define IO_ERROR		4
#define MAX_EVENTS		100

#ifdef HAVE_EPOLL_CREATE
#  define IO_USE_EPOLL		1
#  ifdef HAVE_SELECT
#    define IO_USE_SELECT	1
#  endif
#else
#  ifdef HAVE_KQUEUE
#    define IO_USE_KQUEUE	1
#  else
#    ifdef HAVE_SYS_DEVPOLL_H
#      define IO_USE_DEVPOLL	1
#    else
#      if defined(HAVE_POLL) && defined(HAVE_POLL_H)
#        define IO_USE_POLL	1
#      else
#        ifdef HAVE_SELECT
#          define IO_USE_SELECT	1
#        else
#          error "no IO API available!?"
#        endif /* HAVE_SELECT */
#      endif /* HAVE_POLL */
#    endif /* HAVE_SYS_DEVPOLL_H */
#  endif /* HAVE_KQUEUE */
#endif /* HAVE_EPOLL_CREATE */

static bool library_initialized = false;

#ifdef IO_USE_EPOLL
#include <sys/epoll.h>

static int io_masterfd = -1;
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

#ifndef EV_SET
/* Taken from /usr/include/sys/event.h of FreeBSD 8.1 and required by all
 * platforms that have kqueue but lack EV_SET() -- for example FreeBSD 4. */
#define EV_SET(kevp, a, b, c, d, e, f) do {	\
	struct kevent *__kevp__ = (kevp);	\
	__kevp__->ident = (a);			\
	__kevp__->filter = (b);			\
	__kevp__->flags = (c);			\
	__kevp__->fflags = (d);			\
	__kevp__->data = (e);			\
	__kevp__->udata = (f);			\
} while(0)
#endif
#endif

#ifdef IO_USE_POLL
#include <poll.h>

static array pollfds;
static int poll_maxfd;

static bool io_event_change_poll PARAMS((int fd, short what));
#endif

#ifdef IO_USE_DEVPOLL
#include <sys/devpoll.h>
static int io_masterfd;

static bool io_event_change_devpoll(int fd, short what);
#endif

#ifdef IO_USE_SELECT
#include <sys/time.h>
#include "defines.h"	/* for conn.h */
#include "proc.h"	/* for PROC_STAT (needed by conf.h) */
#include "conn.h"	/* for CONN_ID (needed by conf.h) */
#include "conf.h"	/* for Conf_MaxConnections */

static fd_set readers;
static fd_set writers;
/*
 * this is the first argument for select(), i.e.
 * the largest fd registered, plus one.
 */
static int select_maxfd;
static int io_dispatch_select PARAMS((struct timeval *tv));

#ifndef IO_USE_EPOLL
#define io_masterfd -1
#endif
#endif /* IO_USE_SELECT */

static array io_events;

static void io_docallback PARAMS((int fd, short what));

#if DEBUG_IO
static void
io_debug(const char *s, int fd, int what)
{
	LogDebug("%s: %d, %d\n", s, fd, what);
}
#else
static inline void
io_debug(const char UNUSED *s,int UNUSED a, int UNUSED b)
{ /* NOTHING */ }
#endif

static io_event *
io_event_get(int fd)
{
	io_event *i;

	assert(fd >= 0);

	i = (io_event *) array_get(&io_events, sizeof(io_event), (size_t) fd);

	assert(i != NULL);

	return i;
}


#ifdef IO_USE_DEVPOLL
static int
io_dispatch_devpoll(struct timeval *tv)
{
	struct dvpoll dvp;
	time_t sec = tv->tv_sec * 1000;
	int i, ret, timeout = tv->tv_usec + sec;
	short what;
	struct pollfd p[MAX_EVENTS];

	if (timeout < 0)
		timeout = 1000;

	dvp.dp_timeout = timeout;
	dvp.dp_nfds = MAX_EVENTS;
	dvp.dp_fds = p;
	ret = ioctl(io_masterfd, DP_POLL, &dvp);

	for (i=0; i < ret ; i++) {
		what = 0;
		if (p[i].revents & (POLLIN|POLLPRI))
			what = IO_WANTREAD;

		if (p[i].revents & POLLOUT)
			what |= IO_WANTWRITE;

		if (p[i].revents && !what) {
			/* other flag is set, probably POLLERR */
			what = IO_ERROR;
		}
		io_docallback(p[i].fd, what);
	}

	return ret;
}


static bool
io_event_change_devpoll(int fd, short what)
{
	struct pollfd p;

	p.events = 0;

	if (what & IO_WANTREAD)
		p.events = POLLIN | POLLPRI;
	if (what & IO_WANTWRITE)
		p.events |= POLLOUT;

	p.fd = fd;
	return write(io_masterfd, &p, sizeof p) == (ssize_t)sizeof p;
}

static void
io_close_devpoll(int fd)
{
	struct pollfd p;
	p.events = POLLREMOVE;
	p.fd = fd;
	write(io_masterfd, &p, sizeof p);
}

static void
io_library_init_devpoll(unsigned int eventsize)
{
	io_masterfd = open("/dev/poll", O_RDWR);
	if (io_masterfd >= 0)
		library_initialized = true;
	Log(LOG_INFO, "IO subsystem: /dev/poll (initial maxfd %u, masterfd %d).",
		eventsize, io_masterfd);
}
#else
static inline void
io_close_devpoll(int UNUSED x)
{ /* NOTHING */ }
static inline void
io_library_init_devpoll(unsigned int UNUSED ev)
{ /* NOTHING */ }
#endif


#ifdef IO_USE_POLL
static int
io_dispatch_poll(struct timeval *tv)
{
	time_t sec = tv->tv_sec * 1000;
	int i, ret, timeout = tv->tv_usec + sec;
	int fds_ready;
	short what;
	struct pollfd *p = array_start(&pollfds);

	if (timeout < 0)
		timeout = 1000;

	ret = poll(p, poll_maxfd + 1, timeout);
	if (ret <= 0)
		return ret;

	fds_ready = ret;
	for (i=0; i <= poll_maxfd; i++) {
		what = 0;
		if (p[i].revents & (POLLIN|POLLPRI))
			what = IO_WANTREAD;

		if (p[i].revents & POLLOUT)
			what |= IO_WANTWRITE;

		if (p[i].revents && !what) {
			/* other flag is set, probably POLLERR */
			what = IO_ERROR;
		}
		if (what) {
			fds_ready--;
			io_docallback(i, what);
		}
		if (fds_ready <= 0)
			break;
	}

	return ret;
}

static bool
io_event_change_poll(int fd, short what)
{
	struct pollfd *p;
	short events = 0;

	if (what & IO_WANTREAD)
		events = POLLIN | POLLPRI;
	if (what & IO_WANTWRITE)
		events |= POLLOUT;

	p = array_alloc(&pollfds, sizeof *p, fd);
	if (p) {
		p->events = events;
		p->fd = fd;
		if (fd > poll_maxfd)
			poll_maxfd = fd;
	}
	return p != NULL;
}

static void
io_close_poll(int fd)
{
	struct pollfd *p;
	p = array_get(&pollfds, sizeof *p, fd);
	if (!p) return;

	p->fd = -1;
	if (fd == poll_maxfd) {
		while (poll_maxfd > 0) {
			--poll_maxfd;
			p = array_get(&pollfds, sizeof *p, poll_maxfd);
			if (p && p->fd >= 0)
				break;
		}
	}
}

static void
io_library_init_poll(unsigned int eventsize)
{
	struct pollfd *p;
	array_init(&pollfds);
	poll_maxfd = 0;
	Log(LOG_INFO, "IO subsystem: poll (initial maxfd %u).",
	    eventsize);
	p = array_alloc(&pollfds, sizeof(struct pollfd), eventsize);
	if (p) {
		unsigned i;
		p = array_start(&pollfds);
		for (i = 0; i < eventsize; i++)
			p[i].fd = -1;

		library_initialized = true;
	}
}
#else
static inline void
io_close_poll(int UNUSED x)
{ /* NOTHING */ }
static inline void
io_library_init_poll(unsigned int UNUSED ev)
{ /* NOTHING */ }
#endif


#ifdef IO_USE_SELECT
static int
io_dispatch_select(struct timeval *tv)
{
	fd_set readers_tmp;
	fd_set writers_tmp;
	short what;
	int ret, i;
	int fds_ready;

	readers_tmp = readers;
	writers_tmp = writers;

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

static void
io_library_init_select(unsigned int eventsize)
{
	if (library_initialized)
		return;
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
}

static void
io_close_select(int fd)
{
	io_event *i;

	if (io_masterfd >= 0)	/* Are we using epoll()? */
		return;

	FD_CLR(fd, &writers);
	FD_CLR(fd, &readers);

	i = io_event_get(fd);
	if (!i) return;

	if (fd == select_maxfd) {
		while (select_maxfd>0) {
			--select_maxfd; /* find largest fd */
			i = io_event_get(select_maxfd);
			if (i && i->callback) break;
		}
	}
}
#else
static inline void
io_library_init_select(int UNUSED x)
{ /* NOTHING */ }
static inline void
io_close_select(int UNUSED x)
{ /* NOTHING */ }
#endif /* SELECT */


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

static int
io_dispatch_epoll(struct timeval *tv)
{
	time_t sec = tv->tv_sec * 1000;
	int i, ret, timeout = tv->tv_usec + sec;
	struct epoll_event epoll_ev[MAX_EVENTS];
	short type;

	if (timeout < 0)
		timeout = 1000;

	ret = epoll_wait(io_masterfd, epoll_ev, MAX_EVENTS, timeout);

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

	return ret;
}

static void
io_library_init_epoll(unsigned int eventsize)
{
	int ecreate_hint = (int)eventsize;
	if (ecreate_hint <= 0)
		ecreate_hint = 128;
	io_masterfd = epoll_create(ecreate_hint);
	if (io_masterfd >= 0) {
		library_initialized = true;
		Log(LOG_INFO,
		    "IO subsystem: epoll (hint size %d, initial maxfd %u, masterfd %d).",
		    ecreate_hint, eventsize, io_masterfd);
		return;
	}
#ifdef IO_USE_SELECT
	Log(LOG_INFO, "Can't initialize epoll() IO interface, falling back to select() ...");
#endif
}
#else
static inline void
io_library_init_epoll(unsigned int UNUSED ev)
{ /* NOTHING */ }
#endif /* IO_USE_EPOLL */


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

static int
io_dispatch_kqueue(struct timeval *tv)
{
	int i, ret;
	struct kevent kev[MAX_EVENTS];
	struct kevent *newevents;
	struct timespec ts;
	int newevents_len;
	ts.tv_sec = tv->tv_sec;
	ts.tv_nsec = tv->tv_usec * 1000;

	newevents_len = (int) array_length(&io_evcache, sizeof (struct kevent));
	newevents = (newevents_len > 0) ? array_start(&io_evcache) : NULL;
	assert(newevents_len >= 0);

	ret = kevent(io_masterfd, newevents, newevents_len, kev, MAX_EVENTS, &ts);
	if (newevents && ret != -1)
		array_trunc(&io_evcache);

	for (i = 0; i < ret; i++) {
		io_debug("dispatch_kqueue: fd, kev.flags", (int)kev[i].ident, kev[i].flags);
		if (kev[i].flags & (EV_EOF|EV_ERROR)) {
			if (kev[i].flags & EV_ERROR)
				Log(LOG_ERR, "kevent fd %d: EV_ERROR (%s)",
					(int)kev[i].ident, strerror((int)kev[i].data));
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
			LogDebug("Unknown kev.filter number %d for fd %d",
				kev[i].filter, kev[i].ident);
			/* Fall through */
		case EV_ERROR:
			io_docallback((int)kev[i].ident, IO_ERROR);
			break;
		}
	}

	return ret;
}

static void
io_library_init_kqueue(unsigned int eventsize)
{
	io_masterfd = kqueue();

	Log(LOG_INFO,
	    "IO subsystem: kqueue (initial maxfd %u, masterfd %d).",
	    eventsize, io_masterfd);
	if (io_masterfd >= 0)
		library_initialized = true;
}
#else
static inline void
io_library_init_kqueue(unsigned int UNUSED ev)
{ /* NOTHING */ }
#endif


bool
io_library_init(unsigned int eventsize)
{
	if (library_initialized)
		return true;

	if ((eventsize > 0) && !array_alloc(&io_events, sizeof(io_event), (size_t)eventsize))
		eventsize = 0;

	io_library_init_epoll(eventsize);
	io_library_init_kqueue(eventsize);
	io_library_init_devpoll(eventsize);
	io_library_init_poll(eventsize);
	io_library_init_select(eventsize);

	return library_initialized;
}


void
io_library_shutdown(void)
{
#ifdef IO_USE_SELECT
	FD_ZERO(&readers);
	FD_ZERO(&writers);
#endif
#if defined(IO_USE_EPOLL) || defined(IO_USE_KQUEUE) || defined(IO_USE_DEVPOLL)
	if (io_masterfd >= 0)
		close(io_masterfd);
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


static bool
backend_create_ev(int fd, short what)
{
	bool ret;
#ifdef IO_USE_DEVPOLL
	ret = io_event_change_devpoll(fd, what);
#endif
#ifdef IO_USE_POLL
	ret = io_event_change_poll(fd, what);
#endif
#ifdef IO_USE_EPOLL
	ret = io_event_change_epoll(fd, what, EPOLL_CTL_ADD);
#endif
#ifdef IO_USE_KQUEUE
	ret = io_event_change_kqueue(fd, what, EV_ADD|EV_ENABLE);
#endif
#ifdef IO_USE_SELECT
	if (io_masterfd < 0)
		ret = io_event_add(fd, what);
#endif
	return ret;
}


bool
io_event_create(int fd, short what, void (*cbfunc) (int, short))
{
	bool ret;
	io_event *i;

	assert(fd >= 0);
#if defined(IO_USE_SELECT) && defined(FD_SETSIZE)
	if (io_masterfd < 0 && fd >= FD_SETSIZE) {
		Log(LOG_ERR,
		    "fd %d exceeds FD_SETSIZE (%u) (select can't handle more file descriptors)",
		    fd, FD_SETSIZE);
		return false;
	}
#endif
	i = (io_event *) array_alloc(&io_events, sizeof(io_event), (size_t) fd);
	if (!i) {
		Log(LOG_WARNING,
		    "array_alloc failed: could not allocate space for %d io_event structures",
		    fd);
		return false;
	}

	i->callback = cbfunc;
	i->what = 0;
	ret = backend_create_ev(fd, what);
	if (ret)
		i->what = what;
	return ret;
}


bool
io_event_add(int fd, short what)
{
	io_event *i = io_event_get(fd);

	if (!i) return false;

	if ((i->what & what) == what) /* event type is already registered */
		return true;

	io_debug("io_event_add: fd, what", fd, what);

	i->what |= what;
#ifdef IO_USE_EPOLL
	if (io_masterfd >= 0)
		return io_event_change_epoll(fd, i->what, EPOLL_CTL_MOD);
#endif
#ifdef IO_USE_KQUEUE
	return io_event_change_kqueue(fd, what, EV_ADD | EV_ENABLE);
#endif
#ifdef IO_USE_DEVPOLL
	return io_event_change_devpoll(fd, i->what);
#endif
#ifdef IO_USE_POLL
	return io_event_change_poll(fd, i->what);
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
	return false;
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
io_setcloexec(int fd)
{
	int flags = fcntl(fd, F_GETFD);
	if (flags == -1)
		return false;
#ifdef FD_CLOEXEC
	flags |= FD_CLOEXEC;
#endif

	return fcntl(fd, F_SETFD, flags) == 0;
}

bool
io_close(int fd)
{
	io_event *i;

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
	io_close_devpoll(fd);
	io_close_poll(fd);
	io_close_select(fd);
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

	io_debug("io_event_del: trying to delete eventtype; fd, what", fd, what);
	if (!i) return false;

	if (!(i->what & what)) /* event is already disabled */
		return true;

	i->what &= ~what;
#ifdef IO_USE_DEVPOLL
	return io_event_change_devpoll(fd, i->what);
#endif
#ifdef IO_USE_POLL
	return io_event_change_poll(fd, i->what);
#endif
#ifdef IO_USE_EPOLL
	if (io_masterfd >= 0)
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
	return false;
}


int
io_dispatch(struct timeval *tv)
{
#ifdef IO_USE_EPOLL
	if (io_masterfd >= 0)
		return io_dispatch_epoll(tv);
#endif
#ifdef IO_USE_SELECT
	return io_dispatch_select(tv);
#endif
#ifdef IO_USE_KQUEUE
	return io_dispatch_kqueue(tv);
#endif
#ifdef IO_USE_DEVPOLL
	return io_dispatch_devpoll(tv);
#endif
#ifdef IO_USE_POLL
	return io_dispatch_poll(tv);
#endif
	return -1;
}


/* call the callback function inside the struct matching fd */
static void
io_docallback(int fd, short what)
{
	io_event *i = io_event_get(fd);

	io_debug("io_docallback; fd, what", fd, what);

	if (i->callback) {	/* callback might be NULL if a previous callback function
				   called io_close on this fd */
		i->callback(fd, (what & IO_ERROR) ? i->what : what);
	}
	/* if error indicator is set, we return the event(s) that were registered */
}
