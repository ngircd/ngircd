/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: tool.c,v 1.8 2002/03/22 00:17:27 alex Exp $
 *
 * tool.c: Hilfsfunktionen, ggf. Platformabhaengig
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "exp.h"
#include "tool.h"


GLOBAL VOID ngt_TrimStr( CHAR *String )
{
	/* Mit ngt_TrimStr() werden fuehrende und folgende Leerzeichen,
	 * Tabulatoren und Zeilenumbrueche (ASCII 10 und ASCII 13) aus
	 * dem String entfernt. */
	
	CHAR *start, *ptr;

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


GLOBAL CHAR *ngt_LowerStr( CHAR *String )
{
	/* String in Kleinbuchstaben konvertieren. Der uebergebene
	 * Speicherbereich wird durch das Ergebnis ersetzt, zusaetzlich
	 * wird dieser auch als Pointer geliefert. */

	CHAR *ptr;

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


/* -eof- */
