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
 * $Id: parse.c,v 1.41.2.2 2002/10/04 13:12:46 alex Exp $
 *
 * parse.c: Parsen der Client-Anfragen
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ngircd.h"
#include "defines.h"
#include "conn.h"
#include "client.h"
#include "channel.h"
#include "log.h"
#include "messages.h"
#include "tool.h"

#include "exp.h"
#include "parse.h"

#include "imp.h"
#include "irc.h"
#include "irc-channel.h"
#include "irc-login.h"
#include "irc-mode.h"
#include "irc-op.h"
#include "irc-oper.h"
#include "irc-server.h"
#include "irc-write.h"

#include "exp.h"


LOCAL VOID Init_Request PARAMS(( REQUEST *Req ));

LOCAL BOOLEAN Validate_Prefix PARAMS(( CONN_ID Idx, REQUEST *Req, BOOLEAN *Closed ));
LOCAL BOOLEAN Validate_Command PARAMS(( CONN_ID Idx, REQUEST *Req, BOOLEAN *Closed ));
LOCAL BOOLEAN Validate_Args PARAMS(( CONN_ID Idx, REQUEST *Req, BOOLEAN *Closed ));

LOCAL BOOLEAN Handle_Request PARAMS(( CONN_ID Idx, REQUEST *Req ));


GLOBAL BOOLEAN
Parse_Request( CONN_ID Idx, CHAR *Request )
{
	/* Client-Request parsen. Bei einem schwerwiegenden Fehler wird
	 * die Verbindung geschlossen und FALSE geliefert.
	 * Der Aufbau gueltiger Requests ist in RFC 2812, 2.3 definiert. */

	REQUEST req;
	CHAR *start, *ptr;
	BOOLEAN closed;

	assert( Idx >= 0 );
	assert( Request != NULL );

#ifdef SNIFFER
	if( NGIRCd_Sniffer ) Log( LOG_DEBUG, " <- connection %d: '%s'.", Idx, Request );
#endif

	Init_Request( &req );

	/* Fuehrendes und folgendes "Geraffel" verwerfen */
	ngt_TrimStr( Request );

	/* gibt es ein Prefix? */
	if( Request[0] == ':' )
	{
		/* Prefix vorhanden */
		req.prefix = Request + 1;
		ptr = strchr( Request, ' ' );
		if( ! ptr )
		{
			Log( LOG_DEBUG, "Connection %d: Parse error: prefix without command!?", Idx );
			return Conn_WriteStr( Idx, "ERROR :Prefix without command!?" );
		}
		*ptr = '\0';
#ifndef STRICT_RFC
		/* multiple Leerzeichen als Trenner zwischen
		 * Prefix und Befehl ignorieren */
		while( *(ptr + 1) == ' ' ) ptr++;
#endif
		start = ptr + 1;
	}
	else start = Request;

	if( ! Validate_Prefix( Idx, &req, &closed )) return ! closed;

	/* Befehl */
	ptr = strchr( start, ' ' );
	if( ptr )
	{
		*ptr = '\0';
#ifndef STRICT_RFC
		/* multiple Leerzeichen als Trenner vor
		*Parametertrennern ignorieren */
		while( *(ptr + 1) == ' ' ) ptr++;
#endif
	}
	req.command = start;

	if( ! Validate_Command( Idx, &req, &closed )) return ! closed;

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
				if( ptr )
				{
					*ptr = '\0';
#ifndef STRICT_RFC
					/* multiple Leerzeichen als
					 * Parametertrenner ignorieren */
					while( *(ptr + 1) == ' ' ) ptr++;
#endif
				}
			}

			req.argc++;

			if( start[0] == ':' ) break;
			if( req.argc > 14 ) break;

			if( ptr ) start = ptr + 1;
			else start = NULL;
		}
	}

	if( ! Validate_Args( Idx, &req, &closed )) return ! closed;

	return Handle_Request( Idx, &req );
} /* Parse_Request */


LOCAL VOID
Init_Request( REQUEST *Req )
{
	/* Neue Request-Struktur initialisieren */

	INT i;

	assert( Req != NULL );

	Req->prefix = NULL;
	Req->command = NULL;
	for( i = 0; i < 15; Req->argv[i++] = NULL );
	Req->argc = 0;
} /* Init_Request */


