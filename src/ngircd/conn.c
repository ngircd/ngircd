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
 * Connection management
 */


#define CONN_MODULE

#include "portab.h"

static char UNUSED id[] = "$Id: conn.c,v 1.125 2003/09/11 12:05:28 alex Exp $";

#include "imp.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <netinet/in.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#else
#define PF_INET AF_INET
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>			/* e.g. for Mac OS X */
#endif

#ifdef USE_TCPWRAP
#include <tcpd.h>			/* for TCP Wrappers */
#endif

#include "defines.h"
#include "resolve.h"

#include "exp.h"
#include "conn.h"

#include "imp.h"
#include "ngircd.h"
#include "client.h"
#include "conf.h"
#include "conn-zip.h"
#include "conn-func.h"
#include "log.h"
#include "parse.h"
#include "tool.h"

#ifdef RENDEZVOUS
#include "rendezvous.h"
#endif

#include "exp.h"


#define SERVER_WAIT (NONE - 1)


LOCAL VOID Handle_Read PARAMS(( INT sock ));
LOCAL BOOLEAN Handle_Write PARAMS(( CONN_ID Idx ));
LOCAL VOID New_Connection PARAMS(( INT Sock ));
LOCAL CONN_ID Socket2Index PARAMS(( INT Sock ));
LOCAL VOID Read_Request PARAMS(( CONN_ID Idx ));
LOCAL BOOLEAN Try_Write PARAMS(( CONN_ID Idx ));
LOCAL BOOLEAN Handle_Buffer PARAMS(( CONN_ID Idx ));
LOCAL VOID Check_Connections PARAMS(( VOID ));
LOCAL VOID Check_Servers PARAMS(( VOID ));
LOCAL VOID Init_Conn_Struct PARAMS(( CONN_ID Idx ));
LOCAL BOOLEAN Init_Socket PARAMS(( INT Sock ));
LOCAL VOID New_Server PARAMS(( INT Server, CONN_ID Idx ));
LOCAL VOID Read_Resolver_Result PARAMS(( INT r_fd ));
LOCAL VOID Simple_Message PARAMS(( INT Sock, CHAR *Msg ));

LOCAL fd_set My_Listeners;
LOCAL fd_set My_Sockets;
LOCAL fd_set My_Connects;

#ifdef USE_TCPWRAP
INT allow_severity = LOG_INFO;
INT deny_severity = LOG_ERR;
#endif


GLOBAL VOID
Conn_Init( VOID )
{
	/* Modul initialisieren: statische Strukturen "ausnullen". */

	CONN_ID i;

	/* Speicher fuer Verbindungs-Pool anfordern */
	Pool_Size = CONNECTION_POOL;
	if( Conf_MaxConnections > 0 )
	{
		/* konfiguriertes Limit beachten */
		if( Pool_Size > Conf_MaxConnections ) Pool_Size = Conf_MaxConnections;
	}
	My_Connections = malloc( sizeof( CONNECTION ) * Pool_Size );
	if( ! My_Connections )
	{
		/* Speicher konnte nicht alloziert werden! */
		Log( LOG_EMERG, "Can't allocate memory! [Conn_Init]" );
		exit( 1 );
	}
	Log( LOG_DEBUG, "Allocated connection pool for %d items (%ld bytes).", Pool_Size, sizeof( CONNECTION ) * Pool_Size );

	/* zu Beginn haben wir keine Verbindungen */
	FD_ZERO( &My_Listeners );
	FD_ZERO( &My_Sockets );
	FD_ZERO( &My_Connects );

	/* Groesster File-Descriptor fuer select() */
	Conn_MaxFD = 0;

	/* Connection-Struktur initialisieren */
	for( i = 0; i < Pool_Size; i++ ) Init_Conn_Struct( i );

	/* Global write counter */
	WCounter = 0;
} /* Conn_Init */


GLOBAL VOID
Conn_Exit( VOID )
{
	/* Modul abmelden: alle noch offenen Connections
	 * schliessen und freigeben. */

	CONN_ID idx;
	INT i;

	Log( LOG_DEBUG, "Shutting down all connections ..." );

#ifdef RENDEZVOUS
	Rendezvous_UnregisterListeners( );
#endif

	/* Sockets schliessen */
	for( i = 0; i < Conn_MaxFD + 1; i++ )
	{
		if( FD_ISSET( i, &My_Sockets ))
		{
			for( idx = 0; idx < Pool_Size; idx++ )
			{
				if( My_Connections[idx].sock == i ) break;
			}
			if( FD_ISSET( i, &My_Listeners ))
			{
				close( i );
				Log( LOG_DEBUG, "Listening socket %d closed.", i );
			}
			else if( FD_ISSET( i, &My_Connects ))
			{
				close( i );
				Log( LOG_DEBUG, "Connection %d closed during creation (socket %d).", idx, i );
			}
			else if( idx < Pool_Size )
			{
				if( NGIRCd_SignalRestart ) Conn_Close( idx, NULL, "Server going down (restarting)", TRUE );
				else Conn_Close( idx, NULL, "Server going down", TRUE );
			}
			else
			{
				Log( LOG_WARNING, "Closing unknown connection %d ...", i );
				close( i );
			}
		}
	}

	free( My_Connections );
	My_Connections = NULL;
	Pool_Size = 0;
} /* Conn_Exit */


GLOBAL INT
Conn_InitListeners( VOID )
{
	/* Initialize ports on which the server should accept connections */

	INT created, i;

	created = 0;
	for( i = 0; i < Conf_ListenPorts_Count; i++ )
	{
		if( Conn_NewListener( Conf_ListenPorts[i] )) created++;
		else Log( LOG_ERR, "Can't listen on port %u!", Conf_ListenPorts[i] );
	}
	return created;
} /* Conn_InitListeners */


GLOBAL VOID
Conn_ExitListeners( VOID )
{
	/* Close down all listening sockets */

	INT i;

#ifdef RENDEZVOUS
	Rendezvous_UnregisterListeners( );
#endif

	Log( LOG_INFO, "Shutting down all listening sockets ..." );
	for( i = 0; i < Conn_MaxFD + 1; i++ )
	{
		if( FD_ISSET( i, &My_Sockets ) && FD_ISSET( i, &My_Listeners ))
		{
			close( i );
			Log( LOG_DEBUG, "Listening socket %d closed.", i );
		}
	}
} /* Conn_ExitListeners */


