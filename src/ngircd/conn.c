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
 * $Id: conn.c,v 1.5 2001/12/14 08:16:47 alex Exp $
 *
 * connect.h: Verwaltung aller Netz-Verbindungen ("connections")
 *
 * $Log: conn.c,v $
 * Revision 1.5  2001/12/14 08:16:47  alex
 * - Begonnen, Client-spezifische Lesepuffer zu implementieren.
 * - Umstellung auf Datentyp "CONN_ID".
 *
 * Revision 1.4  2001/12/13 02:04:16  alex
 * - boesen "Speicherschiesser" in Log() gefixt.
 *
 * Revision 1.3  2001/12/13 01:33:09  alex
 * - Conn_Handler() unterstuetzt nun einen Timeout.
 * - fuer Verbindungen werden keine FILE-Handles mehr benutzt.
 * - kleinere "Code Cleanups" ;-)
 *
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


#define MAX_CONNECTIONS 100		/* max. Anzahl von Verbindungen an diesem Server */

#define LINE_LEN 512			/* Laenge eines Befehls, vgl. RFC 2812, 3.2 */
#define BUFFER_LEN 2 * LINE_LEN;	/* Laenge des Lesepuffers je Verbindung */


typedef struct _Connection
{
	INT sock;			/* Socket Handle */
	struct sockaddr_in addr;	/* Adresse des Client */
	CHAR buffer[BUFFER_LEN];	/* Lesepuffer */
	INT datalen;			/* Laenge der Daten im Puffer */
} CONNECTION;


LOCAL VOID Handle_Socket( INT sock );

LOCAL VOID New_Connection( INT Sock );

LOCAL CONN_ID Socket2Index( INT Sock );

LOCAL VOID Close_Connection( CONN_ID Idx );
LOCAL VOID Read_Request( CONN_ID Idx );

LOCAL BOOLEAN Send( CONN_ID Idx, CHAR *Data );


LOCAL fd_set My_Listener;
LOCAL fd_set My_Sockets;

LOCAL INT My_Max_Fd;

LOCAL CONNECTION My_Connections[MAX_CONNECTIONS];


GLOBAL VOID Conn_Init( VOID )
{
	CONN_ID i;

	/* zu Beginn haben wir keine Verbindungen */
	FD_ZERO( &My_Listener );
	FD_ZERO( &My_Sockets );
	
	My_Max_Fd = 0;
	
	/* Connection-Struktur initialisieren */
	for( i = 0; i < MAX_CONNECTIONS; i++ )
	{
		My_Connections[i].sock = NONE;
		My_Connections[i].datalen = 0;
	}
} /* Conn_Init */


GLOBAL VOID Conn_Exit( VOID )
{
	CONN_ID idx;
	INT i;
	
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
	if( sock < 0 )
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

	Log( LOG_INFO, "Now listening on port %d, socket %d.", Port, sock );

	return TRUE;
} /* Conn_New_Listener */


GLOBAL VOID Conn_Handler( INT Timeout )
{
	fd_set read_sockets;
	struct timeval tv;
	INT i;

	/* Timeout initialisieren */
	tv.tv_sec = Timeout;
	tv.tv_usec = 0;
	
	read_sockets = My_Sockets;
	if( select( My_Max_Fd + 1, &read_sockets, NULL, NULL, &tv ) == -1 )
	{
		if( errno != EINTR ) Log( LOG_ALERT, "select(): %s", strerror( errno ));
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

	CONN_ID idx;

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
		Read_Request( idx );
	}
} /* Handle_Socket */


