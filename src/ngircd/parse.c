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
 * $Id: parse.c,v 1.7 2001/12/27 19:13:21 alex Exp $
 *
 * parse.c: Parsen der Client-Anfragen
 *
 * $Log: parse.c,v $
 * Revision 1.7  2001/12/27 19:13:21  alex
 * - neue Befehle NOTICE und PRIVMSG.
 * - Debug-Logging ein wenig reduziert.
 *
 * Revision 1.6  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.5  2001/12/26 03:23:03  alex
 * - PING/PONG-Befehle implementiert.
 *
 * Revision 1.4  2001/12/25 22:04:26  alex
 * - Aenderungen an den Debug- und Logging-Funktionen.
 *
 * Revision 1.3  2001/12/25 19:18:36  alex
 * - Gross- und Kleinschreibung der IRC-Befehle wird ignoriert.
 * - bessere Debug-Ausgaben.
 *
 * Revision 1.2  2001/12/23 21:56:47  alex
 * - bessere Debug-Ausgaben,
 * - Bug im Parameter-Parser behoben (bei "langem" Parameter)
 * - erste IRC-Befehle werden erkannt :-)
 *
 * Revision 1.1  2001/12/21 23:53:16  alex
 * - Modul zum Parsen von Client-Requests begonnen.
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "client.h"
#include "conn.h"
#include "irc.h"
#include "log.h"
#include "messages.h"

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
	/* Client-Request parsen. Bei einem schwerwiegenden Fehler wird
	 * die Verbindung geschlossen und FALSE geliefert.
	 * Der Aufbau gueltiger Requests ist in RFC 2812, 2.3 definiert. */

	REQUEST req;
	CHAR *start, *ptr;

	assert( Idx >= 0 );
	assert( Request != NULL );

#ifdef SNIFFER
	Log( LOG_DEBUG, " <- connection %d: '%s'.", Idx, Request );
#endif
	
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
			if( start[0] == ':' )
			{
				req.argv[req.argc] = start + 1;
				ptr = NULL;
			}
			else
			{
				req.argv[req.argc] = start;
				ptr = strchr( start, ' ' );
				if( ptr ) *ptr = '\0';
			}
			
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
	/* Client-Request verarbeiten. Bei einem schwerwiegenden Fehler
	 * wird die Verbindung geschlossen und FALSE geliefert. */

	CLIENT *client;

	assert( Idx >= 0 );
	assert( Req != NULL );
	assert( Req->command != NULL );

	client = Client_GetFromConn( Idx );
	assert( client != NULL );

	if( strcasecmp( Req->command, "PASS" ) == 0 ) return IRC_PASS( client, Req );
	else if( strcasecmp( Req->command, "NICK" ) == 0 ) return IRC_NICK( client, Req );
	else if( strcasecmp( Req->command, "USER" ) == 0 ) return IRC_USER( client, Req );
	else if( strcasecmp( Req->command, "QUIT" ) == 0 ) return IRC_QUIT( client, Req );
	else if( strcasecmp( Req->command, "PING" ) == 0 ) return IRC_PING( client, Req );
	else if( strcasecmp( Req->command, "PONG" ) == 0 ) return IRC_PONG( client, Req );
	else if( strcasecmp( Req->command, "MOTD" ) == 0 ) return IRC_MOTD( client, Req );
	else if( strcasecmp( Req->command, "PRIVMSG" ) == 0 ) return IRC_PRIVMSG( client, Req );
	else if( strcasecmp( Req->command, "NOTICE" ) == 0 ) return IRC_NOTICE( client, Req );
	
	/* Unbekannter Befehl */
	IRC_WriteStrClient( client, This_Server, ERR_UNKNOWNCOMMAND_MSG, Client_Name( client ), Req->command );

	Log( LOG_DEBUG, "Connection %d: Unknown command '%s', %d %s,%s prefix.", Idx, Req->command, Req->argc, Req->argc == 1 ? "parameter" : "parameters", Req->prefix ? "" : " no" );
	
	return TRUE;
} /* Handle_Request */


/* -eof- */
