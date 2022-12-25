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

/** A single IRC request ("command"). See RFC 2812 section 2.3 for details. */
typedef struct _REQUEST
{
	char *prefix;			/**< Prefix */
	char *command;			/**< IRC command */
	char *argv[15];			/**< Parameters, at most 15 (0..14) */
	int argc;			/**< Number of given parameters */
} REQUEST;

/** IRC command handling structure */
typedef struct _COMMAND
{
	const char *name;		/**< Command name */
	bool (*function) PARAMS(( CLIENT *Client, REQUEST *Request ));
					/**< Function to handle this command */
	CLIENT_TYPE type;		/**< Valid client types (bit mask) */
	int min_argc;			/**< Min parameters */
	int max_argc;			/**< Max parameters */
	int penalty;			/**< Penalty for this command */
	long lcount, rcount;		/**< Number of local and remote calls */
	long bytes;			/**< Number of bytes created */
} COMMAND;

GLOBAL bool Parse_Request PARAMS((CONN_ID Idx, char *Request ));

GLOBAL COMMAND *Parse_GetCommandStruct PARAMS(( void ));

#endif

/* -eof- */
