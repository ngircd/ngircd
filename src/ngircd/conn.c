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
 * $Id: conn.c,v 1.32 2002/01/05 23:25:25 alex Exp $
 *
 * connect.h: Verwaltung aller Netz-Verbindungen ("connections")
 *
 * $Log: conn.c,v $
 * Revision 1.32  2002/01/05 23:25:25  alex
 * - Vorbereitungen fuer Ident-Abfragen bei neuen Client-Strukturen.
 *
 * Revision 1.31  2002/01/05 19:15:03  alex
 * - Fehlerpruefung bei select() in der "Hauptschleife" korrigiert.
 *
 * Revision 1.30  2002/01/05 15:56:23  alex
 * - "arpa/inet.h" wird nur noch includiert, wenn vorhanden.
 * - Ein Fehler bei select() fuerht nun zum Abbruch von ngIRCd.
 * - NO_ADDRESS durch NO_DATA ersetzt: ist wohl portabler.
 *
 * Revision 1.29  2002/01/04 01:36:40  alex
 * - Loglevel ein wenig angepasst.
 *
 * Revision 1.28  2002/01/04 01:20:23  alex
 * - Client-Strukruren werden nur noch ueber Funktionen angesprochen.
 *
 * Revision 1.27  2002/01/03 02:25:36  alex
 * - diverse Aenderungen und Umsetellungen fuer Server-Links.
 *
 * Revision 1.26  2002/01/02 02:50:47  alex
 * - Asyncroner Resolver Hostname->IP.
 * - Server-Links begonnen zu implementieren. Die Verbindung wird aufgebaut,
 *   jedoch noch keine SERVER-Befehle verschickt.
 * - Diverse Bug-Fixes und kleinere Erweiterungen.
 *
 * Revision 1.24  2002/01/01 18:25:44  alex
 * - #include's fuer stdlib.h ergaenzt.
 *
 * Revision 1.23  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.22  2001/12/30 19:26:11  alex
 * - Unterstuetzung fuer die Konfigurationsdatei eingebaut.
 *
 * Revision 1.21  2001/12/29 22:33:36  alex
 * - bessere Dokumentation des Modules bzw. der Funktionen.
 *
 * Revision 1.20  2001/12/29 22:09:43  alex
 * - kleinere Aenderungen ("clean-ups") bei Logging (Resolver).
 *
 * Revision 1.19  2001/12/29 21:53:57  alex
 * - Da hatte ich mich wohl ein wenig verrannt; jetzt sollte der Resolver
 *   aber tatsaechlich funktionieren.
 *
 * Revision 1.18  2001/12/29 20:17:25  alex
 * - asyncronen Resolver (IP->Name) implementiert, dadurch div. Aenderungen.
 *
 * Revision 1.17  2001/12/29 03:06:16  alex
 * - Loglevel (nochmal) angepasst.
 *
 * Revision 1.16  2001/12/27 19:32:44  alex
 * - bei "Null-Requests" wird nichts mehr geloggt. Uberfluessig, da normal.
 *
 * Revision 1.15  2001/12/27 16:35:04  alex
 * - vergessene Variable bei Ping-Timeout-Logmeldung ergaenzt. Opsa.
 *
 * Revision 1.14  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.13  2001/12/26 03:36:57  alex
 * - Verbindungen mit Lesefehlern werden nun korrekt terminiert.
 *
 * Revision 1.12  2001/12/26 03:20:53  alex
 * - PING/PONG-Timeout implementiert.
 *
 * Revision 1.11  2001/12/25 23:15:16  alex
 * - buffer werden nun periodisch geprueft, keine haengenden Clients mehr.
 *
 * Revision 1.10  2001/12/25 22:03:47  alex
 * - Conn_Close() eingefuehrt: war die lokale Funktion Close_Connection().
 *
 * Revision 1.9  2001/12/24 01:32:33  alex
 * - in Conn_WriteStr() wurde das CR+LF nicht angehaengt!
 * - Fehler-Ausgaben vereinheitlicht.
 *
 * Revision 1.8  2001/12/23 22:02:54  alex
 * - Conn_WriteStr() nimmt nun variable Parameter,
 * - diverse kleinere Aenderungen.
 *
 * Revision 1.7  2001/12/21 22:24:25  alex
 * - kleinere Aenderungen an den Log-Meldungen,
 * - Parse_Request() wird aufgerufen.
 *
 * Revision 1.6  2001/12/15 00:11:55  alex
 * - Lese- und Schreib-Puffer implementiert.
 * - einige neue (Unter-)Funktionen eingefuehrt.
 * - diverse weitere kleinere Aenderungen.
 *
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
#include <netdb.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#else
#define PF_INET AF_INET
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>			/* u.a. fuer Mac OS X */
#endif

#include "ngircd.h"
#include "client.h"
#include "conf.h"
#include "log.h"
#include "parse.h"
#include "tool.h"

