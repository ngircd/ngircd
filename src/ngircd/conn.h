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
 * $Id: conn.h,v 1.34 2005/04/18 21:08:31 fw Exp $
 *
 * Connection management (header)
 */


#ifndef __conn_h__
#define __conn_h__


#include <time.h>			/* for time_t, see below */


#define CONN_ISCLOSING		1U	/* Conn_Close() already called */
#define CONN_ISCONNECTING	2U	/* connect() in progress */

#ifdef ZLIB
#define CONN_ZIP		4U	/* zlib compressed link */
#endif


typedef int CONN_ID;


#ifdef CONN_MODULE

#include "defines.h"
#include "resolve.h"

#ifdef ZLIB
#include <zlib.h>
typedef struct _ZipData
{
	z_stream in;			/* "Handle" for input stream */
	z_stream out;			/* "Handle" for output stream */
	char rbuf[READBUFFER_LEN];	/* Read buffer */
	int rdatalen;			/* Length of data in read buffer (compressed) */
	char wbuf[WRITEBUFFER_LEN];	/* Write buffer */
	int wdatalen;			/* Length of data in write buffer (uncompressed) */
	long bytes_in, bytes_out;	/* Counter for statistics (uncompressed!) */
} ZIPDATA;
#endif /* ZLIB */

typedef struct _Connection
{
	int sock;			/* Socket handle */
	struct sockaddr_in addr;	/* Client address */
	RES_STAT *res_stat;		/* Status of resolver process, if any */
	char host[HOST_LEN];		/* Hostname */
	char rbuf[READBUFFER_LEN];	/* Read buffer */
	int rdatalen;			/* Length of data in read buffer */
	char wbuf[WRITEBUFFER_LEN];	/* Write buffer */
	int wdatalen;			/* Length of data in write buffer */
	time_t starttime;		/* Start time of link */
	time_t lastdata;		/* Last activity */
	time_t lastping;		/* Last PING */
	time_t lastprivmsg;		/* Last PRIVMSG */
	time_t delaytime;		/* Ignore link ("penalty") */
	long bytes_in, bytes_out;	/* Received and sent bytes */
	long msg_in, msg_out;		/* Received and sent IRC messages */
	int flag;			/* Flag (see "irc-write" module) */
	UINT16 options;			/* Link options / connection state */
#ifdef ZLIB
	ZIPDATA zip;			/* Compression information */
#endif  /* ZLIB */
} CONNECTION;

GLOBAL CONNECTION *My_Connections;
GLOBAL CONN_ID Pool_Size;
GLOBAL long WCounter;

#endif /* CONN_MODULE */


GLOBAL void Conn_Init PARAMS((void ));
GLOBAL void Conn_Exit PARAMS(( void ));

GLOBAL int Conn_InitListeners PARAMS(( void ));
GLOBAL void Conn_ExitListeners PARAMS(( void ));

GLOBAL bool Conn_NewListener PARAMS(( const UINT16 Port ));

GLOBAL void Conn_Handler PARAMS(( void ));

GLOBAL bool Conn_Write PARAMS(( CONN_ID Idx, char *Data, int Len ));
GLOBAL bool Conn_WriteStr PARAMS(( CONN_ID Idx, char *Format, ... ));

GLOBAL void Conn_Close PARAMS(( CONN_ID Idx, char *LogMsg, char *FwdMsg, bool InformClient ));

GLOBAL void Conn_SyncServerStruct PARAMS(( void ));

GLOBAL int Conn_MaxFD;


#endif


/* -eof- */
