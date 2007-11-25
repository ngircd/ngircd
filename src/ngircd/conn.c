/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2007 Alexander Barton (alex@barton.de)
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
#include "io.h"

static char UNUSED id[] = "$Id: conn.c,v 1.217 2007/11/25 18:42:37 fw Exp $";

#include "imp.h"
#include <assert.h>
#ifdef PROTOTYPES
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <netinet/in.h>

#ifdef HAVE_NETINET_IP_H
# include <netinet/ip.h>
#endif

#ifdef HAVE_STDINT_H
# include <stdint.h>			/* e.g. for Mac OS X */
#endif

#ifdef TCPWRAP
# include <tcpd.h>			/* for TCP Wrappers */
#endif

#include "array.h"
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

#ifdef ZEROCONF
# include "rendezvous.h"
#endif

#include "exp.h"


#define SERVER_WAIT (NONE - 1)


static bool Handle_Write PARAMS(( CONN_ID Idx ));
static bool Conn_Write PARAMS(( CONN_ID Idx, char *Data, size_t Len ));
static int New_Connection PARAMS(( int Sock ));
static CONN_ID Socket2Index PARAMS(( int Sock ));
static void Read_Request PARAMS(( CONN_ID Idx ));
static bool Handle_Buffer PARAMS(( CONN_ID Idx ));
static void Check_Connections PARAMS(( void ));
static void Check_Servers PARAMS(( void ));
static void Init_Conn_Struct PARAMS(( CONN_ID Idx ));
static bool Init_Socket PARAMS(( int Sock ));
static void New_Server PARAMS(( int Server, struct in_addr *dest));
static void Simple_Message PARAMS(( int Sock, const char *Msg ));
static int Count_Connections PARAMS(( struct sockaddr_in addr ));
static int NewListener PARAMS(( const UINT16 Port ));

static array My_Listeners;
static array My_ConnArray;

#ifdef TCPWRAP
int allow_severity = LOG_INFO;
int deny_severity = LOG_ERR;
#endif

static void server_login PARAMS((CONN_ID idx));

static void cb_Read_Resolver_Result PARAMS(( int sock, UNUSED short what));
static void cb_Connect_to_Server PARAMS(( int sock, UNUSED short what));
static void cb_clientserver PARAMS((int sock, short what));

static void
cb_listen(int sock, short irrelevant)
{
	(void) irrelevant;
	New_Connection( sock );
}


static void
cb_connserver(int sock, UNUSED short what)
{
	int res, err;
	socklen_t sock_len;
	CONN_ID idx = Socket2Index( sock );
	if (idx <= NONE) {
		LogDebug("cb_connserver wants to write on unknown socket?!");
		io_close(sock);
		return;
	}

	assert( what & IO_WANTWRITE);

	/* connect() finished, get result. */
	sock_len = sizeof( err );
	res = getsockopt( My_Connections[idx].sock, SOL_SOCKET, SO_ERROR, &err, &sock_len );
	assert( sock_len == sizeof( err ));

	/* Error while connecting? */
 	if ((res != 0) || (err != 0)) {
 		if (res != 0)
 			Log(LOG_CRIT, "getsockopt (connection %d): %s!",
 			    idx, strerror(errno));
 		else
 			Log(LOG_CRIT,
 			    "Can't connect socket to \"%s:%d\" (connection %d): %s!",
 			    My_Connections[idx].host,
 			    Conf_Server[Conf_GetServer(idx)].port,
 			    idx, strerror(err));

		Conn_Close(idx, "Can't connect!", NULL, false);
		return;
	}

	Conn_OPTION_DEL( &My_Connections[idx], CONN_ISCONNECTING );
	server_login(idx);
}


static void
server_login(CONN_ID idx)
{
	Log( LOG_INFO, "Connection %d with \"%s:%d\" established. Now logging in ...", idx,
			My_Connections[idx].host, Conf_Server[Conf_GetServer( idx )].port );

	io_event_setcb( My_Connections[idx].sock, cb_clientserver);
	io_event_add( My_Connections[idx].sock, IO_WANTREAD|IO_WANTWRITE);

	/* Send PASS and SERVER command to peer */
	Conn_WriteStr( idx, "PASS %s %s", Conf_Server[Conf_GetServer( idx )].pwd_out, NGIRCd_ProtoID );
	Conn_WriteStr( idx, "SERVER %s :%s", Conf_ServerName, Conf_ServerInfo );
}


static void
cb_clientserver(int sock, short what)
{
	CONN_ID idx = Socket2Index( sock );
	if (idx <= NONE) {
#ifdef DEBUG
		Log(LOG_WARNING, "WTF: cb_clientserver wants to write on unknown socket?!");
#endif
		io_close(sock);
		return;
	}

	if (what & IO_WANTREAD)
		Read_Request( idx );

	if (what & IO_WANTWRITE)
		Handle_Write( idx );
}


GLOBAL void
Conn_Init( void )
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
	
	if (!array_alloc(&My_ConnArray, sizeof(CONNECTION), (size_t)Pool_Size)) {
		Log( LOG_EMERG, "Can't allocate memory! [Conn_Init]" );
		exit( 1 );
	}

	/* FIXME: My_Connetions/Pool_Size is needed by other parts of the
	 * code; remove them! */
	My_Connections = (CONNECTION*) array_start(&My_ConnArray);

	LogDebug("Allocated connection pool for %d items (%ld bytes).",
		array_length(&My_ConnArray, sizeof( CONNECTION )), array_bytes(&My_ConnArray));

	assert( array_length(&My_ConnArray, sizeof( CONNECTION )) >= (size_t) Pool_Size);
	
	array_free( &My_Listeners );

	/* Connection-Struktur initialisieren */
	for( i = 0; i < Pool_Size; i++ ) Init_Conn_Struct( i );

	/* Global write counter */
	WCounter = 0;
} /* Conn_Init */


GLOBAL void
Conn_Exit( void )
{
	/* Modul abmelden: alle noch offenen Connections
	 * schliessen und freigeben. */

	CONN_ID idx;

	LogDebug("Shutting down all connections ..." );

	Conn_ExitListeners();

	/* Sockets schliessen */
	for( idx = 0; idx < Pool_Size; idx++ ) {
		if( My_Connections[idx].sock > NONE ) {
			Conn_Close( idx, NULL, NGIRCd_SignalRestart ?
				"Server going down (restarting)":"Server going down", true );
		}
	}

	array_free(&My_ConnArray);
	My_Connections = NULL;
	Pool_Size = 0;
	io_library_shutdown();
} /* Conn_Exit */


static unsigned int
ports_initlisteners(array *a, void (*func)(int,short))
{
	unsigned int created = 0;
	size_t len;
	int fd;
	UINT16 *port;

	len = array_length(a, sizeof (UINT16));
	port = array_start(a);
	while(len--) {
		fd = NewListener( *port );
		if (fd < 0) {
			port++;
			continue;
		}
		if (!io_event_create( fd, IO_WANTREAD, func )) {
			Log( LOG_ERR, "io_event_create(): Could not add listening fd %d (port %u): %s!",
							fd, (unsigned int) *port, strerror(errno));
			close(fd);
			port++;
			continue;
		}
		created++;
		port++;
	}

	return created;
}