#include <exp.h>
#include "conn.h"


#define SERVER_WAIT NONE - 1


typedef struct _Connection
{
	INT sock;			/* Socket Handle */
	struct sockaddr_in addr;	/* Adresse des Client */
	RES_STAT *res_stat;		/* "Resolver-Status", s.o. */
	CHAR host[HOST_LEN];		/* Hostname */
	CHAR rbuf[READBUFFER_LEN];	/* Lesepuffer */
	INT rdatalen;			/* Laenge der Daten im Lesepuffer */
	CHAR wbuf[WRITEBUFFER_LEN];	/* Schreibpuffer */
	INT wdatalen;			/* Laenge der Daten im Schreibpuffer */
	INT our_server;			/* wenn von uns zu connectender Server: ID */
	time_t lastdata;		/* Letzte Aktivitaet */
	time_t lastping;		/* Letzter PING */
	time_t lastprivmsg;		/* Letzte PRIVMSG */
} CONNECTION;


LOCAL VOID Handle_Read( INT sock );
LOCAL BOOLEAN Handle_Write( CONN_ID Idx );
LOCAL VOID New_Connection( INT Sock );
LOCAL CONN_ID Socket2Index( INT Sock );
LOCAL VOID Read_Request( CONN_ID Idx );
LOCAL BOOLEAN Try_Write( CONN_ID Idx );
LOCAL VOID Handle_Buffer( CONN_ID Idx );
LOCAL VOID Check_Connections( VOID );
LOCAL VOID Check_Servers( VOID );
LOCAL VOID Init_Conn_Struct( INT Idx );
LOCAL VOID New_Server( INT Server, CONN_ID Idx );

LOCAL RES_STAT *ResolveAddr( struct sockaddr_in *Addr );
LOCAL RES_STAT *ResolveName( CHAR *Host );
LOCAL VOID Do_ResolveAddr( struct sockaddr_in *Addr, INT w_fd );
LOCAL VOID Do_ResolveName( CHAR *Host, INT w_fd );
LOCAL VOID Read_Resolver_Result( INT r_fd );
LOCAL CHAR *Resolv_Error( INT H_Error );


LOCAL fd_set My_Listeners;
LOCAL fd_set My_Sockets;
LOCAL fd_set My_Resolvers;

LOCAL INT My_Max_Fd;

LOCAL CONNECTION My_Connections[MAX_CONNECTIONS];


GLOBAL VOID Conn_Init( VOID )
{
	/* Modul initialisieren: statische Strukturen "ausnullen". */

	CONN_ID i;

	/* zu Beginn haben wir keine Verbindungen */
	FD_ZERO( &My_Listeners );
	FD_ZERO( &My_Sockets );
	FD_ZERO( &My_Resolvers );

	My_Max_Fd = 0;

	/* Connection-Struktur initialisieren */
	for( i = 0; i < MAX_CONNECTIONS; i++ ) Init_Conn_Struct( i );
} /* Conn_Init */


GLOBAL VOID Conn_Exit( VOID )
{
	/* Modul abmelden: alle noch offenen Connections
	 * schliessen und freigeben. */

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
			if( idx < MAX_CONNECTIONS ) Conn_Close( idx, "Server going down ..." );
			else if( FD_ISSET( i, &My_Listeners ))
			{
				close( i );
				Log( LOG_INFO, "Listening socket %d closed.", i );
			}
			else
			{
				close( i );
				Log( LOG_WARNING, "Unknown connection %d closed.", i );
			}
		}
	}
} /* Conn_Exit */


GLOBAL BOOLEAN Conn_NewListener( CONST INT Port )
{
	/* Neuen Listen-Socket erzeugen: der Server wartet dann auf
	 * dem angegebenen Port auf Verbindungen. Kann der Listen-
	 * Socket nicht erteugt werden, so wird NULL geliefert.*/

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
		Log( LOG_ALERT, "Can't create socket: %s!", strerror( errno ));
		return FALSE;
	}

	/* Socket-Optionen setzen */
	if( fcntl( sock, F_SETFL, O_NONBLOCK ) != 0 )
	{
		Log( LOG_ALERT, "Can't enable non-blocking mode: %s!", strerror( errno ));
		close( sock );
		return FALSE;
	}
	if( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &on, (socklen_t)sizeof( on )) != 0)
	{
		Log( LOG_CRIT, "Can't set socket options: %s!", strerror( errno ));
		/* dieser Fehler kann ignoriert werden. */
	}

	/* an Port binden */
	if( bind( sock, (struct sockaddr *)&addr, (socklen_t)sizeof( addr )) != 0 )
	{
		Log( LOG_ALERT, "Can't bind socket: %s!", strerror( errno ));
		close( sock );
		return FALSE;
	}

	/* in "listen mode" gehen :-) */
	if( listen( sock, 10 ) != 0 )
	{
		Log( LOG_ALERT, "Can't listen on soecket: %s!", strerror( errno ));
		close( sock );
		return FALSE;
	}

	/* Neuen Listener in Strukturen einfuegen */
	FD_SET( sock, &My_Listeners );
	FD_SET( sock, &My_Sockets );

	if( sock > My_Max_Fd ) My_Max_Fd = sock;

	Log( LOG_INFO, "Now listening on port %d (socket %d).", Port, sock );

	return TRUE;
} /* Conn_NewListener */


