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
 * $Id: conn.c,v 1.2 2001/12/12 23:32:02 alex Exp $
 *
 * connect.h: Verwaltung aller Netz-Verbindungen ("connections")
 *
 * $Log: conn.c,v $
 * Revision 1.2  2001/12/12 23:32:02  alex
 * - diverse Erweiterungen und Verbesserungen (u.a. sind nun mehrere
 *   Verbindungen und Listen-Sockets moeglich).
 *
 * Revision 1.1  2001/12/12 17:18:38  alex
 * - Modul zur Verwaltung aller Netzwerk-Verbindungen begonnen.
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

#include "ngircd.h"
#include "log.h"
#include "tool.h"

#include <exp.h>
#include "conn.h"


#define MAX_CONNECTIONS 100


typedef struct _Connection
{
	INT sock;			/* Socket Handle */
	struct sockaddr_in addr;	/* Adresse des Client */
	FILE *fd;			/* FILE Handle */
} CONNECTION;


LOCAL VOID Handle_Socket( INT sock );

LOCAL VOID New_Connection( INT Sock );

LOCAL INT Socket2Index( INT Sock );

LOCAL VOID Close_Connection( INT Idx );
LOCAL VOID Read_Data( INT Idx );


LOCAL fd_set My_Listener;
LOCAL fd_set My_Sockets;

LOCAL INT My_Max_Fd;

LOCAL CONNECTION My_Connections[MAX_CONNECTIONS];


GLOBAL VOID Conn_Init( VOID )
{
	INT i;

	/* zu Beginn haben wir keine Verbindungen */
	FD_ZERO( &My_Listener );
	FD_ZERO( &My_Sockets );
	
	My_Max_Fd = 0;
	
	/* Connection-Struktur initialisieren */
	for( i = 0; i < MAX_CONNECTIONS; i++ )
	{
		My_Connections[i].sock = -1;
		My_Connections[i].fd = NULL;
	}
} /* Conn_Init */


GLOBAL VOID Conn_Exit( VOID )
{
	INT idx, i;
	
	/* Sockets schliessen */
	for( i = 0; i < My_Max_Fd + 1; i++ )
	{
		if( FD_ISSET( i, &My_Sockets ))
		{
			for( idx = 0; idx < MAX_CONNECTIONS; idx++ )
			{
				if( My_Connections[idx].sock == i ) break;
			}
			if( idx < MAX_CONNECTIONS ) Close_Connection( idx );
			else if( FD_ISSET( i, &My_Listener ))
			{
				close( i );
				Log( LOG_INFO, "Closed listening socket %d.", i );
			}
			else
			{
				close( i );
				Log( LOG_WARNING, "Unknown connection %d closed.", i );
			}
		}
	}
} /* Conn_Exit */


GLOBAL BOOLEAN Conn_New_Listener( CONST INT Port )
{
	/* Neuen Listen-Socket erzeugen: der Server wartet dann
	 * auf dem angegebenen Port auf Verbindungen. */

	struct sockaddr_in addr;
	INT sock, on = 1;

	/* Server-"Listen"-Socket initialisieren */
	memset( &addr, 0, sizeof( addr ));
	addr.sin_family = AF_INET;
	addr.sin_port = htons( Port );
	addr.sin_addr.s_addr = htonl( INADDR_ANY );

	/* Socket erzeugen */
	sock = socket( PF_INET, SOCK_STREAM, 0);
	if( socket < 0 )
	{
		Log( LOG_ALERT, "Can't create socket: %s", strerror( errno ));
		return FALSE;
	}

	/* Socket-Optionen setzen */
	if( fcntl( sock, F_SETFL, O_NONBLOCK ) != 0 )
	{
		Log( LOG_ALERT, "Can't enable non-blocking mode: %s", strerror( errno ));
		close( sock );
		return FALSE;
	}
	if( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &on, (socklen_t)sizeof( on )) != 0)
	{
		Log( LOG_CRIT, "Can't set socket options: %s", strerror( errno ));
		/* dieser Fehler kann ignoriert werden. */
	}

	/* an Port binden */
	if( bind( sock, (struct sockaddr *)&addr, (socklen_t)sizeof( addr )) != 0 )
	{
		Log( LOG_ALERT, "Can't bind socket: %s", strerror( errno ));
		close( sock );
		return FALSE;
	}

	/* in "listen mode" gehen :-) */
	if( listen( sock, 10 ) != 0 )
	{
		Log( LOG_ALERT, "Can't listen on soecket: %s", strerror( errno ));
		close( sock );
		return FALSE;
	}

	/* Neuen Listener in Strukturen einfuegen */
	FD_SET( sock, &My_Listener );
	FD_SET( sock, &My_Sockets );
	
	if( sock > My_Max_Fd ) My_Max_Fd = sock;

	Log( LOG_INFO, "Now listening on port %d, socked %d.", Port, sock );

	return TRUE;
} /* Conn_New_Listener */