GLOBAL unsigned int
Conn_InitListeners( void )
{
	/* Initialize ports on which the server should accept connections */

	unsigned int created;

	if (!io_library_init(CONNECTION_POOL)) {
		Log(LOG_EMERG, "Cannot initialize IO routines: %s", strerror(errno));
		return -1;
	}

	created = ports_initlisteners(&Conf_ListenPorts, cb_listen);

	return created;
} /* Conn_InitListeners */


GLOBAL void
Conn_ExitListeners( void )
{
	/* Close down all listening sockets */
	int *fd;
	size_t arraylen;
#ifdef ZEROCONF
	Rendezvous_UnregisterListeners( );
#endif

	arraylen = array_length(&My_Listeners, sizeof (int));
	Log( LOG_INFO, "Shutting down all listening sockets (%d total)...", arraylen );
	fd = array_start(&My_Listeners);
	while(arraylen--) {
		assert(fd != NULL);
		assert(*fd >= 0);
		io_close(*fd);
		LogDebug("Listening socket %d closed.", *fd );
		fd++;
	}
	array_free(&My_Listeners);
} /* Conn_ExitListeners */


static void
InitSinaddr(struct sockaddr_in *addr, UINT16 Port)
{
	struct in_addr inaddr;

	memset(addr, 0, sizeof(*addr));
	memset( &inaddr, 0, sizeof(inaddr));

	addr->sin_family = AF_INET;
	addr->sin_port = htons(Port);
	inaddr.s_addr = htonl(INADDR_ANY);
	addr->sin_addr = inaddr;
}


static bool
InitSinaddrListenAddr(struct sockaddr_in *addr, UINT16 Port)
{
	struct in_addr inaddr;

	InitSinaddr(addr, Port);

	if (!Conf_ListenAddress[0])
		return true;

	if (!ngt_IPStrToBin(Conf_ListenAddress, &inaddr)) {
		Log( LOG_CRIT, "Can't bind to %s:%u: can't convert ip address \"%s\"",
				Conf_ListenAddress, Port, Conf_ListenAddress);
		return false;
	}

	addr->sin_addr = inaddr;
	return true;
}


/* return new listening port file descriptor or -1 on failure */
static int
NewListener( const UINT16 Port )
{
	/* Create new listening socket on specified port */

	struct sockaddr_in addr;
	int sock;
#ifdef ZEROCONF
	char name[CLIENT_ID_LEN], *info;
#endif

	InitSinaddrListenAddr(&addr, Port);

	addr.sin_family = AF_INET;
	addr.sin_port = htons( Port );

	sock = socket( PF_INET, SOCK_STREAM, 0);
	if( sock < 0 ) {
		Log( LOG_CRIT, "Can't create socket: %s!", strerror( errno ));
		return -1;
	}

	if( ! Init_Socket( sock )) return -1;

	if (bind(sock, (struct sockaddr *)&addr, (socklen_t)sizeof(addr)) != 0) {
		Log( LOG_CRIT, "Can't bind socket (port %d) : %s!", Port, strerror( errno ));
		close( sock );
		return -1;
	}

	if( listen( sock, 10 ) != 0 ) {
		Log( LOG_CRIT, "Can't listen on socket: %s!", strerror( errno ));
		close( sock );
		return -1;
	}

	/* keep fd in list so we can close it when ngircd restarts/shuts down */
	if (!array_catb( &My_Listeners,(char*) &sock, sizeof(int) )) {
		Log( LOG_CRIT, "Can't add socket to My_Listeners array: %s!", strerror( errno ));
		close( sock );
		return -1;
	}

	if( Conf_ListenAddress[0]) Log( LOG_INFO, "Now listening on %s:%d (socket %d).", Conf_ListenAddress, Port, sock );
	else Log( LOG_INFO, "Now listening on 0.0.0.0:%d (socket %d).", Port, sock );

#ifdef ZEROCONF
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
	if (Port != 6667)
		snprintf(name, sizeof name, "%s (port %u)", info,
		 	 (unsigned int)Port);
	else
	 	strlcpy(name, info, sizeof name);

	/* Register service */
	Rendezvous_Register( name, MDNS_TYPE, Port );
#endif
	return sock;
} /* NewListener */


GLOBAL void
Conn_Handler( void )
{
	/* "Main Loop.": Loop until a signal (for shutdown or restart) arrives.
	 * Call io_dispatch() to check for read/writeable sockets every second
	 * Wait for status change on pending connections (e.g: when the hostname has been resolved)
	 * check for penalty/timeouts
	 * handle input buffers
	 */
	int i;
	unsigned int wdatalen;
	struct timeval tv;
	time_t t;
	bool timeout;

	while(( ! NGIRCd_SignalQuit ) && ( ! NGIRCd_SignalRestart )) {
		timeout = true;

#ifdef ZEROCONF
		Rendezvous_Handler( );
#endif

		/* Should the configuration be reloaded? */
		if (NGIRCd_SignalRehash) {
			NGIRCd_Rehash( );
		}

		/* Check configured servers and established links */
		Check_Servers( );
		Check_Connections( );

		t = time( NULL );

		/* noch volle Lese-Buffer suchen */
		for( i = 0; i < Pool_Size; i++ ) {
			if(( My_Connections[i].sock > NONE ) && ( array_bytes(&My_Connections[i].rbuf) > 0 ) &&
			 ( My_Connections[i].delaytime < t ))
			{
				/* Kann aus dem Buffer noch ein Befehl extrahiert werden? */
				if (Handle_Buffer( i )) timeout = false;
			}
		}

		/* noch volle Schreib-Puffer suchen */
		for( i = 0; i < Pool_Size; i++ ) {
			if ( My_Connections[i].sock <= NONE )
				continue;

			wdatalen = (unsigned int)array_bytes(&My_Connections[i].wbuf);

#ifdef ZLIB
			if (( wdatalen > 0 ) || ( array_bytes(&My_Connections[i].zip.wbuf)> 0 ))
#else
			if ( wdatalen > 0 )
#endif
			{
				/* Socket der Verbindung in Set aufnehmen */
				io_event_add( My_Connections[i].sock, IO_WANTWRITE );
			}
		}

		/* von welchen Sockets koennte gelesen werden? */
		for (i = 0; i < Pool_Size; i++ ) {
			if ( My_Connections[i].sock <= NONE )
				continue;

			if (Resolve_INPROGRESS(&My_Connections[i].res_stat)) {
				/* wait for completion of Resolver Sub-Process */
				io_event_del( My_Connections[i].sock, IO_WANTREAD );
				continue;
			}

			if ( Conn_OPTION_ISSET( &My_Connections[i], CONN_ISCONNECTING ))
				continue;	/* wait for completion of connect() */

			if( My_Connections[i].delaytime > t ) {
				/* Fuer die Verbindung ist eine "Penalty-Zeit" gesetzt */
				io_event_del( My_Connections[i].sock, IO_WANTREAD );
				continue;
			}
			io_event_add( My_Connections[i].sock, IO_WANTREAD );
		}

		/* (re-)set timeout - tv_sec/usec are undefined after io_dispatch() returns */
		tv.tv_usec = 0;
		tv.tv_sec = timeout ? 1 : 0;

		/* wait for activity */
		i = io_dispatch( &tv );
		if (i == -1 && errno != EINTR ) {
			Log(LOG_EMERG, "Conn_Handler(): io_dispatch(): %s!", strerror(errno));
			Log(LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE_NAME);
			exit( 1 );
		}
	}

	if( NGIRCd_SignalQuit ) Log( LOG_NOTICE|LOG_snotice, "Server going down NOW!" );
	else if( NGIRCd_SignalRestart ) Log( LOG_NOTICE|LOG_snotice, "Server restarting NOW!" );
} /* Conn_Handler */