GLOBAL VOID Conn_Handler( INT Timeout )
{
	/* Aktive Verbindungen ueberwachen. Mindestens alle "Timeout"
	 * Sekunden wird die Funktion verlassen. Folgende Aktionen
	 * werden durchgefuehrt:
	 *  - neue Verbindungen annehmen,
	 *  - Server-Verbindungen aufbauen,
	 *  - geschlossene Verbindungen loeschen,
	 *  - volle Schreibpuffer versuchen zu schreiben,
	 *  - volle Lesepuffer versuchen zu verarbeiten,
	 *  - Antworten von Resolver Sub-Prozessen annehmen.
	 */

	fd_set read_sockets, write_sockets;
	struct timeval tv;
	time_t start;
	INT i;

	start = time( NULL );
	while(( time( NULL ) - start < Timeout ) && ( ! NGIRCd_Quit ))
	{
		Check_Servers( );

		Check_Connections( );

		/* Timeout initialisieren */
		tv.tv_sec = 0;
		tv.tv_usec = 50000;

		/* noch volle Lese-Buffer suchen */
		for( i = 0; i < MAX_CONNECTIONS; i++ )
		{
			if(( My_Connections[i].sock > NONE ) && ( My_Connections[i].rdatalen > 0 ))
			{
				/* Kann aus dem Buffer noch ein Befehl extrahiert werden? */
				Handle_Buffer( i );
			}
		}
		
		/* noch volle Schreib-Puffer suchen */
		FD_ZERO( &write_sockets );
		for( i = 0; i < MAX_CONNECTIONS; i++ )
		{
			if(( My_Connections[i].sock > NONE ) && ( My_Connections[i].wdatalen > 0 ))
			{
				/* Socket der Verbindung in Set aufnehmen */
				FD_SET( My_Connections[i].sock, &write_sockets );
			}
		}

		/* von welchen Sockets koennte gelesen werden? */
		read_sockets = My_Sockets;
		for( i = 0; i < MAX_CONNECTIONS; i++ )
		{
			if(( My_Connections[i].sock > NONE ) && ( My_Connections[i].host[0] == '\0' ))
			{
				/* Hier muss noch auf den Resolver Sub-Prozess gewartet werden */
				FD_CLR( My_Connections[i].sock, &read_sockets );
			}
		}
		for( i = 0; i < My_Max_Fd + 1; i++ )
		{
			/* Pipes von Resolver Sub-Prozessen aufnehmen */
			if( FD_ISSET( i, &My_Resolvers ))
			{
				FD_SET( i, &read_sockets );
			}
		}

		/* Auf Aktivitaet warten */
		if( select( My_Max_Fd + 1, &read_sockets, &write_sockets, NULL, &tv ) == -1 )
		{
			if( errno != EINTR )
			{
				Log( LOG_ALERT, "select(): %s!", strerror( errno ));
				Log( LOG_ALERT, PACKAGE" exiting due to fatal errors!" );
				exit( 1 );
			}
			continue;
		}

		/* Koennen Daten geschrieben werden? */
		for( i = 0; i < My_Max_Fd + 1; i++ )
		{
			if( FD_ISSET( i, &write_sockets )) Handle_Write( Socket2Index( i ));
		}

		/* Daten zum Lesen vorhanden? */
		for( i = 0; i < My_Max_Fd + 1; i++ )
		{
			if( FD_ISSET( i, &read_sockets )) Handle_Read( i );
		}
	}
} /* Conn_Handler */


GLOBAL BOOLEAN Conn_WriteStr( CONN_ID Idx, CHAR *Format, ... )
{
	/* String in Socket schreiben. CR+LF wird von dieser Funktion
	 * automatisch angehaengt. Im Fehlerfall wird dir Verbindung
	 * getrennt und FALSE geliefert. */

	CHAR buffer[COMMAND_LEN];
	BOOLEAN ok;
	va_list ap;

	va_start( ap, Format );
	if( vsnprintf( buffer, COMMAND_LEN - 2, Format, ap ) == COMMAND_LEN - 2 )
	{
		Log( LOG_ALERT, "String too long to send (connection %d)!", Idx );
		Conn_Close( Idx, "Server error: String too long to send!" );
		return FALSE;
	}

#ifdef SNIFFER
	Log( LOG_DEBUG, " -> connection %d: '%s'.", Idx, buffer );
#endif

	strcat( buffer, "\r\n" );
	ok = Conn_Write( Idx, buffer, strlen( buffer ));

	va_end( ap );
	return ok;
} /* Conn_WriteStr */


