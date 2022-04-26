/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2018 Alexander Barton (alex@barton.de) and Contributors.
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "ngircd.h"
#include "conn-func.h"
#include "conf.h"
#include "channel.h"
#include "log.h"
#include "messages.h"

#include "parse.h"

#include "irc.h"
#include "irc-cap.h"
#include "irc-channel.h"
#ifdef ICONV
# include "irc-encoding.h"
#endif
#include "irc-info.h"
#include "irc-login.h"
#include "irc-metadata.h"
#include "irc-mode.h"
#include "irc-op.h"
#include "irc-oper.h"
#include "irc-server.h"
#include "irc-write.h"
#include "numeric.h"

struct _NUMERIC {
	int numeric;
	bool (*function) PARAMS(( CLIENT *Client, REQUEST *Request ));
};


static COMMAND My_Commands[] =
{
#define _CMD(name, func, type, min, max, penalty) \
    { (name), (func), (type), (min), (max), (penalty), 0, 0, 0 }
	_CMD("ADMIN", IRC_ADMIN, CLIENT_USER|CLIENT_SERVER, 0, 1, 1),
	_CMD("AWAY", IRC_AWAY, CLIENT_USER, 0, 1, 0),
	_CMD("CAP", IRC_CAP, CLIENT_ANY, 1, 2, 0),
	_CMD("CONNECT", IRC_CONNECT, CLIENT_USER|CLIENT_SERVER, 0, -1, 0),
#ifdef STRICT_RFC
	_CMD("DIE", IRC_DIE, CLIENT_USER, 0, 0, 0),
#else
	_CMD("DIE", IRC_DIE, CLIENT_USER, 0, 1, 0),
#endif
	_CMD("DISCONNECT", IRC_DISCONNECT, CLIENT_USER, 1, 1, 0),
	_CMD("ERROR", IRC_ERROR, CLIENT_ANY, 0, -1, 0),
	_CMD("GLINE", IRC_xLINE, CLIENT_USER|CLIENT_SERVER, 0, -1, 0),
	_CMD("HELP", IRC_HELP, CLIENT_USER, 0, 1, 2),
	_CMD("INFO", IRC_INFO, CLIENT_USER|CLIENT_SERVER, 0, 1, 2),
	_CMD("INVITE", IRC_INVITE, CLIENT_USER|CLIENT_SERVER, 2, 2, 1),
	_CMD("ISON", IRC_ISON, CLIENT_USER, 1, -1, 0),
	_CMD("JOIN", IRC_JOIN, CLIENT_USER|CLIENT_SERVER, 1, 2, 0),
	_CMD("KICK", IRC_KICK, CLIENT_USER|CLIENT_SERVER, 2, 3, 0),
	_CMD("KILL", IRC_KILL, CLIENT_USER|CLIENT_SERVER, 2, 2, 0),
	_CMD("KLINE", IRC_xLINE, CLIENT_USER|CLIENT_SERVER, 0, -1, 0),
	_CMD("LINKS", IRC_LINKS, CLIENT_USER|CLIENT_SERVER, 0, 2, 1),
	_CMD("LIST", IRC_LIST, CLIENT_USER|CLIENT_SERVER, 0, 2, 2),
	_CMD("LUSERS", IRC_LUSERS, CLIENT_USER|CLIENT_SERVER, 0, 2, 1),
	_CMD("METADATA", IRC_METADATA, CLIENT_SERVER, 3, 3, 0),
	_CMD("MODE", IRC_MODE, CLIENT_USER|CLIENT_SERVER, 1, -1, 1),
	_CMD("MOTD", IRC_MOTD, CLIENT_USER|CLIENT_SERVER, 0, 1, 3),
	_CMD("NAMES", IRC_NAMES, CLIENT_USER|CLIENT_SERVER, 0, 2, 1),
	_CMD("NICK", IRC_NICK, CLIENT_ANY, 0, -1, 0),
	_CMD("NJOIN", IRC_NJOIN, CLIENT_SERVER, 2, 2, 0),
	_CMD("NOTICE", IRC_NOTICE, CLIENT_ANY, 0, -1, 0),
	_CMD("OPER", IRC_OPER, CLIENT_USER, 2, 2, 0),
	_CMD("PART", IRC_PART, CLIENT_USER|CLIENT_SERVER, 1, 2, 0),
	_CMD("PASS", IRC_PASS, CLIENT_ANY, 0, -1, 0),
	_CMD("PING", IRC_PING, CLIENT_USER|CLIENT_SERVER, 0, -1, 0),
	_CMD("PONG", IRC_PONG, CLIENT_ANY, 0, -1, 0),
	_CMD("PRIVMSG", IRC_PRIVMSG, CLIENT_USER|CLIENT_SERVER, 0, 2, 0),
	_CMD("QUIT", IRC_QUIT, CLIENT_ANY, 0, 1, 0),
	_CMD("REHASH", IRC_REHASH, CLIENT_USER, 0, 0, 0),
	_CMD("RESTART", IRC_RESTART, CLIENT_USER, 0, 0, 0),
	_CMD("SERVER", IRC_SERVER, CLIENT_ANY, 0, -1, 0),
	_CMD("SERVICE", IRC_SERVICE, CLIENT_ANY, 6, 6, 0),
	_CMD("SERVLIST", IRC_SERVLIST, CLIENT_USER, 0, 2, 1),
	_CMD("SQUERY", IRC_SQUERY, CLIENT_USER|CLIENT_SERVER, 0, 2, 0),
	_CMD("SQUIT", IRC_SQUIT, CLIENT_USER|CLIENT_SERVER, 2, 2, 0),
	_CMD("STATS", IRC_STATS, CLIENT_USER|CLIENT_SERVER, 0, 2, 2),
	_CMD("SVSNICK", IRC_SVSNICK, CLIENT_SERVER, 2, 2, 0),
	_CMD("SUMMON", IRC_SUMMON, CLIENT_USER|CLIENT_SERVER, 0, -1, 0),
	_CMD("TIME", IRC_TIME, CLIENT_USER|CLIENT_SERVER, 0, 1, 1),
	_CMD("TOPIC", IRC_TOPIC, CLIENT_USER|CLIENT_SERVER, 1, 2, 1),
	_CMD("TRACE", IRC_TRACE, CLIENT_USER|CLIENT_SERVER, 0, 1, 3),
	_CMD("USER", IRC_USER, CLIENT_ANY, 0, -1, 0),
	_CMD("USERHOST", IRC_USERHOST, CLIENT_USER, 1, -1, 1),
	_CMD("USERS", IRC_USERS, CLIENT_USER|CLIENT_SERVER, 0, -1, 0),
	_CMD("VERSION", IRC_VERSION, CLIENT_USER|CLIENT_SERVER, 0, 1, 1),
	_CMD("WALLOPS", IRC_WALLOPS, CLIENT_USER|CLIENT_SERVER, 1, 1, 0),
	_CMD("WEBIRC", IRC_WEBIRC, CLIENT_UNKNOWN, 4, 5, 0),
	_CMD("WHO", IRC_WHO, CLIENT_USER, 0, 2, 1),
	_CMD("WHOIS", IRC_WHOIS, CLIENT_USER|CLIENT_SERVER, 0, -1, 1),
	_CMD("WHOWAS", IRC_WHOWAS, CLIENT_USER|CLIENT_SERVER, 0, -1, 0),

#ifdef IRCPLUS
	_CMD("CHANINFO", IRC_CHANINFO, CLIENT_SERVER, 0, -1, 0),
# ifdef ICONV
	_CMD("CHARCONV", IRC_CHARCONV, CLIENT_USER, 1, 1, 0),
# endif
#endif

#ifndef STRICT_RFC
	_CMD("GET",  IRC_QUIT_HTTP, CLIENT_UNKNOWN, 0, -1, 0),
	_CMD("POST", IRC_QUIT_HTTP, CLIENT_UNKNOWN, 0, -1, 0),
#endif
	_CMD(NULL, NULL, 0, 0, 0, 0) /* End-Mark */
#undef _CMD
};

