/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an comBase beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: conn.c,v 1.1 2001/12/12 17:18:38 alex Exp $
 *
 * connect.h: Verwaltung aller Netz-Verbindungen ("connections")
 *
 * $Log: conn.c,v $
 * Revision 1.1  2001/12/12 17:18:38  alex
 * - Modul zur Verwaltung aller Netzwerk-Verbindungen begonnen.
 *
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h> 
#include <sys/time.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>			/* u.a. fuer Mac OS X */
#endif

#include "log.h"

#include <exp.h>
#include "conn.h"


LOCAL INT my_sock;


GLOBAL VOID Conn_Init( VOID )
{
	/* ... */
} /* Conn_Init */


GLOBAL VOID Conn_Exit( VOID )
{
	/* ... */
} /* Conn_Exit */


GLOBAL BOOLEAN Conn_New_Listener( CONST INT Port )
{
	/* Neuen Listen-Socket erzeugen: der Server wartet dann
	 * auf dem angegebenen Port auf Verbindungen. */

	struct sockaddr_in my_addr;
	INT on = 1;
	
	/* Server-"Listen"-Socket initialisieren */
	memset( &my_addr, 0, sizeof( my_addr ));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons( Port );
	my_addr.sin_addr.s_addr = htonl( INADDR_ANY );

	/* Socket erzeugen */
	my_sock = socket( PF_INET, SOCK_STREAM, 0);
	if( socket < 0 )
	{
		Log( LOG_FATAL, "Can't create socket: %s", strerror( errno ));
		return FALSE;
	}

	/* Socket-Optionen setzen */
	if( fcntl( my_sock, F_SETFL, O_NONBLOCK ) != 0 )
	{
		Log( LOG_FATAL, "Can't enable non-blocking mode: %s", strerror( errno ));
		close( my_sock );
		return FALSE;
	}
	if( setsockopt( my_sock, SOL_SOCKET, SO_REUSEADDR, &on, (socklen_t)sizeof( on )) != 0)
	{
		Log( LOG_ERROR, "Can't set socket options: %s", strerror( errno ));
		/* dieser Fehler kann ignoriert werden. */
	}
        	
	/* an Port binden */
	if( bind( my_sock, (struct sockaddr *)&my_addr, (socklen_t)sizeof( my_addr )) != 0 )
	{
		Log( LOG_FATAL, "Can't bind socket: %s", strerror( errno ));
		close( my_sock );
		return FALSE;
	}

	/* in "listen mode" gehen :-) */
	if( listen( my_sock, 10 ) != 0 )
	{
		Log( LOG_FATAL, "Can't listen on soecket: %s", strerror( errno ));
		close( my_sock );
		return FALSE;
	}
	
	return TRUE;
} /* Conn_New_Listener */


GLOBAL VOID Conn_Handler( VOID )
{
	struct sockaddr_in a_addr;
	INT a_sock, a_sock_len;
	FILE *fd;

	/* auf Verbindung warten */
	a_sock_len = sizeof( a_addr );
	a_sock = accept( my_sock, (struct sockaddr *)&a_addr, &a_sock_len );
	if( a_sock < 0 )
	{
		if( errno == EINTR ) return;
		Log( LOG_ERROR, "Can't accept connection: %s", strerror( errno ));
		return;
	}
	Log( LOG_INFO, "Accepted connection from %s:%d.", inet_ntoa( a_addr.sin_addr ), ntohs( a_addr.sin_port));
	fd = fdopen( a_sock, "w" );

	fputs( "hello world!\n", fd ); fflush( fd );

	fclose( fd );
	close( a_sock );
} /* Conn_Handler */


/* -eof- */
