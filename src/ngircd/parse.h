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
 * $Id: parse.h,v 1.9 2002/12/18 13:53:20 alex Exp $
 *
 * IRC command parser and validator (header)
 */


#ifndef __parse_h__
#define __parse_h__


typedef struct _REQUEST			/* vgl. RFC 2812, 2.3 */
{
	CHAR *prefix;			/* Prefix */
	CHAR *command;			/* IRC-Befehl */
	CHAR *argv[15];			/* Parameter (max. 15: 0..14) */
	INT argc;			/* Anzahl vorhandener Parameter */
} REQUEST;


typedef struct _COMMAND
{
	CHAR *name;		/* command name */
	BOOLEAN (*function)( CLIENT *Client, REQUEST *Request );
	CLIENT_TYPE type;	/* valid client types (bit mask) */
	LONG lcount, rcount;	/* number of local and remote calls */
	LONG bytes;		/* number of bytes created */
} COMMAND;


GLOBAL BOOLEAN Parse_Request PARAMS((CONN_ID Idx, CHAR *Request ));

GLOBAL COMMAND *Parse_GetCommandStruct PARAMS(( VOID ));


#endif


/* -eof- */