LOCAL VOID New_Connection( INT Sock )
{
	struct sockaddr_in new_addr;
	INT new_sock, new_sock_len;
	CONN_ID idx;

	new_sock_len = sizeof( new_addr );
	new_sock = accept( Sock, (struct sockaddr *)&new_addr, (socklen_t *)&new_sock_len );
	if( new_sock < 0 )
	{
		Log( LOG_CRIT, "Can't accept connection: %s", strerror( errno ));
		return;
	}
		
	/* Freie Connection-Struktur suschen */
	for( idx = 0; idx < MAX_CONNECTIONS; idx++ ) if( My_Connections[idx].sock < 0 ) break;
	if( idx >= MAX_CONNECTIONS )
	{
		Log( LOG_ALERT, "Can't accept connection: limit (%d) reached!", MAX_CONNECTIONS );
		close( new_sock );
		return;
	}

	/* Verbindung registrieren */
	My_Connections[idx].sock = new_sock;
	My_Connections[idx].addr = new_addr;

	/* Neuen Socket registrieren */
	FD_SET( new_sock, &My_Sockets );

	if( new_sock > My_Max_Fd ) My_Max_Fd = new_sock;

	Send( idx, "hello world!\n" );

	Log( LOG_INFO, "Accepted connection from %s:%d on socket %d.", inet_ntoa( new_addr.sin_addr ), ntohs( new_addr.sin_port), Sock );
} /* New_Connection */


LOCAL CONN_ID Socket2Index( INT Sock )
{
	CONN_ID idx;
	
	for( idx = 0; idx < MAX_CONNECTIONS; idx++ ) if( My_Connections[idx].sock == Sock ) break;
	assert( idx < MAX_CONNECTIONS );
	
	return idx;
} /* Socket2Index */


LOCAL VOID Close_Connection( CONN_ID Idx )
{
	/* Verbindung schlie§en */

	assert( My_Connections[Idx].sock >= 0 );
	
	if( close( My_Connections[Idx].sock ) != 0 )
	{
		Log( LOG_ERR, "Error closing connection with %s:%d - %s", inet_ntoa( My_Connections[Idx].addr.sin_addr ), ntohs( My_Connections[Idx].addr.sin_port), strerror( errno ));
	}
	else
	{
		Log( LOG_INFO, "Closed connection with %s:%d.", inet_ntoa( My_Connections[Idx].addr.sin_addr ), ntohs( My_Connections[Idx].addr.sin_port ));
	}

	FD_CLR( My_Connections[Idx].sock, &My_Sockets );
	My_Connections[Idx].sock = NONE;
} /* Close_Connection */


LOCAL VOID Read_Request( CONN_ID Idx )
{
	/* Daten von Socket einlesen und entsprechend behandeln.
	 * Tritt ein Fehler auf, so wird der Socket geschlossen. */

	INT len;
	
	len = recv( My_Connections[Idx].sock, My_Connection[Idx].buffer + My_Connection[Idx].datalen, BUFFER_LEN - My_Connection[Idx].datalen - 1, 0 );
	My_Connection[Idx].buffer[BUFFER_LEN] = '\0';

	if( len == 0 )
	{
		/* Socket wurde geschlossen */
		Close_Connection( Idx );
		return;
	}

	if( len < 0 )
	{
		/* Fehler beim Lesen */
		Log( LOG_ALERT, "Read error on socket %d!", My_Connections[Idx].sock );
		Close_Connection( Idx );
		return;
	}

	My_Connection[Idx].datalen += len;
	
	ngt_Trim_Str( buffer );
	printf( " in: '%s'\n", buffer );
} /* Read_Data */


LOCAL BOOLEAN Send( CONN_ID Idx, CHAR *Data )
{
	/* Daten in Socket schreiben, ggf. in mehreren Stuecken. Tritt
	 * ein Fehler auf, so wird die Verbindung beendet und FALSE
	 * als Rueckgabewert geliefert. */
	
	INT n, sent, len;
		
	sent = 0;
	len = strlen( Data );
	
	while( sent < len )
	{
		n = send( My_Connections[Idx].sock, Data + sent, len - sent, 0 );
		if( n <= 0 )
		{
			/* Oops, ein Fehler! */
			Log( LOG_ALERT, "Write error on socket %d!", My_Connections[Idx].sock );
			Close_Connection( Idx );
			return FALSE;
		}
		sent += n;
	}
	return TRUE;
} /* Send */


/* -eof- */