/**
 * Write a text string into the socket of a connection.
 * This function automatically appends CR+LF to the string and validates that
 * the result is a valid IRC message (oversized messages are shortened, for
 * example). Then it calls the Conn_Write() function to do the actual sending.
 * @param Idx Index fo the connection.
 * @param Format Format string, see printf().
 * @return true on success, false otherwise.
 */
#ifdef PROTOTYPES
GLOBAL bool
Conn_WriteStr( CONN_ID Idx, char *Format, ... )
#else
GLOBAL bool 
Conn_WriteStr( Idx, Format, va_alist )
CONN_ID Idx;
char *Format;
va_dcl
#endif
{
	char buffer[COMMAND_LEN];
	size_t len;
	bool ok;
	va_list ap;

	assert( Idx > NONE );
	assert( Format != NULL );

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	if (vsnprintf( buffer, COMMAND_LEN - 2, Format, ap ) >= COMMAND_LEN - 2 ) {
		/*
		 * The string that should be written to the socket is longer
		 * than the allowed size of COMMAND_LEN bytes (including both
		 * the CR and LF characters). This can be caused by the
		 * IRC_WriteXXX() functions when the prefix of this server had
		 * to be added to an already "quite long" command line which
		 * has been received from a regular IRC client, for example.
		 * 
		 * We are not allowed to send such "oversized" messages to
		 * other servers and clients, see RFC 2812 2.3 and 2813 3.3
		 * ("these messages SHALL NOT exceed 512 characters in length,
		 * counting all characters including the trailing CR-LF").
		 *
		 * So we have a big problem here: we should send more bytes
		 * to the network than we are allowed to and we don't know
		 * the originator (any more). The "old" behaviour of blaming
		 * the receiver ("next hop") is a bad idea (it could be just
		 * an other server only routing the message!), so the only
		 * option left is to shorten the string and to hope that the
		 * result is still somewhat useful ...
		 *                                                   -alex-
		 */

		strcpy (buffer + sizeof(buffer) - strlen(CUT_TXTSUFFIX) - 2 - 1,
			CUT_TXTSUFFIX);
	}

#ifdef SNIFFER
	if (NGIRCd_Sniffer)
		Log(LOG_DEBUG, " -> connection %d: '%s'.", Idx, buffer);
#endif

	len = strlcat( buffer, "\r\n", sizeof( buffer ));
	ok = Conn_Write(Idx, buffer, len);
	My_Connections[Idx].msg_out++;

	va_end( ap );
	return ok;
} /* Conn_WriteStr */


/**
 * Append Data to the outbound write buffer of a connection.
 * @param Idx Index of the connection.
 * @param Data pointer to the data.
 * @param Len length of Data.
 * @return true on success, false otherwise.
 */
static bool
Conn_Write( CONN_ID Idx, char *Data, size_t Len )
{
	CLIENT *c;
	size_t writebuf_limit = WRITEBUFFER_LEN;
	assert( Idx > NONE );
	assert( Data != NULL );
	assert( Len > 0 );

	c = Conn_GetClient(Idx);
	assert( c != NULL);

	/* Servers do get special write buffer limits, so they can generate
	 * all the messages that are required while peering. */
	if (Client_Type(c) == CLIENT_SERVER)
		writebuf_limit = WRITEBUFFER_SLINK_LEN;

	/* Is the socket still open? A previous call to Conn_Write()
	 * may have closed the connection due to a fatal error.
	 * In this case it is sufficient to return an error, as well. */
	if( My_Connections[Idx].sock <= NONE ) {
		LogDebug("Skipped write on closed socket (connection %d).", Idx);
		return false;
	}

#ifdef ZLIB
	if ( Conn_OPTION_ISSET( &My_Connections[Idx], CONN_ZIP )) {
		/* Compressed link:
		 * Zip_Buffer() does all the dirty work for us: it flushes
		 * the (pre-)compression buffers if required and handles
		 * all error conditions. */
		if (!Zip_Buffer(Idx, Data, Len))
			return false;
	}
	else
#endif
	{
		/* Uncompressed link:
		 * Check if outbound buffer has enough space for the data. */
		if (array_bytes(&My_Connections[Idx].wbuf) + Len >=
		    writebuf_limit) {
			/* Buffer is full, flush it. Handle_Write deals with
			 * low-level errors, if any. */
			if (!Handle_Write(Idx))
				return false;
		}

		/* When the write buffer is still too big after flushing it,
		 * the connection will be killed. */
		if (array_bytes(&My_Connections[Idx].wbuf) + Len >=
		    writebuf_limit) {
			Log(LOG_NOTICE,
			    "Write buffer overflow (connection %d, size %lu byte)!",
			    Idx,
			    (unsigned long)array_bytes(&My_Connections[Idx].wbuf));
			Conn_Close(Idx, "Write buffer overflow!", NULL, false);
			return false;
		}

		/* Copy data to write buffer */
		if (!array_catb(&My_Connections[Idx].wbuf, Data, Len))
			return false;

		My_Connections[Idx].bytes_out += Len;
	}

	/* Adjust global write counter */
	WCounter += Len;

	return true;
} /* Conn_Write */