GLOBAL BOOLEAN
Conn_NewListener( CONST UINT Port )
{
	/* Create new listening socket on specified port */

	struct sockaddr_in addr;
	struct in_addr inaddr;
	INT sock;
#ifdef RENDEZVOUS
	CHAR name[CLIENT_ID_LEN], *info;
#endif

	/* Server-"Listen"-Socket initialisieren */
	memset( &addr, 0, sizeof( addr ));
	memset( &inaddr, 0, sizeof( inaddr ));
	addr.sin_family = AF_INET;
	addr.sin_port = htons( Port );
	if( Conf_ListenAddress[0] )
	{
#ifdef HAVE_INET_ATON
		if( inet_aton( Conf_ListenAddress, &inaddr ) == 0 )
#else
		inaddr.s_addr = inet_addr( Conf_ListenAddress );
		if( inaddr.s_addr == (unsigned)-1 )
#endif
		{
			Log( LOG_CRIT, "Can't listen on %s:%u: can't convert ip address %s!", Conf_ListenAddress, Port, Conf_ListenAddress );
			return FALSE;
		}
	}
	else inaddr.s_addr = htonl( INADDR_ANY );
	addr.sin_addr = inaddr;

	/* Socket erzeugen */
	sock = socket( PF_INET, SOCK_STREAM, 0);
	if( sock < 0 )
	{
		Log( LOG_CRIT, "Can't create socket: %s!", strerror( errno ));
		return FALSE;
	}

	if( ! Init_Socket( sock )) return FALSE;

	/* an Port binden */
	if( bind( sock, (struct sockaddr *)&addr, (socklen_t)sizeof( addr )) != 0 )
	{
		Log( LOG_CRIT, "Can't bind socket: %s!", strerror( errno ));
		close( sock );
		return FALSE;
	}

	/* in "listen mode" gehen :-) */
	if( listen( sock, 10 ) != 0 )
	{
		Log( LOG_CRIT, "Can't listen on soecket: %s!", strerror( errno ));
		close( sock );
		return FALSE;
	}

	/* Neuen Listener in Strukturen einfuegen */
	FD_SET( sock, &My_Listeners );
	FD_SET( sock, &My_Sockets );

	if( sock > Conn_MaxFD ) Conn_MaxFD = sock;

	if( Conf_ListenAddress[0]) Log( LOG_INFO, "Now listening on %s:%d (socket %d).", Conf_ListenAddress, Port, sock );
	else Log( LOG_INFO, "Now listening on 0.0.0.0:%d (socket %d).", Port, sock );

#ifdef RENDEZVOUS
	/* Get best server description text */
	if( ! Conf_ServerInfo[0] ) info = Conf_ServerName;
	else
	{
		/* Use server info string */
		info = NULL;
		if( Conf_ServerInfo[0] == '[' )
		{
			/* Cut off leading hostname part in "[]" */
			info = strchr( Conf_ServerInfo, ']' );
			if( info )
			{
				info++;
				while( *info == ' ' ) info++;
			}
		}
		if( ! info ) info = Conf_ServerInfo;
	}

	/* Add port number to description if non-standard */
	if( Port != 6667 ) snprintf( name, sizeof( name ), "%s (port %u)", info, Port );
	else strlcpy( name, info, sizeof( name ));

	/* Register service */
	Rendezvous_Register( name, RENDEZVOUS_TYPE, Port );
#endif

	return TRUE;
} /* Conn_NewListener */


GLOBAL VOID
Conn_Handler( VOID )
{
	/* "Hauptschleife": Aktive Verbindungen ueberwachen. Folgende Aktionen
	 * werden dabei durchgefuehrt, bis der Server terminieren oder neu
	 * starten soll:
	 *
	 *  - neue Verbindungen annehmen,
	 *  - Server-Verbindungen aufbauen,
	 *  - geschlossene Verbindungen loeschen,
	 *  - volle Schreibpuffer versuchen zu schreiben,
	 *  - volle Lesepuffer versuchen zu verarbeiten,
	 *  - Antworten von Resolver Sub-Prozessen annehmen.
	 */

	fd_set read_sockets, write_sockets;
	struct timeval tv;
	time_t start, t;
	CONN_ID i, idx;
	BOOLEAN timeout;

	start = time( NULL );
	while(( ! NGIRCd_SignalQuit ) && ( ! NGIRCd_SignalRestart ))
	{
		timeout = TRUE;

#ifdef RENDEZVOUS
		Rendezvous_Handler( );
#endif

		/* Should the configuration be reloaded? */
		if( NGIRCd_SignalRehash ) NGIRCd_Rehash( );

		/* Check configured servers and established links */
		Check_Servers( );
		Check_Connections( );

		/* noch volle Lese-Buffer suchen */
		for( i = 0; i < Pool_Size; i++ )
		{
			if(( My_Connections[i].sock > NONE ) && ( My_Connections[i].rdatalen > 0 ))
			{
				/* Kann aus dem Buffer noch ein Befehl extrahiert werden? */
				if( Handle_Buffer( i )) timeout = FALSE;
			}
		}

		/* noch volle Schreib-Puffer suchen */
		FD_ZERO( &write_sockets );
		for( i = 0; i < Pool_Size; i++ )
		{
#ifdef USE_ZLIB
			if(( My_Connections[i].sock > NONE ) && (( My_Connections[i].wdatalen > 0 ) || ( My_Connections[i].zip.wdatalen > 0 )))
#else
			if(( My_Connections[i].sock > NONE ) && ( My_Connections[i].wdatalen > 0 ))
#endif
			{
				/* Socket der Verbindung in Set aufnehmen */
				FD_SET( My_Connections[i].sock, &write_sockets );
			}
		}

		/* Sockets mit im Aufbau befindlichen ausgehenden Verbindungen suchen */
		for( i = 0; i < Pool_Size; i++ )
		{
			if(( My_Connections[i].sock > NONE ) && ( FD_ISSET( My_Connections[i].sock, &My_Connects ))) FD_SET( My_Connections[i].sock, &write_sockets );
		}

		/* von welchen Sockets koennte gelesen werden? */
		t = time( NULL );
		read_sockets = My_Sockets;
		for( i = 0; i < Pool_Size; i++ )
		{
			if(( My_Connections[i].sock > NONE ) && ( My_Connections[i].host[0] == '\0' ))
			{
				/* Hier muss noch auf den Resolver Sub-Prozess gewartet werden */
				FD_CLR( My_Connections[i].sock, &read_sockets );
			}
			if(( My_Connections[i].sock > NONE ) && ( FD_ISSET( My_Connections[i].sock, &My_Connects )))
			{
				/* Hier laeuft noch ein asyncrones connect() */
				FD_CLR( My_Connections[i].sock, &read_sockets );
			}
			if( My_Connections[i].delaytime > t )
			{
				/* Fuer die Verbindung ist eine "Penalty-Zeit" gesetzt */
				FD_CLR( My_Connections[i].sock, &read_sockets );
			}
		}
		for( i = 0; i < Conn_MaxFD + 1; i++ )
		{
			/* Pipes von Resolver Sub-Prozessen aufnehmen */
			if( FD_ISSET( i, &Resolver_FDs ))
			{
				FD_SET( i, &read_sockets );
			}
		}

		/* Timeout initialisieren */
		tv.tv_usec = 0;
		if( timeout ) tv.tv_sec = TIME_RES;
		else tv.tv_sec = 0;

		/* Auf Aktivitaet warten */
		i = select( Conn_MaxFD + 1, &read_sockets, &write_sockets, NULL, &tv );
		if( i == 0 )
		{
			/* keine Veraenderung an den Sockets */
			continue;
		}
		if( i == -1 )
		{
			/* Fehler (z.B. Interrupt) */
			if( errno != EINTR )
			{
				Log( LOG_EMERG, "Conn_Handler(): select(): %s!", strerror( errno ));
				Log( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE_NAME );
				exit( 1 );
			}
			continue;
		}

		/* Koennen Daten geschrieben werden? */
		for( i = 0; i < Conn_MaxFD + 1; i++ )
		{
			if( ! FD_ISSET( i, &write_sockets )) continue;

			/* Es kann geschrieben werden ... */
			idx = Socket2Index( i );
			if( idx == NONE ) continue;

			if( ! Handle_Write( idx ))
			{
				/* Fehler beim Schreiben! Diesen Socket nun
				 * auch aus dem Read-Set entfernen: */
				FD_CLR( i, &read_sockets );
			}
		}

		/* Daten zum Lesen vorhanden? */
		for( i = 0; i < Conn_MaxFD + 1; i++ )
		{
			if( FD_ISSET( i, &read_sockets )) Handle_Read( i );
		}
	}

	if( NGIRCd_SignalQuit ) Log( LOG_NOTICE|LOG_snotice, "Server going down NOW!" );
	else if( NGIRCd_SignalRestart ) Log( LOG_NOTICE|LOG_snotice, "Server restarting NOW!" );
} /* Conn_Handler */


