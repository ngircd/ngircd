/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an comBase beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: tool.c,v 1.2 2001/12/12 17:20:33 alex Exp $
 *
 * tool.c: Hilfsfunktionen, ggf. Platformabhaengig
 *
 * $Log: tool.c,v $
 * Revision 1.2  2001/12/12 17:20:33  alex
 * - Tool-Funktionen haben nun das Praefix "ngt_".
 *
 * Revision 1.1.1.1  2001/12/11 21:53:04  alex
 * - Imported sources to CVS.
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <exp.h>
#include "tool.h"


GLOBAL VOID ngt_Trim_Str( CHAR *String )
{
	/* Mit ngt_Trim_Str() werden fuehrende und folgende Leerzeichen,
	 * Tabulatoren und Zeilenumbrueche (ASCII 10 und ASCII 13) aus
	 * dem String entfernt. */
	
	CHAR *start, *ptr;

	assert( String != NULL );

	start = String;
	
	/* Zeichen am Anfang pruefen ... */
	while(( *start == ' ' ) || ( *start == 9 )) start++;
	
	/* Zeichen am Ende pruefen ... */
	ptr = strchr( start, '\0' ) - 1;
	while(( *ptr == ' ' ) || ( *ptr == 9 ) || ( *ptr == 10 ) || ( *ptr == 13 )) ptr--;
	*(++ptr) = '\0';

	memmove( String, start, strlen( start ) + 1 );
} /* ngt_Trim_Str */


/* -eof- */
