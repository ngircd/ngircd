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
 * $Id: ngircd.c,v 1.4 2001/12/12 01:58:53 alex Exp $
 *
 * ngircd.c: Hier beginnt alles ;-)
 *
 * $Log: ngircd.c,v $
 * Revision 1.4  2001/12/12 01:58:53  alex
 * - Test auf socklen_t verbessert.
 *
 * Revision 1.3  2001/12/12 01:40:39  alex
 * - ein paar mehr Kommentare; Variablennamen verstaendlicher gemacht.
 * - fehlenden Header <arpa/inet.h> ergaenz.
 * - SIGINT und SIGQUIT werden nun ebenfalls behandelt.
 *
 * Revision 1.2  2001/12/11 22:04:21  alex
 * - Test auf stdint.h (HAVE_STDINT_H) hinzugefuegt.
 *
 * Revision 1.1.1.1  2001/12/11 21:53:04  alex
 * Imported sources to CVS.
 */


#define PORTAB_CHECK_TYPES		/* Prueffunktion einbinden, s.u. */


#include <portab.h>
#include "global.h"

#include <imp.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>			/* u.a. fuer Mac OS X */
#endif

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

      
#include "log.h"

#include <exp.h>
#include "ngircd.h"


BOOLEAN do_quit_now = FALSE;		/* TRUE: Hauptschleife beenden */


LOCAL VOID Signal_Handler( INT Signal );


GLOBAL INT main( INT argc, CONST CHAR *argv[] )
{
	FILE *fd;
	struct sigaction saction;
	struct sockaddr_in my_addr, a_addr;
	int my_sock, a_sock;
	int a_sock_len;

	portab_check_types( );

	Log_Init( );
	
	/* Signal-Handler initialisieren */
	memset( &saction, 0, sizeof( saction ));
	saction.sa_handler = Signal_Handler;

	/* Signal-Handler einhaengen */
	sigaction( SIGALRM, &saction, NULL );
	sigaction( SIGHUP, &saction, NULL);
	sigaction( SIGINT, &saction, NULL );
	sigaction( SIGQUIT, &saction, NULL );
	sigaction( SIGTERM, &saction, NULL);
	sigaction( SIGUSR1, &saction, NULL);
	sigaction( SIGUSR2, &saction, NULL);
	
	/* Server-"Listen"-Socket initialisieren */
	memset( &my_addr, 0, sizeof( my_addr ));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons( 6668 );
	my_addr.sin_addr.s_addr = htonl( INADDR_ANY );

	/* Socket erzeugen, ... */
	my_sock = socket( AF_INET, SOCK_STREAM, 0);
	if( socket < 0 )
	{
		Log( LOG_FATAL, "Can't create socket: %s", strerror( errno ));
		exit( 1 );
	}
	
	/* ... an Port binden ... */
	if( bind( my_sock, (struct sockaddr *)&my_addr, (socklen_t)sizeof( my_addr )) < 0 )
	{
		Log( LOG_FATAL, "Can't bind socket: %s", strerror( errno ));
		exit( 1 );
	}

	/* ... und in "listen mode" gehen :-) */
	if( listen( my_sock, 10 ) < 0 )
	{
		Log( LOG_FATAL, "Can't listen on soecket: %s", strerror( errno ));
		exit( 1 );
	}
	
	/* Hauptschleife */
	while( ! do_quit_now )
	{
		/* auf Verbindung warten */
		a_sock_len = sizeof( a_addr );
		memset( &a_addr, 0, a_sock_len );
		a_sock = accept( my_sock, (struct sockaddr *)&a_addr, &a_sock_len );
		if( a_sock < 0 )
		{
			if( errno == EINTR ) continue;
			
			Log( LOG_FATAL, "Can't accept connection: %s", strerror( errno ));
			exit( 1 );
		}
		Log( LOG_INFO, "Accepted connection from %s:%d.", inet_ntoa( a_addr.sin_addr ), ntohs( a_addr.sin_port));
		fd = fdopen( a_sock, "w" );

		fputs( "hello world!\n", fd ); fflush( fd );
		
		fclose( fd );
		close( a_sock );
        }
        
        /* Aufraeumen (Sockets etc.!?) */
        close( my_sock );

	Log_Exit( );
	return 0;
} /* main */


LOCAL VOID Signal_Handler( INT Signal )
{
	switch( Signal )
	{
		case SIGTERM:
		case SIGINT:
		case SIGQUIT:
			Log( LOG_WARN, "Got signal %d, terminating now ...", Signal );
			do_quit_now = TRUE;
			break;
		default:
			Log( LOG_WARN, "Got signal %d! Ignored.", Signal );
	}
} /* Signal_Handler */


/* -eof- */
