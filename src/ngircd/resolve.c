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

static char UNUSED id[] = "$Id: resolve.c,v 1.20 2005/09/11 11:42:48 fw Exp $";

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


#ifdef IDENTAUTH
static void Do_ResolveAddr PARAMS(( struct sockaddr_in *Addr, int Sock, int w_fd ));
#else
static void Do_ResolveAddr PARAMS(( struct sockaddr_in *Addr, int w_fd ));
#endif

static void Do_ResolveName PARAMS(( char *Host, int w_fd ));

#ifdef h_errno
static char *Get_Error PARAMS(( int H_Error ));
#endif

static RES_STAT *New_Res_Stat PARAMS(( void ));


static void
cb_resolver(int fd, short unused) {
	(void) unused;	/* shut up compiler warning */
	Read_Resolver_Result(fd);
}


#ifdef IDENTAUTH
GLOBAL RES_STAT *
Resolve_Addr( struct sockaddr_in *Addr, int Sock )
#else
GLOBAL RES_STAT *
Resolve_Addr( struct sockaddr_in *Addr )
#endif
{
	/* Resolve IP (asynchronous!). On errors, e.g. if the child process
	 * can't be forked, this functions returns NULL. */

	RES_STAT *s;
	int pid;

	s = New_Res_Stat( );
	if( ! s ) return NULL;

	/* For sub-process */
	pid = fork( );
	if (pid > 0) {
		close( s->pipe[1] );
		/* Main process */
		Log( LOG_DEBUG, "Resolver for %s created (PID %d).", inet_ntoa( Addr->sin_addr ), pid );
		if (!io_setnonblock( s->pipe[0] )) {
			Log( LOG_DEBUG, "Could not set Non-Blocking mode for pipefd %d", s->pipe[0] );
			goto out;
		}
		if (!io_event_create( s->pipe[0], IO_WANTREAD, cb_resolver )) {
			Log( LOG_DEBUG, "Could not add pipefd %dto event watchlist: %s",
								s->pipe[0], strerror(errno) );
			goto out;
		}
		s->pid = pid;
		return s;
	} else if( pid == 0 ) {
		close( s->pipe[0] );
		/* Sub process */
		Log_Init_Resolver( );
#ifdef IDENTAUTH
		Do_ResolveAddr( Addr, Sock, s->pipe[1] );
#else
		Do_ResolveAddr( Addr, s->pipe[1] );
#endif
		Log_Exit_Resolver( );
		exit(0);
	}
	
	Log( LOG_CRIT, "Resolver: Can't fork: %s!", strerror( errno ));

out: /* Error! */
	close( s->pipe[0] );
	close( s->pipe[1] );
	free( s );
return NULL;
} /* Resolve_Addr */


GLOBAL RES_STAT *
Resolve_Name( char *Host )
{
	/* Resolve hostname (asynchronous!). On errors, e.g. if the child
	 * process can't be forked, this functions returns NULL. */

	RES_STAT *s;
	int pid;

	s = New_Res_Stat( );
	if( ! s ) return NULL;

	/* Fork sub-process */
	pid = fork( );
	if (pid > 0) {
		close( s->pipe[1] );
		/* Main process */
		Log( LOG_DEBUG, "Resolver for \"%s\" created (PID %d).", Host, pid );
		if (!io_setnonblock( s->pipe[0] )) {
			Log( LOG_DEBUG, "Could not set Non-Blocking mode for pipefd %d", s->pipe[0] );
			goto out;
		}
		if (!io_event_create( s->pipe[0], IO_WANTREAD, cb_resolver )) {
			Log( LOG_DEBUG, "Could not add pipefd %dto event watchlist: %s",
								s->pipe[0], strerror(errno) );
			goto out;
		}
		s->pid = pid;
		return s;
	} else if( pid == 0 ) {
		close( s->pipe[0] );
		/* Sub process */
		Log_Init_Resolver( );
		Do_ResolveName( Host, s->pipe[1] );
		Log_Exit_Resolver( );
		exit(0);
	}

	Log( LOG_CRIT, "Resolver: Can't fork: %s!", strerror( errno ));

out: /* Error! */
	close( s->pipe[0] );
	close( s->pipe[1] );
	free( s );
	return NULL;
} /* Resolve_Name */


#ifdef IDENTAUTH
static void
Do_ResolveAddr( struct sockaddr_in *Addr, int Sock, int w_fd )
#else
static void
Do_ResolveAddr( struct sockaddr_in *Addr, int w_fd )
#endif
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
	/* Do "IDENT" (aka "AUTH") lookup and append result to resolved_addr array */
	Log_Resolver( LOG_DEBUG, "Doing IDENT lookup on socket %d ...", Sock );
	res = ident_id( Sock, 10 );
	Log_Resolver( LOG_DEBUG, "Ok, IDENT lookup on socket %d done: \"%s\"", Sock, res ? res : "" );

	if (res) {
		if (!array_cats(&resolved_addr, res))
			Log_Resolver(LOG_WARNING, "Resolver: Cannot copy IDENT result: %s!", strerror(errno));
		/* try to omit ident and return hostname only */ 
	}

	if (!array_catb(&resolved_addr, "\n", 1)) {
		close(w_fd);
		Log_Resolver(LOG_CRIT, "Resolver: Cannot copy result: %s!", strerror(errno));
		array_free(&resolved_addr);
		return;
	}

	if (res) free(res);
#endif
	len = array_bytes(&resolved_addr);
	if( (size_t)write( w_fd, array_start(&resolved_addr), len) != len )
		Log_Resolver( LOG_CRIT, "Resolver: Can't write result to parent: %s!", strerror( errno ));

	close(w_fd);
	array_free(&resolved_addr);
} /* Do_ResolveAddr */


static void
Do_ResolveName( char *Host, int w_fd )
{
	/* Resolver sub-process: resolve name and write result into pipe
	 * to parent. */

	char ip[16];
	struct hostent *h;
	struct in_addr *addr;
	int len;

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
		ip[0] = '\0';
	}
	if( ip[0] ) Log_Resolver( LOG_DEBUG, "Ok, translated \"%s\" to %s.", Host, ip );

	/* Write result into pipe to parent */
	len = strlen( ip );
	ip[len] = '\n'; len++;
	if( (size_t)write( w_fd, ip, len ) != (size_t)len ) {
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


static RES_STAT *
New_Res_Stat( void )
{
	RES_STAT *s;

	/* Allocate memory */
	s = (RES_STAT *)malloc( sizeof( RES_STAT ));
	if( ! s )
	{
		Log( LOG_EMERG, "Resolver: Can't allocate memory! [Resolve_Addr]" );
		return NULL;
	}

	/* Initialize pipe for result */
	if( pipe( s->pipe ) != 0 )
	{
		free( s );
		Log( LOG_ALERT, "Resolver: Can't create output pipe: %s!", strerror( errno ));
		return NULL;
	}

	s->stage = 0;
	array_init(&s->buffer);
	s->pid = -1;

	return s;
} /* New_Res_Stat */


/* -eof- */