GLOBAL void
Conn_Close( CONN_ID Idx, char *LogMsg, char *FwdMsg, bool InformClient )
{
	/* Close connection. Open pipes of asyncronous resolver
	 * sub-processes are closed down. */

	CLIENT *c;
	char *txt;
	double in_k, out_k;
#ifdef ZLIB
	double in_z_k, out_z_k;
	int in_p, out_p;
#endif

	assert( Idx > NONE );

	/* Is this link already shutting down? */
	if( Conn_OPTION_ISSET( &My_Connections[Idx], CONN_ISCLOSING )) {
		/* Conn_Close() has been called recursively for this link;
		 * probabe reason: Handle_Write() failed  -- see below. */
		LogDebug("Recursive request to close connection: %d", Idx );
		return;
	}

	assert( My_Connections[Idx].sock > NONE );

	/* Mark link as "closing" */
	Conn_OPTION_ADD( &My_Connections[Idx], CONN_ISCLOSING );

	if (LogMsg)
		txt = LogMsg;
	else
		txt = FwdMsg;
	if (! txt)
		txt = "Reason unknown";

	Log(LOG_INFO, "Shutting down connection %d (%s) with %s:%d ...", Idx,
	    LogMsg ? LogMsg : FwdMsg, My_Connections[Idx].host,
	    ntohs(My_Connections[Idx].addr.sin_port));

	/* Search client, if any */
	c = Conn_GetClient( Idx );

	/* Should the client be informed? */
	if (InformClient) {
#ifndef STRICT_RFC
		/* Send statistics to client if registered as user: */
		if ((c != NULL) && (Client_Type(c) == CLIENT_USER)) {
			Conn_WriteStr( Idx,
			 ":%s NOTICE %s :%sConnection statistics: client %.1f kb, server %.1f kb.",
			 Client_ID(Client_ThisServer()), Client_ID(c),
			 NOTICE_TXTPREFIX,
			 (double)My_Connections[Idx].bytes_in / 1024,
			 (double)My_Connections[Idx].bytes_out / 1024);
		}
#endif
		/* Send ERROR to client (see RFC!) */
		if (FwdMsg)
			Conn_WriteStr(Idx, "ERROR :%s", FwdMsg);
		else
			Conn_WriteStr(Idx, "ERROR :Closing connection.");
	}

	/* Try to write out the write buffer. Note: Handle_Write() eventually
	 * removes the CLIENT structure associated with this connection if an
	 * error occurs! So we have to re-check if there is still an valid
	 * CLIENT structure after calling Handle_Write() ...*/
	(void)Handle_Write( Idx );

	/* Search client, if any (re-check!) */
	c = Conn_GetClient( Idx );

	/* Shut down socket */
	if (! io_close(My_Connections[Idx].sock)) {
		/* Oops, we can't close the socket!? This is ... ugly! */
		Log(LOG_CRIT,
		    "Error closing connection %d (socket %d) with %s:%d - %s! (ignored)",
		    Idx, My_Connections[Idx].sock, My_Connections[Idx].host,
		    ntohs(My_Connections[Idx].addr.sin_port), strerror(errno));
	}

	/* Mark socket as invalid: */
	My_Connections[Idx].sock = NONE;

	/* If there is still a client, unregister it now */
	if (c)
		Client_Destroy(c, LogMsg, FwdMsg, true);

	/* Calculate statistics and log information */
	in_k = (double)My_Connections[Idx].bytes_in / 1024;
	out_k = (double)My_Connections[Idx].bytes_out / 1024;
#ifdef ZLIB
	if (Conn_OPTION_ISSET( &My_Connections[Idx], CONN_ZIP)) {
		in_z_k = (double)My_Connections[Idx].zip.bytes_in / 1024;
		out_z_k = (double)My_Connections[Idx].zip.bytes_out / 1024;
		/* Make sure that no division by zero can occur during
		 * the calculation of in_p and out_p: in_z_k and out_z_k
		 * are non-zero, that's guaranteed by the protocol until
		 * compression can be enabled. */
		if (! in_z_k)
			in_z_k = in_k;
		if (! out_z_k)
			out_z_k = out_k;
		in_p = (int)(( in_k * 100 ) / in_z_k );
		out_p = (int)(( out_k * 100 ) / out_z_k );
		Log(LOG_INFO,
		    "Connection %d with %s:%d closed (in: %.1fk/%.1fk/%d%%, out: %.1fk/%.1fk/%d%%).",
		    Idx, My_Connections[Idx].host,
		    ntohs(My_Connections[Idx].addr.sin_port),
		    in_k, in_z_k, in_p, out_k, out_z_k, out_p);
	}
	else
#endif
	{
		Log(LOG_INFO,
		    "Connection %d with %s:%d closed (in: %.1fk, out: %.1fk).",
		    Idx, My_Connections[Idx].host,
		    ntohs(My_Connections[Idx].addr.sin_port),
		    in_k, out_k);
	}

	/* cancel running resolver */
	if (Resolve_INPROGRESS(&My_Connections[Idx].res_stat))
		Resolve_Shutdown(&My_Connections[Idx].res_stat);

	/* Servers: Modify time of next connect attempt? */
	Conf_UnsetServer( Idx );

#ifdef ZLIB
	/* Clean up zlib, if link was compressed */
	if ( Conn_OPTION_ISSET( &My_Connections[Idx], CONN_ZIP )) {
		inflateEnd( &My_Connections[Idx].zip.in );
		deflateEnd( &My_Connections[Idx].zip.out );
		array_free(&My_Connections[Idx].zip.rbuf);
		array_free(&My_Connections[Idx].zip.wbuf);
	}
#endif

	array_free(&My_Connections[Idx].rbuf);
	array_free(&My_Connections[Idx].wbuf);

	/* Clean up connection structure (=free it) */
	Init_Conn_Struct( Idx );

	LogDebug("Shutdown of connection %d completed.", Idx );
} /* Conn_Close */


GLOBAL void
Conn_SyncServerStruct( void )
{
	/* Synchronize server structures (connection IDs):
	 * connections <-> configuration */

	CLIENT *client;
	CONN_ID i;
	int c;

	for( i = 0; i < Pool_Size; i++ ) {
		/* Established connection? */
		if (My_Connections[i].sock < 0)
			continue;

		/* Server connection? */
		client = Conn_GetClient( i );
		if(( ! client ) || ( Client_Type( client ) != CLIENT_SERVER )) continue;

		for( c = 0; c < MAX_SERVERS; c++ )
		{
			/* Configured server? */
			if( ! Conf_Server[c].host[0] ) continue;

			/* Duplicate? */
			if( strcmp( Conf_Server[c].name, Client_ID( client )) == 0 )
				Conf_Server[c].conn_id = i;
		}
	}
} /* SyncServerStruct */


/**
 * Send out data of write buffer; connect new sockets.
 */