#ifdef PROTOTYPES
GLOBAL BOOLEAN
Conn_WriteStr( CONN_ID Idx, CHAR *Format, ... )
#else
GLOBAL BOOLEAN
Conn_WriteStr( Idx, Format, va_alist )
CONN_ID Idx;
CHAR *Format;
va_dcl
#endif
{
	/* String in Socket schreiben. CR+LF wird von dieser Funktion
	 * automatisch angehaengt. Im Fehlerfall wird dir Verbindung
	 * getrennt und FALSE geliefert. */

	CHAR buffer[COMMAND_LEN];
	BOOLEAN ok;
	va_list ap;

	assert( Idx > NONE );
	assert( Format != NULL );

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	if( vsnprintf( buffer, COMMAND_LEN - 2, Format, ap ) == COMMAND_LEN - 2 )
	{
		Log( LOG_CRIT, "Text too long to send (connection %d)!", Idx );
		Conn_Close( Idx, "Text too long to send!", NULL, FALSE );
		return FALSE;
	}

#ifdef SNIFFER
	if( NGIRCd_Sniffer ) Log( LOG_DEBUG, " -> connection %d: '%s'.", Idx, buffer );
#endif

	strlcat( buffer, "\r\n", sizeof( buffer ));
	ok = Conn_Write( Idx, buffer, strlen( buffer ));
	My_Connections[Idx].msg_out++;

	va_end( ap );
	return ok;
} /* Conn_WriteStr */


GLOBAL BOOLEAN
Conn_Write( CONN_ID Idx, CHAR *Data, INT Len )
{
	/* Daten in Socket schreiben. Bei "fatalen" Fehlern wird
	 * der Client disconnectiert und FALSE geliefert. */

	assert( Idx > NONE );
	assert( Data != NULL );
	assert( Len > 0 );

	/* Ist der entsprechende Socket ueberhaupt noch offen? In einem
	 * "Handler-Durchlauf" kann es passieren, dass dem nicht mehr so
	 * ist, wenn einer von mehreren Conn_Write()'s fehlgeschlagen ist.
	 * In diesem Fall wird hier einfach ein Fehler geliefert. */
	if( My_Connections[Idx].sock <= NONE )
	{
		Log( LOG_DEBUG, "Skipped write on closed socket (connection %d).", Idx );
		return FALSE;
	}

	/* Pruefen, ob im Schreibpuffer genuegend Platz ist. Ziel ist es,
	 * moeglichts viel im Puffer zu haben und _nicht_ gleich alles auf den
	 * Socket zu schreiben (u.a. wg. Komprimierung). */
	if( WRITEBUFFER_LEN - My_Connections[Idx].wdatalen - Len <= 0 )
	{
		/* Der Puffer ist dummerweise voll. Jetzt versuchen, den Puffer
		 * zu schreiben, wenn das nicht klappt, haben wir ein Problem ... */
		if( ! Try_Write( Idx )) return FALSE;

		/* nun neu pruefen: */
		if( WRITEBUFFER_LEN - My_Connections[Idx].wdatalen - Len <= 0 )
		{
			Log( LOG_NOTICE, "Write buffer overflow (connection %d)!", Idx );
			Conn_Close( Idx, "Write buffer overflow!", NULL, FALSE );
			return FALSE;
		}
	}

#ifdef USE_ZLIB
	if( My_Connections[Idx].options & CONN_ZIP )
	{
		/* Daten komprimieren und in Puffer kopieren */
		if( ! Zip_Buffer( Idx, Data, Len )) return FALSE;
	}
	else
#endif
	{
		/* Daten in Puffer kopieren */
		memcpy( My_Connections[Idx].wbuf + My_Connections[Idx].wdatalen, Data, Len );
		My_Connections[Idx].wdatalen += Len;
		My_Connections[Idx].bytes_out += Len;
	}

	/* Adjust global write counter */
	WCounter += Len;

	return TRUE;
} /* Conn_Write */


GLOBAL VOID
Conn_Close( CONN_ID Idx, CHAR *LogMsg, CHAR *FwdMsg, BOOLEAN InformClient )
{
	/* Close connection. Open pipes of asyncronous resolver
	 * sub-processes are closed down. */

	CLIENT *c;
	DOUBLE in_k, out_k;
#ifdef USE_ZLIB
	DOUBLE in_z_k, out_z_k;
	INT in_p, out_p;
#endif

	assert( Idx > NONE );
	assert( My_Connections[Idx].sock > NONE );

	/* Is this link already shutting down? */
	if( My_Connections[Idx].options & CONN_ISCLOSING )
	{
		/* Conn_Close() has been called recursively for this link;
		 * probabe reason: Try_Write() failed  -- see below. */
		return;
	}

	/* Mark link as "closing" */
	My_Connections[Idx].options |= CONN_ISCLOSING;

	/* Search client, if any */
	c = Client_GetFromConn( Idx );

	/* Should the client be informed? */
	if( InformClient )
	{
#ifndef STRICT_RFC
		/* Send statistics to client if registered as user: */
		if(( c != NULL ) && ( Client_Type( c ) == CLIENT_USER ))
		{
			Conn_WriteStr( Idx, "NOTICE %s :%sConnection statistics: client %.1f kb, server %.1f kb.", Client_ThisServer( ), NOTICE_TXTPREFIX, (DOUBLE)My_Connections[Idx].bytes_in / 1024,  (DOUBLE)My_Connections[Idx].bytes_out / 1024 );
		}
#endif

		/* Send ERROR to client (see RFC!) */
		if( FwdMsg ) Conn_WriteStr( Idx, "ERROR :%s", FwdMsg );
		else Conn_WriteStr( Idx, "ERROR :Closing connection." );
		if( My_Connections[Idx].sock == NONE ) return;
	}

	/* Try to write out the write buffer */
	(VOID)Try_Write( Idx );

	/* Shut down socket */
	if( close( My_Connections[Idx].sock ) != 0 )
	{
		/* Oops, we can't close the socket!? This is fatal! */
		Log( LOG_EMERG, "Error closing connection %d (socket %d) with %s:%d - %s!", Idx, My_Connections[Idx].sock, My_Connections[Idx].host, ntohs( My_Connections[Idx].addr.sin_port), strerror( errno ));
		Log( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE_NAME );
		exit( 1 );
	}

	/* Mark socket as invalid: */
	FD_CLR( My_Connections[Idx].sock, &My_Sockets );
	FD_CLR( My_Connections[Idx].sock, &My_Connects );
	My_Connections[Idx].sock = NONE;

	/* If there is still a client, unregister it now */
	if( c ) Client_Destroy( c, LogMsg, FwdMsg, TRUE );

	/* Calculate statistics and log information */
	in_k = (DOUBLE)My_Connections[Idx].bytes_in / 1024;
	out_k = (DOUBLE)My_Connections[Idx].bytes_out / 1024;
#ifdef USE_ZLIB
	if( My_Connections[Idx].options & CONN_ZIP )
	{
		in_z_k = (DOUBLE)My_Connections[Idx].zip.bytes_in / 1024;
		out_z_k = (DOUBLE)My_Connections[Idx].zip.bytes_out / 1024;
		in_p = (INT)(( in_k * 100 ) / in_z_k );
		out_p = (INT)(( out_k * 100 ) / out_z_k );
		Log( LOG_INFO, "Connection %d with %s:%d closed (in: %.1fk/%.1fk/%d%%, out: %.1fk/%.1fk/%d%%).", Idx, My_Connections[Idx].host, ntohs( My_Connections[Idx].addr.sin_port ), in_k, in_z_k, in_p, out_k, out_z_k, out_p );
	}
	else
#endif
	{
		Log( LOG_INFO, "Connection %d with %s:%d closed (in: %.1fk, out: %.1fk).", Idx, My_Connections[Idx].host, ntohs( My_Connections[Idx].addr.sin_port ), in_k, out_k );
	}

	/* Is there a resolver sub-process running? */
	if( My_Connections[Idx].res_stat )
	{
		/* Free resolver structures */
		FD_CLR( My_Connections[Idx].res_stat->pipe[0], &Resolver_FDs );
		close( My_Connections[Idx].res_stat->pipe[0] );
		close( My_Connections[Idx].res_stat->pipe[1] );
		free( My_Connections[Idx].res_stat );
	}

	/* Servers: Modify time of next connect attempt? */
	Conf_UnsetServer( Idx );

#ifdef USE_ZLIB
	/* Clean up zlib, if link was compressed */
	if( Conn_Options( Idx ) & CONN_ZIP )
	{
		inflateEnd( &My_Connections[Idx].zip.in );
		deflateEnd( &My_Connections[Idx].zip.out );
	}
#endif

	/* Clean up connection structure (=free it) */
	Init_Conn_Struct( Idx );
} /* Conn_Close */