GLOBAL BOOLEAN Conn_Write( CONN_ID Idx, CHAR *Data, INT Len )
{
	/* Daten in Socket schreiben. Bei "fatalen" Fehlern wird
	 * der Client disconnectiert und FALSE geliefert. */

	assert( Idx >= 0 );
	assert( My_Connections[Idx].sock > NONE );
	assert( Data != NULL );
	assert( Len > 0 );

	/* pruefen, ob Daten im Schreibpuffer sind. Wenn ja, zunaechst
	 * pruefen, ob diese gesendet werden koennen */
	if( My_Connections[Idx].wdatalen > 0 )
	{
		if( ! Try_Write( Idx )) return FALSE;
	}

	/* pruefen, ob im Schreibpuffer genuegend Platz ist */
	if( WRITEBUFFER_LEN - My_Connections[Idx].wdatalen - Len <= 0 )
	{
		/* der Puffer ist dummerweise voll ... */
		Log( LOG_NOTICE, "Write buffer overflow (connection %d)!", Idx );
		Conn_Close( Idx, NULL );
		return FALSE;
	}

	/* Daten in Puffer kopieren */
	memcpy( My_Connections[Idx].wbuf + My_Connections[Idx].wdatalen, Data, Len );
	My_Connections[Idx].wdatalen += Len;

	/* pruefen, on Daten vorhanden sind und geschrieben werden koennen */
	if( My_Connections[Idx].wdatalen > 0 )
	{
		if( ! Try_Write( Idx )) return FALSE;
	}

	return TRUE;
} /* Conn_Write */


GLOBAL VOID Conn_Close( CONN_ID Idx, CHAR *Msg )
{
	/* Verbindung schliessen. Evtl. noch von Resolver
	 * Sub-Prozessen offene Pipes werden geschlossen. */

	CLIENT *c;
	
	assert( Idx >= 0 );
	assert( My_Connections[Idx].sock > NONE );

	if( Msg )
	{
		Conn_WriteStr( Idx, "ERROR :%s", Msg );
		if( My_Connections[Idx].sock == NONE ) return;
	}

	if( close( My_Connections[Idx].sock ) != 0 )
	{
		Log( LOG_ERR, "Error closing connection %d with %s:%d - %s!", Idx, inet_ntoa( My_Connections[Idx].addr.sin_addr ), ntohs( My_Connections[Idx].addr.sin_port), strerror( errno ));
	}
	else
	{
		Log( LOG_INFO, "Connection %d with %s:%d closed.", Idx, inet_ntoa( My_Connections[Idx].addr.sin_addr ), ntohs( My_Connections[Idx].addr.sin_port ));
	}

	c = Client_GetFromConn( Idx );
	if( c ) Client_Destroy( c );

	if( My_Connections[Idx].res_stat )
	{
		/* Resolver-Strukturen freigeben, wenn noch nicht geschehen */
		FD_CLR( My_Connections[Idx].res_stat->pipe[0], &My_Resolvers );
		close( My_Connections[Idx].res_stat->pipe[0] );
		close( My_Connections[Idx].res_stat->pipe[1] );
		free( My_Connections[Idx].res_stat );
	}

	/* Bei Server-Verbindungen lasttry-Zeitpunkt auf "jetzt" setzen */
	if( My_Connections[Idx].our_server >= 0 ) Conf_Server[My_Connections[Idx].our_server].lasttry = time( NULL );

	FD_CLR( My_Connections[Idx].sock, &My_Sockets );
	My_Connections[Idx].sock = NONE;
} /* Conn_Close */


GLOBAL VOID Conn_UpdateIdle( CONN_ID Idx )
{
	/* Idle-Timer zuruecksetzen */

	assert( Idx >= 0 );
	My_Connections[Idx].lastprivmsg = time( NULL );
}


GLOBAL INT32 Conn_GetIdle( CONN_ID Idx )
{
	/* Idle-Time einer Verbindung liefern (in Sekunden) */

	assert( Idx >= 0 );
	return time( NULL ) - My_Connections[Idx].lastprivmsg;
} /* Conn_GetIdle */


LOCAL BOOLEAN Try_Write( CONN_ID Idx )
{
	/* Versuchen, Daten aus dem Schreib-Puffer in den
	 * Socket zu schreiben. */

	fd_set write_socket;

	assert( Idx >= 0 );
	assert( My_Connections[Idx].sock > NONE );
	assert( My_Connections[Idx].wdatalen > 0 );

	FD_ZERO( &write_socket );
	FD_SET( My_Connections[Idx].sock, &write_socket );
	if( select( My_Connections[Idx].sock + 1, NULL, &write_socket, NULL, 0 ) == -1 )
	{
		/* Fehler! */
		if( errno != EINTR )
		{
			Log( LOG_ALERT, "select(): %s!", strerror( errno ));
			Conn_Close( Idx, NULL );
			return FALSE;
		}
	}

	if( FD_ISSET( My_Connections[Idx].sock, &write_socket )) return Handle_Write( Idx );
	else return TRUE;
} /* Try_Write */


