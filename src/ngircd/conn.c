/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton <alex@barton.de>
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
#include "conf-ssl.h"
#include "io.h"

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
# ifdef HAVE_NETINET_IN_SYSTM_H
#  include <netinet/in_systm.h>
# endif
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
#include "conn-ssl.h"
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

#define MAX_COMMANDS 3
#define MAX_COMMANDS_SERVER 10


static bool Handle_Write PARAMS(( CONN_ID Idx ));
static bool Conn_Write PARAMS(( CONN_ID Idx, char *Data, size_t Len ));
static int New_Connection PARAMS(( int Sock ));
static CONN_ID Socket2Index PARAMS(( int Sock ));
static void Read_Request PARAMS(( CONN_ID Idx ));
static unsigned int Handle_Buffer PARAMS(( CONN_ID Idx ));
static void Check_Connections PARAMS(( void ));
static void Check_Servers PARAMS(( void ));
static void Init_Conn_Struct PARAMS(( CONN_ID Idx ));
static bool Init_Socket PARAMS(( int Sock ));
static void New_Server PARAMS(( int Server, ng_ipaddr_t *dest ));
static void Simple_Message PARAMS(( int Sock, const char *Msg ));
static int NewListener PARAMS(( const char *listen_addr, UINT16 Port ));
static void Account_Connection PARAMS((void));


static array My_Listeners;
static array My_ConnArray;
static size_t NumConnections, NumConnectionsMax, NumConnectionsAccepted;

#ifdef TCPWRAP
int allow_severity = LOG_INFO;
int deny_severity = LOG_ERR;
#endif

static void server_login PARAMS((CONN_ID idx));

#ifdef SSL_SUPPORT
extern struct SSLOptions Conf_SSLOptions;
static void cb_connserver_login_ssl PARAMS((int sock, short what));
static void cb_clientserver_ssl PARAMS((int sock, short what));
#endif
static void cb_Read_Resolver_Result PARAMS((int sock, UNUSED short what));
static void cb_Connect_to_Server PARAMS((int sock, UNUSED short what));
static void cb_clientserver PARAMS((int sock, short what));


/**
 * IO callback for listening sockets: handle new connections. This callback
 * gets called when a new non-SSL connection should be accepted.
 * @param sock Socket descriptor
 * @param irrelevant (ignored IO specification)
 */
static void
cb_listen(int sock, short irrelevant)
{
	(void) irrelevant;
	(void) New_Connection(sock);
}


#ifdef SSL_SUPPORT
/**
 * IO callback for listening SSL sockets: handle new connections. This callback
 * gets called when a new SSL-enabled connection should be accepted.
 * @param sock Socket descriptor
 * @param irrelevant (ignored IO specification)
 */
static void
cb_listen_ssl(int sock, short irrelevant)
{
	int fd;

	(void) irrelevant;
	fd = New_Connection(sock);
	if (fd < 0)
		return;
	io_event_setcb(My_Connections[fd].sock, cb_clientserver_ssl);
}
#endif


/**
 * IO callback for new outgoing non-SSL server connections.
 * @param sock Socket descriptor
 * @param what IO specification (IO_WANTREAD/IO_WANTWRITE/...)
 */
static void
cb_connserver(int sock, UNUSED short what)
{
	int res, err, server;
	socklen_t sock_len;
	CONN_ID idx = Socket2Index( sock );

	if (idx <= NONE) {
		LogDebug("cb_connserver wants to write on unknown socket?!");
		io_close(sock);
		return;
	}

	assert(what & IO_WANTWRITE);

	/* Make sure that the server is still configured; it could have been
	 * removed in the meantime! */
	server = Conf_GetServer(idx);
	if (server < 0) {
		Log(LOG_ERR, "Connection on socket %d to \"%s\" aborted!",
		    sock, My_Connections[idx].host);
		Conn_Close(idx, "Connection aborted!", NULL, false);
		return;
	}

	/* connect() finished, get result. */
	sock_len = (socklen_t)sizeof(err);
	res = getsockopt(My_Connections[idx].sock, SOL_SOCKET, SO_ERROR,
			 &err, &sock_len );
	assert(sock_len == sizeof(err));

	/* Error while connecting? */
 	if ((res != 0) || (err != 0)) {
 		if (res != 0)
 			Log(LOG_CRIT, "getsockopt (connection %d): %s!",
 			    idx, strerror(errno));
 		else
 			Log(LOG_CRIT,
 			    "Can't connect socket to \"%s:%d\" (connection %d): %s!",
			    My_Connections[idx].host, Conf_Server[server].port,
 			    idx, strerror(err));

		Conn_Close(idx, "Can't connect!", NULL, false);

		if (ng_ipaddr_af(&Conf_Server[server].dst_addr[0])) {
			/* more addresses to try... */
			New_Server(res, &Conf_Server[server].dst_addr[0]);
			/* connection to dst_addr[0] is now in progress, so
			 * remove this address... */
			Conf_Server[server].dst_addr[0] =
				Conf_Server[server].dst_addr[1];
			memset(&Conf_Server[server].dst_addr[1], 0,
			       sizeof(Conf_Server[server].dst_addr[1]));
		}
		return;
	}

	/* connect() succeeded, remove all additional addresses */
	memset(&Conf_Server[server].dst_addr, 0,
	       sizeof(Conf_Server[server].dst_addr));

	Conn_OPTION_DEL( &My_Connections[idx], CONN_ISCONNECTING );
#ifdef SSL_SUPPORT
	if ( Conn_OPTION_ISSET( &My_Connections[idx], CONN_SSL_CONNECT )) {
		io_event_setcb( sock, cb_connserver_login_ssl );
		io_event_add( sock, IO_WANTWRITE|IO_WANTREAD );
		return;
	}
#endif
	server_login(idx);
}


