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
 * $Id: parse.h,v 1.8 2002/12/12 12:23:43 alex Exp $
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


GLOBAL BOOLEAN Parse_Request PARAMS((CONN_ID Idx, CHAR *Request ));


#endif


/* -eof- */