LOCAL VOID Handle_Read( INT Sock )
{
	/* Aktivitaet auf einem Socket verarbeiten:
	 *  - neue Clients annehmen,
	 *  - Daten von Clients verarbeiten,
	 *  - Resolver-Rueckmeldungen annehmen. */

	CONN_ID idx;

	assert( Sock >= 0 );
	
	if( FD_ISSET( Sock, &My_Listeners ))
	{
		/* es ist einer unserer Listener-Sockets: es soll
		 * also eine neue Verbindung aufgebaut werden. */

		New_Connection( Sock );
	}
	else if( FD_ISSET( Sock, &My_Resolvers ))
	{
		/* Rueckmeldung von einem Resolver Sub-Prozess */

		Read_Resolver_Result( Sock );
	}
	else
	{
		/* Ein Client Socket: entweder ein User oder Server */

		idx = Socket2Index( Sock );
		Read_Request( idx );
	}
} /* Handle_Read */


LOCAL BOOLEAN Handle_Write( CONN_ID Idx )
{
	/* Daten aus Schreibpuffer versenden */

	INT len;

	assert( Idx >= 0 );
	assert( My_Connections[Idx].sock > NONE );
	assert( My_Connections[Idx].wdatalen > 0 );

	/* Daten schreiben */
	len = send( My_Connections[Idx].sock, My_Connections[Idx].wbuf, My_Connections[Idx].wdatalen, 0 );
	if( len < 0 )
	{
		/* Oops, ein Fehler! */
		Log( LOG_ALERT, "Write error (buffer) on connection %d: %s!", Idx, strerror( errno ));
		Conn_Close( Idx, NULL );
		return FALSE;
	}

	/* Puffer anpassen */
	My_Connections[Idx].wdatalen -= len;
	memmove( My_Connections[Idx].wbuf, My_Connections[Idx].wbuf + len, My_Connections[Idx].wdatalen );

	return TRUE;
} /* Handle_Write */


LOCAL VOID New_Connection( INT Sock )
{
	/* Neue Client-Verbindung von Listen-Socket annehmen und
	 * CLIENT-Struktur anlegen. */

	struct sockaddr_in new_addr;
	INT new_sock, new_sock_len;
	RES_STAT *s;
	CONN_ID idx;
	
	assert( Sock >= 0 );

	new_sock_len = sizeof( new_addr );
	new_sock = accept( Sock, (struct sockaddr *)&new_addr, (socklen_t *)&new_sock_len );
	if( new_sock < 0 )
	{
		Log( LOG_CRIT, "Can't accept connection: %s!", strerror( errno ));
		return;
	}

	/* Freie Connection-Struktur suschen */
	for( idx = 0; idx < MAX_CONNECTIONS; idx++ ) if( My_Connections[idx].sock == NONE ) break;
	if( idx >= MAX_CONNECTIONS )
	{
		Log( LOG_ALERT, "Can't accept connection: limit reached (%d)!", MAX_CONNECTIONS );
		close( new_sock );
		return;
	}

	/* Client-Struktur initialisieren */
	if( ! Client_NewLocal( idx, inet_ntoa( new_addr.sin_addr ), CLIENT_UNKNOWN, FALSE ))
	{
		Log( LOG_ALERT, "Can't accept connection: can't create client structure!" );
		close( new_sock );
		return;
	}

	/* Verbindung registrieren */
	Init_Conn_Struct( idx );
	My_Connections[idx].sock = new_sock;
	My_Connections[idx].addr = new_addr;

	/* Neuen Socket registrieren */
	FD_SET( new_sock, &My_Sockets );
	if( new_sock > My_Max_Fd ) My_Max_Fd = new_sock;

	Log( LOG_INFO, "Accepted connection %d from %s:%d on socket %d.", idx, inet_ntoa( new_addr.sin_addr ), ntohs( new_addr.sin_port), Sock );

	/* Hostnamen ermitteln */
	s = ResolveAddr( &new_addr );
	if( s )
	{
		/* Sub-Prozess wurde asyncron gestartet */
		My_Connections[idx].res_stat = s;
	}
	else
	{
		/* kann Namen nicht aufloesen */
		strcpy( My_Connections[idx].host, inet_ntoa( new_addr.sin_addr ));
	}
} /* New_Connection */