static bool
Handle_Write( CONN_ID Idx )
{
	ssize_t len;
	size_t wdatalen;

	assert( Idx > NONE );
	if ( My_Connections[Idx].sock < 0 ) {
		LogDebug("Handle_Write() on closed socket, connection %d", Idx);
		return false;
	}
	assert( My_Connections[Idx].sock > NONE );

	wdatalen = array_bytes(&My_Connections[Idx].wbuf );

#ifdef ZLIB
	if (wdatalen == 0) {
		/* Write buffer is empty, so we try to flush the compression
		 * buffer and get some data to work with from there :-) */
		if (!Zip_Flush(Idx))
			return false;

		/* Now the write buffer most probably has changed: */
		wdatalen = array_bytes(&My_Connections[Idx].wbuf);
	}
#endif

	if (wdatalen == 0) {
		/* Still no data, fine. */
		io_event_del(My_Connections[Idx].sock, IO_WANTWRITE );
		return true;
	}

	LogDebug
	    ("Handle_Write() called for connection %d, %ld bytes pending ...",
	     Idx, wdatalen);

	len = write(My_Connections[Idx].sock,
	            array_start(&My_Connections[Idx].wbuf), wdatalen );

	if( len < 0 ) {
		if (errno == EAGAIN || errno == EINTR)
			return true;

		Log(LOG_ERR, "Write error on connection %d (socket %d): %s!",
		    Idx, My_Connections[Idx].sock, strerror(errno));
		Conn_Close(Idx, "Write error!", NULL, false);
		return false;
	}

	/* move any data not yet written to beginning */
	array_moveleft(&My_Connections[Idx].wbuf, 1, (size_t)len);

	return true;
} /* Handle_Write */


static int
New_Connection( int Sock )
{
	/* Neue Client-Verbindung von Listen-Socket annehmen und
	 * CLIENT-Struktur anlegen. */

#ifdef TCPWRAP
	struct request_info req;
#endif
	struct sockaddr_in new_addr;
	int new_sock, new_sock_len, new_Pool_Size;
	CLIENT *c;
	long cnt;

	assert( Sock > NONE );
	/* Connection auf Listen-Socket annehmen */
	new_sock_len = (int)sizeof new_addr;
	new_sock = accept(Sock, (struct sockaddr *)&new_addr,
			  (socklen_t *)&new_sock_len);
	if (new_sock < 0) {
		Log(LOG_CRIT, "Can't accept connection: %s!", strerror(errno));
		return -1;
	}

#ifdef TCPWRAP
	/* Validate socket using TCP Wrappers */
	request_init( &req, RQ_DAEMON, PACKAGE_NAME, RQ_FILE, new_sock, RQ_CLIENT_SIN, &new_addr, NULL );
	fromhost(&req);
	if( ! hosts_access( &req ))
	{
		/* Access denied! */
		Log( deny_severity, "Refused connection from %s (by TCP Wrappers)!", inet_ntoa( new_addr.sin_addr ));
		Simple_Message( new_sock, "ERROR :Connection refused" );
		close( new_sock );
		return -1;
	}
#endif

	/* Socket initialisieren */
	if (!Init_Socket( new_sock ))
		return -1;
	
	/* Check IP-based connection limit */
	cnt = Count_Connections( new_addr );
	if(( Conf_MaxConnectionsIP > 0 ) && ( cnt >= Conf_MaxConnectionsIP ))
	{
		/* Access denied, too many connections from this IP address! */
		Log( LOG_ERR, "Refused connection from %s: too may connections (%ld) from this IP address!", inet_ntoa( new_addr.sin_addr ), cnt);
		Simple_Message( new_sock, "ERROR :Connection refused, too many connections from your IP address!" );
		close( new_sock );
		return -1;
	}

	if( new_sock >= Pool_Size ) {
		new_Pool_Size = new_sock + 1;
		/* No free Connection Structures, check if we may accept further connections */
		if ((( Conf_MaxConnections > 0) && Pool_Size >= Conf_MaxConnections) ||
			(new_Pool_Size < Pool_Size))
		{
			Log( LOG_ALERT, "Can't accept connection: limit (%d) reached!", Pool_Size );
			Simple_Message( new_sock, "ERROR :Connection limit reached" );
			close( new_sock );
			return -1;
		}

		if (!array_alloc(&My_ConnArray, sizeof(CONNECTION),
				 (size_t)new_sock)) {
			Log( LOG_EMERG, "Can't allocate memory! [New_Connection]" );
			Simple_Message( new_sock, "ERROR: Internal error" );
			close( new_sock );
			return -1;
		}
		LogDebug("Bumped connection pool to %ld items (internal: %ld items, %ld bytes)",
			new_sock, array_length(&My_ConnArray, sizeof(CONNECTION)), array_bytes(&My_ConnArray));

		/* Adjust pointer to new block */
		My_Connections = array_start(&My_ConnArray);
		while (Pool_Size < new_Pool_Size)
			Init_Conn_Struct(Pool_Size++);
	}

	/* register callback */
	if (!io_event_create( new_sock, IO_WANTREAD, cb_clientserver)) {
		Log(LOG_ALERT, "Can't accept connection: io_event_create failed!");
		Simple_Message(new_sock, "ERROR :Internal error");
		close(new_sock);
		return -1;
	}

	c = Client_NewLocal( new_sock, inet_ntoa( new_addr.sin_addr ), CLIENT_UNKNOWN, false );
	if( ! c ) {
		Log(LOG_ALERT, "Can't accept connection: can't create client structure!");
		Simple_Message(new_sock, "ERROR :Internal error");
		io_close(new_sock);
		return -1;
	}

	Init_Conn_Struct( new_sock );
	My_Connections[new_sock].sock = new_sock;
	My_Connections[new_sock].addr = new_addr;
	My_Connections[new_sock].client = c;

	Log( LOG_INFO, "Accepted connection %d from %s:%d on socket %d.", new_sock,
			inet_ntoa( new_addr.sin_addr ), ntohs( new_addr.sin_port), Sock );

	/* Hostnamen ermitteln */
	strlcpy( My_Connections[new_sock].host, inet_ntoa( new_addr.sin_addr ),
						sizeof( My_Connections[new_sock].host ));

	Client_SetHostname( c, My_Connections[new_sock].host );

	if (!Conf_NoDNS)
		Resolve_Addr(&My_Connections[new_sock].res_stat, &new_addr,
			My_Connections[new_sock].sock, cb_Read_Resolver_Result);

	Conn_SetPenalty(new_sock, 4);
	return new_sock;
} /* New_Connection */


static CONN_ID
Socket2Index( int Sock )
{
	/* zum Socket passende Connection suchen */

	assert( Sock >= 0 );

	if( Sock >= Pool_Size || My_Connections[Sock].sock != Sock ) {
		/* die Connection wurde vermutlich (wegen eines
		 * Fehlers) bereits wieder abgebaut ... */
		LogDebug("Socket2Index: can't get connection for socket %d!", Sock);
		return NONE;
	}
	return Sock;
} /* Socket2Index */


/**
 * Read data from the network to the read buffer. If an error occures,
 * the socket of this connection will be shut down.
 */
