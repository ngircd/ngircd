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

static char UNUSED id[] = "$Id: tool.c,v 1.3 2005/03/19 18:43:52 fw Exp $";

#include "imp.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "exp.h"
#include "tool.h"


GLOBAL void
ngt_TrimStr( char *String )
{
	/* Mit ngt_TrimStr() werden fuehrende und folgende Leerzeichen,
	 * Tabulatoren und Zeilenumbrueche (ASCII 10 und ASCII 13) aus
	 * dem String entfernt. */
	
	char *start, *ptr;

	assert( String != NULL );

	start = String;
	
	/* Zeichen am Anfang pruefen ... */
	while(( *start == ' ' ) || ( *start == 9 )) start++;
	
	/* Zeichen am Ende pruefen ... */
	ptr = strchr( start, '\0' ) - 1;
	while((( *ptr == ' ' ) || ( *ptr == 9 ) || ( *ptr == 10 ) || ( *ptr == 13 )) && ptr >= start ) ptr--;
	*(++ptr) = '\0';

	memmove( String, start, strlen( start ) + 1 );
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
