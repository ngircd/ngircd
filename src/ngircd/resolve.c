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

static char UNUSED id[] = "$Id: resolve.c,v 1.8 2004/03/11 22:16:31 alex Exp $";

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


#ifdef IDENTAUTH
LOCAL VOID Do_ResolveAddr PARAMS(( struct sockaddr_in *Addr, INT Sock, INT w_fd ));
#else
LOCAL VOID Do_ResolveAddr PARAMS(( struct sockaddr_in *Addr, INT w_fd ));
#endif

LOCAL VOID Do_ResolveName PARAMS(( CHAR *Host, INT w_fd ));

#ifdef h_errno
LOCAL CHAR *Get_Error PARAMS(( INT H_Error ));
#endif


GLOBAL VOID
Resolve_Init( VOID )
{
	/* Initialize module */

	FD_ZERO( &Resolver_FDs );
} /* Resolve_Init */


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
	INT pid;

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

	/* For sub-process */
	pid = fork( );
	if( pid > 0 )
	{
		/* Main process */
		Log( LOG_DEBUG, "Resolver for %s created (PID %d).", inet_ntoa( Addr->sin_addr ), pid );
		FD_SET( s->pipe[0], &Resolver_FDs );
		if( s->pipe[0] > Conn_MaxFD ) Conn_MaxFD = s->pipe[0];
		s->pid = pid;
		return s;
	}
	else if( pid == 0 )
	{
		/* Sub process */
		Log_Init_Resolver( );
#ifdef IDENTAUTH
		Do_ResolveAddr( Addr, Sock, s->pipe[1] );
#else
		Do_ResolveAddr( Addr, s->pipe[1] );
#endif
		Log_Exit_Resolver( );
		exit( 0 );
	}
	else
	{
		/* Error! */
		free( s );
		Log( LOG_CRIT, "Resolver: Can't fork: %s!", strerror( errno ));
		return NULL;
	}
} /* Resolve_Addr */


GLOBAL RES_STAT *
Resolve_Name( CHAR *Host )
{
	/* Resolve hostname (asynchronous!). On errors, e.g. if the child
	 * process can't be forked, this functions returns NULL. */

	RES_STAT *s;
	INT pid;

	/* Allocate memory */
	s = (RES_STAT *)malloc( sizeof( RES_STAT ));
	if( ! s )
	{
		Log( LOG_EMERG, "Resolver: Can't allocate memory! [Resolve_Name]" );
		return NULL;
	}

	/* Initialize the pipe for the result */
	if( pipe( s->pipe ) != 0 )
	{
		free( s );
		Log( LOG_ALERT, "Resolver: Can't create output pipe: %s!", strerror( errno ));
		return NULL;
	}

	/* Fork sub-process */
	pid = fork( );
	if( pid > 0 )
	{
		/* Main process */
		Log( LOG_DEBUG, "Resolver for \"%s\" created (PID %d).", Host, pid );
		FD_SET( s->pipe[0], &Resolver_FDs );
		if( s->pipe[0] > Conn_MaxFD ) Conn_MaxFD = s->pipe[0];
		s->pid = pid;
		return s;
	}
	else if( pid == 0 )
	{
		/* Sub process */
		Log_Init_Resolver( );
		Do_ResolveName( Host, s->pipe[1] );
		Log_Exit_Resolver( );
		exit( 0 );
	}
	else
	{
		/* Error! */
		free( s );
		Log( LOG_CRIT, "Resolver: Can't fork: %s!", strerror( errno ));
		return NULL;
	}
} /* Resolve_Name */


