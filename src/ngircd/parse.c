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
 * $Id: parse.c,v 1.50 2002/12/06 17:02:39 alex Exp $
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
#include "irc-info.h"
#include "irc-login.h"
#include "irc-mode.h"
#include "irc-op.h"
#include "irc-oper.h"
#include "irc-server.h"
#include "irc-write.h"

#include "exp.h"


typedef struct _COMMAND
{
	CHAR *name;		/* Name des Befehls */
	BOOLEAN (*function)( CLIENT *Client, REQUEST *Request );
	CLIENT_TYPE type;	/* Erlaubte Client-Typen (Bitmaske) */
	LONG count;		/* Anzahl der Aufrufe */
} COMMAND;


COMMAND My_Commands[] =
{
	{ "ADMIN", IRC_ADMIN, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "AWAY", IRC_AWAY, CLIENT_USER, 0 },
	{ "CONNECT", IRC_CONNECT, CLIENT_USER, 0 },
	{ "DIE", IRC_DIE, CLIENT_USER, 0 },
	{ "ERROR", IRC_ERROR, 0xFFFF, 0 },
	{ "INVITE", IRC_INVITE, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "ISON", IRC_ISON, CLIENT_USER, 0 },
	{ "JOIN", IRC_JOIN, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "KICK", IRC_KICK, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "KILL", IRC_KILL, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "LINKS", IRC_LINKS, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "LIST", IRC_LIST, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "LUSERS", IRC_LUSERS, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "MODE", IRC_MODE, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "MOTD", IRC_MOTD, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "NAMES", IRC_NAMES, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "NICK", IRC_NICK, 0xFFFF, 0 },
	{ "NJOIN", IRC_NJOIN, CLIENT_SERVER, 0 },
	{ "NOTICE", IRC_NOTICE, 0xFFFF, 0 },
	{ "OPER", IRC_OPER, CLIENT_USER, 0 },
	{ "PART", IRC_PART, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "PASS", IRC_PASS, 0xFFFF, 0 },
	{ "PING", IRC_PING, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "PONG", IRC_PONG, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "PRIVMSG", IRC_PRIVMSG, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "QUIT", IRC_QUIT, 0xFFFF, 0 },
	{ "REHASH", IRC_REHASH, CLIENT_USER, 0 },
	{ "RESTART", IRC_RESTART, CLIENT_USER, 0 },
	{ "SERVER", IRC_SERVER, 0xFFFF, 0 },
	{ "SQUIT", IRC_SQUIT, CLIENT_SERVER, 0 },
	{ "STATS", IRC_STATS, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "TIME", IRC_TIME, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "TOPIC", IRC_TOPIC, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "USER", IRC_USER, 0xFFFF, 0 },
	{ "USERHOST", IRC_USERHOST, CLIENT_USER, 0 },
	{ "VERSION", IRC_VERSION, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "WHO", IRC_WHO, CLIENT_USER, 0 },
	{ "WHOIS", IRC_WHOIS, CLIENT_USER|CLIENT_SERVER, 0 },
	{ "WHOWAS", IRC_WHOWAS, CLIENT_USER|CLIENT_SERVER, 0 },
#ifdef IRCPLUS
	{ "CHANINFO", IRC_CHANINFO, CLIENT_SERVER, 0 },
#endif
	{ NULL, NULL, 0 } /* Ende-Marke */
};


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

	/* Daten validieren */
	if( ! Validate_Prefix( Idx, &req, &closed )) return ! closed;
	if( ! Validate_Command( Idx, &req, &closed )) return ! closed;
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
		Log( LOG_ERR, "Invalid prefix \"%s\", client not known (connection %d, command %s)!?", Req->prefix, Idx, Req->command );
		if( ! Conn_WriteStr( Idx, "ERROR :Invalid prefix \"%s\", client not known!?", Req->prefix )) *Closed = TRUE;
		return FALSE;
	}

	/* pruefen, ob der Client mit dem angegebenen Prefix in Richtung
	 * des Senders liegt, d.h. sicherstellen, dass das Prefix nicht
	 * gefaelscht ist */
	if( Client_NextHop( c ) != client )
	{
		/* das angegebene Prefix ist aus dieser Richtung, also
		 * aus der gegebenen Connection, ungueltig! */
		Log( LOG_ERR, "Spoofed prefix \"%s\" from \"%s\" (connection %d, command %s)!", Req->prefix, Client_Mask( Client_GetFromConn( Idx )), Idx, Req->command );
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
	COMMAND *cmd;
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
			if( Req->argc > 0 ) Log( LOG_WARNING, "Unknown target for status code %s: \"%s\"", Req->command, Req->argv[0] );
			else Log( LOG_WARNING, "Unknown target for status code %s!", Req->command );
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

	cmd = My_Commands;
	while( cmd->name )
	{
		/* Befehl suchen */
		if( strcasecmp( Req->command, cmd->name ) != 0 )
		{
			cmd++; continue;
		}

		if( Client_Type( client ) & cmd->type )
		{
			/* Der Befehl ist fuer diesen Client-Typ erlaubt.
			 * Entsprechende Funktion zaehlen nun aufrufen: */
			cmd->count++;
			return (cmd->function)( client, Req );
		}
		else
		{
			/* Befehl ist fuer diesen Client-Typ nicht erlaubt! */
			return IRC_WriteStrClient( client, ERR_NOTREGISTERED_MSG, Client_ID( client ));
		}
	}
	
	/* Unbekannter Befehl */
	Log( LOG_DEBUG, "Connection %d: Unknown command \"%s\", %d %s,%s prefix.", Client_Conn( client ), Req->command, Req->argc, Req->argc == 1 ? "parameter" : "parameters", Req->prefix ? "" : " no" );
	if( Client_Type( client ) != CLIENT_SERVER ) return IRC_WriteStrClient( client, ERR_UNKNOWNCOMMAND_MSG, Client_ID( client ), Req->command );
	else return TRUE;
} /* Handle_Request */


/* -eof- */