GLOBAL VOID Conn_Handler( VOID )
{
	fd_set read_sockets;
	INT i;
	
	read_sockets = My_Sockets;
	if( select( My_Max_Fd + 1, &read_sockets, NULL, NULL, NULL ) == -1 )
	{
		if( NGIRCd_Quit ) return;
		Log( LOG_ALERT, "select(): %s", strerror( errno ));
		return;
	}
	
	for( i = 0; i < My_Max_Fd + 1; i++ )
	{
		if( FD_ISSET( i, &read_sockets )) Handle_Socket( i );
	}
} /* Conn_Handler */


LOCAL VOID Handle_Socket( INT Sock )
{
	/* Aktivitaet auf einem Socket verarbeiten */

	INT idx;

	if( FD_ISSET( Sock, &My_Listener ))
	{
		/* es ist einer unserer Listener-Sockets: es soll
		 * also eine neue Verbindung aufgebaut werden. */

		New_Connection( Sock );
	}
	else
	{
		/* Ein Client Socket: entweder ein User oder Server */
		
		idx = Socket2Index( Sock );
		Read_Data( idx );
	}
} /* Handle_Socket */


LOCAL VOID New_Connection( INT Sock )
{
	struct sockaddr_in new_addr;
	INT new_sock, new_sock_len, idx;
	FILE *fd;

	new_sock_len = sizeof( new_addr );
	new_sock = accept( Sock, (struct sockaddr *)&new_addr, &new_sock_len );
	if( new_sock < 0 )
	{
		Log( LOG_CRIT, "Can't accept connection: %s", strerror( errno ));
		return;
	}
		
	/* Freie Connection-Struktur suschen */
	for( idx = 0; idx < MAX_CONNECTIONS; idx++ ) if( My_Connections[idx].sock < 0 ) break;
	if( idx >= MAX_CONNECTIONS )
	{
		Log( LOG_ALERT, "Connection limit (%d) reached!", MAX_CONNECTIONS );
		close( new_sock );
		return;
	}

	fd = fdopen( new_sock, "r+" );
		
	/* Verbindung registrieren */
	My_Connections[idx].sock = new_sock;
	My_Connections[idx].fd = fd;
	My_Connections[idx].addr = new_addr;

	/* Neuen Socket registrieren */
	FD_SET( new_sock, &My_Sockets );

	if( new_sock > My_Max_Fd ) My_Max_Fd = new_sock;

	fputs( "hello world!\n", fd ); fflush( fd );

	Log( LOG_INFO, "Accepted connection from %s:%d.", inet_ntoa( new_addr.sin_addr ), ntohs( new_addr.sin_port));
} /* New_Connection */


LOCAL INT Socket2Index( INT Sock )
{
	INT idx;
	
	for( idx = 0; idx < MAX_CONNECTIONS; idx++ ) if( My_Connections[idx].sock == Sock ) break;
	assert( idx < MAX_CONNECTIONS );
	
	return idx;
} /* Socket2Index */


LOCAL VOID Close_Connection( INT Idx )
{
	/* Verbindung schlie§en */

	assert( My_Connections[Idx].sock >= 0 );
	
	if( fclose( My_Connections[Idx].fd ) != 0 )
	{
		Log( LOG_ERR, "Error closing connection with %s:%d - %s", inet_ntoa( My_Connections[Idx].addr.sin_addr ), ntohs( My_Connections[Idx].addr.sin_port), strerror( errno ));
	}
	else
	{
		Log( LOG_INFO, "Closed connection with %s:%d.", inet_ntoa( My_Connections[Idx].addr.sin_addr ), ntohs( My_Connections[Idx].addr.sin_port ));
		close( My_Connections[Idx].sock );
	}

	FD_CLR( My_Connections[Idx].sock, &My_Sockets );

	My_Connections[Idx].sock = -1;
	My_Connections[Idx].fd = NULL;
} /* Close_Connection */


LOCAL VOID Read_Data( INT Idx )
{
	/* Daten von Socket einlesen */

	#define SIZE 256
	
	CHAR buffer[SIZE];
	
	if( ! fgets( buffer, SIZE, My_Connections[Idx].fd ))
	{
		Close_Connection( Idx );
		return;
	}
	
	ngt_Trim_Str( buffer );
	printf( " in: '%s'\n", buffer );
} /* Read_Data */


/* -eof- */
