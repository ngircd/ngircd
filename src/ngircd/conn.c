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
 * $Id: conn.c,v 1.98 2002/11/28 16:56:20 alex Exp $
 *
 * connect.h: Verwaltung aller Netz-Verbindungen ("connections")
 */


#include "portab.h"

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
#include <stdint.h>			/* u.a. fuer Mac OS X */
#endif

#ifdef USE_ZLIB
#include <zlib.h>
#endif

#include "exp.h"
#include "conn.h"

#include "imp.h"
#include "ngircd.h"
#include "client.h"
#include "resolve.h"
#include "conf.h"
#include "log.h"
#include "parse.h"
#include "tool.h"

#include "exp.h"


#define SERVER_WAIT (NONE - 1)


#ifdef USE_ZLIB
typedef struct _ZipData
{
	z_stream in;			/* "Handle" fuer Input-Stream */
	z_stream out;			/* "Handle" fuer Output-Stream */
	CHAR rbuf[READBUFFER_LEN];	/* Lesepuffer */
	INT rdatalen;			/* Laenge der Daten im Lesepuffer (komprimiert) */
	CHAR wbuf[WRITEBUFFER_LEN];	/* Schreibpuffer */
	INT wdatalen;			/* Laenge der Daten im Schreibpuffer (unkomprimiert) */
	LONG bytes_in, bytes_out;	/* Counter fuer Statistik (unkomprimiert!) */
} ZIPDATA;
#endif


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
	time_t delaytime;		/* Nicht beachten bis ("penalty") */
	LONG bytes_in, bytes_out;	/* Counter fuer Statistik */
	INT flag;			/* "Markierungs-Flag" (vgl. "irc-write"-Modul) */
	INT options;			/* Link-Optionen */
#ifdef USE_ZLIB
	ZIPDATA zip;			/* Kompressionsinformationen */
#endif
} CONNECTION;


LOCAL VOID Handle_Read PARAMS(( INT sock ));
LOCAL BOOLEAN Handle_Write PARAMS(( CONN_ID Idx ));
LOCAL VOID New_Connection PARAMS(( INT Sock ));
LOCAL CONN_ID Socket2Index PARAMS(( INT Sock ));
LOCAL VOID Read_Request PARAMS(( CONN_ID Idx ));
LOCAL BOOLEAN Try_Write PARAMS(( CONN_ID Idx ));
LOCAL BOOLEAN Handle_Buffer PARAMS(( CONN_ID Idx ));
LOCAL VOID Check_Connections PARAMS(( VOID ));
LOCAL VOID Check_Servers PARAMS(( VOID ));
LOCAL VOID Init_Conn_Struct PARAMS(( LONG Idx ));
LOCAL BOOLEAN Init_Socket PARAMS(( INT Sock ));
LOCAL VOID New_Server PARAMS(( INT Server, CONN_ID Idx ));
LOCAL VOID Read_Resolver_Result PARAMS(( INT r_fd ));

#ifdef USE_ZLIB
LOCAL BOOLEAN Zip_Buffer PARAMS(( CONN_ID Idx, CHAR *Data, INT Len ));
LOCAL BOOLEAN Zip_Flush PARAMS(( CONN_ID Idx ));
LOCAL BOOLEAN Unzip_Buffer PARAMS(( CONN_ID Idx ));
#endif


LOCAL fd_set My_Listeners;
LOCAL fd_set My_Sockets;
LOCAL fd_set My_Connects;

LOCAL CONNECTION *My_Connections;
LOCAL LONG Pool_Size;


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
	Log( LOG_DEBUG, "Allocted connection pool for %ld items.", Pool_Size );

	/* zu Beginn haben wir keine Verbindungen */
	FD_ZERO( &My_Listeners );
	FD_ZERO( &My_Sockets );
	FD_ZERO( &My_Connects );

	/* Groesster File-Descriptor fuer select() */
	Conn_MaxFD = 0;

	/* Connection-Struktur initialisieren */
	for( i = 0; i < Pool_Size; i++ ) Init_Conn_Struct( i );
} /* Conn_Init */