static void Init_Request PARAMS(( REQUEST *Req ));

static bool Validate_Prefix PARAMS(( CONN_ID Idx, REQUEST *Req, bool *Closed ));
static bool Validate_Command PARAMS(( CONN_ID Idx, REQUEST *Req, bool *Closed ));
static bool Validate_Args PARAMS(( CONN_ID Idx, REQUEST *Req, bool *Closed ));

static bool Handle_Request PARAMS(( CONN_ID Idx, REQUEST *Req ));

static bool ScrubCTCP PARAMS((char *Request));

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
 * terminated line of text: we assume that this is a valid IRC command and
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
 * @return CONNECTED on success (valid command or "regular" error), DISCONNECTED
 *	if a fatal error occurred and the connection has been shut down.
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
	if( NGIRCd_Sniffer ) LogDebug( " <- connection %d: '%s'.", Idx, Request );
#endif

	Init_Request( &req );

	/* remove leading & trailing whitespace */
	ngt_TrimStr( Request );

	if (Conf_ScrubCTCP && ScrubCTCP(Request))
		return true;

	if (Request[0] == ':') {
		/* Prefix */
		req.prefix = Request + 1;
		ptr = strchr( Request, ' ' );
		if( ! ptr )
		{
			LogDebug("Connection %d: Parse error: prefix without command!?", Idx);
			return Conn_WriteStr(Idx, "ERROR :Prefix without command");
		}
		*ptr = '\0';
#ifndef STRICT_RFC
		/* ignore multiple spaces between prefix and command */
		while( *(ptr + 1) == ' ' ) ptr++;
#endif
		start = ptr + 1;
	}
	else start = Request;

	ptr = strchr( start, ' ' );
	if( ptr )
	{
		*ptr = '\0';
#ifndef STRICT_RFC
		/* ignore multiple spaces between parameters */
		while( *(ptr + 1) == ' ' ) ptr++;
#endif
	}
	req.command = start;

	/* Arguments, Parameters */
	if( ptr )
	{
		start = ptr + 1;
		while( start )
		{
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

	if(!Validate_Prefix(Idx, &req, &closed))
		return !closed;
	if(!Validate_Command(Idx, &req, &closed))
		return !closed;
	if(!Validate_Args(Idx, &req, &closed))
		return !closed;

	return Handle_Request(Idx, &req);
} /* Parse_Request */


/**
 * Initialize request structure.
 * @param Req Request structure to be initialized.
 */
static void
Init_Request( REQUEST *Req )
{
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

	client = Conn_GetClient( Idx );
	assert( client != NULL );

	if (!Req->prefix && Client_Type(client) == CLIENT_SERVER
	    && !(Conn_Options(Idx) & CONN_RFC1459)
	    && strcasecmp(Req->command, "ERROR") != 0
	    && strcasecmp(Req->command, "PING") != 0)
	{
		Log(LOG_ERR,
		    "Received command without prefix (connection %d, command \"%s\")!?",
		    Idx, Req->command);
		if (!Conn_WriteStr(Idx, "ERROR :Prefix missing"))
			*Closed = true;
		return false;
	}

	if (!Req->prefix)
		return true;

	/* only validate if this connection is already registered */
	if (Client_Type(client) != CLIENT_USER
	    && Client_Type(client) != CLIENT_SERVER
	    && Client_Type(client) != CLIENT_SERVICE) {
		/* not registered, ignore prefix */
		Req->prefix = NULL;
		return true;
	}

	/* check if client in prefix is known */
	c = Client_Search(Req->prefix);
	if (!c) {
		if (Client_Type(client) != CLIENT_SERVER) {
			Log(LOG_ERR,
			    "Ignoring command with invalid prefix \"%s\" from \"%s\" (connection %d, command \"%s\")!",
			    Req->prefix, Client_ID(client), Idx, Req->command);
			if (!Conn_WriteStr(Idx,
					   "ERROR :Invalid prefix \"%s\"",
					   Req->prefix))
				*Closed = true;
			IRC_SetPenalty(client, 2);
		} else
			LogDebug("Ignoring command with invalid prefix \"%s\" from \"%s\" (connection %d, command \"%s\")!",
			    Req->prefix, Client_ID(client), Idx, Req->command);
		return false;
	}

	/* check if the client named in the prefix is expected
	 * to come from that direction */
	if (Client_NextHop(c) != client) {
		if (Client_Type(client) != CLIENT_SERVER) {
			Log(LOG_ERR,
			    "Spoofed prefix \"%s\" from \"%s\" (connection %d, command \"%s\"), closing connection!",
			    Req->prefix, Client_ID(client), Idx, Req->command);
			Conn_Close(Idx, NULL, "Spoofed prefix", true);
			*Closed = true;
		} else {
			Log(LOG_WARNING,
			    "Ignoring command with spoofed prefix \"%s\" from \"%s\" (connection %d, command \"%s\")!",
			    Req->prefix, Client_ID(client), Idx, Req->command);
		}
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
} /* Validate_Command */


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
		{   5, IRC_Num_ISUPPORT },
		{  20, NULL },
		{ 376, IRC_Num_ENDOFMOTD }
	};
	int i, num;
	char str[COMMAND_LEN];
	CLIENT *prefix, *target = NULL;

	/* Determine target */
	if (Req->argc > 0) {
		if (strcmp(Req->argv[0], "*") != 0)
			target = Client_Search(Req->argv[0]);
		else
			target = Client_ThisServer();
	}

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

		for (i = 0; i < (int) C_ARRAY_SIZE(Numerics); i++) {
			if (num == Numerics[i].numeric) {
				if (!Numerics[i].function)
					return CONNECTED;
				return Numerics[i].function(client, Req);
			}
		}

		LogDebug("Ignored status code %s from \"%s\".",
			 Req->command, Client_ID(client));
		return true;
	}

	/* Determine source */
	if (!Req->prefix) {
		Log(LOG_WARNING,
		    "Got status code %s from \"%s\" without prefix!?",
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
	CLIENT *client;
	bool result = CONNECTED;
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
		if (strcasecmp(Req->command, cmd->name) != 0) {
			cmd++;
			continue;
		}

		if (!(client_type & cmd->type)) {
			if (client_type == CLIENT_USER
			    && cmd->type & CLIENT_SERVER)
				return IRC_WriteErrClient(client,
						 ERR_NOTREGISTEREDSERVER_MSG,
						 Client_ID(client));
			else
				return IRC_WriteErrClient(client,
						ERR_NOTREGISTERED_MSG,
						Client_ID(client));
		}

		if (cmd->penalty)
			IRC_SetPenalty(client, cmd->penalty);

		if (Req->argc < cmd->min_argc ||
		    (cmd->max_argc != -1 && Req->argc > cmd->max_argc))
			return IRC_WriteErrClient(client, ERR_NEEDMOREPARAMS_MSG,
						  Client_ID(client), Req->command);

		/* Command is allowed for this client: call it and count
		 * generated bytes in output */
		Conn_ResetWCounter();
		result = (cmd->function)(client, Req);
		cmd->bytes += Conn_WCounter();

		/* Adjust counters */
		if (client_type != CLIENT_SERVER)
			cmd->lcount++;
		else
			cmd->rcount++;

		/* Return result of command (CONNECTED/DISCONNECTED). */
		return result;
	}

	if (client_type != CLIENT_USER &&
	    client_type != CLIENT_SERVER &&
	    client_type != CLIENT_SERVICE )
		return true;

	LogDebug("Connection %d: Unknown command \"%s\", %d %s,%s prefix.",
			Client_Conn( client ), Req->command, Req->argc,
			Req->argc == 1 ? "parameter" : "parameters",
			Req->prefix ? "" : " no" );

	/* Unknown command and registered connection: generate error: */
	if (client_type != CLIENT_SERVER)
		result = IRC_WriteErrClient(client, ERR_UNKNOWNCOMMAND_MSG,
				Client_ID(client), Req->command);

	return result;
} /* Handle_Request */


/**
 * Check if incoming messages contains CTCP commands and should be dropped.
 *
 * @param Request NULL terminated incoming command.
 * @returns true, when the message should be dropped.
 */
static bool
ScrubCTCP(char *Request)
{
	static const char me_cmd[] = "ACTION ";
	static const char ctcp_char = 0x1;
	bool dropCommand = false;
	char *ptr = Request;
	char *ptrEnd = strchr(Request, '\0');

	if (Request[0] == ':' && ptrEnd > ptr)
		ptr++;

	while (ptr != ptrEnd && *ptr != ':')
		ptr++;

	if ((ptrEnd - ptr) > 1) {
		ptr++;
		if (*ptr == ctcp_char) {
			dropCommand = true;
			ptr++;
			/* allow /me commands */
			if ((size_t)(ptrEnd - ptr) >= strlen(me_cmd)
			    && !strncmp(ptr, me_cmd, strlen(me_cmd)))
				dropCommand = false;
		}
	}
	return dropCommand;
}

/* -eof- */