LOCAL CONN_ID Socket2Index( INT Sock )
{
	/* zum Socket passende Connection suchen */

	CONN_ID idx;

	assert( Sock >= 0 );

	for( idx = 0; idx < MAX_CONNECTIONS; idx++ ) if( My_Connections[idx].sock == Sock ) break;

	assert( idx < MAX_CONNECTIONS );
	return idx;
} /* Socket2Index */


LOCAL VOID Read_Request( CONN_ID Idx )
{
	/* Daten von Socket einlesen und entsprechend behandeln.
	 * Tritt ein Fehler auf, so wird der Socket geschlossen. */

	INT len;

	assert( Idx >= 0 );
	assert( My_Connections[Idx].sock > NONE );

	if( READBUFFER_LEN - My_Connections[Idx].rdatalen - 2 < 0 )
	{
		/* Der Lesepuffer ist voll */
		Log( LOG_ALERT, "Read buffer overflow (connection %d): %d bytes!", Idx, My_Connections[Idx].rdatalen );
		Conn_Close( Idx, "Read buffer overflow!" );
		return;
	}

	len = recv( My_Connections[Idx].sock, My_Connections[Idx].rbuf + My_Connections[Idx].rdatalen, READBUFFER_LEN - My_Connections[Idx].rdatalen - 2, 0 );

	if( len == 0 )
	{
		/* Socket wurde geschlossen */
		Log( LOG_INFO, "%s:%d is closing the connection ...", inet_ntoa( My_Connections[Idx].addr.sin_addr ), ntohs( My_Connections[Idx].addr.sin_port));
		Conn_Close( Idx, NULL );
		return;
	}

	if( len < 0 )
	{
		/* Fehler beim Lesen */
		Log( LOG_ALERT, "Read error on connection %d: %s!", Idx, strerror( errno ));
		Conn_Close( Idx, NULL );
		return;
	}

	/* Lesebuffer updaten */
	My_Connections[Idx].rdatalen += len;
	assert( My_Connections[Idx].rdatalen < READBUFFER_LEN );
	My_Connections[Idx].rbuf[My_Connections[Idx].rdatalen] = '\0';

	/* Timestamp aktualisieren */
	My_Connections[Idx].lastdata = time( NULL );

	Handle_Buffer( Idx );
} /* Read_Request */


LOCAL VOID Handle_Buffer( CONN_ID Idx )
{
	/* Daten im Lese-Puffer einer Verbindung verarbeiten. */

	CHAR *ptr, *ptr1, *ptr2;
	INT len, delta;
	
	/* Eine komplette Anfrage muss mit CR+LF enden, vgl.
	 * RFC 2812. Haben wir eine? */
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
	
	if( ptr )
	{
		/* Ende der Anfrage wurde gefunden */
		*ptr = '\0';
		len = ( ptr - My_Connections[Idx].rbuf ) + delta;
		if( len > COMMAND_LEN )
		{
			/* Eine Anfrage darf(!) nicht laenger als 512 Zeichen
			* (incl. CR+LF!) werden; vgl. RFC 2812. Wenn soetwas
			* empfangen wird, wird der Client disconnectiert. */
			Log( LOG_ALERT, "Request too long (connection %d): %d bytes!", Idx, My_Connections[Idx].rdatalen );
			Conn_Close( Idx, "Request too long!" );
			return;
		}
		
		if( len > delta )
		{
			/* Es wurde ein Request gelesen */
			if( ! Parse_Request( Idx, My_Connections[Idx].rbuf )) return;
		}

		/* Puffer anpassen */
		My_Connections[Idx].rdatalen -= len;
		memmove( My_Connections[Idx].rbuf, My_Connections[Idx].rbuf + len, My_Connections[Idx].rdatalen );
	}
} /* Handle_Buffer */


LOCAL VOID Check_Connections( VOID )
{
	/* Pruefen, ob Verbindungen noch "alive" sind. Ist dies
	 * nicht der Fall, zunaechst PING-PONG spielen und, wenn
	 * auch das nicht "hilft", Client disconnectieren. */

	CLIENT *c;
	INT i;

	for( i = 0; i < MAX_CONNECTIONS; i++ )
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
					Log( LOG_INFO, "Connection %d: PING timeout.", i );
					Conn_Close( i, "PING timeout" );
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
				Log( LOG_INFO, "Connection %d: Timeout.", i );
				Conn_Close( i, "Timeout" );
			}
		}
	}
} /* Check_Connections */