GLOBAL VOID
Conn_SyncServerStruct( VOID )
{
	/* Synchronize server structures (connection IDs):
	 * connections <-> configuration */

	CLIENT *client;
	CONN_ID i;
	INT c;

	for( i = 0; i < Pool_Size; i++ )
	{
		/* Established connection? */
		if( My_Connections[i].sock <= NONE ) continue;

		/* Server connection? */
		client = Client_GetFromConn( i );
		if(( ! client ) || ( Client_Type( client ) != CLIENT_SERVER )) continue;

		for( c = 0; c < MAX_SERVERS; c++ )
		{
			/* Configured server? */
			if( ! Conf_Server[c].host[0] ) continue;

			/* Duplicate? */
			if( strcmp( Conf_Server[c].name, Client_ID( client )) == 0 ) Conf_Server[c].conn_id = i;
		}
	}
} /* SyncServerStruct */


LOCAL BOOLEAN
Try_Write( CONN_ID Idx )
{
	/* Versuchen, Daten aus dem Schreib-Puffer in den Socket zu
	 * schreiben. TRUE wird geliefert, wenn entweder keine Daten
	 * zum Versenden vorhanden sind oder erfolgreich bearbeitet
	 * werden konnten. Im Fehlerfall wird FALSE geliefert und
	 * die Verbindung geschlossen. */

	fd_set write_socket;
	struct timeval tv;

	assert( Idx > NONE );
	assert( My_Connections[Idx].sock > NONE );

	/* sind ueberhaupt Daten vorhanden? */
#ifdef USE_ZLIB
	if(( ! My_Connections[Idx].wdatalen > 0 ) && ( ! My_Connections[Idx].zip.wdatalen )) return TRUE;
#else
	if( ! My_Connections[Idx].wdatalen > 0 ) return TRUE;
#endif

	/* Timeout initialisieren: 0 Sekunden, also nicht blockieren */
	tv.tv_sec = 0; tv.tv_usec = 0;

	FD_ZERO( &write_socket );
	FD_SET( My_Connections[Idx].sock, &write_socket );
	if( select( My_Connections[Idx].sock + 1, NULL, &write_socket, NULL, &tv ) == -1 )
	{
		/* Fehler! */
		if( errno != EINTR )
		{
			Log( LOG_ALERT, "Try_Write(): select() failed: %s (con=%d, sock=%d)!", strerror( errno ), Idx, My_Connections[Idx].sock );
			Conn_Close( Idx, "Server error!", NULL, FALSE );
			return FALSE;
		}
	}

	if( FD_ISSET( My_Connections[Idx].sock, &write_socket )) return Handle_Write( Idx );
	else return TRUE;
} /* Try_Write */


LOCAL VOID
Handle_Read( INT Sock )
{
	/* Aktivitaet auf einem Socket verarbeiten:
	 *  - neue Clients annehmen,
	 *  - Daten von Clients verarbeiten,
	 *  - Resolver-Rueckmeldungen annehmen. */

	CONN_ID idx;

	assert( Sock > NONE );

	if( FD_ISSET( Sock, &My_Listeners ))
	{
		/* es ist einer unserer Listener-Sockets: es soll
		 * also eine neue Verbindung aufgebaut werden. */

		New_Connection( Sock );
	}
	else if( FD_ISSET( Sock, &Resolver_FDs ))
	{
		/* Rueckmeldung von einem Resolver Sub-Prozess */

		Read_Resolver_Result( Sock );
	}
	else
	{
		/* Ein Client Socket: entweder ein User oder Server */

		idx = Socket2Index( Sock );
		if( idx > NONE ) Read_Request( idx );
	}
} /* Handle_Read */


LOCAL BOOLEAN
Handle_Write( CONN_ID Idx )
{
	/* Daten aus Schreibpuffer versenden bzw. Connection aufbauen */

	INT len, res, err;
	socklen_t sock_len;
	CLIENT *c;

	assert( Idx > NONE );
	assert( My_Connections[Idx].sock > NONE );

	if( FD_ISSET( My_Connections[Idx].sock, &My_Connects ))
	{
		/* es soll nichts geschrieben werden, sondern ein
		 * connect() hat ein Ergebnis geliefert */

		FD_CLR( My_Connections[Idx].sock, &My_Connects );

		/* Ergebnis des connect() ermitteln */
		sock_len = sizeof( err );
		res = getsockopt( My_Connections[Idx].sock, SOL_SOCKET, SO_ERROR, &err, &sock_len );
		assert( sock_len == sizeof( err ));

		/* Fehler aufgetreten? */
		if(( res != 0 ) || ( err != 0 ))
		{
			/* Fehler! */
			if( res != 0 ) Log( LOG_CRIT, "getsockopt (connection %d): %s!", Idx, strerror( errno ));
			else Log( LOG_CRIT, "Can't connect socket to \"%s:%d\" (connection %d): %s!", My_Connections[Idx].host, Conf_Server[Conf_GetServer( Idx )].port, Idx, strerror( err ));

			/* Clean up socket, connection and client structures */
			FD_CLR( My_Connections[Idx].sock, &My_Sockets );
			c = Client_GetFromConn( Idx );
			if( c ) Client_DestroyNow( c );
			close( My_Connections[Idx].sock );
			Init_Conn_Struct( Idx );

			/* Bei Server-Verbindungen lasttry-Zeitpunkt auf "jetzt" setzen */
			Conf_Server[Conf_GetServer( Idx )].lasttry = time( NULL );
			Conf_UnsetServer( Idx );

			return FALSE;
		}
		Log( LOG_DEBUG, "Connection %d with \"%s:%d\" established, now sendig PASS and SERVER ...", Idx, My_Connections[Idx].host, Conf_Server[Conf_GetServer( Idx )].port );

		/* PASS und SERVER verschicken */
		Conn_WriteStr( Idx, "PASS %s %s", Conf_Server[Conf_GetServer( Idx )].pwd_out, NGIRCd_ProtoID );
		return Conn_WriteStr( Idx, "SERVER %s :%s", Conf_ServerName, Conf_ServerInfo );
	}

#ifdef USE_ZLIB
	/* Schreibpuffer leer, aber noch Daten im Kompressionsbuffer?
	 * Dann muss dieser nun geflushed werden! */
	if( My_Connections[Idx].wdatalen == 0 ) Zip_Flush( Idx );
#endif

	assert( My_Connections[Idx].wdatalen > 0 );

	/* Daten schreiben */
	len = send( My_Connections[Idx].sock, My_Connections[Idx].wbuf, My_Connections[Idx].wdatalen, 0 );
	if( len < 0 )
	{
		/* Operation haette Socket "nur" blockiert ... */
		if( errno == EAGAIN ) return TRUE;

		/* Oops, ein Fehler! */
		Log( LOG_ERR, "Write error on connection %d (socket %d): %s!", Idx, My_Connections[Idx].sock, strerror( errno ));
		Conn_Close( Idx, "Write error!", NULL, FALSE );
		return FALSE;
	}

	/* Puffer anpassen */
	My_Connections[Idx].wdatalen -= len;
	memmove( My_Connections[Idx].wbuf, My_Connections[Idx].wbuf + len, My_Connections[Idx].wdatalen );

	return TRUE;
} /* Handle_Write */


