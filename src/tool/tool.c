/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2005 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Tool functions
 */


#include "portab.h"

static char UNUSED id[] = "$Id: tool.c,v 1.5 2006/03/24 23:25:39 fw Exp $";

#include "imp.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "exp.h"
#include "tool.h"


/**
 * Removes all leading and trailing whitespaces of a string.
 * @param String The string to remove whitespaces from.
 */
GLOBAL void
ngt_TrimStr(char *String)
{
	char *start, *end;

	assert(String != NULL);

	start = String;

	/* Remove whitespaces at the beginning of the string ... */
	while (*start == ' ' || *start == '\t')
		start++;

	if (!*start) {
		*String = 0;
		return;
	}
	/* ... and at the end: */
	end = strchr(start, '\0');
	end--;
	while ((*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')
	       && end > start)
		end--;

	/* New trailing NULL byte */
	*(++end) = '\0';

	memmove(String, start, (size_t)(end - start)+1);
} /* ngt_TrimStr */


GLOBAL char *
ngt_LowerStr( char *String )
{
	/* String in Kleinbuchstaben konvertieren. Der uebergebene
	 * Speicherbereich wird durch das Ergebnis ersetzt, zusaetzlich
	 * wird dieser auch als Pointer geliefert. */

	char *ptr;

	assert( String != NULL );

	/* Zeichen konvertieren */
	ptr = String;
	while( *ptr )
	{
		*ptr = tolower( *ptr );
		ptr++;
	}
	
	return String;
} /* ngt_LowerStr */


GLOBAL void
ngt_TrimLastChr( char *String, const char Chr)
{
	/* If last character in the string matches Chr, remove it.
	 * Empty strings are handled correctly. */

        unsigned int len;

	assert( String != NULL );

	len = strlen( String );
	if( len == 0 ) return;

	len--;

	if( String[len] == Chr ) String[len] = '\0';
} /* ngt_TrimLastChr */


/* -eof- */