/**
 * Login to a remote server.
 * @param idx Connection index
 */
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


#ifdef SSL_SUPPORT
/**
 * IO callback for new outgoing SSL-enabled server connections.
 * @param sock Socket descriptor
 * @param what IO specification (IO_WANTREAD/IO_WANTWRITE/...)
 */
static void
cb_connserver_login_ssl(int sock, short unused)
{
	CONN_ID idx = Socket2Index(sock);

	assert(idx >= 0);
	if (idx < 0) {
		io_close(sock);
		return;
	}
	(void) unused;
	switch (ConnSSL_Connect( &My_Connections[idx])) {
	case 1: break;
	case 0: LogDebug("ConnSSL_Connect: not ready");
		return;
	case -1:
		Log(LOG_ERR, "SSL connection on socket %d failed!", sock);
		Conn_Close(idx, "Can't connect!", NULL, false);
		return;
	}

	Log( LOG_INFO, "SSL connection %d with \"%s:%d\" established.", idx,
			My_Connections[idx].host, Conf_Server[Conf_GetServer( idx )].port );

	server_login(idx);
}
#endif


/**
 * IO callback for established non-SSL client and server connections.
 * @param sock Socket descriptor
 * @param what IO specification (IO_WANTREAD/IO_WANTWRITE/...)
 */
static void
cb_clientserver(int sock, short what)
{
	CONN_ID idx = Socket2Index(sock);

	assert(idx >= 0);

	if (idx < 0) {
		io_close(sock);
		return;
	}
#ifdef SSL_SUPPORT
	if (what & IO_WANTREAD
	    || (Conn_OPTION_ISSET(&My_Connections[idx], CONN_SSL_WANT_WRITE))) {
		/* if TLS layer needs to write additional data, call
		 * Read_Request() instead so that SSL/TLS can continue */
		Read_Request(idx);
	}
#else
	if (what & IO_WANTREAD)
		Read_Request(idx);
#endif
	if (what & IO_WANTWRITE)
		Handle_Write(idx);
}


#ifdef SSL_SUPPORT
/**
 * IO callback for established SSL-enabled client and server connections.
 * @param sock Socket descriptor
 * @param what IO specification (IO_WANTREAD/IO_WANTWRITE/...)
 */
static void
cb_clientserver_ssl(int sock, short what)
{
	CONN_ID idx = Socket2Index(sock);

	assert(idx >= 0);

	if (idx < 0) {
		io_close(sock);
		return;
	}

	switch (ConnSSL_Accept(&My_Connections[idx])) {
	case 1:
		break;	/* OK */
	case 0:
		return;	/* EAGAIN: callback will be invoked again by IO layer */
	default:
		Conn_Close(idx, "Socket closed!", "SSL accept error", false);
		return;
	}
	if (what & IO_WANTREAD)
		Read_Request(idx);

	if (what & IO_WANTWRITE)
		Handle_Write(idx);

	io_event_setcb(sock, cb_clientserver);	/* SSL handshake completed */
}
#endif


/**
 * Initialite connecion module.
 */
GLOBAL void
Conn_Init( void )
{
	CONN_ID i;

	/* Speicher fuer Verbindungs-Pool anfordern */
	Pool_Size = CONNECTION_POOL;
	if ((Conf_MaxConnections > 0) &&
		(Pool_Size > Conf_MaxConnections))
			Pool_Size = Conf_MaxConnections;

	if (!array_alloc(&My_ConnArray, sizeof(CONNECTION), (size_t)Pool_Size)) {
		Log(LOG_EMERG, "Can't allocate memory! [Conn_Init]");
		exit(1);
	}

	/* FIXME: My_Connetions/Pool_Size is needed by other parts of the
	 * code; remove them! */
	My_Connections = (CONNECTION*) array_start(&My_ConnArray);

	LogDebug("Allocated connection pool for %d items (%ld bytes).",
		array_length(&My_ConnArray, sizeof(CONNECTION)),
		array_bytes(&My_ConnArray));

	assert(array_length(&My_ConnArray, sizeof(CONNECTION)) >= (size_t)Pool_Size);
	
	array_free( &My_Listeners );

	for (i = 0; i < Pool_Size; i++)
		Init_Conn_Struct(i);
} /* Conn_Init */


/**
 * Clean up connection module.
 */
