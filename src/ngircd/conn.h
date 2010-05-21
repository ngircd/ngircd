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
 * Connection management (header)
 */


#ifndef __conn_h__
#define __conn_h__


#include <time.h>			/* for time_t, see below */

/*
 * connection state flags. this is a bitmask -- all values must
 * be unique and a power of two.
 *
 * If you introduce new ones in between, make sure to adjust all
 * remaining ones.
 */
#define CONN_ISCLOSING		1	/* Conn_Close() already called */
#define CONN_ISCONNECTING	2	/* connect() in progress */
#define CONN_RFC1459		4	/* RFC 1459 compatibility mode */
#ifdef ZLIB
#define CONN_ZIP		8	/* zlib compressed link */
#endif

#include "conf-ssl.h"

#ifdef SSL_SUPPORT
#define CONN_SSL_CONNECT	16	/* wait for ssl connect to finish */
#define CONN_SSL		32	/* this connection is SSL encrypted */
#define CONN_SSL_WANT_WRITE	64	/* SSL/TLS library needs to write protocol data */
#define CONN_SSL_WANT_READ	128	/* SSL/TLS library needs to read protocol data */
#define CONN_SSL_FLAGS_ALL	(CONN_SSL_CONNECT|CONN_SSL|CONN_SSL_WANT_WRITE|CONN_SSL_WANT_READ)
#endif
typedef long CONN_ID;

#include "client.h"

#ifdef CONN_MODULE

#include "defines.h"
#include "proc.h"
#include "array.h"
#include "tool.h"
#include "ng_ipaddr.h"

#ifdef ZLIB
#include <zlib.h>
typedef struct _ZipData
{
	z_stream in;			/* "Handle" for input stream */
	z_stream out;			/* "Handle" for output stream */
	array rbuf;			/* Read buffer (compressed) */
	array wbuf;			/* Write buffer (uncompressed) */
	long bytes_in, bytes_out;	/* Counter for statistics (uncompressed!) */
} ZIPDATA;
#endif /* ZLIB */

typedef struct _Connection
{
	int sock;			/* Socket handle */
	ng_ipaddr_t addr;		/* Client address */
	PROC_STAT proc_stat;		/* Status of resolver process */
	char host[HOST_LEN];		/* Hostname */
	array rbuf;			/* Read buffer */
	array wbuf;			/* Write buffer */
	time_t signon;			/* Signon ("connect") time */
	time_t lastdata;		/* Last activity */
	time_t lastping;		/* Last PING */
	time_t lastprivmsg;		/* Last PRIVMSG */
	time_t delaytime;		/* Ignore link ("penalty") */
	long bytes_in, bytes_out;	/* Received and sent bytes */
	long msg_in, msg_out;		/* Received and sent IRC messages */
	int flag;			/* Flag (see "irc-write" module) */
	UINT16 options;			/* Link options / connection state */
	UINT16 bps;			/* bytes processed within last second */
	CLIENT *client;			/* pointer to client structure */
#ifdef ZLIB
	ZIPDATA zip;			/* Compression information */
#endif  /* ZLIB */
#ifdef SSL_SUPPORT
	struct ConnSSL_State ssl_state;	/* SSL/GNUTLS state information */
#endif
} CONNECTION;

GLOBAL CONNECTION *My_Connections;
GLOBAL CONN_ID Pool_Size;
GLOBAL long WCounter;

#endif /* CONN_MODULE */


GLOBAL void Conn_Init PARAMS((void ));
GLOBAL void Conn_Exit PARAMS(( void ));

GLOBAL unsigned int Conn_InitListeners PARAMS(( void ));
GLOBAL void Conn_ExitListeners PARAMS(( void ));

GLOBAL void Conn_Handler PARAMS(( void ));

GLOBAL bool Conn_WriteStr PARAMS(( CONN_ID Idx, const char *Format, ... ));

GLOBAL void Conn_Close PARAMS(( CONN_ID Idx, const char *LogMsg, const char *FwdMsg, bool InformClient ));

GLOBAL void Conn_SyncServerStruct PARAMS(( void ));

GLOBAL CLIENT* Conn_GetClient PARAMS((CONN_ID i));
#ifdef SSL_SUPPORT
GLOBAL bool Conn_GetCipherInfo PARAMS((CONN_ID Idx, char *buf, size_t len));
GLOBAL bool Conn_UsesSSL PARAMS((CONN_ID Idx));
#else
static inline bool Conn_UsesSSL(UNUSED CONN_ID Idx) { return false; }
#endif

GLOBAL long Conn_Count PARAMS((void));
GLOBAL long Conn_CountMax PARAMS((void));
GLOBAL long Conn_CountAccepted PARAMS((void));

#endif


/* -eof- */