LOCAL VOID
New_Connection( INT Sock )
{
	/* Neue Client-Verbindung von Listen-Socket annehmen und
	 * CLIENT-Struktur anlegen. */

#ifdef USE_TCPWRAP
	struct request_info req;
#endif
	struct sockaddr_in new_addr;
	INT new_sock, new_sock_len;
	RES_STAT *s;
	CONN_ID idx;
	CLIENT *c;
	POINTER *ptr;
	LONG new_size;

	assert( Sock > NONE );

	/* Connection auf Listen-Socket annehmen */
	new_sock_len = sizeof( new_addr );
	new_sock = accept( Sock, (struct sockaddr *)&new_addr, (socklen_t *)&new_sock_len );
	if( new_sock < 0 )
	{
		Log( LOG_CRIT, "Can't accept connection: %s!", strerror( errno ));
		return;
	}

#ifdef USE_TCPWRAP
	/* Validate socket using TCP Wrappers */
	request_init( &req, RQ_DAEMON, PACKAGE_NAME, RQ_FILE, new_sock, RQ_CLIENT_SIN, &new_addr, NULL );
	if( ! hosts_access( &req ))
	{
		/* Access denied! */
		Log( deny_severity, "Refused connection from %s (by TCP Wrappers)!", inet_ntoa( new_addr.sin_addr ));
		Simple_Message( new_sock, "ERROR :Connection refused" );
		close( new_sock );
		return;
	}
#endif

	/* Socket initialisieren */
	Init_Socket( new_sock );

	/* Freie Connection-Struktur suchen */
	for( idx = 0; idx < Pool_Size; idx++ ) if( My_Connections[idx].sock == NONE ) break;
	if( idx >= Pool_Size )
	{
		new_size = Pool_Size + CONNECTION_POOL;

		/* Im bisherigen Pool wurde keine freie Connection-Struktur mehr gefunden.
		 * Wenn erlaubt und moeglich muss nun der Pool vergroessert werden: */

		if( Conf_MaxConnections > 0 )
		{
			/* Es ist ein Limit konfiguriert */
			if( Pool_Size >= Conf_MaxConnections )
			{
				/* Mehr Verbindungen duerfen wir leider nicht mehr annehmen ... */
				Log( LOG_ALERT, "Can't accept connection: limit (%d) reached!", Pool_Size );
				Simple_Message( new_sock, "ERROR :Connection limit reached" );
				close( new_sock );
				return;
			}
			if( new_size > Conf_MaxConnections ) new_size = Conf_MaxConnections;
		}
		if( new_size < Pool_Size )
		{
			Log( LOG_ALERT, "Can't accespt connection: limit (%d) reached -- overflow!", Pool_Size );
			Simple_Message( new_sock, "ERROR :Connection limit reached" );
			close( new_sock );
			return;
		}

		/* zunaechst realloc() versuchen; wenn das scheitert, malloc() versuchen
		 * und Daten ggf. "haendisch" umkopieren. (Haesslich! Eine wirklich
		 * dynamische Verwaltung waere wohl _deutlich_ besser ...) */
		ptr = realloc( My_Connections, sizeof( CONNECTION ) * new_size );
		if( ! ptr )
		{
			/* realloc() ist fehlgeschlagen. Nun malloc() probieren: */
			ptr = malloc( sizeof( CONNECTION ) * new_size );
			if( ! ptr )
			{
				/* Offenbar steht kein weiterer Sepeicher zur Verfuegung :-( */
				Log( LOG_EMERG, "Can't allocate memory! [New_Connection]" );
				Simple_Message( new_sock, "ERROR: Internal error" );
				close( new_sock );
				return;
			}

			/* Struktur umkopieren ... */
			memcpy( ptr, My_Connections, sizeof( CONNECTION ) * Pool_Size );

			Log( LOG_DEBUG, "Allocated new connection pool for %ld items (%ld bytes). [malloc()/memcpy()]", new_size, sizeof( CONNECTION ) * new_size );
		}
		else Log( LOG_DEBUG, "Allocated new connection pool for %ld items (%ld bytes). [realloc()]", new_size, sizeof( CONNECTION ) * new_size );

		/* Adjust pointer to new block */
		My_Connections = ptr;

		/* Initialize new items */
		for( idx = Pool_Size; idx < new_size; idx++ ) Init_Conn_Struct( idx );
		idx = Pool_Size;

		/* Adjust new pool size */
		Pool_Size = new_size;
	}

	/* Client-Struktur initialisieren */
	c = Client_NewLocal( idx, inet_ntoa( new_addr.sin_addr ), CLIENT_UNKNOWN, FALSE );
	if( ! c )
	{
		Log( LOG_ALERT, "Can't accept connection: can't create client structure!" );
		Simple_Message( new_sock, "ERROR :Internal error" );
		close( new_sock );
		return;
	}

	/* Verbindung registrieren */
	Init_Conn_Struct( idx );
	My_Connections[idx].sock = new_sock;
	My_Connections[idx].addr = new_addr;

	/* Neuen Socket registrieren */
	FD_SET( new_sock, &My_Sockets );
	if( new_sock > Conn_MaxFD ) Conn_MaxFD = new_sock;

	Log( LOG_INFO, "Accepted connection %d from %s:%d on socket %d.", idx, inet_ntoa( new_addr.sin_addr ), ntohs( new_addr.sin_port), Sock );

	/* Hostnamen ermitteln */
	strlcpy( My_Connections[idx].host, inet_ntoa( new_addr.sin_addr ), sizeof( My_Connections[idx].host ));
	Client_SetHostname( c, My_Connections[idx].host );
	s = Resolve_Addr( &new_addr );
	if( s )
	{
		/* Sub-Prozess wurde asyncron gestartet */
		My_Connections[idx].res_stat = s;
	}

	/* Penalty-Zeit setzen */
	Conn_SetPenalty( idx, 4 );
} /* New_Connection */


