/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __parse_h__
#define __parse_h__

/**
 * @file
 * IRC command parser and validator (header)
 */

#include "portab.h"

typedef struct _REQUEST			/* vgl. RFC 2812, 2.3 */
{
	char *prefix;			/* Prefix */
	char *command;			/* IRC-Befehl */
	char *argv[15];			/* Parameter (max. 15: 0..14) */
	int argc;			/* Anzahl vorhandener Parameter */
} REQUEST;


typedef struct _COMMAND
{
	const char *name;		/* command name */
	bool (*function) PARAMS(( CLIENT *Client, REQUEST *Request ));
	CLIENT_TYPE type;		/* valid client types (bit mask) */
	long lcount, rcount;		/* number of local and remote calls */
	long bytes;			/* number of bytes created */
} COMMAND;

GLOBAL bool Parse_Request PARAMS((CONN_ID Idx, char *Request ));

GLOBAL COMMAND *Parse_GetCommandStruct PARAMS(( void ));

#endif

/* -eof- */