GLOBAL VOID
Conn_Exit( VOID )
{
	/* Modul abmelden: alle noch offenen Connections
	 * schliessen und freigeben. */

	CONN_ID idx;
	INT i;

	/* Sockets schliessen */
	Log( LOG_DEBUG, "Shutting down all connections ..." );
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
				if( NGIRCd_Restart ) Conn_Close( idx, NULL, "Server going down (restarting)", TRUE );
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
	/* Ports, auf denen der Server Verbindungen entgegennehmen
	* soll, initialisieren */

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
	/* Alle "Listen-Sockets" schliessen */

	INT i;

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
	/* Neuen Listen-Socket erzeugen: der Server wartet dann auf
	 * dem angegebenen Port auf Verbindungen. Kann der Listen-
	 * Socket nicht erteugt werden, so wird NULL geliefert.*/

	struct sockaddr_in addr;
	INT sock;

	/* Server-"Listen"-Socket initialisieren */
	memset( &addr, 0, sizeof( addr ));
	addr.sin_family = AF_INET;
	addr.sin_port = htons( Port );
	addr.sin_addr.s_addr = htonl( INADDR_ANY );

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

	Log( LOG_INFO, "Now listening on port %d (socket %d).", Port, sock );

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
	LONG i, idx;
	BOOLEAN timeout;

	start = time( NULL );
	while(( ! NGIRCd_Quit ) && ( ! NGIRCd_Restart ))
	{
		timeout = TRUE;
	
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
				FD_CLR( My_Connections[i].sock, &write_sockets );
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
				Log( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE );
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

	strcat( buffer, "\r\n" );
	ok = Conn_Write( Idx, buffer, strlen( buffer ));

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

	return TRUE;
} /* Conn_Write */


GLOBAL VOID
Conn_Close( CONN_ID Idx, CHAR *LogMsg, CHAR *FwdMsg, BOOLEAN InformClient )
{
	/* Verbindung schliessen. Evtl. noch von Resolver
	 * Sub-Prozessen offene Pipes werden geschlossen. */

	CLIENT *c;
	DOUBLE in_k, out_k;
#ifdef USE_ZLIB
	DOUBLE in_z_k, out_z_k;
	INT in_p, out_p;
#endif

	assert( Idx > NONE );
	assert( My_Connections[Idx].sock > NONE );

	c = Client_GetFromConn( Idx );

	if( InformClient )
	{
		/* Statistik an Client melden, wenn User */
		if(( c != NULL ) && ( Client_Type( c ) == CLIENT_USER ))
		{
			Conn_WriteStr( Idx, "NOTICE %s :%sConnection statistics: client %.1f kb, server %.1f kb.", Client_ThisServer( ), NOTICE_TXTPREFIX, (DOUBLE)My_Connections[Idx].bytes_in / 1024,  (DOUBLE)My_Connections[Idx].bytes_out / 1024 );
		}

		/* ERROR an Client schicken (von RFC so vorgesehen!) */
		if( FwdMsg ) Conn_WriteStr( Idx, "ERROR :%s", FwdMsg );
		else Conn_WriteStr( Idx, "ERROR :Closing connection." );
		if( My_Connections[Idx].sock == NONE ) return;
	}

	/* zunaechst versuchen, noch im Schreibpuffer vorhandene
	 * Daten auf den Socket zu schreiben ... */
	Try_Write( Idx );

	if( close( My_Connections[Idx].sock ) != 0 )
	{
		Log( LOG_ERR, "Error closing connection %d (socket %d) with %s:%d - %s!", Idx, My_Connections[Idx].sock, My_Connections[Idx].host, ntohs( My_Connections[Idx].addr.sin_port), strerror( errno ));
	}
	else
	{
		in_k = (DOUBLE)My_Connections[Idx].bytes_in / 1024;
		out_k = (DOUBLE)My_Connections[Idx].bytes_out / 1024;
#ifdef USE_ZLIB
		if( My_Connections[Idx].options & CONN_ZIP )
		{
			in_z_k = (DOUBLE)My_Connections[Idx].zip.bytes_in / 1024;
			out_z_k = (DOUBLE)My_Connections[Idx].zip.bytes_out / 1024;
			in_p = (INT)(( in_k * 100 ) / in_z_k );
			out_p = (INT)(( out_k * 100 ) / out_z_k );
			Log( LOG_INFO, "Connection %d (socket %d) with %s:%d closed (in: %.1fk/%.1fk/%d%%, out: %.1fk/%.1fk/%d%%).", Idx, My_Connections[Idx].sock, My_Connections[Idx].host, ntohs( My_Connections[Idx].addr.sin_port ), in_k, in_z_k, in_p, out_k, out_z_k, out_p );
		}
		else
#endif
		{
			Log( LOG_INFO, "Connection %d (socket %d) with %s:%d closed (in: %.1fk, out: %.1fk).", Idx, My_Connections[Idx].sock, My_Connections[Idx].host, ntohs( My_Connections[Idx].addr.sin_port ), in_k, out_k );
		}
	}
	
	/* Socket als "ungueltig" markieren */
	FD_CLR( My_Connections[Idx].sock, &My_Sockets );
	FD_CLR( My_Connections[Idx].sock, &My_Connects );
	My_Connections[Idx].sock = NONE;

	if( c ) Client_Destroy( c, LogMsg, FwdMsg, TRUE );

	if( My_Connections[Idx].res_stat )
	{
		/* Resolver-Strukturen freigeben, wenn noch nicht geschehen */
		FD_CLR( My_Connections[Idx].res_stat->pipe[0], &Resolver_FDs );
		close( My_Connections[Idx].res_stat->pipe[0] );
		close( My_Connections[Idx].res_stat->pipe[1] );
		free( My_Connections[Idx].res_stat );
	}

	/* Startzeit des naechsten Connect-Versuchs modifizieren? */
	if(( My_Connections[Idx].our_server > NONE ) && ( Conf_Server[My_Connections[Idx].our_server].lasttry <  time( NULL ) - Conf_ConnectRetry ))
	{
		/* Okay, die Verbindung stand schon "genuegend lange":
		 * lasttry-Zeitpunkt so setzen, dass der naechste
		 * Verbindungsversuch in RECONNECT_DELAY Sekunden
		 * gestartet wird. */
		Conf_Server[My_Connections[Idx].our_server].lasttry = time( NULL ) - Conf_ConnectRetry + RECONNECT_DELAY;
	}

#ifdef USE_ZLIB
	/* Ggf. zlib abmelden */
	if( Conn_Options( Idx ) & CONN_ZIP )
	{
		inflateEnd( &My_Connections[Idx].zip.in );
		deflateEnd( &My_Connections[Idx].zip.out );
	}
#endif

	/* Connection-Struktur loeschen (=freigeben) */
	Init_Conn_Struct( Idx );
} /* Conn_Close */


GLOBAL VOID
Conn_UpdateIdle( CONN_ID Idx )
{
	/* Idle-Timer zuruecksetzen */

	assert( Idx > NONE );
	My_Connections[Idx].lastprivmsg = time( NULL );
}


GLOBAL time_t
Conn_GetIdle( CONN_ID Idx )
{
	/* Idle-Time einer Verbindung liefern (in Sekunden) */

	assert( Idx > NONE );
	return time( NULL ) - My_Connections[Idx].lastprivmsg;
} /* Conn_GetIdle */


GLOBAL time_t
Conn_LastPing( CONN_ID Idx )
{
	/* Zeitpunkt des letzten PING liefern */

	assert( Idx > NONE );
	return My_Connections[Idx].lastping;
} /* Conn_LastPing */


GLOBAL VOID
Conn_SetPenalty( CONN_ID Idx, time_t Seconds )
{
	/* Penalty-Delay fuer eine Verbindung (in Sekunden) setzen;
	 * waehrend dieser Zeit wird der entsprechende Socket vom Server
	 * bei Lese-Operationen komplett ignoriert. Der Delay kann mit
	 * dieser Funktion nur erhoeht, nicht aber verringert werden. */
	
	time_t t;
	
	assert( Idx > NONE );
	assert( Seconds >= 0 );
	
	t = time( NULL ) + Seconds;
	if( t > My_Connections[Idx].delaytime ) My_Connections[Idx].delaytime = t;
} /* Conn_SetPenalty */


GLOBAL VOID
Conn_ResetPenalty( CONN_ID Idx )
{
	assert( Idx > NONE );
	My_Connections[Idx].delaytime = 0;
} /* Conn_ResetPenalty */


GLOBAL VOID
Conn_ClearFlags( VOID )
{
	/* Alle Connection auf "nicht-markiert" setzen */

	LONG i;

	for( i = 0; i < Pool_Size; i++ ) My_Connections[i].flag = 0;
} /* Conn_ClearFlags */


GLOBAL INT
Conn_Flag( CONN_ID Idx )
{
	/* Ist eine Connection markiert (TRUE) oder nicht? */

	assert( Idx > NONE );
	return My_Connections[Idx].flag;
} /* Conn_Flag */


GLOBAL VOID
Conn_SetFlag( CONN_ID Idx, INT Flag )
{
	/* Connection markieren */

	assert( Idx > NONE );
	My_Connections[Idx].flag = Flag;
} /* Conn_SetFlag */


GLOBAL CONN_ID
Conn_First( VOID )
{
	/* Connection-Struktur der ersten Verbindung liefern;
	 * Ist keine Verbindung vorhanden, wird NONE geliefert. */

	LONG i;
	
	for( i = 0; i < Pool_Size; i++ )
	{
		if( My_Connections[i].sock != NONE ) return i;
	}
	return NONE;
} /* Conn_First */


GLOBAL CONN_ID
Conn_Next( CONN_ID Idx )
{
	/* Naechste Verbindungs-Struktur liefern; existiert keine
	 * weitere, so wird NONE geliefert. */

	LONG i = NONE;

	assert( Idx > NONE );
	
	for( i = Idx + 1; i < Pool_Size; i++ )
	{
		if( My_Connections[i].sock != NONE ) return i;
	}
	return NONE;
} /* Conn_Next */


GLOBAL VOID
Conn_SetServer( CONN_ID Idx, INT ConfServer )
{
	/* Connection als Server markieren: Index des konfigurierten
	 * Servers speichern. Verbindung muss bereits bestehen! */
	
	assert( Idx > NONE );
	assert( My_Connections[Idx].sock > NONE );
	
	My_Connections[Idx].our_server = ConfServer;
} /* Conn_SetServer */


GLOBAL VOID
Conn_SetOption( CONN_ID Idx, INT Option )
{
	/* Option fuer Verbindung setzen.
	 * Initial sind alle Optionen _nicht_ gesetzt. */

	assert( Idx > NONE );
	assert( Option != 0 );

	My_Connections[Idx].options |= Option;
} /* Conn_SetOption */


GLOBAL VOID
Conn_UnsetOption( CONN_ID Idx, INT Option )
{
	/* Option fuer Verbindung loeschen */

	assert( Idx > NONE );
	assert( Option != 0 );

	My_Connections[Idx].options &= ~Option;
} /* Conn_UnsetOption */


GLOBAL INT
Conn_Options( CONN_ID Idx )
{
	assert( Idx > NONE );
	return My_Connections[Idx].options;
} /* Conn_Options */


#ifdef USE_ZLIB

GLOBAL BOOLEAN
Conn_InitZip( CONN_ID Idx )
{
	/* Kompression fuer Link initialisieren */

	assert( Idx > NONE );

	My_Connections[Idx].zip.in.avail_in = 0;
	My_Connections[Idx].zip.in.total_in = 0;
	My_Connections[Idx].zip.in.total_out = 0;
	My_Connections[Idx].zip.in.zalloc = NULL;
	My_Connections[Idx].zip.in.zfree = NULL;
	My_Connections[Idx].zip.in.data_type = Z_ASCII;

	if( inflateInit( &My_Connections[Idx].zip.in ) != Z_OK )
	{
		/* Fehler! */
		Log( LOG_ALERT, "Can't initialize compression on connection %d (zlib inflate)!", Idx );
		return FALSE;
	}
	
	My_Connections[Idx].zip.out.total_in = 0;
	My_Connections[Idx].zip.out.total_in = 0;
	My_Connections[Idx].zip.out.zalloc = NULL;
	My_Connections[Idx].zip.out.zfree = NULL;
	My_Connections[Idx].zip.out.data_type = Z_ASCII;

	if( deflateInit( &My_Connections[Idx].zip.out, Z_DEFAULT_COMPRESSION ) != Z_OK )
	{
		/* Fehler! */
		Log( LOG_ALERT, "Can't initialize compression on connection %d (zlib deflate)!", Idx );
		return FALSE;
	}

	My_Connections[Idx].zip.bytes_in = My_Connections[Idx].bytes_in;
	My_Connections[Idx].zip.bytes_out = My_Connections[Idx].bytes_out;

	Log( LOG_INFO, "Enabled link compression (zlib) on connection %d.", Idx );
	Conn_SetOption( Idx, CONN_ZIP );

	return TRUE;
} /* Conn_InitZip */

#endif


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

	assert( Idx > NONE );
	assert( My_Connections[Idx].sock > NONE );

	if( FD_ISSET( My_Connections[Idx].sock, &My_Connects ))
	{
		/* es soll nichts geschrieben werden, sondern ein
		 * connect() hat ein Ergebnis geliefert */

		FD_CLR( My_Connections[Idx].sock, &My_Connects );

		/* Ergebnis des connect() ermitteln */
		len = sizeof( err );
		res = getsockopt( My_Connections[Idx].sock, SOL_SOCKET, SO_ERROR, &err, &len );
		assert( len == sizeof( err ));

		/* Fehler aufgetreten? */
		if(( res != 0 ) || ( err != 0 ))
		{
			/* Fehler! */
			if( res != 0 ) Log( LOG_CRIT, "getsockopt (connection %d): %s!", Idx, strerror( errno ));
			else Log( LOG_CRIT, "Can't connect socket to \"%s:%d\" (connection %d): %s!", My_Connections[Idx].host, Conf_Server[My_Connections[Idx].our_server].port, Idx, strerror( err ));

			/* Socket etc. pp. aufraeumen */
			FD_CLR( My_Connections[Idx].sock, &My_Sockets );
			close( My_Connections[Idx].sock );
			Init_Conn_Struct( Idx );

			/* Bei Server-Verbindungen lasttry-Zeitpunkt auf "jetzt" setzen */
			Conf_Server[My_Connections[Idx].our_server].lasttry = time( NULL );

			return FALSE;
		}
		Log( LOG_DEBUG, "Connection %d with \"%s:%d\" established, now sendig PASS and SERVER ...", Idx, My_Connections[Idx].host, Conf_Server[My_Connections[Idx].our_server].port );

		/* PASS und SERVER verschicken */
		Conn_WriteStr( Idx, "PASS %s %s", Conf_Server[My_Connections[Idx].our_server].pwd_out, NGIRCd_ProtoID );
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
				close( new_sock );
				return;
			}
			if( new_size > Conf_MaxConnections ) new_size = Conf_MaxConnections;
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
				close( new_sock );
				return;
			}
			
			/* Struktur umkopieren ... */
			memcpy( ptr, My_Connections, sizeof( CONNECTION ) * Pool_Size );
			
			Log( LOG_DEBUG, "Allocated new connection pool for %ld items. [malloc()/memcpy()]", new_size );
		}
		else Log( LOG_DEBUG, "Allocated new connection pool for %ld items. [realloc()]", new_size );
		
		My_Connections = ptr;
		Pool_Size = new_size;
	}

	/* Client-Struktur initialisieren */
	c = Client_NewLocal( idx, inet_ntoa( new_addr.sin_addr ), CLIENT_UNKNOWN, FALSE );
	if( ! c )
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
	if( new_sock > Conn_MaxFD ) Conn_MaxFD = new_sock;

	Log( LOG_INFO, "Accepted connection %d from %s:%d on socket %d.", idx, inet_ntoa( new_addr.sin_addr ), ntohs( new_addr.sin_port), Sock );

	/* Hostnamen ermitteln */
	strcpy( My_Connections[idx].host, inet_ntoa( new_addr.sin_addr ));
	Client_SetHostname( c, My_Connections[idx].host );
	s = Resolve_Addr( &new_addr );
	if( s )
	{
		/* Sub-Prozess wurde asyncron gestartet */
		Conn_WriteStr( idx, "NOTICE AUTH :%sLooking up your hostname ...", NOTICE_TXTPREFIX );
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
	CLIENT *c;

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
		Log( LOG_INFO, "%s:%d is closing the connection ...", inet_ntoa( My_Connections[Idx].addr.sin_addr ), ntohs( My_Connections[Idx].addr.sin_port));
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
	LONG i;

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
	/* Pruefen, ob Server-Verbindungen aufgebaut werden
	 * muessen bzw. koennen */

	RES_STAT *s;
	LONG idx, n;
	INT i;

	/* Wenn "Passive-Mode" aktiv: nicht verbinden */
	if( NGIRCd_Passive ) return;

	for( i = 0; i < Conf_Server_Count; i++ )
	{
		/* Ist ein Hostname und Port definiert? */
		if(( ! Conf_Server[i].host[0] ) || ( ! Conf_Server[i].port > 0 )) continue;

		/* Haben wir schon eine Verbindung? */
		for( n = 0; n < Pool_Size; n++ )
		{
			if( My_Connections[n].sock == NONE ) continue;
			
			/* Verbindung zu diesem Server? */
			if( My_Connections[n].our_server == i )
			{
				/* Komplett aufgebaute Verbindung? */
				if( My_Connections[n].sock > NONE ) break;

				/* IP schon aufgeloest? */
				if( My_Connections[n].res_stat == NULL ) New_Server( i, n );
			}

			/* Verbindung in dieser Server-Gruppe? */
			if(( My_Connections[n].our_server != NONE ) && ( Conf_Server[i].group != NONE ))
			{
				if( Conf_Server[My_Connections[n].our_server].group == Conf_Server[i].group ) break;
			}
		}
		if( n < Pool_Size ) continue;

		/* Wann war der letzte Connect-Versuch? */
		if( Conf_Server[i].lasttry > time( NULL ) - Conf_ConnectRetry ) continue;

		/* Okay, Verbindungsaufbau versuchen */
		Conf_Server[i].lasttry = time( NULL );

		/* Freie Connection-Struktur suschen */
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
		My_Connections[idx].our_server = i;

		/* Hostnamen in IP aufloesen (Default bzw. im Fehlerfall: versuchen, den
		 * konfigurierten Text direkt als IP-Adresse zu verwenden ... */
		strcpy( Conf_Server[My_Connections[idx].our_server].ip, Conf_Server[i].host );
		strcpy( My_Connections[idx].host, Conf_Server[i].host );
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
	/* Neue Server-Verbindung aufbauen */

	struct sockaddr_in new_addr;
	struct in_addr inaddr;
	INT res, new_sock;
	CLIENT *c;

	assert( Server > NONE );
	assert( Idx > NONE );

	/* Wurde eine gueltige IP-Adresse gefunden? */
	if( ! Conf_Server[Server].ip[0] )
	{
		/* Nein. Verbindung wieder freigeben: */
		Init_Conn_Struct( Idx );
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
		Log( LOG_CRIT, "Can't create socket: %s!", strerror( errno ));
		return;
	}

	if( ! Init_Socket( new_sock )) return;

	res = connect( new_sock, (struct sockaddr *)&new_addr, sizeof( new_addr ));
	if(( res != 0 ) && ( errno != EINPROGRESS ))
	{
		Log( LOG_CRIT, "Can't connect socket: %s!", strerror( errno ));
		close( new_sock );
		Init_Conn_Struct( Idx );
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
	Client_SetToken( c, TOKEN_OUTBOUND );

	/* Verbindung registrieren */
	My_Connections[Idx].sock = new_sock;
	My_Connections[Idx].addr = new_addr;
	strcpy( My_Connections[Idx].host, Conf_Server[Server].host );

	/* Neuen Socket registrieren */
	FD_SET( new_sock, &My_Sockets );
	FD_SET( new_sock, &My_Connects );
	if( new_sock > Conn_MaxFD ) Conn_MaxFD = new_sock;
	
	Log( LOG_DEBUG, "Registered new connection %d on socket %d.", Idx, My_Connections[Idx].sock );
} /* New_Server */


LOCAL VOID
Init_Conn_Struct( LONG Idx )
{
	/* Connection-Struktur initialisieren */

	My_Connections[Idx].sock = NONE;
	My_Connections[Idx].res_stat = NULL;
	My_Connections[Idx].host[0] = '\0';
	My_Connections[Idx].rbuf[0] = '\0';
	My_Connections[Idx].rdatalen = 0;
	My_Connections[Idx].wbuf[0] = '\0';
	My_Connections[Idx].wdatalen = 0;
	My_Connections[Idx].our_server = NONE;
	My_Connections[Idx].lastdata = time( NULL );
	My_Connections[Idx].lastping = 0;
	My_Connections[Idx].lastprivmsg = time( NULL );
	My_Connections[Idx].delaytime = 0;
	My_Connections[Idx].bytes_in = 0;
	My_Connections[Idx].bytes_out = 0;
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
	INT len, i;

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
		strcpy( My_Connections[i].host, result );
		Client_SetHostname( c, result );

		Conn_WriteStr( i, "NOTICE AUTH :%sGot your hostname.", NOTICE_TXTPREFIX );
	}
	else
	{
		/* Ausgehende Verbindung (=Server): IP setzen */
		assert( My_Connections[i].our_server > NONE );
		strcpy( Conf_Server[My_Connections[i].our_server].ip, result );
	}

	/* Penalty-Zeit zurueck setzen */
	Conn_ResetPenalty( i );
} /* Read_Resolver_Result */


#ifdef USE_ZLIB

LOCAL BOOLEAN
Zip_Buffer( CONN_ID Idx, CHAR *Data, INT Len )
{
	/* Daten zum Komprimieren im "Kompressions-Puffer" sammeln.
	 * Es wird TRUE bei Erfolg, sonst FALSE geliefert. */

	assert( Idx > NONE );
	assert( Data != NULL );
	assert( Len > 0 );

	/* Ist noch Platz im Kompressions-Puffer? */
	if( ZWRITEBUFFER_LEN - My_Connections[Idx].zip.wdatalen < Len + 50 )
	{
		/* Nein! Puffer zunaechst leeren ...*/
		if( ! Zip_Flush( Idx )) return FALSE;
	}

	/* Daten kopieren */
	memmove( My_Connections[Idx].zip.wbuf + My_Connections[Idx].zip.wdatalen, Data, Len );
	My_Connections[Idx].zip.wdatalen += Len;

	return TRUE;
} /* Zip_Buffer */


LOCAL BOOLEAN
Zip_Flush( CONN_ID Idx )
{
	/* Daten komprimieren und in Schreibpuffer kopieren.
	 * Es wird TRUE bei Erfolg, sonst FALSE geliefert. */

	INT result, out_len;
	z_stream *out;

	out = &My_Connections[Idx].zip.out;

	out->next_in = My_Connections[Idx].zip.wbuf;
	out->avail_in = My_Connections[Idx].zip.wdatalen;
	out->next_out = My_Connections[Idx].wbuf + My_Connections[Idx].wdatalen;
	out->avail_out = WRITEBUFFER_LEN - My_Connections[Idx].wdatalen;

	result = deflate( out, Z_SYNC_FLUSH );
	if(( result != Z_OK ) || ( out->avail_in > 0 ))
	{
		Log( LOG_ALERT, "Compression error: code %d!?", result );
		Conn_Close( Idx, "Compression error!", NULL, FALSE );
		return FALSE;
	}

	out_len = WRITEBUFFER_LEN - My_Connections[Idx].wdatalen - out->avail_out;
	My_Connections[Idx].wdatalen += out_len;
	My_Connections[Idx].bytes_out += out_len;
	My_Connections[Idx].zip.bytes_out += My_Connections[Idx].zip.wdatalen;
	My_Connections[Idx].zip.wdatalen = 0;
	
	return TRUE;
} /* Zip_Flush */


LOCAL BOOLEAN
Unzip_Buffer( CONN_ID Idx )
{
	/* Daten entpacken und in Lesepuffer kopieren. Bei Fehlern
	 * wird FALSE geliefert, ansonsten TRUE. Der Fall, dass keine
	 * Daten mehr zu entpacken sind, ist _kein_ Fehler! */

	INT result, in_len, out_len;
	z_stream *in;

	assert( Idx > NONE );

	if( My_Connections[Idx].zip.rdatalen <= 0 ) return TRUE;

	in = &My_Connections[Idx].zip.in;

	in->next_in = My_Connections[Idx].zip.rbuf;
	in->avail_in = My_Connections[Idx].zip.rdatalen;
	in->next_out = My_Connections[Idx].rbuf + My_Connections[Idx].rdatalen;
	in->avail_out = READBUFFER_LEN - My_Connections[Idx].rdatalen - 1;

	result = inflate( in, Z_SYNC_FLUSH );
	if( result != Z_OK )
	{
		Log( LOG_ALERT, "Decompression error: code %d (ni=%d, ai=%d, no=%d, ao=%d)!?", result, in->next_in, in->avail_in, in->next_out, in->avail_out );
		Conn_Close( Idx, "Decompression error!", NULL, FALSE );
		return FALSE;
	}

	in_len = My_Connections[Idx].zip.rdatalen - in->avail_in;
	out_len = READBUFFER_LEN - My_Connections[Idx].rdatalen - 1 - in->avail_out;
	My_Connections[Idx].rdatalen += out_len;

	if( in->avail_in > 0 )
	{
		/* es konnten nicht alle Daten entpackt werden, vermutlich war
		 * im Ziel-Puffer kein Platz mehr. Umkopieren ... */
		My_Connections[Idx].zip.rdatalen -= in_len;
		memmove( My_Connections[Idx].zip.rbuf, My_Connections[Idx].zip.rbuf + in_len, My_Connections[Idx].zip.rdatalen );
	}
	else My_Connections[Idx].zip.rdatalen = 0;
	My_Connections[Idx].zip.bytes_in += out_len;

	return TRUE;
} /* Unzip_Buffer */


#endif


/* -eof- */