LOCAL BOOLEAN
Validate_Prefix( CONN_ID Idx, REQUEST *Req, BOOLEAN *Closed )
{
	CLIENT *client, *c;

	assert( Idx >= 0 );
	assert( Req != NULL );

	*Closed = FALSE;

	/* ist ueberhaupt ein Prefix vorhanden? */
	if( ! Req->prefix ) return TRUE;

	/* Client-Struktur der Connection ermitteln */
	client = Client_GetFromConn( Idx );
	assert( client != NULL );

	/* nur validieren, wenn bereits registrierte Verbindung */
	if(( Client_Type( client ) != CLIENT_USER ) && ( Client_Type( client ) != CLIENT_SERVER ) && ( Client_Type( client ) != CLIENT_SERVICE ))
	{
		/* noch nicht registrierte Verbindung.
		 * Das Prefix wird ignoriert. */
		Req->prefix = NULL;
		return TRUE;
	}

	/* pruefen, ob der im Prefix angegebene Client bekannt ist */
	c = Client_Search( Req->prefix );
	if( ! c )
	{
		/* im Prefix angegebener Client ist nicht bekannt */
		Log( LOG_ERR, "Invalid prefix, client not known (connection %d)!?", Idx );
		if( ! Conn_WriteStr( Idx, "ERROR :Invalid prefix, client not known!?" )) *Closed = TRUE;
		return FALSE;
	}

	/* pruefen, ob der Client mit dem angegebenen Prefix in Richtung
	 * des Senders liegt, d.h. sicherstellen, dass das Prefix nicht
	 * gefaelscht ist */
	if( Client_NextHop( c ) != client )
	{
		/* das angegebene Prefix ist aus dieser Richtung, also
		 * aus der gegebenen Connection, ungueltig! */
		Log( LOG_ERR, "Spoofed prefix \"%s\" from \"%s\" (connection %d)!", Req->prefix, Client_Mask( Client_GetFromConn( Idx )), Idx );
		Conn_Close( Idx, NULL, "Spoofed prefix", TRUE );
		*Closed = TRUE;
		return FALSE;
	}

	return TRUE;
} /* Validate_Prefix */


LOCAL BOOLEAN
Validate_Command( CONN_ID Idx, REQUEST *Req, BOOLEAN *Closed )
{
	assert( Idx >= 0 );
	assert( Req != NULL );
	*Closed = FALSE;

	return TRUE;
} /* Validate_Comman */


LOCAL BOOLEAN
Validate_Args( CONN_ID Idx, REQUEST *Req, BOOLEAN *Closed )
{
	assert( Idx >= 0 );
	assert( Req != NULL );
	*Closed = FALSE;

	return TRUE;
} /* Validate_Args */