#ifdef IDENTAUTH
LOCAL VOID
Do_ResolveAddr( struct sockaddr_in *Addr, int Sock, INT w_fd )
#else
LOCAL VOID
Do_ResolveAddr( struct sockaddr_in *Addr, INT w_fd )
#endif
{
	/* Resolver sub-process: resolve IP address and write result into
	 * pipe to parent. */

	CHAR hostname[HOST_LEN];
	struct hostent *h;
#ifdef IDENTAUTH
	CHAR *res;
#endif

	Log_Resolver( LOG_DEBUG, "Now resolving %s ...", inet_ntoa( Addr->sin_addr ));

	/* Resolve IP address */
	h = gethostbyaddr( (CHAR *)&Addr->sin_addr, sizeof( Addr->sin_addr ), AF_INET );
	if( h ) strlcpy( hostname, h->h_name, sizeof( hostname ));
	else
	{
#ifdef h_errno
		Log_Resolver( LOG_WARNING, "Can't resolve address \"%s\": %s!", inet_ntoa( Addr->sin_addr ), Get_Error( h_errno ));
#else
		Log_Resolver( LOG_WARNING, "Can't resolve address \"%s\"!", inet_ntoa( Addr->sin_addr ));
#endif	
		strlcpy( hostname, inet_ntoa( Addr->sin_addr ), sizeof( hostname ));
	}

#ifdef IDENTAUTH
	/* Do "IDENT" (aka "AUTH") lookup and write result to parent */
	Log_Resolver( LOG_DEBUG, "Doing IDENT lookup on socket %d ...", Sock );
	res = ident_id( Sock, 10 );
	Log_Resolver( LOG_DEBUG, "IDENT lookup on socket %d done.", Sock );
#endif

	/* Write result into pipe to parent */
	if( (size_t)write( w_fd, hostname, strlen( hostname ) + 1 ) != (size_t)( strlen( hostname ) + 1 ))
	{
		Log_Resolver( LOG_CRIT, "Resolver: Can't write to parent: %s!", strerror( errno ));
		close( w_fd );
		return;
	}
#ifdef IDENTAUTH
	if( (size_t)write( w_fd, res ? res : "", strlen( res ? res : "" ) + 1 ) != (size_t)( strlen( res ? res : "" ) + 1 ))
	{
		Log_Resolver( LOG_CRIT, "Resolver: Can't write to parent (IDENT): %s!", strerror( errno ));
		close( w_fd );
		free( res );
		return;
	}
	free( res );
#endif

	Log_Resolver( LOG_DEBUG, "Ok, translated %s to \"%s\".", inet_ntoa( Addr->sin_addr ), hostname );
} /* Do_ResolveAddr */


LOCAL VOID
Do_ResolveName( CHAR *Host, INT w_fd )
{
	/* Resolver sub-process: resolve name and write result into pipe
	 * to parent. */

	CHAR ip[16];
	struct hostent *h;
	struct in_addr *addr;

	Log_Resolver( LOG_DEBUG, "Now resolving \"%s\" ...", Host );

	/* Resolve hostname */
	h = gethostbyname( Host );
	if( h )
	{
		addr = (struct in_addr *)h->h_addr;
		strlcpy( ip, inet_ntoa( *addr ), sizeof( ip ));
	}
	else
	{
#ifdef h_errno
		Log_Resolver( LOG_WARNING, "Can't resolve \"%s\": %s!", Host, Get_Error( h_errno ));
#else
		Log_Resolver( LOG_WARNING, "Can't resolve \"%s\"!", Host );
#endif
		strcpy( ip, "" );
	}

	/* Write result into pipe to parent */
	if( (size_t)write( w_fd, ip, strlen( ip ) + 1 ) != (size_t)( strlen( ip ) + 1 ))
	{
		Log_Resolver( LOG_CRIT, "Resolver: Can't write to parent: %s!", strerror( errno ));
		close( w_fd );
		return;
	}

	if( ip[0] ) Log_Resolver( LOG_DEBUG, "Ok, translated \"%s\" to %s.", Host, ip );
} /* Do_ResolveName */


#ifdef h_errno

LOCAL CHAR *
Get_Error( INT H_Error )
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


/* -eof- */
