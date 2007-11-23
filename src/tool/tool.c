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

static char UNUSED id[] = "$Id: tool.c,v 1.7 2007/11/23 16:26:05 fw Exp $";

#include "imp.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <netinet/in.h>
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

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
	while (*start == ' ' || *start == '\t' ||
	       *start == '\n' || *start == '\r')
		start++;

	if (!*start) {
		*String = '\0';
		return;
	}

	/* ... and at the end: */
	end = strchr(start, '\0');
	end--;
	while ((*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')
	       && end >= start)
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


GLOBAL bool
ngt_IPStrToBin(const char *ip_str, struct in_addr *inaddr)
{
	/* AF is always AF_INET for now */
#ifdef HAVE_INET_ATON
	if (inet_aton(ip_str, inaddr) == 0)
		return false;
#else
	inaddr->s_addr = inet_addr(ip_str);
	if (inaddr->s_addr == (unsigned)-1)
		return false;
#endif
	return true;
}




/* -eof- */