LOCAL VOID Check_Servers( VOID )
{
	/* Pruefen, ob Server-Verbindungen aufgebaut werden
	 * muessen bzw. koennen */

	INT idx, i, n;
	RES_STAT *s;
	
	for( i = 0; i < Conf_Server_Count; i++ )
	{
		/* Ist ein Hostname und Port definiert? */
		if(( ! Conf_Server[i].host[0] ) || ( ! Conf_Server[i].port > 0 )) continue;
		
		/* Haben wir schon eine Verbindung? */
		for( n = 0; n < MAX_CONNECTIONS; n++ )
		{
			if(( My_Connections[n].sock != NONE ) && ( My_Connections[n].our_server == i ))
			{
				/* Komplett aufgebaute Verbindung? */
				if( My_Connections[n].sock > NONE ) break;

				/* IP schon aufgeloest? */
				if( My_Connections[n].res_stat == NULL ) New_Server( i, n );
			}
		}
		if( n < MAX_CONNECTIONS ) continue;
		
		/* Wann war der letzte Connect-Versuch? */
		if( Conf_Server[i].lasttry > time( NULL ) - Conf_ConnectRetry ) continue;

		/* Okay, Verbindungsaufbau versuchen */
		Conf_Server[i].lasttry = time( NULL );

		/* Freie Connection-Struktur suschen */
		for( idx = 0; idx < MAX_CONNECTIONS; idx++ ) if( My_Connections[idx].sock == NONE ) break;
		if( idx >= MAX_CONNECTIONS )
		{
			Log( LOG_ALERT, "Can't establist server connection: connection limit reached (%d)!", MAX_CONNECTIONS );
			return;
		}
		Log( LOG_DEBUG, "Preparing connection %d for \"%s\" ...", idx, Conf_Server[i].host );

		/* Verbindungs-Struktur initialisieren */
		Init_Conn_Struct( idx );
		My_Connections[idx].sock = SERVER_WAIT;
		My_Connections[idx].our_server = i;
		
		/* Hostnamen in IP aufloesen */
		s = ResolveName( Conf_Server[i].host );
		if( s )
		{
			/* Sub-Prozess wurde asyncron gestartet */
			My_Connections[idx].res_stat = s;
		}
		else
		{
			/* kann Namen nicht aufloesen: Connection-Struktur freigeben */
			Init_Conn_Struct( idx );
		}
	}
} /* Check_Servers */


LOCAL VOID New_Server( INT Server, CONN_ID Idx )
{
	/* Neue Server-Verbindung aufbauen */

	struct sockaddr_in new_addr;
	struct in_addr inaddr;
	INT new_sock;
	CLIENT *c;

	assert( Server >= 0 );
	assert( Idx >= 0 );

	/* Wurde eine gueltige IP-Adresse gefunden? */
	if( ! Conf_Server[Server].ip[0] )
	{
		/* Nein. Verbindung wieder freigeben: */
		Init_Conn_Struct( Idx );
		Log( LOG_ERR, "Can't connect to \"%s\" (connection %d): ip address unknown!", Conf_Server[Server].host, Idx );
		return;
	}
	
	Log( LOG_INFO, "Establishing connection to \"%s\", %s (connection %d) ... ", Conf_Server[Server].host, Conf_Server[Server].ip, Idx );

	if( inet_aton( Conf_Server[Server].ip, &inaddr ) == 0 )
	{
		/* Konnte Adresse nicht konvertieren */
		Init_Conn_Struct( Idx );
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
		Init_Conn_Struct( Idx );
		Log( LOG_ALERT, "Can't create socket: %s!", strerror( errno ));
		return;
	}
	if( connect( new_sock, (struct sockaddr *)&new_addr, sizeof( new_addr )) < 0)
	{
		close( new_sock );
		Init_Conn_Struct( Idx );
		Log( LOG_ALERT, "Can't connect socket: %s!", strerror( errno ));
		return;
	}

	/* Client-Struktur initialisieren */
	c = Client_NewLocal( Idx, inet_ntoa( new_addr.sin_addr ), CLIENT_UNKNOWNSERVER, FALSE );
	if( ! c )
	{
		close( new_sock );
		Init_Conn_Struct( Idx );
		Log( LOG_ALERT, "Can't establish connection: can't create client structure!" );
		return;
	}
	Client_SetIntroducer( c, c );
	
	/* Verbindung registrieren */
	My_Connections[Idx].sock = new_sock;
	My_Connections[Idx].addr = new_addr;
	strcpy( My_Connections[Idx].host, Conf_Server[Server].host );

	/* Neuen Socket registrieren */
	FD_SET( new_sock, &My_Sockets );
	if( new_sock > My_Max_Fd ) My_Max_Fd = new_sock;

	/* PASS und SERVER verschicken */
	Conn_WriteStr( Idx, "PASS %s "PROTOVER""PROTOSUFFIX" IRC|"PACKAGE"-"VERSION" P", Conf_Server[Server].pwd );
	Conn_WriteStr( Idx, "SERVER %s :%s", Conf_ServerName, Conf_ServerInfo );
} /* New_Server */