static void
Read_Request( CONN_ID Idx )
{
	ssize_t len;
	char readbuf[READBUFFER_LEN];
	CLIENT *c;
	assert( Idx > NONE );
	assert( My_Connections[Idx].sock > NONE );

#ifdef ZLIB
	if ((array_bytes(&My_Connections[Idx].rbuf) >= READBUFFER_LEN) ||
		(array_bytes(&My_Connections[Idx].zip.rbuf) >= READBUFFER_LEN))
#else
	if (array_bytes(&My_Connections[Idx].rbuf) >= READBUFFER_LEN)
#endif
	{
		/* Read buffer is full */
		Log(LOG_ERR,
		    "Receive buffer overflow (connection %d): %d bytes!",
		    Idx, array_bytes(&My_Connections[Idx].rbuf));
		Conn_Close( Idx, "Receive buffer overflow!", NULL, false );
		return;
	}

	len = read(My_Connections[Idx].sock, readbuf, sizeof(readbuf));
	if (len == 0) {
		Log(LOG_INFO, "%s:%d (%s) is closing the connection ...",
		    My_Connections[Idx].host,
		    ntohs(My_Connections[Idx].addr.sin_port),
		    inet_ntoa( My_Connections[Idx].addr.sin_addr));
		Conn_Close(Idx,
			   "Socket closed!", "Client closed connection",
			   false);
		return;
	}

	if (len < 0) {
		if( errno == EAGAIN ) return;
		Log(LOG_ERR, "Read error on connection %d (socket %d): %s!",
		    Idx, My_Connections[Idx].sock, strerror(errno));
		Conn_Close(Idx, "Read error!", "Client closed connection",
			   false);
		return;
	}
#ifdef ZLIB
	if (Conn_OPTION_ISSET(&My_Connections[Idx], CONN_ZIP)) {
		if (!array_catb(&My_Connections[Idx].zip.rbuf, readbuf,
				(size_t) len)) {
			Log(LOG_ERR,
			    "Could not append recieved data to zip input buffer (connn %d): %d bytes!",
			    Idx, len);
			Conn_Close(Idx, "Receive buffer overflow!", NULL,
				   false);
			return;
		}
	} else
#endif
	{
		if (!array_catb( &My_Connections[Idx].rbuf, readbuf, len)) {
			Log( LOG_ERR, "Could not append recieved data to input buffer (connn %d): %d bytes!", Idx, len );
			Conn_Close( Idx, "Receive buffer overflow!", NULL, false );
		}
	}

	/* Update connection statistics */
	My_Connections[Idx].bytes_in += len;

	/* Update timestamp of last data received if this connection is
	 * registered as a user, server or service connection. Don't update
	 * otherwise, so users have at least Conf_PongTimeout seconds time to
	 * register with the IRC server -- see Check_Connections().
	 * Set "lastping", too, so we can handle time shifts backwards ... */
	c = Conn_GetClient(Idx);
	if (c && (Client_Type(c) == CLIENT_USER
		  || Client_Type(c) == CLIENT_SERVER
		  || Client_Type(c) == CLIENT_SERVICE)) {
		My_Connections[Idx].lastdata = time(NULL);
		My_Connections[Idx].lastping = My_Connections[Idx].lastdata;
	}

	/* Look at the data in the (read-) buffer of this connection */
	Handle_Buffer(Idx);
} /* Read_Request */


static bool
Handle_Buffer( CONN_ID Idx )
{
	/* Handle Data in Connections Read-Buffer.
	 * Return true if a reuqest was handled, false otherwise (also returned on errors). */
#ifndef STRICT_RFC
	char *ptr1, *ptr2;
#endif
	char *ptr;
	size_t len, delta;
	bool result;
	time_t starttime;
#ifdef ZLIB
	bool old_z;
#endif

	starttime = time(NULL);
	result = false;
	for (;;) {
		/* Check penalty */
		if( My_Connections[Idx].delaytime > starttime) return result;
#ifdef ZLIB
		/* unpack compressed data */
		if ( Conn_OPTION_ISSET( &My_Connections[Idx], CONN_ZIP ))
			if( ! Unzip_Buffer( Idx )) return false;
#endif

		if (0 == array_bytes(&My_Connections[Idx].rbuf))
			break;

		if (!array_cat0_temporary(&My_Connections[Idx].rbuf)) /* make sure buf is NULL terminated */
			return false;

		/* A Complete Request end with CR+LF, see RFC 2812. */
		ptr = strstr( array_start(&My_Connections[Idx].rbuf), "\r\n" );

		if( ptr ) delta = 2; /* complete request */
#ifndef STRICT_RFC
		else {
			/* Check for non-RFC-compliant request (only CR or LF)? Unfortunately,
			 * there are quite a few clients that do this (incl. "mIRC" :-( */
			ptr1 = strchr( array_start(&My_Connections[Idx].rbuf), '\r' );
			ptr2 = strchr( array_start(&My_Connections[Idx].rbuf), '\n' );
			delta = 1;
			if( ptr1 && ptr2 ) ptr = ptr1 > ptr2 ? ptr2 : ptr1;
			else if( ptr1 ) ptr = ptr1;
			else if( ptr2 ) ptr = ptr2;
		}
#endif

		if( ! ptr )
			break;

		/* End of request found */
		*ptr = '\0';

		len = ( ptr - (char*) array_start(&My_Connections[Idx].rbuf)) + delta;

		if( len > ( COMMAND_LEN - 1 )) {
			/* Request must not exceed 512 chars (incl. CR+LF!), see
			 * RFC 2812. Disconnect Client if this happens. */
			Log( LOG_ERR, "Request too long (connection %d): %d bytes (max. %d expected)!",
						Idx, array_bytes(&My_Connections[Idx].rbuf), COMMAND_LEN - 1 );
			Conn_Close( Idx, NULL, "Request too long", true );
			return false;
		}

		if (len <= 2) { /* request was empty (only '\r\n') */
			array_moveleft(&My_Connections[Idx].rbuf, 1, delta); /* delta is either 1 or 2 */
			break;
		}
#ifdef ZLIB
		/* remember if stream is already compressed */
		old_z = My_Connections[Idx].options & CONN_ZIP;
#endif

		My_Connections[Idx].msg_in++;
		if (!Parse_Request(Idx, (char*)array_start(&My_Connections[Idx].rbuf) ))
			return false;

		result = true;

		array_moveleft(&My_Connections[Idx].rbuf, 1, len);
		LogDebug("Connection %d: %d bytes left in read buffer.",
		    Idx, array_bytes(&My_Connections[Idx].rbuf));
#ifdef ZLIB
		if(( ! old_z ) && ( My_Connections[Idx].options & CONN_ZIP ) &&
				( array_bytes(&My_Connections[Idx].rbuf) > 0 ))
		{
			/* The last Command activated Socket-Compression.
			 * Data that was read after that needs to be copied to Unzip-buf
			 * for decompression */
			if (!array_copy( &My_Connections[Idx].zip.rbuf, &My_Connections[Idx].rbuf ))
				return false;

			array_trunc(&My_Connections[Idx].rbuf);
			LogDebug("Moved already received data (%u bytes) to uncompression buffer.",
								array_bytes(&My_Connections[Idx].zip.rbuf));
		}
#endif /* ZLIB */
	}
	return result;
} /* Handle_Buffer */


