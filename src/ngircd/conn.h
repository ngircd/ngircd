/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: conn.h,v 1.30 2003/02/21 19:18:48 alex Exp $
 *
 * Connection management (header)
 */


#ifndef __conn_h__
#define __conn_h__


#include <time.h>			/* fro time_t, see below */


#define CONN_ISCLOSING 1		/* Conn_Close() already called */

#ifdef USE_ZLIB
#define CONN_ZIP 2			/* zlib compressed link */
#endif


typedef INT CONN_ID;


#ifdef CONN_MODULE

#include "defines.h"
#include "resolve.h"

#ifdef USE_ZLIB
#include <zlib.h>
typedef struct _ZipData
{
	z_stream in;			/* "Handle" for input stream */
	z_stream out;			/* "Handle" for output stream */
	CHAR rbuf[READBUFFER_LEN];	/* Read buffer */
	INT rdatalen;			/* Length of data in read buffer (compressed) */
	CHAR wbuf[WRITEBUFFER_LEN];	/* Write buffer */
	INT wdatalen;			/* Length of data in write buffer (uncompressed) */
	LONG bytes_in, bytes_out;	/* Counter for statistics (uncompressed!) */
} ZIPDATA;
#endif /* USE_ZLIB */

typedef struct _Connection
{
	INT sock;			/* Socket handle */
	struct sockaddr_in addr;	/* Client address */
	RES_STAT *res_stat;		/* Status of resolver process, if any */
	CHAR host[HOST_LEN];		/* Hostname */
	CHAR rbuf[READBUFFER_LEN];	/* Read buffer */
	INT rdatalen;			/* Length of data in read buffer */
	CHAR wbuf[WRITEBUFFER_LEN];	/* Write buffer */
	INT wdatalen;			/* Length of data in write buffer */
	time_t starttime;		/* Start time of link */
	time_t lastdata;		/* Last activity */
	time_t lastping;		/* Last PING */
	time_t lastprivmsg;		/* Last PRIVMSG */
	time_t delaytime;		/* Ignore link ("penalty") */
	LONG bytes_in, bytes_out;	/* Received and sent bytes */
	LONG msg_in, msg_out;		/* Received and sent IRC messages */
	INT flag;			/* Flag (see "irc-write" module) */
	INT options;			/* Link options */
#ifdef USE_ZLIB
	ZIPDATA zip;			/* Compression information */
#endif  /* USE_ZLIB */
} CONNECTION;

GLOBAL CONNECTION *My_Connections;
GLOBAL CONN_ID Pool_Size;
GLOBAL LONG WCounter;

#endif /* CONN_MODULE */


GLOBAL VOID Conn_Init PARAMS((VOID ));
GLOBAL VOID Conn_Exit PARAMS(( VOID ));

GLOBAL INT Conn_InitListeners PARAMS(( VOID ));
GLOBAL VOID Conn_ExitListeners PARAMS(( VOID ));

GLOBAL BOOLEAN Conn_NewListener PARAMS(( CONST UINT Port ));

GLOBAL VOID Conn_Handler PARAMS(( VOID ));

GLOBAL BOOLEAN Conn_Write PARAMS(( CONN_ID Idx, CHAR *Data, INT Len ));
GLOBAL BOOLEAN Conn_WriteStr PARAMS(( CONN_ID Idx, CHAR *Format, ... ));

GLOBAL VOID Conn_Close PARAMS(( CONN_ID Idx, CHAR *LogMsg, CHAR *FwdMsg, BOOLEAN InformClient ));


GLOBAL INT Conn_MaxFD;


#endif


/* -eof- */