LOCAL VOID Init_Conn_Struct( INT Idx )
{
	/* Connection-Struktur initialisieren */

	My_Connections[Idx].sock = NONE;
	My_Connections[Idx].res_stat = NULL;
	My_Connections[Idx].host[0] = '\0';
	My_Connections[Idx].rbuf[0] = '\0';
	My_Connections[Idx].rdatalen = 0;
	My_Connections[Idx].wbuf[0] = '\0';
	My_Connections[Idx].wdatalen = 0;
	My_Connections[Idx].our_server = -1;
	My_Connections[Idx].lastdata = time( NULL );
	My_Connections[Idx].lastping = 0;
	My_Connections[Idx].lastprivmsg = time( NULL );
} /* Init_Conn_Struct */


LOCAL RES_STAT *ResolveAddr( struct sockaddr_in *Addr )
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
		Log( LOG_ALERT, "Resolver: Can't alloc memory!" );
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
		FD_SET( s->pipe[0], &My_Resolvers );
		if( s->pipe[0] > My_Max_Fd ) My_Max_Fd = s->pipe[0];
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
		Log( LOG_ALERT, "Resolver: Can't fork: %s!", strerror( errno ));
		return NULL;
	}
} /* ResolveAddr */


LOCAL RES_STAT *ResolveName( CHAR *Host )
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
		Log( LOG_ALERT, "Resolver: Can't alloc memory!" );
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
		FD_SET( s->pipe[0], &My_Resolvers );
		if( s->pipe[0] > My_Max_Fd ) My_Max_Fd = s->pipe[0];
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
		Log( LOG_ALERT, "Resolver: Can't fork: %s!", strerror( errno ));
		return NULL;
	}
} /* ResolveName */


LOCAL VOID Do_ResolveAddr( struct sockaddr_in *Addr, INT w_fd )
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
		Log_Resolver( LOG_WARNING, "Can't resolve address %s: code %s!", inet_ntoa( Addr->sin_addr ), Resolv_Error( h_errno ));
		strcpy( hostname, inet_ntoa( Addr->sin_addr ));
	}

	/* Antwort an Parent schreiben */
	if( write( w_fd, hostname, strlen( hostname ) + 1 ) != ( strlen( hostname ) + 1 ))
	{
		Log_Resolver( LOG_ALERT, "Resolver: Can't write to parent: %s!", strerror( errno ));
		close( w_fd );
		return;
	}

	Log_Resolver( LOG_DEBUG, "Ok, translated %s to \"%s\".", inet_ntoa( Addr->sin_addr ), hostname );
} /* Do_ResolveAddr */


LOCAL VOID Do_ResolveName( CHAR *Host, INT w_fd )
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
		Log_Resolver( LOG_WARNING, "Can't resolve \"%s\": %s!", Host, Resolv_Error( h_errno ));
		strcpy( ip, "" );
	}

	/* Antwort an Parent schreiben */
	if( write( w_fd, ip, strlen( ip ) + 1 ) != ( strlen( ip ) + 1 ))
	{
		Log_Resolver( LOG_ALERT, "Resolver: Can't write to parent: %s!", strerror( errno ));
		close( w_fd );
		return;
	}

	if( ip[0] ) Log_Resolver( LOG_DEBUG, "Ok, translated \"%s\" to %s.", Host, ip );
} /* Do_ResolveName */


LOCAL VOID Read_Resolver_Result( INT r_fd )
{
	/* Ergebnis von Resolver Sub-Prozess aus Pipe lesen
	* und entsprechende Connection aktualisieren */

	CHAR result[HOST_LEN];
	CLIENT *c;
	INT len, i;

	FD_CLR( r_fd, &My_Resolvers );

	/* Anfrage vom Parent lesen */
	len = read( r_fd, result, HOST_LEN);
	if( len < 0 )
	{
		/* Fehler beim Lesen aus der Pipe */
		close( r_fd );
		Log( LOG_ALERT, "Resolver: Can't read result: %s!", strerror( errno ));
		return;
	}
	result[len] = '\0';

	/* zugehoerige Connection suchen */
	for( i = 0; i < MAX_CONNECTIONS; i++ )
	{
		if(( My_Connections[i].sock != NONE ) && ( My_Connections[i].res_stat ) && ( My_Connections[i].res_stat->pipe[0] == r_fd )) break;
	}
	if( i >= MAX_CONNECTIONS )
	{
		/* Opsa! Keine passende Connection gefunden!? Vermutlich
		 * wurde sie schon wieder geschlossen. */
		close( r_fd );
		Log( LOG_DEBUG, "Resolver: Got result for unknown connection!?" );
		return;
	}

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
		strcpy( My_Connections[i].host, result );
		Client_SetHostname( c, result );
	}
	else
	{
		/* Ausgehende Verbindung (=Server): IP setzen */
		assert( My_Connections[i].our_server >= 0 );
		strcpy( Conf_Server[My_Connections[i].our_server].ip, result );
	}
} /* Read_Resolver_Result */


LOCAL CHAR *Resolv_Error( INT H_Error )
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
} /* Resolv_Error */


/* -eof- */
