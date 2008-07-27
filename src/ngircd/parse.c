/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2008 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#include "portab.h"

/**
 * @file
 * IRC command parser and validator.
 */

#include "imp.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "ngircd.h"
#include "defines.h"
#include "conn-func.h"
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
#include "numeric.h"

#include "exp.h"

struct _NUMERIC {
	int numeric;
	bool (*function) PARAMS(( CLIENT *Client, REQUEST *Request ));
};


static COMMAND My_Commands[] =
{
	{ "ADMIN", IRC_ADMIN, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "AWAY", IRC_AWAY, CLIENT_USER, 0, 0, 0 },
	{ "CONNECT", IRC_CONNECT, CLIENT_USER, 0, 0, 0 },
	{ "DIE", IRC_DIE, CLIENT_USER, 0, 0, 0 },
	{ "DISCONNECT", IRC_DISCONNECT, CLIENT_USER, 0, 0, 0 },
	{ "ERROR", IRC_ERROR, 0xFFFF, 0, 0, 0 },
	{ "HELP", IRC_HELP, CLIENT_USER, 0, 0, 0 },
	{ "INFO", IRC_INFO, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "INVITE", IRC_INVITE, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "ISON", IRC_ISON, CLIENT_USER, 0, 0, 0 },
	{ "JOIN", IRC_JOIN, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "KICK", IRC_KICK, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "KILL", IRC_KILL, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "LINKS", IRC_LINKS, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "LIST", IRC_LIST, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "LUSERS", IRC_LUSERS, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "MODE", IRC_MODE, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "MOTD", IRC_MOTD, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "NAMES", IRC_NAMES, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "NICK", IRC_NICK, 0xFFFF, 0, 0, 0 },
	{ "NJOIN", IRC_NJOIN, CLIENT_SERVER, 0, 0, 0 },
	{ "NOTICE", IRC_NOTICE, 0xFFFF, 0, 0, 0 },
	{ "OPER", IRC_OPER, CLIENT_USER, 0, 0, 0 },
	{ "PART", IRC_PART, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "PASS", IRC_PASS, 0xFFFF, 0, 0, 0 },
	{ "PING", IRC_PING, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "PONG", IRC_PONG, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "PRIVMSG", IRC_PRIVMSG, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "QUIT", IRC_QUIT, 0xFFFF, 0, 0, 0 },
	{ "REHASH", IRC_REHASH, CLIENT_USER, 0, 0, 0 },
	{ "RESTART", IRC_RESTART, CLIENT_USER, 0, 0, 0 },
	{ "SERVER", IRC_SERVER, 0xFFFF, 0, 0, 0 },
	{ "SERVICE", IRC_SERVICE, 0xFFFF, 0, 0, 0 },
	{ "SERVLIST", IRC_SERVLIST, CLIENT_USER, 0, 0, 0 },
	{ "SQUERY", IRC_SQUERY, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "SQUIT", IRC_SQUIT, CLIENT_SERVER, 0, 0, 0 },
	{ "STATS", IRC_STATS, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "SUMMON", IRC_SUMMON, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "TIME", IRC_TIME, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "TOPIC", IRC_TOPIC, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "TRACE", IRC_TRACE, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "USER", IRC_USER, 0xFFFF, 0, 0, 0 },
	{ "USERHOST", IRC_USERHOST, CLIENT_USER, 0, 0, 0 },
	{ "USERS", IRC_USERS, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "VERSION", IRC_VERSION, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "WALLOPS", IRC_WALLOPS, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "WHO", IRC_WHO, CLIENT_USER, 0, 0, 0 },
	{ "WHOIS", IRC_WHOIS, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
	{ "WHOWAS", IRC_WHOWAS, CLIENT_USER|CLIENT_SERVER, 0, 0, 0 },
#ifdef IRCPLUS
	{ "CHANINFO", IRC_CHANINFO, CLIENT_SERVER, 0, 0, 0 },
#endif
	{ NULL, NULL, 0x0, 0, 0, 0 } /* Ende-Marke */
};

static void Init_Request PARAMS(( REQUEST *Req ));

static bool Validate_Prefix PARAMS(( CONN_ID Idx, REQUEST *Req, bool *Closed ));
static bool Validate_Command PARAMS(( CONN_ID Idx, REQUEST *Req, bool *Closed ));
static bool Validate_Args PARAMS(( CONN_ID Idx, REQUEST *Req, bool *Closed ));

static bool Handle_Request PARAMS(( CONN_ID Idx, REQUEST *Req ));

#define ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))

/**
 * Return the pointer to the global "IRC command structure".
 * This structure, an array of type "COMMAND" describes all the IRC commands
 * implemented by ngIRCd and how to handle them.
 * @return Pointer to the global command structure.
 */
GLOBAL COMMAND *
Parse_GetCommandStruct( void )
{
	return My_Commands;
} /* Parse_GetCommandStruct */


/**
 * Parse a command ("request") received from a client.
 * 
 * This function is called after the connection layer received a valid CR+LF
 * terminated line of text: we asume that this is a valid IRC command and
 * try to do something useful with it :-)
 *
 * All errors are reported to the client from which the command has been
 * received, and if the error is fatal this connection is closed down.
 *
 * This function is able to parse the syntax as described in RFC 2812,
 * section 2.3.
 *
 * @param Idx Index of the connection from which the command has been received.
 * @param Request NULL terminated line of text (the "command").
 * @return true on success (valid command or "regular" error), false if a
 * 	fatal error occured and the connection has been shut down.
 */
GLOBAL bool
Parse_Request( CONN_ID Idx, char *Request )
{
	REQUEST req;
	char *start, *ptr;
	bool closed;

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
		 * Parametern ignorieren */
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


/**
 * Initialize request structure.
 * @param Req Request structure to be initialized.
 */
static void
Init_Request( REQUEST *Req )
{
	/* Neue Request-Struktur initialisieren */

	int i;

	assert( Req != NULL );

	Req->prefix = NULL;
	Req->command = NULL;
	for( i = 0; i < 15; Req->argv[i++] = NULL );
	Req->argc = 0;
} /* Init_Request */


static bool
Validate_Prefix( CONN_ID Idx, REQUEST *Req, bool *Closed )
{
	CLIENT *client, *c;

	assert( Idx >= 0 );
	assert( Req != NULL );

	*Closed = false;

	/* ist ueberhaupt ein Prefix vorhanden? */
	if( ! Req->prefix ) return true;

	/* Client-Struktur der Connection ermitteln */
	client = Conn_GetClient( Idx );
	assert( client != NULL );

	/* nur validieren, wenn bereits registrierte Verbindung */
	if(( Client_Type( client ) != CLIENT_USER ) && ( Client_Type( client ) != CLIENT_SERVER ) && ( Client_Type( client ) != CLIENT_SERVICE ))
	{
		/* noch nicht registrierte Verbindung.
		 * Das Prefix wird ignoriert. */
		Req->prefix = NULL;
		return true;
	}

	/* pruefen, ob der im Prefix angegebene Client bekannt ist */
	c = Client_Search( Req->prefix );
	if( ! c )
	{
		/* im Prefix angegebener Client ist nicht bekannt */
		Log( LOG_ERR, "Invalid prefix \"%s\", client not known (connection %d, command %s)!?", Req->prefix, Idx, Req->command );
		if( ! Conn_WriteStr( Idx, "ERROR :Invalid prefix \"%s\", client not known!?", Req->prefix )) *Closed = true;
		return false;
	}

	/* pruefen, ob der Client mit dem angegebenen Prefix in Richtung
	 * des Senders liegt, d.h. sicherstellen, dass das Prefix nicht
	 * gefaelscht ist */
	if( Client_NextHop( c ) != client )
	{
		/* das angegebene Prefix ist aus dieser Richtung, also
		 * aus der gegebenen Connection, ungueltig! */
		Log( LOG_ERR, "Spoofed prefix \"%s\" from \"%s\" (connection %d, command %s)!", Req->prefix, Client_Mask( Conn_GetClient( Idx )), Idx, Req->command );
		Conn_Close( Idx, NULL, "Spoofed prefix", true);
		*Closed = true;
		return false;
	}

	return true;
} /* Validate_Prefix */


static bool
Validate_Command( UNUSED CONN_ID Idx, UNUSED REQUEST *Req, bool *Closed )
{
	assert( Idx >= 0 );
	assert( Req != NULL );
	*Closed = false;

	return true;
} /* Validate_Comman */


static bool
#ifdef STRICT_RFC
Validate_Args(CONN_ID Idx, REQUEST *Req, bool *Closed)
#else
Validate_Args(UNUSED CONN_ID Idx, UNUSED REQUEST *Req, bool *Closed)
#endif
{
#ifdef STRICT_RFC
	int i;
#endif

	*Closed = false;

#ifdef STRICT_RFC
	assert( Idx >= 0 );
	assert( Req != NULL );

	/* CR and LF are never allowed in command parameters.
	 * But since we do accept lines terminated only with CR or LF in
	 * "non-RFC-compliant mode" (besides the correct CR+LF combination),
	 * this check can only trigger in "strict RFC" mode; therefore we
	 * optimize it away otherwise ... */
	for (i = 0; i < Req->argc; i++) {
		if (strchr(Req->argv[i], '\r') || strchr(Req->argv[i], '\n')) {
			Log(LOG_ERR,
			    "Invalid character(s) in parameter (connection %d, command %s)!?",
			    Idx, Req->command);
			if (!Conn_WriteStr(Idx,
					   "ERROR :Invalid character(s) in parameter!"))
				*Closed = true;
			return false;
		}
	}
#endif

	return true;
} /* Validate_Args */


/* Command is a status code ("numeric") from another server */
static bool
Handle_Numeric(CLIENT *client, REQUEST *Req)
{
	static const struct _NUMERIC Numerics[] = {
		{ 005, IRC_Num_ISUPPORT },
		{ 376, IRC_Num_ENDOFMOTD }
	};
	int i, num;
	char str[LINE_LEN];
	CLIENT *prefix, *target = NULL;

	/* Determine target */
	if (Req->argc > 0)
		target = Client_Search(Req->argv[0]);

	if (!target) {
		/* Status code without target!? */
		if (Req->argc > 0)
			Log(LOG_WARNING,
			    "Unknown target for status code %s: \"%s\"",
			    Req->command, Req->argv[0]);
		else
			Log(LOG_WARNING,
			    "Unknown target for status code %s!",
			    Req->command);
		return true;
	}
	if (target == Client_ThisServer()) {
		/* This server is the target of the numeric */
		num = atoi(Req->command);

		for (i = 0; i < (int) ARRAY_SIZE(Numerics); i++) {
			if (num == Numerics[i].numeric)
				return Numerics[i].function(client, Req);
		}

		LogDebug("Ignored status code %s from \"%s\".",
			 Req->command, Client_ID(client));
		return true;
	}

	/* Determine source */
	if (! Req->prefix[0]) {
		/* Oops, no prefix!? */
		Log(LOG_WARNING, "Got status code %s from \"%s\" without prefix!?",
						Req->command, Client_ID(client));
		return true;
	}

	prefix = Client_Search(Req->prefix);
	if (! prefix) { /* Oops, unknown prefix!? */
		Log(LOG_WARNING, "Got status code %s from unknown source: \"%s\"", Req->command, Req->prefix);
		return true;
	}

	/* Forward status code */
	strlcpy(str, Req->command, sizeof(str));
	for (i = 0; i < Req->argc; i++) {
		if (i < Req->argc - 1)
			strlcat(str, " ", sizeof(str));
		else
			strlcat(str, " :", sizeof(str));
		strlcat(str, Req->argv[i], sizeof(str));
	}
	return IRC_WriteStrClientPrefix(target, prefix, "%s", str);
}


static bool
Handle_Request( CONN_ID Idx, REQUEST *Req )
{
	/* Client-Request verarbeiten. Bei einem schwerwiegenden Fehler
	 * wird die Verbindung geschlossen und false geliefert. */
	CLIENT *client;
	bool result = true;
	int client_type;
	COMMAND *cmd;

	assert( Idx >= 0 );
	assert( Req != NULL );
	assert( Req->command != NULL );

	client = Conn_GetClient( Idx );
	assert( client != NULL );

	/* Numeric? */
	client_type = Client_Type(client);
	if ((client_type == CLIENT_SERVER ||
	     client_type == CLIENT_UNKNOWNSERVER)
	    && strlen(Req->command) == 3 && atoi(Req->command) > 1)
		return Handle_Numeric(client, Req);

	cmd = My_Commands;
	while (cmd->name) {
		/* Befehl suchen */
		if (strcasecmp(Req->command, cmd->name) != 0) {
			cmd++;
			continue;
		}

		if (!(client_type & cmd->type))
			return IRC_WriteStrClient(client, ERR_NOTREGISTERED_MSG, Client_ID(client));

		/* Command is allowed for this client: call it and count produced bytes */
		Conn_ResetWCounter();
		result = (cmd->function)(client, Req);
		cmd->bytes += Conn_WCounter();

		/* Adjust counters */
		if (client_type != CLIENT_SERVER)
			cmd->lcount++;
		else
			cmd->rcount++;
		return result;
	}

	if (client_type != CLIENT_USER &&
	    client_type != CLIENT_SERVER &&
	    client_type != CLIENT_SERVICE )
		return true;

	/* Unknown command and registered connection: generate error: */
	LogDebug("Connection %d: Unknown command \"%s\", %d %s,%s prefix.",
			Client_Conn( client ), Req->command, Req->argc,
			Req->argc == 1 ? "parameter" : "parameters",
			Req->prefix ? "" : " no" );

	if (Client_Type(client) != CLIENT_SERVER) {
		result = IRC_WriteStrClient(client, ERR_UNKNOWNCOMMAND_MSG,
				Client_ID(client), Req->command);
		Conn_SetPenalty(Idx, 1);
	}
	return result;
} /* Handle_Request */


/* -eof- */