LOCAL CONN_ID
Socket2Index( INT Sock )
{
	/* zum Socket passende Connection suchen */

	CONN_ID idx;

	assert( Sock > NONE );

	for( idx = 0; idx < Pool_Size; idx++ ) if( My_Connections[idx].sock == Sock ) break;

	if( idx >= Pool_Size )
	{
		/* die Connection wurde vermutlich (wegen eines
		 * Fehlers) bereits wieder abgebaut ... */
		Log( LOG_DEBUG, "Socket2Index: can't get connection for socket %d!", Sock );
		return NONE;
	}
	else return idx;
} /* Socket2Index */


LOCAL VOID
Read_Request( CONN_ID Idx )
{
	/* Daten von Socket einlesen und entsprechend behandeln.
	 * Tritt ein Fehler auf, so wird der Socket geschlossen. */

	INT len, bsize;
#ifdef USE_ZLIB
	CLIENT *c;
#endif

	assert( Idx > NONE );
	assert( My_Connections[Idx].sock > NONE );

	/* wenn noch nicht registriert: maximal mit ZREADBUFFER_LEN arbeiten,
	 * ansonsten koennen Daten ggf. nicht umkopiert werden. */
	bsize = READBUFFER_LEN;
#ifdef USE_ZLIB
	c = Client_GetFromConn( Idx );
	if(( Client_Type( c ) != CLIENT_USER ) && ( Client_Type( c ) != CLIENT_SERVER ) && ( Client_Type( c ) != CLIENT_SERVICE ) && ( bsize > ZREADBUFFER_LEN )) bsize = ZREADBUFFER_LEN;
#endif

#ifdef USE_ZLIB
	if(( bsize - My_Connections[Idx].rdatalen - 1 < 1 ) || ( ZREADBUFFER_LEN - My_Connections[Idx].zip.rdatalen < 1 ))
#else
	if( bsize - My_Connections[Idx].rdatalen - 1 < 1 )
#endif
	{
		/* Der Lesepuffer ist voll */
		Log( LOG_ERR, "Read buffer overflow (connection %d): %d bytes!", Idx, My_Connections[Idx].rdatalen );
		Conn_Close( Idx, "Read buffer overflow!", NULL, FALSE );
		return;
	}

#ifdef USE_ZLIB
	if( My_Connections[Idx].options & CONN_ZIP )
	{
		len = recv( My_Connections[Idx].sock, My_Connections[Idx].zip.rbuf + My_Connections[Idx].zip.rdatalen, ( ZREADBUFFER_LEN - My_Connections[Idx].zip.rdatalen ), 0 );
		if( len > 0 ) My_Connections[Idx].zip.rdatalen += len;
	}
	else
#endif
	{
		len = recv( My_Connections[Idx].sock, My_Connections[Idx].rbuf + My_Connections[Idx].rdatalen, bsize - My_Connections[Idx].rdatalen - 1, 0 );
		if( len > 0 ) My_Connections[Idx].rdatalen += len;
	}

	if( len == 0 )
	{
		/* Socket wurde geschlossen */
		Log( LOG_INFO, "%s:%d (%s) is closing the connection ...", My_Connections[Idx].host, ntohs( My_Connections[Idx].addr.sin_port), inet_ntoa( My_Connections[Idx].addr.sin_addr ));
		Conn_Close( Idx, "Socket closed!", "Client closed connection", FALSE );
		return;
	}

	if( len < 0 )
	{
		/* Operation haette Socket "nur" blockiert ... */
		if( errno == EAGAIN ) return;

		/* Fehler beim Lesen */
		Log( LOG_ERR, "Read error on connection %d (socket %d): %s!", Idx, My_Connections[Idx].sock, strerror( errno ));
		Conn_Close( Idx, "Read error!", "Client closed connection", FALSE );
		return;
	}

	/* Connection-Statistik aktualisieren */
	My_Connections[Idx].bytes_in += len;

	/* Timestamp aktualisieren */
	My_Connections[Idx].lastdata = time( NULL );

	Handle_Buffer( Idx );
} /* Read_Request */


LOCAL BOOLEAN
Handle_Buffer( CONN_ID Idx )
{
	/* Daten im Lese-Puffer einer Verbindung verarbeiten.
	 * Wurde ein Request verarbeitet, so wird TRUE geliefert,
	 * ansonsten FALSE (auch bei Fehlern). */

#ifndef STRICT_RFC
	CHAR *ptr1, *ptr2;
#endif
	CHAR *ptr;
	INT len, delta;
	BOOLEAN action, result;
#ifdef USE_ZLIB
	BOOLEAN old_z;
#endif

	result = FALSE;
	do
	{
#ifdef USE_ZLIB
		/* ggf. noch unkomprimiete Daten weiter entpacken */
		if( My_Connections[Idx].options & CONN_ZIP )
		{
			if( ! Unzip_Buffer( Idx )) return FALSE;
		}
#endif

		if( My_Connections[Idx].rdatalen < 1 ) break;

		/* Eine komplette Anfrage muss mit CR+LF enden, vgl.
		 * RFC 2812. Haben wir eine? */
		My_Connections[Idx].rbuf[My_Connections[Idx].rdatalen] = '\0';
		ptr = strstr( My_Connections[Idx].rbuf, "\r\n" );

		if( ptr ) delta = 2;
#ifndef STRICT_RFC
		else
		{
			/* Nicht RFC-konforme Anfrage mit nur CR oder LF? Leider
			 * machen soetwas viele Clients, u.a. "mIRC" :-( */
			ptr1 = strchr( My_Connections[Idx].rbuf, '\r' );
			ptr2 = strchr( My_Connections[Idx].rbuf, '\n' );
			delta = 1;
			if( ptr1 && ptr2 ) ptr = ptr1 > ptr2 ? ptr2 : ptr1;
			else if( ptr1 ) ptr = ptr1;
			else if( ptr2 ) ptr = ptr2;
		}
#endif

		action = FALSE;
		if( ptr )
		{
			/* Ende der Anfrage wurde gefunden */
			*ptr = '\0';
			len = ( ptr - My_Connections[Idx].rbuf ) + delta;
			if( len > ( COMMAND_LEN - 1 ))
			{
				/* Eine Anfrage darf(!) nicht laenger als 512 Zeichen
				 * (incl. CR+LF!) werden; vgl. RFC 2812. Wenn soetwas
				 * empfangen wird, wird der Client disconnectiert. */
				Log( LOG_ERR, "Request too long (connection %d): %d bytes (max. %d expected)!", Idx, My_Connections[Idx].rdatalen, COMMAND_LEN - 1 );
				Conn_Close( Idx, NULL, "Request too long", TRUE );
				return FALSE;
			}

#ifdef USE_ZLIB
			/* merken, ob Stream bereits komprimiert wird */
			old_z = My_Connections[Idx].options & CONN_ZIP;
#endif

			if( len > delta )
			{
				/* Es wurde ein Request gelesen */
				My_Connections[Idx].msg_in++;
				if( ! Parse_Request( Idx, My_Connections[Idx].rbuf )) return FALSE;
				else action = TRUE;
			}

			/* Puffer anpassen */
			My_Connections[Idx].rdatalen -= len;
			memmove( My_Connections[Idx].rbuf, My_Connections[Idx].rbuf + len, My_Connections[Idx].rdatalen );

#ifdef USE_ZLIB
			if(( ! old_z ) && ( My_Connections[Idx].options & CONN_ZIP ) && ( My_Connections[Idx].rdatalen > 0 ))
			{
				/* Mit dem letzten Befehl wurde Socket-Kompression aktiviert.
				 * Evtl. schon vom Socket gelesene Daten in den Unzip-Puffer
				 * umkopieren, damit diese nun zunaechst entkomprimiert werden */
				{
					if( My_Connections[Idx].rdatalen > ZREADBUFFER_LEN )
					{
						/* Hupsa! Soviel Platz haben wir aber gar nicht! */
						Log( LOG_ALERT, "Can't move read buffer: No space left in unzip buffer (need %d bytes)!", My_Connections[Idx].rdatalen );
						return FALSE;
					}
					memcpy( My_Connections[Idx].zip.rbuf, My_Connections[Idx].rbuf, My_Connections[Idx].rdatalen );
					My_Connections[Idx].zip.rdatalen = My_Connections[Idx].rdatalen;
					My_Connections[Idx].rdatalen = 0;
					Log( LOG_DEBUG, "Moved already received data (%d bytes) to uncompression buffer.", My_Connections[Idx].zip.rdatalen );
				}
			}
#endif
		}

		if( action ) result = TRUE;
	} while( action );

	return result;
} /* Handle_Buffer */


