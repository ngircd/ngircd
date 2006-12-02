/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2003 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Asynchronous resolver
 */


#include "portab.h"

static char UNUSED id[] = "$Id: resolve.c,v 1.24.2.1 2006/12/02 13:00:25 fw Exp $";

#include "imp.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef IDENTAUTH
#ifdef HAVE_IDENT_H
#include <ident.h>
#endif
#endif

#include "conn.h"
#include "defines.h"
#include "log.h"

#include "exp.h"
#include "resolve.h"
#include "io.h"


static void Do_ResolveAddr PARAMS(( struct sockaddr_in *Addr, int Sock, int w_fd ));
static void Do_ResolveName PARAMS(( const char *Host, int w_fd ));
static bool register_callback PARAMS((RES_STAT *s, void (*cbfunc)(int, short)));

#ifdef h_errno
static char *Get_Error PARAMS(( int H_Error ));
#endif

static pid_t
Resolver_fork(int *pipefds)
{
	pid_t pid;

	if (pipe(pipefds) != 0) {
                Log( LOG_ALERT, "Resolver: Can't create output pipe: %s!", strerror( errno ));
                return -1;
	}

	pid = fork();
	switch(pid) {
		case -1:
			Log( LOG_CRIT, "Resolver: Can't fork: %s!", strerror( errno ));
			close(pipefds[0]);
			close(pipefds[1]);
			return -1;
		case 0: /* child */
			close(pipefds[0]);
			Log_Init_Resolver( );
			return 0;
	}
	/* parent */
	close(pipefds[1]);
	return pid; 
}


/**
 * Resolve IP (asynchronous!).
 */
GLOBAL bool
Resolve_Addr(RES_STAT * s, struct sockaddr_in *Addr, int identsock,
	     void (*cbfunc) (int, short))
{
	int pipefd[2];
	pid_t pid;

	assert(s != NULL);

	pid = Resolver_fork(pipefd);
	if (pid > 0) {
#ifdef DEBUG
		Log( LOG_DEBUG, "Resolver for %s created (PID %d).", inet_ntoa( Addr->sin_addr ), pid );
#endif
		s->pid = pid;
		s->resolver_fd = pipefd[0];
		return register_callback(s, cbfunc);
	} else if( pid == 0 ) {
		/* Sub process */
		Do_ResolveAddr( Addr, identsock, pipefd[1]);
		Log_Exit_Resolver( );
		exit(0);
	}
	return false;
} /* Resolve_Addr */


/**
 * Resolve hostname (asynchronous!).
 */
GLOBAL bool
Resolve_Name( RES_STAT *s, const char *Host, void (*cbfunc)(int, short))
{
	int pipefd[2];
	pid_t pid;

	assert(s != NULL);

	pid = Resolver_fork(pipefd);
	if (pid > 0) {
		/* Main process */
#ifdef DEBUG
		Log( LOG_DEBUG, "Resolver for \"%s\" created (PID %d).", Host, pid );
#endif
		s->pid = pid;
		s->resolver_fd = pipefd[0];
		return register_callback(s, cbfunc);
	} else if( pid == 0 ) {
		/* Sub process */
		Do_ResolveName(Host, pipefd[1]);
		Log_Exit_Resolver( );
		exit(0);
	}
	return false;
} /* Resolve_Name */


GLOBAL void
Resolve_Init(RES_STAT *s)
{
	assert(s != NULL);
	s->resolver_fd = -1;
	s->pid = 0;
}