GLOBAL void
Conn_Exit( void )
{
	CONN_ID idx;

	Conn_ExitListeners();

	LogDebug("Shutting down all connections ..." );
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
ports_initlisteners(array *a, const char *listen_addr, void (*func)(int,short))
{
	unsigned int created = 0;
	size_t len;
	int fd;
	UINT16 *port;

	len = array_length(a, sizeof (UINT16));
	port = array_start(a);
	while (len--) {
		fd = NewListener(listen_addr, *port);
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


/**
 * Initialize all listening sockets.
 * @return Number of created listening sockets
 */
GLOBAL unsigned int
Conn_InitListeners( void )
{
	/* Initialize ports on which the server should accept connections */
	unsigned int created = 0;
	char *copy, *listen_addr;

	if (!io_library_init(CONNECTION_POOL)) {
		Log(LOG_EMERG, "Cannot initialize IO routines: %s", strerror(errno));
		return -1;
	}

	assert(Conf_ListenAddress);

	/* can't use Conf_ListenAddress directly, see below */
	copy = strdup(Conf_ListenAddress);
	if (!copy) {
		Log(LOG_CRIT, "Cannot copy %s: %s", Conf_ListenAddress, strerror(errno));
		return 0;
	}
	listen_addr = strtok(copy, ",");

	while (listen_addr) {
		ngt_TrimStr(listen_addr);
		if (*listen_addr) {
			created += ports_initlisteners(&Conf_ListenPorts, listen_addr, cb_listen);
#ifdef SSL_SUPPORT
			created += ports_initlisteners(&Conf_SSLOptions.ListenPorts, listen_addr, cb_listen_ssl);
#endif
		}

		listen_addr = strtok(NULL, ",");
	}

	/* Can't free() Conf_ListenAddress here: on REHASH, if the config file
	 * cannot be re-loaded, we'd end up with a NULL Conf_ListenAddress.
	 * Instead, free() takes place in conf.c, before the config file
	 * is being parsed. */
	free(copy);

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
	Log(LOG_INFO,
	    "Shutting down all listening sockets (%d total) ...", arraylen);
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


static bool
InitSinaddrListenAddr(ng_ipaddr_t *addr, const char *listen_addrstr, UINT16 Port)
{
	bool ret;

	ret = ng_ipaddr_init(addr, listen_addrstr, Port);
	if (!ret) {
		assert(listen_addrstr);
		Log(LOG_CRIT, "Can't bind to [%s]:%u: can't convert ip address \"%s\"",
						listen_addrstr, Port, listen_addrstr);
	}
	return ret;
}


static void
set_v6_only(int af, int sock)
{
#if defined(IPV6_V6ONLY) && defined(WANT_IPV6)
	int on = 1;

	if (af != AF_INET6)
		return;

	if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &on, (socklen_t)sizeof(on)))
		Log(LOG_ERR, "Could not set IPV6_V6ONLY: %s", strerror(errno));
#else
	(void)af;
	(void)sock;
#endif
}


/* return new listening port file descriptor or -1 on failure */
static int
NewListener(const char *listen_addr, UINT16 Port)
{
	/* Create new listening socket on specified port */
	ng_ipaddr_t addr;
	int sock, af;
#ifdef ZEROCONF
	char name[CLIENT_ID_LEN], *info;
#endif
	if (!InitSinaddrListenAddr(&addr, listen_addr, Port))
		return -1;

	af = ng_ipaddr_af(&addr);
	sock = socket(af, SOCK_STREAM, 0);
	if( sock < 0 ) {
		Log(LOG_CRIT, "Can't create socket (af %d) : %s!", af, strerror(errno));
		return -1;
	}

	set_v6_only(af, sock);

	if (!Init_Socket(sock))
		return -1;

	if (bind(sock, (struct sockaddr *)&addr, ng_ipaddr_salen(&addr)) != 0) {
		Log(LOG_CRIT, "Can't bind socket to address %s:%d - %s",
			ng_ipaddr_tostr(&addr), Port, strerror(errno));
		close(sock);
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

	Log(LOG_INFO, "Now listening on [%s]:%d (socket %d).", ng_ipaddr_tostr(&addr), Port, sock);

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


#ifdef SSL_SUPPORT
/*
 * SSL/TLS connections require extra treatment:
 * When either CONN_SSL_WANT_WRITE or CONN_SSL_WANT_READ is set, we
 * need to take care of that first, before checking read/write buffers.
 * For instance, while we might have data in our write buffer, the
 * TLS/SSL protocol might need to read internal data first for TLS/SSL
 * writes to succeed.
 *
 * If this function returns true, such a condition is met and we have
 * to reverse the condition (check for read even if we've data to write,
 * do not check for read but writeability even if write-buffer is empty).
 */
static bool
SSL_WantRead(const CONNECTION *c)
{
	if (Conn_OPTION_ISSET(c, CONN_SSL_WANT_READ)) {
		io_event_add(c->sock, IO_WANTREAD);
		return true;
	}
	return false;
}
static bool
SSL_WantWrite(const CONNECTION *c)
{
	if (Conn_OPTION_ISSET(c, CONN_SSL_WANT_WRITE)) {
		io_event_add(c->sock, IO_WANTWRITE);
		return true;
	}
	return false;
}
#else
static inline bool
SSL_WantRead(UNUSED const CONNECTION *c) { return false; }
static inline bool
SSL_WantWrite(UNUSED const CONNECTION *c) { return false; }
#endif


/**
 * "Main Loop": Loop until shutdown or restart is signalled.
 * This function loops until a shutdown or restart of ngIRCd is signalled and
 * calls io_dispatch() to check for readable and writable sockets every second.
 * It checks for status changes on pending connections (e. g. when a hostname
 * has been resolved), checks for "penalties" and timeouts, and handles the
 * input buffers.
 */
GLOBAL void
Conn_Handler(void)
{
	int i;
	unsigned int wdatalen, bytes_processed;
	struct timeval tv;
	time_t t;

	while (!NGIRCd_SignalQuit && !NGIRCd_SignalRestart) {
		t = time(NULL);

#ifdef ZEROCONF
		Rendezvous_Handler();
#endif

		/* Should the configuration be reloaded? */
		if (NGIRCd_SignalRehash)
			NGIRCd_Rehash();

		/* Check configured servers and established links */
		Check_Servers();
		Check_Connections();

		/* Look for non-empty read buffers ... */
		for (i = 0; i < Pool_Size; i++) {
			if ((My_Connections[i].sock > NONE)
			    && (array_bytes(&My_Connections[i].rbuf) > 0)
			    && (My_Connections[i].delaytime <= t)) {
				/* ... and try to handle the received data */
				bytes_processed = Handle_Buffer(i);
				/* if we processed data, and there might be
				 * more commands in the input buffer, do not
				 * try to read any more data now */
				if (bytes_processed &&
				    array_bytes(&My_Connections[i].rbuf) > 2) {
					LogDebug
					    ("Throttling connection %d: command limit reached!",
					     i);
					Conn_SetPenalty(i, 1);
				}
			}
		}

		/* Look for non-empty write buffers ... */
		for (i = 0; i < Pool_Size; i++) {
			if (My_Connections[i].sock <= NONE)
				continue;

			wdatalen = (unsigned int)array_bytes(&My_Connections[i].wbuf);
#ifdef ZLIB
			if (wdatalen > 0 ||
			    array_bytes(&My_Connections[i].zip.wbuf) > 0)
#else
			if (wdatalen > 0)
#endif
			{
				if (SSL_WantRead(&My_Connections[i]))
					continue;
				io_event_add(My_Connections[i].sock,
					     IO_WANTWRITE);
			}
		}

		/* Check from which sockets we possibly could read ... */
		for (i = 0; i < Pool_Size; i++) {
			if (My_Connections[i].sock <= NONE)
				continue;
#ifdef SSL_SUPPORT
			if (SSL_WantWrite(&My_Connections[i]))
				continue; /* TLS/SSL layer needs to write data; deal with this first */
#endif
			if (Resolve_INPROGRESS(&My_Connections[i].res_stat)) {
				/* Wait for completion of resolver sub-process ... */
				io_event_del(My_Connections[i].sock,
					     IO_WANTREAD);
				continue;
			}

			if (Conn_OPTION_ISSET(&My_Connections[i], CONN_ISCONNECTING))
				/* Wait for completion of connect() ... */
				continue;

			if (My_Connections[i].delaytime > t) {
				/* There is a "penalty time" set: ignore socket! */
				io_event_del(My_Connections[i].sock,
					     IO_WANTREAD);
				continue;
			}
			io_event_add(My_Connections[i].sock, IO_WANTREAD);
		}

		/* Set the timeout for reading from the network to 1 second,
		 * which is the granularity with witch we handle "penalty
		 * times" for example.
		 * Note: tv_sec/usec are undefined(!) after io_dispatch()
		 * returns, so we have to set it beforce each call to it! */
		tv.tv_usec = 0;
		tv.tv_sec = 1;

		/* Wait for activity ... */
		i = io_dispatch(&tv);
		if (i == -1 && errno != EINTR) {
			Log(LOG_EMERG, "Conn_Handler(): io_dispatch(): %s!",
			    strerror(errno));
			Log(LOG_ALERT, "%s exiting due to fatal errors!",
			    PACKAGE_NAME);
			exit(1);
		}
	}

	if (NGIRCd_SignalQuit)
		Log(LOG_NOTICE | LOG_snotice, "Server going down NOW!");
	else if (NGIRCd_SignalRestart)
		Log(LOG_NOTICE | LOG_snotice, "Server restarting NOW!");
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
Conn_Close( CONN_ID Idx, const char *LogMsg, const char *FwdMsg, bool InformClient )
{
	/* Close connection. Open pipes of asyncronous resolver
	 * sub-processes are closed down. */

	CLIENT *c;
	double in_k, out_k;
	UINT16 port;
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

	port = ng_ipaddr_getport(&My_Connections[Idx].addr);
	Log(LOG_INFO, "Shutting down connection %d (%s) with %s:%d ...", Idx,
	    LogMsg ? LogMsg : FwdMsg, My_Connections[Idx].host, port);

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
		/* Send ERROR to client (see RFC 2812, section 3.1.7) */
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
#ifdef SSL_SUPPORT
	if ( Conn_OPTION_ISSET( &My_Connections[Idx], CONN_SSL )) {
		Log(LOG_INFO, "SSL connection %d shutting down ...", Idx);
		ConnSSL_Free(&My_Connections[Idx]);
	}
#endif
	/* Shut down socket */
	if (! io_close(My_Connections[Idx].sock)) {
		/* Oops, we can't close the socket!? This is ... ugly! */
		Log(LOG_CRIT,
		    "Error closing connection %d (socket %d) with %s:%d - %s! (ignored)",
		    Idx, My_Connections[Idx].sock, My_Connections[Idx].host,
		    port, strerror(errno));
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
		    Idx, My_Connections[Idx].host, port,
		    in_k, in_z_k, in_p, out_k, out_z_k, out_p);
	}
	else
#endif
	{
		Log(LOG_INFO,
		    "Connection %d with %s:%d closed (in: %.1fk, out: %.1fk).",
		    Idx, My_Connections[Idx].host, port,
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

	assert(NumConnections > 0);
	if (NumConnections)
		NumConnections--;
	LogDebug("Shutdown of connection %d completed, %ld connection%s left.",
		 Idx, NumConnections, NumConnections != 1 ? "s" : "");
} /* Conn_Close */


GLOBAL long
Conn_Count(void)
{
	return NumConnections;
} /* Conn_Count */


GLOBAL long
Conn_CountMax(void)
{
	return NumConnectionsMax;
} /* Conn_CountMax */


GLOBAL long
Conn_CountAccepted(void)
{
	return NumConnectionsAccepted;
} /* Conn_CountAccepted */


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

#ifdef SSL_SUPPORT
	if ( Conn_OPTION_ISSET( &My_Connections[Idx], CONN_SSL )) {
		len = ConnSSL_Write(&My_Connections[Idx], array_start(&My_Connections[Idx].wbuf), wdatalen);
	} else
#endif
	{
		len = write(My_Connections[Idx].sock,
			    array_start(&My_Connections[Idx].wbuf), wdatalen );
	}
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
Count_Connections(ng_ipaddr_t *a)
{
	int i, cnt;

	cnt = 0;
	for (i = 0; i < Pool_Size; i++) {
		if (My_Connections[i].sock <= NONE)
			continue;
		if (ng_ipaddr_ipequal(&My_Connections[i].addr, a))
			cnt++;
	}
	return cnt;
} /* Count_Connections */


/**
 * Initialize new client connection on a listening socket.
 * @param Sock Listening socket descriptor
 * @return Accepted socket descriptor or -1 on error
 */
static int
New_Connection(int Sock)
{
#ifdef TCPWRAP
	struct request_info req;
#endif
	ng_ipaddr_t new_addr;
	char ip_str[NG_INET_ADDRSTRLEN];
	int new_sock, new_sock_len, identsock;
	CLIENT *c;
	long cnt;

	assert(Sock > NONE);

	new_sock_len = (int)sizeof(new_addr);
	new_sock = accept(Sock, (struct sockaddr *)&new_addr,
			  (socklen_t *)&new_sock_len);
	if (new_sock < 0) {
		Log(LOG_CRIT, "Can't accept connection: %s!", strerror(errno));
		return -1;
	}
	NumConnectionsAccepted++;

	if (!ng_ipaddr_tostr_r(&new_addr, ip_str)) {
		Log(LOG_CRIT, "fd %d: Can't convert IP address!", new_sock);
		Simple_Message(new_sock, "ERROR :Internal Server Error");
		close(new_sock);
		return -1;
	}

#ifdef TCPWRAP
	/* Validate socket using TCP Wrappers */
	request_init(&req, RQ_DAEMON, PACKAGE_NAME, RQ_FILE, new_sock,
		     RQ_CLIENT_SIN, &new_addr, NULL);
	fromhost(&req);
	if (!hosts_access(&req)) {
		Log(deny_severity,
		    "Refused connection from %s (by TCP Wrappers)!", ip_str);
		Simple_Message(new_sock, "ERROR :Connection refused");
		close(new_sock);
		return -1;
	}
#endif

	if (!Init_Socket(new_sock))
		return -1;

	/* Check global connection limit */
	if ((Conf_MaxConnections > 0) &&
	    (NumConnections >= (size_t) Conf_MaxConnections)) {
		Log(LOG_ALERT, "Can't accept connection: limit (%d) reached!",
		    Conf_MaxConnections);
		Simple_Message(new_sock, "ERROR :Connection limit reached");
		close(new_sock);
		return -1;
	}

	/* Check IP-based connection limit */
	cnt = Count_Connections(&new_addr);
	if ((Conf_MaxConnectionsIP > 0) && (cnt >= Conf_MaxConnectionsIP)) {
		/* Access denied, too many connections from this IP address! */
		Log(LOG_ERR,
		    "Refused connection from %s: too may connections (%ld) from this IP address!",
		    ip_str, cnt);
		Simple_Message(new_sock,
			       "ERROR :Connection refused, too many connections from your IP address!");
		close(new_sock);
		return -1;
	}

	if (new_sock >= Pool_Size) {
		if (!array_alloc(&My_ConnArray, sizeof(CONNECTION),
				 (size_t) new_sock)) {
			Log(LOG_EMERG,
			    "Can't allocate memory! [New_Connection]");
			Simple_Message(new_sock, "ERROR: Internal error");
			close(new_sock);
			return -1;
		}
		LogDebug("Bumped connection pool to %ld items (internal: %ld items, %ld bytes)",
			 new_sock, array_length(&My_ConnArray,
			 sizeof(CONNECTION)), array_bytes(&My_ConnArray));

		/* Adjust pointer to new block */
		My_Connections = array_start(&My_ConnArray);
		while (Pool_Size <= new_sock)
			Init_Conn_Struct(Pool_Size++);
	}

	/* register callback */
	if (!io_event_create(new_sock, IO_WANTREAD, cb_clientserver)) {
		Log(LOG_ALERT,
		    "Can't accept connection: io_event_create failed!");
		Simple_Message(new_sock, "ERROR :Internal error");
		close(new_sock);
		return -1;
	}

	c = Client_NewLocal(new_sock, ip_str, CLIENT_UNKNOWN, false);
	if (!c) {
		Log(LOG_ALERT,
		    "Can't accept connection: can't create client structure!");
		Simple_Message(new_sock, "ERROR :Internal error");
		io_close(new_sock);
		return -1;
	}

	Init_Conn_Struct(new_sock);
	My_Connections[new_sock].sock = new_sock;
	My_Connections[new_sock].addr = new_addr;
	My_Connections[new_sock].client = c;

	/* Set initial hostname to IP address. This becomes overwritten when
	 * the DNS lookup is enabled and succeeds, but is used otherwise. */
	if (ng_ipaddr_af(&new_addr) != AF_INET)
		snprintf(My_Connections[new_sock].host,
			 sizeof(My_Connections[new_sock].host), "[%s]", ip_str);
	else
		strlcpy(My_Connections[new_sock].host, ip_str,
			sizeof(My_Connections[new_sock].host));

	Client_SetHostname(c, My_Connections[new_sock].host);

	Log(LOG_INFO, "Accepted connection %d from %s:%d on socket %d.",
	    new_sock, My_Connections[new_sock].host,
	    ng_ipaddr_getport(&new_addr), Sock);

	identsock = new_sock;
#ifdef IDENTAUTH
	if (Conf_NoIdent)
		identsock = -1;
#endif
	if (!Conf_NoDNS)
		Resolve_Addr(&My_Connections[new_sock].res_stat, &new_addr,
			     identsock, cb_Read_Resolver_Result);

	/* ngIRCd waits up to 4 seconds for the result of the asynchronous
	 * DNS and IDENT resolver subprocess using the "penalty" mechanism.
	 * If there are results earlier, the delay is aborted. */
	Conn_SetPenalty(new_sock, 4);

	Account_Connection();
	return new_sock;
} /* New_Connection */


static void
Account_Connection(void)
{
	NumConnections++;
	if (NumConnections > NumConnectionsMax)
		NumConnectionsMax = NumConnections;
	LogDebug("Total number of connections now %lu (max %lu).",
		 NumConnections, NumConnectionsMax);
} /* Account_Connection */


static CONN_ID
Socket2Index( int Sock )
{
	assert( Sock >= 0 );

	if( Sock >= Pool_Size || My_Connections[Sock].sock != Sock ) {
		/* the Connection was already closed again, likely due to
		 * an error. */
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
	static const unsigned int maxbps = COMMAND_LEN / 2;
	char readbuf[READBUFFER_LEN];
	time_t t;
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

#ifdef SSL_SUPPORT
	if (Conn_OPTION_ISSET(&My_Connections[Idx], CONN_SSL))
		len = ConnSSL_Read( &My_Connections[Idx], readbuf, sizeof(readbuf));
	else
#endif
	len = read(My_Connections[Idx].sock, readbuf, sizeof(readbuf));
	if (len == 0) {
		Log(LOG_INFO, "%s:%u (%s) is closing the connection ...",
				My_Connections[Idx].host,
				(unsigned int) ng_ipaddr_getport(&My_Connections[Idx].addr),
				ng_ipaddr_tostr(&My_Connections[Idx].addr));
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
		t = time(NULL);
		if (My_Connections[Idx].lastdata != t)
			My_Connections[Idx].bps = 0;

		My_Connections[Idx].lastdata = t;
		My_Connections[Idx].lastping = My_Connections[Idx].lastdata;
	}

	/* Look at the data in the (read-) buffer of this connection */
	My_Connections[Idx].bps += Handle_Buffer(Idx);
	if (Client_Type(c) != CLIENT_SERVER
	    && My_Connections[Idx].bps >= maxbps) {
		LogDebug("Throttling connection %d: BPS exceeded! (%u >= %u)",
			 Idx, My_Connections[Idx].bps, maxbps);
		Conn_SetPenalty(Idx, 1);
	}
} /* Read_Request */


/**
 * Handle all data in the connection read-buffer.
 * Data is processed until no complete command is left in the read buffer,
 * or MAX_COMMANDS[_SERVER] commands were processed.
 * When a fatal error occurs, the connection is shut down.
 * @param Idx Index of the connection.
 * @return number of bytes processed.
 */
static unsigned int
Handle_Buffer(CONN_ID Idx)
{
#ifndef STRICT_RFC
	char *ptr1, *ptr2, *first_eol;
#endif
	char *ptr;
	size_t len, delta;
	time_t starttime;
#ifdef ZLIB
	bool old_z;
#endif
	unsigned int i, maxcmd = MAX_COMMANDS, len_processed = 0;
	CLIENT *c;

	c = Conn_GetClient(Idx);
	assert( c != NULL);

	/* Servers do get special command limits, so they can process
	 * all the messages that are required while peering. */
	if (Client_Type(c) == CLIENT_SERVER)
		maxcmd = MAX_COMMANDS_SERVER;

	starttime = time(NULL);
	for (i=0; i < maxcmd; i++) {
		/* Check penalty */
		if (My_Connections[Idx].delaytime > starttime)
			return 0;
#ifdef ZLIB
		/* Unpack compressed data, if compression is in use */
		if (Conn_OPTION_ISSET(&My_Connections[Idx], CONN_ZIP)) {
			/* When unzipping fails, Unzip_Buffer() shuts
			 * down the connection itself */
			if (!Unzip_Buffer(Idx))
				return 0;
		}
#endif

		if (0 == array_bytes(&My_Connections[Idx].rbuf))
			break;

		/* Make sure that the buffer is NULL terminated */
		if (!array_cat0_temporary(&My_Connections[Idx].rbuf)) {
			Conn_Close(Idx, NULL,
				   "Can't allocate memory [Handle_Buffer]",
				   true);
			return 0;
		}

		/* RFC 2812, section "2.3 Messages", 5th paragraph:
		 * "IRC messages are always lines of characters terminated
		 * with a CR-LF (Carriage Return - Line Feed) pair [...]". */
		delta = 2;
		ptr = strstr(array_start(&My_Connections[Idx].rbuf), "\r\n");

#ifndef STRICT_RFC
		/* Check for non-RFC-compliant request (only CR or LF)?
		 * Unfortunately, there are quite a few clients out there
		 * that do this -- e. g. mIRC, BitchX, and Trillian :-( */
		ptr1 = strchr(array_start(&My_Connections[Idx].rbuf), '\r');
		ptr2 = strchr(array_start(&My_Connections[Idx].rbuf), '\n');
		if (ptr) {
			/* Check if there is a single CR or LF _before_ the
			 * corerct CR+LF line terminator:  */
			first_eol = ptr1 < ptr2 ? ptr1 : ptr2;
			if (first_eol < ptr) {
				/* Single CR or LF before CR+LF found */
				ptr = first_eol;
				delta = 1;
			}
		} else if (ptr1 || ptr2) {
			/* No CR+LF terminated command found, but single
			 * CR or LF found ... */
			if (ptr1 && ptr2)
				ptr = ptr1 < ptr2 ? ptr1 : ptr2;
			else
				ptr = ptr1 ? ptr1 : ptr2;
			delta = 1;
		}
#endif

		if (!ptr)
			break;

		/* Complete (=line terminated) request found, handle it! */
		*ptr = '\0';

		len = ptr - (char *)array_start(&My_Connections[Idx].rbuf) + delta;

		if (len > (COMMAND_LEN - 1)) {
			/* Request must not exceed 512 chars (incl. CR+LF!),
			 * see RFC 2812. Disconnect Client if this happens. */
			Log(LOG_ERR,
			    "Request too long (connection %d): %d bytes (max. %d expected)!",
			    Idx, array_bytes(&My_Connections[Idx].rbuf),
			    COMMAND_LEN - 1);
			Conn_Close(Idx, NULL, "Request too long", true);
			return 0;
		}

		len_processed += (unsigned int)len;
		if (len <= delta) {
			/* Request is empty (only '\r\n', '\r' or '\n');
			 * delta is 2 ('\r\n') or 1 ('\r' or '\n'), see above */
			array_moveleft(&My_Connections[Idx].rbuf, 1, len);
			continue;
		}
#ifdef ZLIB
		/* remember if stream is already compressed */
		old_z = My_Connections[Idx].options & CONN_ZIP;
#endif

		My_Connections[Idx].msg_in++;
		if (!Parse_Request
		    (Idx, (char *)array_start(&My_Connections[Idx].rbuf)))
			return 0; /* error -> connection has been closed */

		array_moveleft(&My_Connections[Idx].rbuf, 1, len);
		LogDebug("Connection %d: %d bytes left in read buffer.",
			 Idx, array_bytes(&My_Connections[Idx].rbuf));
#ifdef ZLIB
		if ((!old_z) && (My_Connections[Idx].options & CONN_ZIP) &&
		    (array_bytes(&My_Connections[Idx].rbuf) > 0)) {
			/* The last command activated socket compression.
			 * Data that was read after that needs to be copied
			 * to the unzip buffer for decompression: */
			if (!array_copy
			    (&My_Connections[Idx].zip.rbuf,
			     &My_Connections[Idx].rbuf)) {
				Conn_Close(Idx, NULL,
					   "Can't allocate memory [Handle_Buffer]",
					   true);
				return 0;
			}

			array_trunc(&My_Connections[Idx].rbuf);
			LogDebug
			    ("Moved already received data (%u bytes) to uncompression buffer.",
			     array_bytes(&My_Connections[Idx].zip.rbuf));
		}
#endif
	}
	return len_processed;
} /* Handle_Buffer */


static void
Check_Connections(void)
{
	/* check if connections are alive. if not, play PING-PONG first.
	 * if this doesn't help either, disconnect client. */
	CLIENT *c;
	CONN_ID i;
	char msg[64];

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
					snprintf(msg, sizeof(msg), "Ping timeout: %d seconds", Conf_PongTimeout);
					Conn_Close(i, NULL, msg, true);
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
New_Server( int Server , ng_ipaddr_t *dest)
{
	/* Establish new server link */
	char ip_str[NG_INET_ADDRSTRLEN];
	int af_dest, res, new_sock;
	CLIENT *c;

	assert( Server > NONE );

	if (!ng_ipaddr_tostr_r(dest, ip_str)) {
		Log(LOG_WARNING, "New_Server: Could not convert IP to string");
		return;
	}

	Log( LOG_INFO, "Establishing connection to \"%s\", %s, port %d ... ",
			Conf_Server[Server].host, ip_str, Conf_Server[Server].port );

	af_dest = ng_ipaddr_af(dest);
	new_sock = socket(af_dest, SOCK_STREAM, 0);
	if (new_sock < 0) {
		Log( LOG_CRIT, "Can't create socket (af %d) : %s!", af_dest, strerror( errno ));
		return;
	}

	if (!Init_Socket(new_sock))
		return;

	/* is a bind address configured? */
	res = ng_ipaddr_af(&Conf_Server[Server].bind_addr);
	/* if yes, bind now. If it fails, warn and let connect() pick a source address */
	if (res && bind(new_sock, (struct sockaddr *) &Conf_Server[Server].bind_addr,
				ng_ipaddr_salen(&Conf_Server[Server].bind_addr)))
	{
		ng_ipaddr_tostr_r(&Conf_Server[Server].bind_addr, ip_str);
		Log(LOG_WARNING, "Can't bind socket to %s: %s!", ip_str, strerror(errno));
	}
	ng_ipaddr_setport(dest, Conf_Server[Server].port);
	res = connect(new_sock, (struct sockaddr *) dest, ng_ipaddr_salen(dest));
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

	ng_ipaddr_tostr_r(dest, ip_str);
	c = Client_NewLocal(new_sock, ip_str, CLIENT_UNKNOWNSERVER, false);
	if (!c) {
		Log( LOG_ALERT, "Can't establish connection: can't create client structure!" );
		close( new_sock );
		return;
	}

	/* Conn_Close() decrements this counter again */
	Account_Connection();
	Client_SetIntroducer( c, c );
	Client_SetToken( c, TOKEN_OUTBOUND );

	/* Register connection */
	Conf_Server[Server].conn_id = new_sock;
	My_Connections[new_sock].sock = new_sock;
	My_Connections[new_sock].addr = *dest;
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
#ifdef SSL_SUPPORT
	if (Conf_Server[Server].SSLConnect && !ConnSSL_PrepareConnect( &My_Connections[new_sock],
								&Conf_Server[Server] ))
	{
		Log(LOG_ALERT, "Could not initialize SSL for outgoing connection");
		Conn_Close( new_sock, "Could not initialize SSL for outgoing connection", NULL, false );
		Init_Conn_Struct( new_sock );
		Conf_Server[Server].conn_id = NONE;
		return;
	}
#endif
	LogDebug("Registered new connection %d on socket %d (%ld in total).",
		 new_sock, My_Connections[new_sock].sock, NumConnections);
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
#if defined(IPPROTO_IP) && defined(IPTOS_LOWDELAY)
	value = IPTOS_LOWDELAY;
	LogDebug("Setting IP_TOS on socket %d to IPTOS_LOWDELAY.", Sock);
	if (setsockopt(Sock, IPPROTO_IP, IP_TOS, &value,
		       (socklen_t) sizeof(value))) {
		Log(LOG_ERR, "Can't set socket option IP_TOS: %s!",
		    strerror(errno));
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
	ng_ipaddr_t dest_addrs[4];	/* we can handle at most 3; but we read up to
					   four so we can log the 'more than we can handle'
					   condition. First result is tried immediately, rest
					   is saved for later if needed. */

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
	len = Resolve_Read(&Conf_Server[i].res_stat, dest_addrs, sizeof(dest_addrs));
	if (len == 0)
		return;

	assert((len % sizeof(ng_ipaddr_t)) == 0);

	LogDebug("Got result from resolver: %u structs (%u bytes).", len/sizeof(ng_ipaddr_t), len);

	memset(&Conf_Server[i].dst_addr, 0, sizeof(Conf_Server[i].dst_addr));
	if (len > sizeof(ng_ipaddr_t)) {
		/* more than one address for this hostname, remember them
		 * in case first address is unreachable/not available */
		len -= sizeof(ng_ipaddr_t);
		if (len > sizeof(Conf_Server[i].dst_addr)) {
			len = sizeof(Conf_Server[i].dst_addr);
			Log(LOG_NOTICE,
				"Notice: Resolver returned more IP Addresses for host than we can handle, additional addresses dropped.");
		}
		memcpy(&Conf_Server[i].dst_addr, &dest_addrs[1], len);
	}
	/* connect() */
	New_Server(i, dest_addrs);
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

	/* Only update client information of unregistered clients.
	 * Note: user commands (e. g. WEBIRC) are always read _after_ reading
	 * the resolver results, so we don't have to worry to override settings
	 * from these commands here. */
	if(Client_Type(c) == CLIENT_UNKNOWN) {
		strlcpy(My_Connections[i].host, readbuf,
			sizeof(My_Connections[i].host));
		Client_SetHostname(c, readbuf);
#ifdef IDENTAUTH
		++identptr;
		if (*identptr) {
			Log(LOG_INFO, "IDENT lookup for connection %d: \"%s\".", i, identptr);
			Client_SetUser(c, identptr, true);
		} else {
			Log(LOG_INFO, "IDENT lookup for connection %d: no result.", i);
		}
#endif
	}
#ifdef DEBUG
		else Log( LOG_DEBUG, "Resolver: discarding result for already registered connection %d.", i );
#endif
	/* Reset penalty time */
	Conn_ResetPenalty( i );
} /* cb_Read_Resolver_Result */


/**
 * Write a "simple" (error) message to a socket.
 * The message is sent without using the connection write buffers, without
 * compression/encryption, and even without any error reporting. It is
 * designed for error messages of e.g. New_Connection(). */
static void
Simple_Message(int Sock, const char *Msg)
{
	char buf[COMMAND_LEN];
	size_t len;

	assert(Sock > NONE);
	assert(Msg != NULL);

	strlcpy(buf, Msg, sizeof buf - 2);
	len = strlcat(buf, "\r\n", sizeof buf);
	if (write(Sock, buf, len) < 0) {
		/* Because this function most probably got called to log
		 * an error message, any write error is ignored here to
		 * avoid an endless loop. But casting the result of write()
		 * to "void" doesn't satisfy the GNU C code attribute
		 * "warn_unused_result" which is used by some versions of
		 * glibc (e.g. 2.11.1), therefore this silly error
		 * "handling" code here :-( */
		return;
	}
} /* Simple_Error */


/**
 * Get CLIENT structure that belongs to a local connection identified by its
 * index number. Each connection belongs to a client by definition, so it is
 * not required that the caller checks for NULL return values.
 * @param Idx Connection index number
 * @return Pointer to CLIENT structure
 */
GLOBAL CLIENT *
Conn_GetClient( CONN_ID Idx ) 
{
	CONNECTION *c;

	assert(Idx >= 0);
	c = array_get(&My_ConnArray, sizeof (CONNECTION), (size_t)Idx);
	assert(c != NULL);
	return c ? c->client : NULL;
}


#ifdef SSL_SUPPORT

/**
 * Get information about used SSL chiper.
 * @param Idx Connection index number
 * @param buf Buffer for returned information text
 * @param len Size of return buffer "buf"
 * @return true on success, false otherwise
 */
GLOBAL bool
Conn_GetCipherInfo(CONN_ID Idx, char *buf, size_t len)
{
	if (Idx < 0)
		return false;
	assert(Idx < (int) array_length(&My_ConnArray, sizeof(CONNECTION)));
	return ConnSSL_GetCipherInfo(&My_Connections[Idx], buf, len);
}


/**
 * Check if a connection is SSL-enabled or not.
 * @param Idx Connection index number
 * @return true if connection is SSL-enabled, false otherwise.
 */
GLOBAL bool
Conn_UsesSSL(CONN_ID Idx)
{
	if (Idx < 0)
		return false;
	assert(Idx < (int) array_length(&My_ConnArray, sizeof(CONNECTION)));
	return Conn_OPTION_ISSET(&My_Connections[Idx], CONN_SSL);
}

#endif


/* -eof- */