LOCAL VOID
Check_Connections( VOID )
{
	/* Pruefen, ob Verbindungen noch "alive" sind. Ist dies
	 * nicht der Fall, zunaechst PING-PONG spielen und, wenn
	 * auch das nicht "hilft", Client disconnectieren. */

	CLIENT *c;
	CONN_ID i;

	for( i = 0; i < Pool_Size; i++ )
	{
		if( My_Connections[i].sock == NONE ) continue;

		c = Client_GetFromConn( i );
		if( c && (( Client_Type( c ) == CLIENT_USER ) || ( Client_Type( c ) == CLIENT_SERVER ) || ( Client_Type( c ) == CLIENT_SERVICE )))
		{
			/* verbundener User, Server oder Service */
			if( My_Connections[i].lastping > My_Connections[i].lastdata )
			{
				/* es wurde bereits ein PING gesendet */
				if( My_Connections[i].lastping < time( NULL ) - Conf_PongTimeout )
				{
					/* Timeout */
					Log( LOG_DEBUG, "Connection %d: Ping timeout: %d seconds.", i, Conf_PongTimeout );
					Conn_Close( i, NULL, "Ping timeout", TRUE );
				}
			}
			else if( My_Connections[i].lastdata < time( NULL ) - Conf_PingTimeout )
			{
				/* es muss ein PING gesendet werden */
				Log( LOG_DEBUG, "Connection %d: sending PING ...", i );
				My_Connections[i].lastping = time( NULL );
				Conn_WriteStr( i, "PING :%s", Client_ID( Client_ThisServer( )));
			}
		}
		else
		{
			/* noch nicht vollstaendig aufgebaute Verbindung */
			if( My_Connections[i].lastdata < time( NULL ) - Conf_PingTimeout )
			{
				/* Timeout */
				Log( LOG_DEBUG, "Connection %d timed out ...", i );
				Conn_Close( i, NULL, "Timeout", FALSE );
			}
		}
	}
} /* Check_Connections */


LOCAL VOID
Check_Servers( VOID )
{
	/* Check if we can establish further server links */

	RES_STAT *s;
	CONN_ID idx;
	INT i, n;

	/* Serach all connections, are there results from the resolver? */
	for( idx = 0; idx < Pool_Size; idx++ )
	{
		if( My_Connections[idx].sock != SERVER_WAIT ) continue;

		/* IP resolved? */
		if( My_Connections[idx].res_stat == NULL ) New_Server( Conf_GetServer( idx ), idx );
	}

	/* Check all configured servers */
	for( i = 0; i < MAX_SERVERS; i++ )
	{
		/* Valid outgoing server which isn't already connected or disabled? */
		if(( ! Conf_Server[i].host[0] ) || ( ! Conf_Server[i].port > 0 ) || ( Conf_Server[i].conn_id > NONE ) || ( Conf_Server[i].flags & CONF_SFLAG_DISABLED )) continue;

		/* Is there already a connection in this group? */
		if( Conf_Server[i].group > NONE )
		{
			for( n = 0; n < MAX_SERVERS; n++ )
			{
				if( n == i ) continue;
				if(( Conf_Server[n].conn_id > NONE ) && ( Conf_Server[n].group == Conf_Server[i].group )) break;
			}
			if( n < MAX_SERVERS ) continue;
		}

		/* Check last connect attempt? */
		if( Conf_Server[i].lasttry > time( NULL ) - Conf_ConnectRetry ) continue;

		/* Okay, try to connect now */
		Conf_Server[i].lasttry = time( NULL );

		/* Search free connection structure */
		for( idx = 0; idx < Pool_Size; idx++ ) if( My_Connections[idx].sock == NONE ) break;
		if( idx >= Pool_Size )
		{
			Log( LOG_ALERT, "Can't establist server connection: connection limit reached (%d)!", Pool_Size );
			return;
		}
		Log( LOG_DEBUG, "Preparing connection %d for \"%s\" ...", idx, Conf_Server[i].host );

		/* Verbindungs-Struktur initialisieren */
		Init_Conn_Struct( idx );
		My_Connections[idx].sock = SERVER_WAIT;
		Conf_Server[i].conn_id = idx;

		/* Resolve Hostname. If this fails, try to use it as an IP address */
		strlcpy( Conf_Server[i].ip, Conf_Server[i].host, sizeof( Conf_Server[i].ip ));
		strlcpy( My_Connections[idx].host, Conf_Server[i].host, sizeof( My_Connections[idx].host ));
		s = Resolve_Name( Conf_Server[i].host );
		if( s )
		{
			/* Sub-Prozess wurde asyncron gestartet */
			My_Connections[idx].res_stat = s;
		}
	}
} /* Check_Servers */