static void
Do_ResolveAddr( struct sockaddr_in *Addr, int identsock, int w_fd )
{
	/* Resolver sub-process: resolve IP address and write result into
	 * pipe to parent. */

	char hostname[HOST_LEN];
	char ipstr[HOST_LEN];
	struct hostent *h;
	size_t len;
	struct in_addr *addr;
	char *ntoaptr;
	array resolved_addr;
#ifdef IDENTAUTH
	char *res;
#endif
	array_init(&resolved_addr);
	/* Resolve IP address */
#ifdef DEBUG
	Log_Resolver( LOG_DEBUG, "Now resolving %s ...", inet_ntoa( Addr->sin_addr ));
#endif
	h = gethostbyaddr( (char *)&Addr->sin_addr, sizeof( Addr->sin_addr ), AF_INET );
	if (!h) {
#ifdef h_errno
		Log_Resolver( LOG_WARNING, "Can't resolve address \"%s\": %s!", inet_ntoa( Addr->sin_addr ), Get_Error( h_errno ));
#else
		Log_Resolver( LOG_WARNING, "Can't resolve address \"%s\"!", inet_ntoa( Addr->sin_addr ));
#endif	
		strlcpy( hostname, inet_ntoa( Addr->sin_addr ), sizeof( hostname ));
	} else {
 		strlcpy( hostname, h->h_name, sizeof( hostname ));

		h = gethostbyname( hostname );
		if ( h ) {
			if (memcmp(h->h_addr, &Addr->sin_addr, sizeof (struct in_addr))) {
				addr = (struct in_addr*) h->h_addr;
				strlcpy(ipstr, inet_ntoa(*addr), sizeof ipstr); 
				ntoaptr = inet_ntoa( Addr->sin_addr );
				Log(LOG_WARNING,"Possible forgery: %s resolved to %s (which is at ip %s!)",
										ntoaptr, hostname, ipstr);
				strlcpy( hostname, ntoaptr, sizeof hostname);
			}
		} else {
			ntoaptr = inet_ntoa( Addr->sin_addr );
			Log(LOG_WARNING, "Possible forgery: %s resolved to %s (which has no ip address)",
											ntoaptr, hostname);
			strlcpy( hostname, ntoaptr, sizeof hostname);
		}
	}
	Log_Resolver( LOG_DEBUG, "Ok, translated %s to \"%s\".", inet_ntoa( Addr->sin_addr ), hostname );

	len = strlen( hostname ); 
	hostname[len] = '\n'; len++;
	if (!array_copyb(&resolved_addr, hostname, len )) {
		Log_Resolver( LOG_CRIT, "Resolver: Can't copy resolved name: %s!", strerror( errno ));
		close( w_fd );
		return;
	}

#ifdef IDENTAUTH
	assert(identsock >= 0);
	if (identsock >= 0) {
		/* Do "IDENT" (aka "AUTH") lookup and append result to resolved_addr array */
#ifdef DEBUG
		Log_Resolver( LOG_DEBUG, "Doing IDENT lookup on socket %d ...", identsock );
#endif
		res = ident_id( identsock, 10 );
#ifdef DEBUG
		Log_Resolver(LOG_DEBUG, "Ok, IDENT lookup on socket %d done: \"%s\"",
							identsock, res ? res : "(NULL)" );
#endif
		if (res && !array_cats(&resolved_addr, res)) {
			Log_Resolver(LOG_WARNING, "Resolver: Cannot copy IDENT result: %s!", strerror(errno));
			/* omit ident and return hostname only */ 
		}

		if (res) free(res);
	}
#else
	(void)identsock;
#endif
	len = array_bytes(&resolved_addr);
	if( (size_t)write( w_fd, array_start(&resolved_addr), len) != len )
		Log_Resolver( LOG_CRIT, "Resolver: Can't write result to parent: %s!", strerror( errno ));

	close(w_fd);
	array_free(&resolved_addr);
} /* Do_ResolveAddr */


static void
Do_ResolveName( const char *Host, int w_fd )
{
	/* Resolver sub-process: resolve name and write result into pipe
	 * to parent. */

	char ip[16];
	struct hostent *h;
	struct in_addr *addr;
	size_t len;

	Log_Resolver( LOG_DEBUG, "Now resolving \"%s\" ...", Host );

	/* Resolve hostname */
	h = gethostbyname( Host );
	if( h ) {
		addr = (struct in_addr *)h->h_addr;
		strlcpy( ip, inet_ntoa( *addr ), sizeof( ip ));
	} else {
#ifdef h_errno
		Log_Resolver( LOG_WARNING, "Can't resolve \"%s\": %s!", Host, Get_Error( h_errno ));
#else
		Log_Resolver( LOG_WARNING, "Can't resolve \"%s\"!", Host );
#endif
		close(w_fd);
		return;
	}
#ifdef DEBUG
	Log_Resolver( LOG_DEBUG, "Ok, translated \"%s\" to %s.", Host, ip );
#endif
	/* Write result into pipe to parent */
	len = strlen( ip );
	if ((size_t)write( w_fd, ip, len ) != len) {
		Log_Resolver( LOG_CRIT, "Resolver: Can't write to parent: %s!", strerror( errno ));
		close( w_fd );
	}
} /* Do_ResolveName */


#ifdef h_errno

static char *
Get_Error( int H_Error )
{
	/* Get error message for H_Error */

	switch( H_Error )
	{
		case HOST_NOT_FOUND:
			return "host not found";
		case NO_DATA:
			return "name valid but no IP address defined";
		case NO_RECOVERY:
			return "name server error";
		case TRY_AGAIN:
			return "name server temporary not available";
		default:
			return "unknown error";
	}
} /* Get_Error */

#endif


static bool
register_callback( RES_STAT *s, void (*cbfunc)(int, short))
{
	assert(cbfunc != NULL);
	assert(s != NULL);
	assert(s->resolver_fd >= 0);

	if (io_setnonblock(s->resolver_fd) &&
		io_event_create(s->resolver_fd, IO_WANTREAD, cbfunc))
			return true;

	Log( LOG_CRIT, "Resolver: Could not register callback function: %s!", strerror(errno));
	Resolve_Shutdown(s);
	return false;
}


GLOBAL bool
Resolve_Shutdown( RES_STAT *s)
{
	bool ret = false;

	assert(s != NULL);
	assert(s->resolver_fd >= 0);

	if (s->resolver_fd >= 0)
		ret = io_close(s->resolver_fd);

	Resolve_Init(s);
	return ret;
}

                
/**
 * Read result of resolver sub-process from pipe
 */
GLOBAL size_t
Resolve_Read( RES_STAT *s, void* readbuf, size_t buflen)
{
	ssize_t bytes_read;

	assert(buflen > 0);

	/* Read result from pipe */
	bytes_read = read(s->resolver_fd, readbuf, buflen);
	if (bytes_read < 0) {
		if (errno == EAGAIN)
			return 0;

		Log( LOG_CRIT, "Resolver: Can't read result: %s!", strerror(errno));
		bytes_read = 0;
	}
#ifdef DEBUG
	else if (bytes_read == 0)
		Log( LOG_DEBUG, "Resolver: Can't read result: EOF");
#endif
	Resolve_Shutdown(s);
	return (size_t)bytes_read;
}
/* -eof- */

