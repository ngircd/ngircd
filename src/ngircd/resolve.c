/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: resolve.c,v 1.1 2002/05/27 11:23:27 alex Exp $
 *
 * resolve.c: asyncroner Resolver
 */


#include "portab.h"

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

#include "conn.h"
#include "defines.h"
#include "log.h"

#include "exp.h"
#include "resolve.h"


LOCAL VOID Do_ResolveAddr PARAMS(( struct sockaddr_in *Addr, INT w_fd ));
LOCAL VOID Do_ResolveName PARAMS(( CHAR *Host, INT w_fd ));

#ifdef h_errno
LOCAL CHAR *Resolve_Error PARAMS(( INT H_Error ));
#endif


GLOBAL VOID
Resolve_Init( VOID )
{
	/* Modul initialisieren */

	FD_ZERO( &Resolver_FDs );
} /* Resolve_Init */


GLOBAL RES_STAT *
Resolve_Addr( struct sockaddr_in *Addr )
{
	/* IP (asyncron!) aufloesen. Bei Fehler, z.B. wenn der
	 * Child-Prozess nicht erzeugt werden kann, wird NULL geliefert.
	 * Der Host kann dann nicht aufgeloest werden. */

	RES_STAT *s;
	INT pid;

	/* Speicher anfordern */
	s = malloc( sizeof( RES_STAT ));
	if( ! s )
	{
		Log( LOG_EMERG, "Resolver: Can't allocate memory!" );
		return NULL;
	}

	/* Pipe fuer Antwort initialisieren */
	if( pipe( s->pipe ) != 0 )
	{
		free( s );
		Log( LOG_ALERT, "Resolver: Can't create output pipe: %s!", strerror( errno ));
		return NULL;
	}

	/* Sub-Prozess erzeugen */
	pid = fork( );
	if( pid > 0 )
	{
		/* Haupt-Prozess */
		Log( LOG_DEBUG, "Resolver for %s created (PID %d).", inet_ntoa( Addr->sin_addr ), pid );
		FD_SET( s->pipe[0], &Resolver_FDs );
		if( s->pipe[0] > Conn_MaxFD ) Conn_MaxFD = s->pipe[0];
		s->pid = pid;
		return s;
	}
	else if( pid == 0 )
	{
		/* Sub-Prozess */
		Log_Init_Resolver( );
		Do_ResolveAddr( Addr, s->pipe[1] );
		Log_Exit_Resolver( );
		exit( 0 );
	}
	else
	{
		/* Fehler */
		free( s );
		Log( LOG_CRIT, "Resolver: Can't fork: %s!", strerror( errno ));
		return NULL;
	}
} /* Resolve_Addr */


GLOBAL RES_STAT *
Resolve_Name( CHAR *Host )
{
	/* Hostnamen (asyncron!) aufloesen. Bei Fehler, z.B. wenn der
	 * Child-Prozess nicht erzeugt werden kann, wird NULL geliefert.
	 * Der Host kann dann nicht aufgeloest werden. */

	RES_STAT *s;
	INT pid;

	/* Speicher anfordern */
	s = malloc( sizeof( RES_STAT ));
	if( ! s )
	{
		Log( LOG_EMERG, "Resolver: Can't allocate memory!" );
		return NULL;
	}

	/* Pipe fuer Antwort initialisieren */
	if( pipe( s->pipe ) != 0 )
	{
		free( s );
		Log( LOG_ALERT, "Resolver: Can't create output pipe: %s!", strerror( errno ));
		return NULL;
	}

	/* Sub-Prozess erzeugen */
	pid = fork( );
	if( pid > 0 )
	{
		/* Haupt-Prozess */
		Log( LOG_DEBUG, "Resolver for \"%s\" created (PID %d).", Host, pid );
		FD_SET( s->pipe[0], &Resolver_FDs );
		if( s->pipe[0] > Conn_MaxFD ) Conn_MaxFD = s->pipe[0];
		s->pid = pid;
		return s;
	}
	else if( pid == 0 )
	{
		/* Sub-Prozess */
		Log_Init_Resolver( );
		Do_ResolveName( Host, s->pipe[1] );
		Log_Exit_Resolver( );
		exit( 0 );
	}
	else
	{
		/* Fehler */
		free( s );
		Log( LOG_CRIT, "Resolver: Can't fork: %s!", strerror( errno ));
		return NULL;
	}
} /* Resolve_Name */


LOCAL VOID
Do_ResolveAddr( struct sockaddr_in *Addr, INT w_fd )
{
	/* Resolver Sub-Prozess: IP aufloesen und Ergebnis in Pipe schreiben. */

	CHAR hostname[HOST_LEN];
	struct hostent *h;

	Log_Resolver( LOG_DEBUG, "Now resolving %s ...", inet_ntoa( Addr->sin_addr ));

	/* Namen aufloesen */
	h = gethostbyaddr( (CHAR *)&Addr->sin_addr, sizeof( Addr->sin_addr ), AF_INET );
	if( h ) strcpy( hostname, h->h_name );
	else
	{
#ifdef h_errno
		Log_Resolver( LOG_WARNING, "Can't resolve address \"%s\": %s!", inet_ntoa( Addr->sin_addr ), Get_Error( h_errno ));
#else
		Log_Resolver( LOG_WARNING, "Can't resolve address \"%s\"!", inet_ntoa( Addr->sin_addr ));
#endif	
		strcpy( hostname, inet_ntoa( Addr->sin_addr ));
	}

	/* Antwort an Parent schreiben */
	if( write( w_fd, hostname, strlen( hostname ) + 1 ) != ( strlen( hostname ) + 1 ))
	{
		Log_Resolver( LOG_CRIT, "Resolver: Can't write to parent: %s!", strerror( errno ));
		close( w_fd );
		return;
	}

	Log_Resolver( LOG_DEBUG, "Ok, translated %s to \"%s\".", inet_ntoa( Addr->sin_addr ), hostname );
} /* Do_ResolveAddr */


LOCAL VOID
Do_ResolveName( CHAR *Host, INT w_fd )
{
	/* Resolver Sub-Prozess: Name aufloesen und Ergebnis in Pipe schreiben. */

	CHAR ip[16];
	struct hostent *h;
	struct in_addr *addr;

	Log_Resolver( LOG_DEBUG, "Now resolving \"%s\" ...", Host );

	/* Namen aufloesen */
	h = gethostbyname( Host );
	if( h )
	{
		addr = (struct in_addr *)h->h_addr;
		strcpy( ip, inet_ntoa( *addr ));
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

	/* Antwort an Parent schreiben */
	if( write( w_fd, ip, strlen( ip ) + 1 ) != ( strlen( ip ) + 1 ))
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
	/* Fehlerbeschreibung fuer H_Error liefern */

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