LOCAL VOID
New_Server( INT Server, CONN_ID Idx )
{
	/* Establish new server link */

	struct sockaddr_in new_addr;
	struct in_addr inaddr;
	INT res, new_sock;
	CLIENT *c;

	assert( Server > NONE );
	assert( Idx > NONE );

	/* Did we get a valid IP address? */
	if( ! Conf_Server[Server].ip[0] )
	{
		/* No. Free connection structure and abort: */
		Init_Conn_Struct( Idx );
		Conf_Server[Server].conn_id = NONE;
		Log( LOG_ERR, "Can't connect to \"%s\" (connection %d): ip address unknown!", Conf_Server[Server].host, Idx );
		return;
	}

	Log( LOG_INFO, "Establishing connection to \"%s\", %s, port %d (connection %d) ... ", Conf_Server[Server].host, Conf_Server[Server].ip, Conf_Server[Server].port, Idx );

#ifdef HAVE_INET_ATON
	if( inet_aton( Conf_Server[Server].ip, &inaddr ) == 0 )
#else
	memset( &inaddr, 0, sizeof( inaddr ));
	inaddr.s_addr = inet_addr( Conf_Server[Server].ip );
	if( inaddr.s_addr == (unsigned)-1 )
#endif
	{
		/* Can't convert IP address */
		Init_Conn_Struct( Idx );
		Conf_Server[Server].conn_id = NONE;
		Log( LOG_ERR, "Can't connect to \"%s\" (connection %d): can't convert ip address %s!", Conf_Server[Server].host, Idx, Conf_Server[Server].ip );
		return;
	}

	memset( &new_addr, 0, sizeof( new_addr ));
	new_addr.sin_family = AF_INET;
	new_addr.sin_addr = inaddr;
	new_addr.sin_port = htons( Conf_Server[Server].port );

	new_sock = socket( PF_INET, SOCK_STREAM, 0 );
	if ( new_sock < 0 )
	{
		/* Can't create socket */
		Init_Conn_Struct( Idx );
		Conf_Server[Server].conn_id = NONE;
		Log( LOG_CRIT, "Can't create socket: %s!", strerror( errno ));
		return;
	}

	if( ! Init_Socket( new_sock )) return;

	res = connect( new_sock, (struct sockaddr *)&new_addr, sizeof( new_addr ));
	if(( res != 0 ) && ( errno != EINPROGRESS ))
	{
		/* Can't connect socket */
		Log( LOG_CRIT, "Can't connect socket: %s!", strerror( errno ));
		close( new_sock );
		Init_Conn_Struct( Idx );
		Conf_Server[Server].conn_id = NONE;
		return;
	}

	/* Client-Struktur initialisieren */
	c = Client_NewLocal( Idx, inet_ntoa( new_addr.sin_addr ), CLIENT_UNKNOWNSERVER, FALSE );
	if( ! c )
	{
		/* Can't create new client structure */
		close( new_sock );
		Init_Conn_Struct( Idx );
		Conf_Server[Server].conn_id = NONE;
		Log( LOG_ALERT, "Can't establish connection: can't create client structure!" );
		return;
	}
	Client_SetIntroducer( c, c );
	Client_SetToken( c, TOKEN_OUTBOUND );

	/* Register connection */
	My_Connections[Idx].sock = new_sock;
	My_Connections[Idx].addr = new_addr;
	strlcpy( My_Connections[Idx].host, Conf_Server[Server].host, sizeof( My_Connections[Idx].host ));

	/* Register new socket */
	FD_SET( new_sock, &My_Sockets );
	FD_SET( new_sock, &My_Connects );
	if( new_sock > Conn_MaxFD ) Conn_MaxFD = new_sock;

	Log( LOG_DEBUG, "Registered new connection %d on socket %d.", Idx, My_Connections[Idx].sock );
} /* New_Server */


LOCAL VOID
Init_Conn_Struct( CONN_ID Idx )
{
	/* Connection-Struktur initialisieren */

	My_Connections[Idx].sock = NONE;
	My_Connections[Idx].res_stat = NULL;
	My_Connections[Idx].host[0] = '\0';
	My_Connections[Idx].rbuf[0] = '\0';
	My_Connections[Idx].rdatalen = 0;
	My_Connections[Idx].wbuf[0] = '\0';
	My_Connections[Idx].wdatalen = 0;
	My_Connections[Idx].starttime = time( NULL );
	My_Connections[Idx].lastdata = time( NULL );
	My_Connections[Idx].lastping = 0;
	My_Connections[Idx].lastprivmsg = time( NULL );
	My_Connections[Idx].delaytime = 0;
	My_Connections[Idx].bytes_in = 0;
	My_Connections[Idx].bytes_out = 0;
	My_Connections[Idx].msg_in = 0;
	My_Connections[Idx].msg_out = 0;
	My_Connections[Idx].flag = 0;
	My_Connections[Idx].options = 0;

#ifdef USE_ZLIB
	My_Connections[Idx].zip.rbuf[0] = '\0';
	My_Connections[Idx].zip.rdatalen = 0;
	My_Connections[Idx].zip.wbuf[0] = '\0';
	My_Connections[Idx].zip.wdatalen = 0;
	My_Connections[Idx].zip.bytes_in = 0;
	My_Connections[Idx].zip.bytes_out = 0;
#endif
} /* Init_Conn_Struct */


LOCAL BOOLEAN
Init_Socket( INT Sock )
{
	/* Socket-Optionen setzen */

	INT on = 1;

#ifdef O_NONBLOCK	/* A/UX kennt das nicht? */
	if( fcntl( Sock, F_SETFL, O_NONBLOCK ) != 0 )
	{
		Log( LOG_CRIT, "Can't enable non-blocking mode: %s!", strerror( errno ));
		close( Sock );
		return FALSE;
	}
#endif
	if( setsockopt( Sock, SOL_SOCKET, SO_REUSEADDR, &on, (socklen_t)sizeof( on )) != 0)
	{
		Log( LOG_ERR, "Can't set socket options: %s!", strerror( errno ));
		/* dieser Fehler kann ignoriert werden. */
	}

	return TRUE;
} /* Init_Socket */


LOCAL VOID
Read_Resolver_Result( INT r_fd )
{
	/* Ergebnis von Resolver Sub-Prozess aus Pipe lesen
	 * und entsprechende Connection aktualisieren */

	CHAR result[HOST_LEN];
	CLIENT *c;
	INT len, i, n;

	FD_CLR( r_fd, &Resolver_FDs );

	/* Anfrage vom Parent lesen */
	len = read( r_fd, result, HOST_LEN - 1 );
	if( len < 0 )
	{
		/* Fehler beim Lesen aus der Pipe */
		close( r_fd );
		Log( LOG_CRIT, "Resolver: Can't read result: %s!", strerror( errno ));
		return;
	}
	result[len] = '\0';

	/* zugehoerige Connection suchen */
	for( i = 0; i < Pool_Size; i++ )
	{
		if(( My_Connections[i].sock != NONE ) && ( My_Connections[i].res_stat ) && ( My_Connections[i].res_stat->pipe[0] == r_fd )) break;
	}
	if( i >= Pool_Size )
	{
		/* Opsa! Keine passende Connection gefunden!? Vermutlich
		 * wurde sie schon wieder geschlossen. */
		close( r_fd );
		Log( LOG_DEBUG, "Resolver: Got result for unknown connection!?" );
		return;
	}

	Log( LOG_DEBUG, "Resolver: %s is \"%s\".", My_Connections[i].host, result );

	/* Aufraeumen */
	close( My_Connections[i].res_stat->pipe[0] );
	close( My_Connections[i].res_stat->pipe[1] );
	free( My_Connections[i].res_stat );
	My_Connections[i].res_stat = NULL;

	if( My_Connections[i].sock > NONE )
	{
		/* Eingehende Verbindung: Hostnamen setzen */
		c = Client_GetFromConn( i );
		assert( c != NULL );
		strlcpy( My_Connections[i].host, result, sizeof( My_Connections[i].host ));
		Client_SetHostname( c, result );
	}
	else
	{
		/* Ausgehende Verbindung (=Server): IP setzen */
		n = Conf_GetServer( i );
		if( n > NONE ) strlcpy( Conf_Server[n].ip, result, sizeof( Conf_Server[n].ip ));
		else Log( LOG_ERR, "Got resolver result for non-configured server!?" );
	}

	/* Penalty-Zeit zurueck setzen */
	Conn_ResetPenalty( i );
} /* Read_Resolver_Result */


LOCAL VOID
Simple_Message( INT Sock, CHAR *Msg )
{
	/* Write "simple" message to socket, without using compression
	 * or even the connection write buffers. Used e.g. for error
	 * messages by New_Connection(). */

	assert( Sock > NONE );
	assert( Msg != NULL );

	(VOID)send( Sock, Msg, strlen( Msg ), 0 );
	(VOID)send( Sock, "\r\n", 2, 0 );
} /* Simple_Error */


/* -eof- */
