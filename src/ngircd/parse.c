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
 * $Id: parse.c,v 1.1 2001/12/21 23:53:16 alex Exp $
 *
 * parse.c: Parsen der Client-Anfragen
 *
 * $Log: parse.c,v $
 * Revision 1.1  2001/12/21 23:53:16  alex
 * - Modul zum Parsen von Client-Requests begonnen.
 *
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "conn.h"
#include "log.h"

#include <exp.h>
#include "parse.h"


LOCAL VOID Init_Request( REQUEST *Req );

LOCAL BOOLEAN Parse_Error( CONN_ID Idx, CHAR *Error );

LOCAL BOOLEAN Validate_Prefix( REQUEST *Req );
LOCAL BOOLEAN Validate_Command( REQUEST *Req );
LOCAL BOOLEAN Validate_Args( REQUEST *Req );

LOCAL BOOLEAN Handle_Request( CONN_ID Idx, REQUEST *Req );


GLOBAL VOID Parse_Init( VOID )
{
} /* Parse_Init */


GLOBAL VOID Parse_Exit( VOID )
{
} /* Parse_Exit */


GLOBAL BOOLEAN Parse_Request( CONN_ID Idx, CHAR *Request )
{
	/* Client-Request parsen und verarbeiten. Bei einem schwerwiegenden
	 * Fehler wird die Verbindung geschlossen und FALSE geliefert.
	 * Der Aufbau gueltiger Requests ist in RFC 2812, 2.3 definiert. */

	REQUEST req;
	CHAR *start, *ptr;

	assert( Idx >= 0 );
	assert( Request != NULL );
	
	Init_Request( &req );

	/* gibt es ein Prefix? */
	if( Request[0] == ':' )
	{
		/* Prefix vorhanden */
		req.prefix = Request + 1;
		ptr = strchr( Request, ' ' );
		if( ! ptr ) return Parse_Error( Idx, "Invalid prefix (command missing!?)" );
		*ptr = '\0';
		start = ptr + 1;
	}
	else start = Request;

	if( ! Validate_Prefix( &req )) return Parse_Error( Idx, "Invalid prefix");

	/* Befehl */
	ptr = strchr( start, ' ' );
	if( ptr ) *ptr = '\0';
	req.command = start;

	if( ! Validate_Command( &req )) return Parse_Error( Idx, "Invalid command" );

	/* Argumente, Parameter */
	if( ptr )
	{
		/* Prinzipiell gibt es welche :-) */
		start = ptr + 1;
		while( start )
		{
			/* Parameter-String "zerlegen" */
			ptr = strchr( start, ' ' );
			if( ptr ) *ptr = '\0';

			if( start[0] == ':' ) req.argv[req.argc] = start + 1;
			else req.argv[req.argc] = start;
			
			req.argc++;

			if( start[0] == ':' ) break;
			if( req.argc > 14 ) break;
			
			if( ptr ) start = ptr + 1;
			else start = NULL;
		}
	}
	
	if( ! Validate_Args( &req )) return Parse_Error( Idx, "Invalid argument(s)" );

	return Handle_Request( Idx, &req );
} /* Parse_Request */


LOCAL VOID Init_Request( REQUEST *Req )
{
	/* Neue Request-Struktur initialisieren */

	INT i;
	
	assert( Req != NULL );

	Req->prefix = NULL;
	Req->command = NULL;
	for( i = 0; i < 15; Req->argv[i++] = NULL );
	Req->argc = 0;
} /* Init_Request */


LOCAL BOOLEAN Parse_Error( CONN_ID Idx, CHAR *Error )
{
	/* Fehler beim Parsen. Fehlermeldung an den Client schicken.
	 * TRUE: Connection wurde durch diese Funktion nicht geschlossen,
	 * FALSE: Connection wurde terminiert. */
	
	CHAR msg[256];
	
	assert( Idx >= 0 );
	assert( Error != NULL );

	sprintf( msg, "Parse error: %s!", Error );
	return Conn_WriteStr( Idx, msg );
} /* Parse_Error */


LOCAL BOOLEAN Validate_Prefix( REQUEST *Req )
{
	assert( Req != NULL );
	return TRUE;
} /* Validate_Prefix */


LOCAL BOOLEAN Validate_Command( REQUEST *Req )
{
	assert( Req != NULL );
	return TRUE;
} /* Validate_Comman */


LOCAL BOOLEAN Validate_Args( REQUEST *Req )
{
	assert( Req != NULL );
	return TRUE;
} /* Validate_Args */


LOCAL BOOLEAN Handle_Request( CONN_ID Idx, REQUEST *Req )
{
	assert( Idx >= 0 );
	assert( Req != NULL );
	assert( Req->command != NULL );

#ifdef DEBUG
	Log( LOG_DEBUG, " -> connection %d: '%s', %d %s,%s prefix.", Idx, Req->command, Req->argc, Req->argc == 1 ? "parameter" : "parameters", Req->prefix ? "" : " no" );
#endif

	return TRUE;
} /* Handle_Request */


/* -eof- */