static void
Check_Connections(void)
{
	/* check if connections are alive. if not, play PING-PONG first.
	 * if this doesn't help either, disconnect client. */
	CLIENT *c;
	CONN_ID i;

	for (i = 0; i < Pool_Size; i++) {
		if (My_Connections[i].sock < 0)
			continue;

		c = Conn_GetClient(i);
		if (c && ((Client_Type(c) == CLIENT_USER)
			  || (Client_Type(c) == CLIENT_SERVER)
			  || (Client_Type(c) == CLIENT_SERVICE))) {
			/* connected User, Server or Service */
			if (My_Connections[i].lastping >
			    My_Connections[i].lastdata) {
				/* We already sent a ping */
				if (My_Connections[i].lastping <
				    time(NULL) - Conf_PongTimeout) {
					/* Timeout */
					LogDebug
					    ("Connection %d: Ping timeout: %d seconds.",
					     i, Conf_PongTimeout);
					Conn_Close(i, NULL, "Ping timeout",
						   true);
				}
			} else if (My_Connections[i].lastdata <
				   time(NULL) - Conf_PingTimeout) {
				/* We need to send a PING ... */
				LogDebug("Connection %d: sending PING ...", i);
				My_Connections[i].lastping = time(NULL);
				Conn_WriteStr(i, "PING :%s",
					      Client_ID(Client_ThisServer()));
			}
		} else {
			/* The connection is not fully established yet, so
			 * we don't do the PING-PONG game here but instead
			 * disconnect the client after "a short time" if it's
			 * still not registered. */

			if (My_Connections[i].lastdata <
			    time(NULL) - Conf_PongTimeout) {
				LogDebug
				    ("Unregistered connection %d timed out ...",
				     i);
				Conn_Close(i, NULL, "Timeout", false);
			}
		}
	}
} /* Check_Connections */


static void
Check_Servers( void )
{
	/* Check if we can establish further server links */

	int i, n;
	time_t time_now;

	/* Check all configured servers */
	for( i = 0; i < MAX_SERVERS; i++ ) {
		/* Valid outgoing server which isn't already connected or disabled? */
		if(( ! Conf_Server[i].host[0] ) || ( ! Conf_Server[i].port > 0 ) ||
			( Conf_Server[i].conn_id > NONE ) || ( Conf_Server[i].flags & CONF_SFLAG_DISABLED ))
				continue;

		/* Is there already a connection in this group? */
		if( Conf_Server[i].group > NONE ) {
			for (n = 0; n < MAX_SERVERS; n++) {
				if (n == i) continue;
				if ((Conf_Server[n].conn_id != NONE) &&
					(Conf_Server[n].group == Conf_Server[i].group))
						break;
			}
			if (n < MAX_SERVERS) continue;
		}

		/* Check last connect attempt? */
		time_now = time(NULL);
		if( Conf_Server[i].lasttry > (time_now - Conf_ConnectRetry))
			continue;

		/* Okay, try to connect now */
		Conf_Server[i].lasttry = time_now;
		Conf_Server[i].conn_id = SERVER_WAIT;
		assert(Resolve_Getfd(&Conf_Server[i].res_stat) < 0);
		Resolve_Name(&Conf_Server[i].res_stat, Conf_Server[i].host, cb_Connect_to_Server);
	}
} /* Check_Servers */


static void
New_Server( int Server , struct in_addr *dest)
{
	/* Establish new server link */
	struct sockaddr_in local_addr;
	struct sockaddr_in new_addr;
	int res, new_sock;
	CLIENT *c;

	assert( Server > NONE );

	memset(&new_addr, 0, sizeof( new_addr ));
	new_addr.sin_family = AF_INET;
	new_addr.sin_addr = *dest;
	new_addr.sin_port = htons( Conf_Server[Server].port );

	new_sock = socket( PF_INET, SOCK_STREAM, 0 );
	if ( new_sock < 0 ) {
		Log( LOG_CRIT, "Can't create socket: %s!", strerror( errno ));
		return;
	}

	if( ! Init_Socket( new_sock )) return;

	/* if we fail to bind, just continue and let connect() pick a source address */
	InitSinaddr(&local_addr, 0);
	local_addr.sin_addr = Conf_Server[Server].bind_addr;
	if (bind(new_sock, (struct sockaddr *)&local_addr, (socklen_t)sizeof(local_addr)))
		Log(LOG_WARNING, "Can't bind socket to %s!", Conf_ListenAddress, strerror( errno ));

	res = connect(new_sock, (struct sockaddr *)&new_addr,
			(socklen_t)sizeof(new_addr));
	if(( res != 0 ) && ( errno != EINPROGRESS )) {
		Log( LOG_CRIT, "Can't connect socket: %s!", strerror( errno ));
		close( new_sock );
		return;
	}

	if (!array_alloc(&My_ConnArray, sizeof(CONNECTION), (size_t)new_sock)) {
		Log(LOG_ALERT,
		    "Cannot allocate memory for server connection (socket %d)",
		    new_sock);
		close( new_sock );
		return;
	}

	My_Connections = array_start(&My_ConnArray);

	assert(My_Connections[new_sock].sock <= 0);

	Init_Conn_Struct(new_sock);

	c = Client_NewLocal( new_sock, inet_ntoa( new_addr.sin_addr ), CLIENT_UNKNOWNSERVER, false );
	if( ! c ) {
		Log( LOG_ALERT, "Can't establish connection: can't create client structure!" );
		close( new_sock );
		return;
	}

	Client_SetIntroducer( c, c );
	Client_SetToken( c, TOKEN_OUTBOUND );

	/* Register connection */
	Conf_Server[Server].conn_id = new_sock;
	My_Connections[new_sock].sock = new_sock;
	My_Connections[new_sock].addr = new_addr;
	My_Connections[new_sock].client = c;
	strlcpy( My_Connections[new_sock].host, Conf_Server[Server].host,
				sizeof(My_Connections[new_sock].host ));

	/* Register new socket */
	if (!io_event_create( new_sock, IO_WANTWRITE, cb_connserver)) {
		Log( LOG_ALERT, "io_event_create(): could not add fd %d", strerror(errno));
		Conn_Close( new_sock, "io_event_create() failed", NULL, false );
		Init_Conn_Struct( new_sock );
		Conf_Server[Server].conn_id = NONE;
	}

	LogDebug("Registered new connection %d on socket %d.",
				new_sock, My_Connections[new_sock].sock );
	Conn_OPTION_ADD( &My_Connections[new_sock], CONN_ISCONNECTING );
} /* New_Server */


/**
 * Initialize connection structure.
 */
static void
Init_Conn_Struct(CONN_ID Idx)
{
	time_t now = time(NULL);

	memset(&My_Connections[Idx], 0, sizeof(CONNECTION));
	My_Connections[Idx].sock = -1;
	My_Connections[Idx].signon = now;
	My_Connections[Idx].lastdata = now;
	My_Connections[Idx].lastprivmsg = now;
	Resolve_Init(&My_Connections[Idx].res_stat);
} /* Init_Conn_Struct */


