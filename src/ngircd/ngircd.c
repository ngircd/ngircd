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
 * $Id: ngircd.c,v 1.2 2001/12/11 22:04:21 alex Exp $
 *
 * ngircd.c: Hier beginnt alles ;-)
 *
 * $Log: ngircd.c,v $
 * Revision 1.2  2001/12/11 22:04:21  alex
 * - Test auf stdint.h (HAVE_STDINT_H) hinzugefuegt.
 *
 * Revision 1.1.1.1  2001/12/11 21:53:04  alex
 * Imported sources to CVS.
 *
 */


#define PORTAB_CHECK_TYPES

#ifndef socklen_t
#define socklen_t int
#endif

#include <portab.h>
#include "global.h"

#include <imp.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include <netinet/in.h>
      
#include "log.h"

#include <exp.h>
#include "ngircd.h"


BOOLEAN do_quit_now = FALSE;


LOCAL VOID Signal_Handler( INT Signal );


GLOBAL INT main( INT argc, CONST CHAR *argv[] )
{
	FILE *fd;
	struct sigaction saction;
	struct sockaddr_in my_sock, a_sock;
	int my_sock_hndl;
	int a_sock_len, a_hndl;

	portab_check_types( );

	Log_Init( );
	
	/* Signal-Handler initialisieren */
	memset( &saction, 0, sizeof( saction ));
	saction.sa_handler = Signal_Handler;

	/* Signal-Handler einhaengen */
	sigaction( SIGHUP, &saction, NULL);
	sigaction( SIGTERM, &saction, NULL);
	sigaction( SIGUSR1, &saction, NULL);
	sigaction( SIGUSR2, &saction, NULL);
	
	/* Server-"Listen"-Socket initialisieren */
	memset( &my_sock, 0, sizeof( my_sock ));
	my_sock.sin_family = AF_INET;
	my_sock.sin_port = htons( 6668 );
	my_sock.sin_addr.s_addr = htonl( INADDR_ANY );

	/* Socket erzeugen, ... */
	my_sock_hndl = socket( AF_INET, SOCK_STREAM, 0);
	if( socket < 0 )
	{
		Log( LOG_FATAL, "Can't create socket: %s", strerror( errno ));
		exit( 1 );
	}
	
	/* ... an Port binden ... */
	if( bind( my_sock_hndl, (struct sockaddr *)&my_sock, (socklen_t)sizeof( my_sock )) < 0 )
	{
		Log( LOG_FATAL, "Can't bind socket: %s", strerror( errno ));
		exit( 1 );
	}

	/* ... und in "listen mode" gehen :-) */
	if( listen( my_sock_hndl, 4 ) < 0 )
	{
		Log( LOG_FATAL, "Can't listen on soecket: %s", strerror( errno ));
		exit( 1 );
	}
	
	/* Hauptschleife */
	while( ! do_quit_now )
	{
		/* auf Verbindung warten */
		a_sock_len = sizeof( a_sock );
		memset( &a_sock, 0, a_sock_len );
		a_hndl = accept( my_sock_hndl, (struct sockaddr *)&a_sock, &a_sock_len );
		if( a_hndl < 0 )
		{
			if( errno == EINTR ) continue;
			
			Log( LOG_FATAL, "Can't accept connection: %s", strerror( errno ));
			exit( 1 );
		}
		Log( LOG_INFO, "Accepted connection from %s:%d (handle %d).", inet_ntoa( a_sock.sin_addr ), ntohs( a_sock.sin_port), a_hndl );
		fd = fdopen( a_hndl, "w" );

		fputs( "hello world!\n", fd ); fflush( fd );
		
		fclose( fd );
		close( a_hndl );
        }
        
        /* Aufraeumen (Sockets etc.!?) */

	Log_Exit( );
	return 0;
} /* main */


LOCAL VOID Signal_Handler( INT Signal )
{
	switch( Signal )
	{
		case SIGTERM:
			Log( LOG_WARN, "Got SIGTERM, terminating now ..." );
			do_quit_now = TRUE;
			break;
		default:
			Log( LOG_WARN, "Got signal %d! I'll ignore it.", Signal );
	}
} /* Signal_Handler */


/* -eof- */