LOCAL BOOLEAN
Handle_Request( CONN_ID Idx, REQUEST *Req )
{
	/* Client-Request verarbeiten. Bei einem schwerwiegenden Fehler
	 * wird die Verbindung geschlossen und FALSE geliefert. */

	CLIENT *client, *target, *prefix;
	CHAR str[LINE_LEN];
	INT i;

	assert( Idx >= 0 );
	assert( Req != NULL );
	assert( Req->command != NULL );

	client = Client_GetFromConn( Idx );
	assert( client != NULL );

	/* Statuscode, der geforwarded werden muss? */
	if(( strlen( Req->command ) == 3 ) && ( atoi( Req->command ) > 100 ))
	{
		/* Befehl ist ein Statuscode */

		/* Zielserver ermitteln */
		if(( Client_Type( client ) == CLIENT_SERVER ) && ( Req->argc > 0 )) target = Client_Search( Req->argv[0] );
		else target = NULL;
		if( ! target )
		{
			if( Req->argc > 0 ) Log( LOG_WARNING, "Unknown target for status code: \"%s\"", Req->argv[0] );
			else Log( LOG_WARNING, "Unknown target for status code!" );
			return TRUE;
		}
		if( target == Client_ThisServer( ))
		{
			Log( LOG_DEBUG, "Ignored status code %s from \"%s\".", Req->command, Client_ID( client ));
			return TRUE;
		}

		/* Quell-Client ermitteln */
		if( ! Req->prefix[0] )
		{
			Log( LOG_WARNING, "Got status code without prefix!?" );
			return TRUE;
		}
		else prefix = Client_Search( Req->prefix );
		if( ! prefix )
		{
			Log( LOG_WARNING, "Got status code from unknown source: \"%s\"", Req->prefix );
			return TRUE;
		}

		/* Statuscode weiterleiten */
		strcpy( str, Req->command );
		for( i = 0; i < Req->argc; i++ )
		{
			if( i < Req->argc - 1 ) strcat( str, " " );
			else strcat( str, " :" );
			strcat( str, Req->argv[i] );
		}
		return IRC_WriteStrClientPrefix( target, prefix, "%s", str );
	}

	if( strcasecmp( Req->command, "PASS" ) == 0 ) return IRC_PASS( client, Req );
	else if( strcasecmp( Req->command, "NICK" ) == 0 ) return IRC_NICK( client, Req );
	else if( strcasecmp( Req->command, "USER" ) == 0 ) return IRC_USER( client, Req );
	else if( strcasecmp( Req->command, "SERVER" ) == 0 ) return IRC_SERVER( client, Req );
	else if( strcasecmp( Req->command, "NJOIN" ) == 0 ) return IRC_NJOIN( client, Req );
	else if( strcasecmp( Req->command, "QUIT" ) == 0 ) return IRC_QUIT( client, Req );
	else if( strcasecmp( Req->command, "SQUIT" ) == 0 ) return IRC_SQUIT( client, Req );
	else if( strcasecmp( Req->command, "PING" ) == 0 ) return IRC_PING( client, Req );
	else if( strcasecmp( Req->command, "PONG" ) == 0 ) return IRC_PONG( client, Req );
	else if( strcasecmp( Req->command, "MOTD" ) == 0 ) return IRC_MOTD( client, Req );
	else if( strcasecmp( Req->command, "PRIVMSG" ) == 0 ) return IRC_PRIVMSG( client, Req );
	else if( strcasecmp( Req->command, "NOTICE" ) == 0 ) return IRC_NOTICE( client, Req );
	else if( strcasecmp( Req->command, "MODE" ) == 0 ) return IRC_MODE( client, Req );
	else if( strcasecmp( Req->command, "NAMES" ) == 0 ) return IRC_NAMES( client, Req );
	else if( strcasecmp( Req->command, "ISON" ) == 0 ) return IRC_ISON( client, Req );
	else if( strcasecmp( Req->command, "WHOIS" ) == 0 ) return IRC_WHOIS( client, Req );
	else if( strcasecmp( Req->command, "USERHOST" ) == 0 ) return IRC_USERHOST( client, Req );
	else if( strcasecmp( Req->command, "OPER" ) == 0 ) return IRC_OPER( client, Req );
	else if( strcasecmp( Req->command, "DIE" ) == 0 ) return IRC_DIE( client, Req );
	else if( strcasecmp( Req->command, "RESTART" ) == 0 ) return IRC_RESTART( client, Req );
	else if( strcasecmp( Req->command, "ERROR" ) == 0 ) return IRC_ERROR( client, Req );
	else if( strcasecmp( Req->command, "LUSERS" ) == 0 ) return IRC_LUSERS( client, Req );
	else if( strcasecmp( Req->command, "LINKS" ) == 0 ) return IRC_LINKS( client, Req );
	else if( strcasecmp( Req->command, "JOIN" ) == 0 ) return IRC_JOIN( client, Req );
	else if( strcasecmp( Req->command, "PART" ) == 0 ) return IRC_PART( client, Req );
	else if( strcasecmp( Req->command, "VERSION" ) == 0 ) return IRC_VERSION( client, Req );
	else if( strcasecmp( Req->command, "KILL" ) == 0 ) return IRC_KILL( client, Req );
	else if( strcasecmp( Req->command, "AWAY" ) == 0 ) return IRC_AWAY( client, Req );
	else if( strcasecmp( Req->command, "TOPIC" ) == 0 ) return IRC_TOPIC( client, Req );
	else if( strcasecmp( Req->command, "WHO" ) == 0 ) return IRC_WHO( client, Req );
	else if( strcasecmp( Req->command, "LIST" ) == 0 ) return IRC_LIST( client, Req );
	else if( strcasecmp( Req->command, "INVITE" ) == 0 ) return IRC_INVITE( client, Req );
	else if( strcasecmp( Req->command, "KICK" ) == 0 ) return IRC_KICK( client, Req );
	else if( strcasecmp( Req->command, "CONNECT" ) == 0 ) return IRC_CONNECT( client, Req );
	else if( strcasecmp( Req->command, "ADMIN" ) == 0 ) return IRC_ADMIN( client, Req );
#ifdef IRCPLUS
	else if( strcasecmp( Req->command, "CHANINFO" ) == 0 ) return IRC_CHANINFO( client, Req );
#endif
	
	/* Unbekannter Befehl */
	if( Client_Type( client ) != CLIENT_SERVER ) IRC_WriteStrClient( client, ERR_UNKNOWNCOMMAND_MSG, Client_ID( client ), Req->command );
	Log( LOG_DEBUG, "Connection %d: Unknown command \"%s\", %d %s,%s prefix.", Client_Conn( client ), Req->command, Req->argc, Req->argc == 1 ? "parameter" : "parameters", Req->prefix ? "" : " no" );

	return TRUE;
} /* Handle_Request */


/* -eof- */