static bool
Init_Socket( int Sock )
{
	/* Initialize socket (set options) */

	int value;

	if (!io_setnonblock(Sock)) {
		Log( LOG_CRIT, "Can't enable non-blocking mode for socket: %s!", strerror( errno ));
		close( Sock );
		return false;
	}

	/* Don't block this port after socket shutdown */
	value = 1;
	if( setsockopt( Sock, SOL_SOCKET, SO_REUSEADDR, &value, (socklen_t)sizeof( value )) != 0 )
	{
		Log( LOG_ERR, "Can't set socket option SO_REUSEADDR: %s!", strerror( errno ));
		/* ignore this error */
	}

	/* Set type of service (TOS) */
#if defined(IP_TOS) && defined(IPTOS_LOWDELAY)
	value = IPTOS_LOWDELAY;
	LogDebug("Setting option IP_TOS on socket %d to IPTOS_LOWDELAY (%d).", Sock, value );
	if( setsockopt( Sock, SOL_IP, IP_TOS, &value, (socklen_t)sizeof( value )) != 0 )
	{
		Log( LOG_ERR, "Can't set socket option IP_TOS: %s!", strerror( errno ));
		/* ignore this error */
	}
#endif

	return true;
} /* Init_Socket */



static void
cb_Connect_to_Server(int fd, UNUSED short events)
{
	/* Read result of resolver sub-process from pipe and start connection */
	int i;
	size_t len;
	struct in_addr dest_addr;
	char readbuf[HOST_LEN + 1];

	LogDebug("Resolver: Got forward lookup callback on fd %d, events %d", fd, events);

	for (i=0; i < MAX_SERVERS; i++) {
		  if (Resolve_Getfd(&Conf_Server[i].res_stat) == fd )
			  break;
	}

	if( i >= MAX_SERVERS) {
		/* Ops, no matching server found?! */
		io_close( fd );
		LogDebug("Resolver: Got Forward Lookup callback for unknown server!?");
		return;
	}

	/* Read result from pipe */
	len = Resolve_Read(&Conf_Server[i].res_stat, readbuf, sizeof(readbuf)-1);
	if (len == 0)
		return;

	readbuf[len] = '\0';
	LogDebug("Got result from resolver: \"%s\" (%u bytes read).", readbuf, len);

	if (!ngt_IPStrToBin(readbuf, &dest_addr)) {
		Log(LOG_ERR, "Can't connect to \"%s\": can't convert ip address %s!",
						Conf_Server[i].host, readbuf);
		return;
	}

	Log( LOG_INFO, "Establishing connection to \"%s\", %s, port %d ... ",
			Conf_Server[i].host, readbuf, Conf_Server[i].port );
	/* connect() */
	New_Server(i, &dest_addr);
} /* cb_Read_Forward_Lookup */


static void
cb_Read_Resolver_Result( int r_fd, UNUSED short events )
{
	/* Read result of resolver sub-process from pipe and update the
	 * apropriate connection/client structure(s): hostname and/or
	 * IDENT user name.*/

	CLIENT *c;
	int i;
	size_t len;
	char *identptr;
#ifdef IDENTAUTH
	char readbuf[HOST_LEN + 2 + CLIENT_USER_LEN];
#else
	char readbuf[HOST_LEN + 1];
#endif

	LogDebug("Resolver: Got callback on fd %d, events %d", r_fd, events );

	/* Search associated connection ... */
	for( i = 0; i < Pool_Size; i++ ) {
		if(( My_Connections[i].sock != NONE )
		  && ( Resolve_Getfd(&My_Connections[i].res_stat) == r_fd ))
			break;
	}
	if( i >= Pool_Size ) {
		/* Ops, none found? Probably the connection has already
		 * been closed!? We'll ignore that ... */
		io_close( r_fd );
		LogDebug("Resolver: Got callback for unknown connection!?");
		return;
	}

	/* Read result from pipe */
	len = Resolve_Read(&My_Connections[i].res_stat, readbuf, sizeof readbuf -1);
	if (len == 0)
		return;

	readbuf[len] = '\0';
	identptr = strchr(readbuf, '\n');
	assert(identptr != NULL);
	if (!identptr) {
		Log( LOG_CRIT, "Resolver: Got malformed result!");
		return;
	}

	*identptr = '\0';
	LogDebug("Got result from resolver: \"%s\" (%u bytes read).", readbuf, len);
	/* Okay, we got a complete result: this is a host name for outgoing
	 * connections and a host name and IDENT user name (if enabled) for
	 * incoming connections.*/
	assert ( My_Connections[i].sock >= 0 );
	/* Incoming connection. Search client ... */
	c = Conn_GetClient( i );
	assert( c != NULL );

	/* Only update client information of unregistered clients */
	if( Client_Type( c ) == CLIENT_UNKNOWN ) {
		strlcpy(My_Connections[i].host, readbuf, sizeof( My_Connections[i].host));
		Client_SetHostname( c, readbuf);
#ifdef IDENTAUTH
		++identptr;
		if (*identptr) {
			Log( LOG_INFO, "IDENT lookup for connection %ld: \"%s\".", i, identptr);
			Client_SetUser( c, identptr, true );
		} else {
			Log( LOG_INFO, "IDENT lookup for connection %ld: no result.", i );
		}
#endif
	}
#ifdef DEBUG
		else Log( LOG_DEBUG, "Resolver: discarding result for already registered connection %d.", i );
#endif
	/* Reset penalty time */
	Conn_ResetPenalty( i );
} /* cb_Read_Resolver_Result */


static void
Simple_Message( int Sock, const char *Msg )
{
	char buf[COMMAND_LEN];
	size_t len;
	/* Write "simple" message to socket, without using compression
	 * or even the connection write buffers. Used e.g. for error
	 * messages by New_Connection(). */
	assert( Sock > NONE );
	assert( Msg != NULL );

	strlcpy( buf, Msg, sizeof buf - 2);
	len = strlcat( buf, "\r\n", sizeof buf);
	(void)write(Sock, buf, len);
} /* Simple_Error */


static int
Count_Connections( struct sockaddr_in addr_in )
{
	int i, cnt;
	
	cnt = 0;
	for( i = 0; i < Pool_Size; i++ ) {
		if(( My_Connections[i].sock > NONE ) && ( My_Connections[i].addr.sin_addr.s_addr == addr_in.sin_addr.s_addr )) cnt++;
	}
	return cnt;
} /* Count_Connections */


GLOBAL CLIENT *
Conn_GetClient( CONN_ID Idx ) 
{
	/* return Client-Structure that belongs to the local Connection Idx.
	 * If none is found, return NULL.
	 */
	CONNECTION *c;
	assert( Idx >= 0 );

	c = array_get(&My_ConnArray, sizeof (CONNECTION), (size_t)Idx);

	assert(c != NULL);

	return c ? c->client : NULL;
}

/* -eof- */
