/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2008 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Login and logout
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "ngircd.h"
#include "resolve.h"
#include "conn-func.h"
#include "conf.h"
#include "client.h"
#include "channel.h"
#include "log.h"
#include "messages.h"
#include "parse.h"
#include "irc.h"
#include "irc-info.h"
#include "irc-write.h"

#include "exp.h"
#include "irc-login.h"


static bool Hello_User PARAMS(( CLIENT *Client ));
static void Kill_Nick PARAMS(( char *Nick, char *Reason ));


/**
 * Handler for the IRC command "PASS".
 * See RFC 2813 section 4.1.1, and RFC 2812 section 3.1.1.
 */
GLOBAL bool
IRC_PASS( CLIENT *Client, REQUEST *Req )
{
	char *type, *orig_flags;
	int protohigh, protolow;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Return an error if this is not a local client */
	if (Client_Conn(Client) <= NONE)
		return IRC_WriteStrClient(Client, ERR_UNKNOWNCOMMAND_MSG,
					  Client_ID(Client), Req->command);

	if (Client_Type(Client) == CLIENT_UNKNOWN && Req->argc == 1) {
		/* Not yet registered "unknown" connection, PASS with one
		 * argument: either a regular client, service, or server
		 * using the old RFC 1459 section 4.1.1 syntax. */
		LogDebug("Connection %d: got PASS command ...",
			 Client_Conn(Client));
	} else if ((Client_Type(Client) == CLIENT_UNKNOWN ||
		    Client_Type(Client) == CLIENT_UNKNOWNSERVER) &&
		   (Req->argc == 3 || Req->argc == 4)) {
		/* Not yet registered "unknown" connection or outgoing server
		 * link, PASS with three or four argument: server using the
		 * RFC 2813 section 4.1.1 syntax. */
		LogDebug("Connection %d: got PASS command (new server link) ...",
			 Client_Conn(Client));
	} else if (Client_Type(Client) == CLIENT_UNKNOWN ||
		   Client_Type(Client) == CLIENT_UNKNOWNSERVER) {
		/* Unregistered connection, but wrong number of arguments: */
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);
	} else {
		/* Registered connection, PASS command is not allowed! */
		return IRC_WriteStrClient(Client, ERR_ALREADYREGISTRED_MSG,
					  Client_ID(Client));
	}

	Client_SetPassword(Client, Req->argv[0]);
	Client_SetType(Client, CLIENT_GOTPASS);

	/* Protocol version */
	if (Req->argc >= 2 && strlen(Req->argv[1]) >= 4) {
		int c2, c4;

		c2 = Req->argv[1][2];
		c4 = Req->argv[1][4];

		Req->argv[1][4] = '\0';
		protolow = atoi(&Req->argv[1][2]);
		Req->argv[1][2] = '\0';
		protohigh = atoi(Req->argv[1]);

		Req->argv[1][2] = c2;
		Req->argv[1][4] = c4;
	} else
		protohigh = protolow = 0;

	/* Protocol type, see doc/Protocol.txt */
	if (Req->argc >= 2 && strlen(Req->argv[1]) > 4)
		type = &Req->argv[1][4];
	else
		type = NULL;

	/* Protocol flags/options */
	if (Req->argc >= 4)
		orig_flags = Req->argv[3];
	else
		orig_flags = "";

	/* Implementation, version and IRC+ flags */
	if (Req->argc >= 3) {
		char *impl, *ptr, *serverver, *flags;

		impl = Req->argv[2];
		ptr = strchr(impl, '|');
		if (ptr)
			*ptr = '\0';

		if (type && strcmp(type, PROTOIRCPLUS) == 0) {
			/* The peer seems to be a server which supports the
			 * IRC+ protocol (see doc/Protocol.txt). */
			serverver = ptr + 1;
			flags = strchr(serverver, ':');
			if (flags) {
				*flags = '\0';
				flags++;
			} else
				flags = "";
			Log(LOG_INFO,
			    "Peer announces itself as %s-%s using protocol %d.%d/IRC+ (flags: \"%s\").",
			    impl, serverver, protohigh, protolow, flags);
		} else {
			/* The peer seems to be a server supporting the
			 * "original" IRC protocol (RFC 2813). */
			serverver = "";
			if (strchr(orig_flags, 'Z'))
				flags = "Z";
			else
				flags = "";
			Log(LOG_INFO,
			    "Peer announces itself as \"%s\" using protocol %d.%d (flags: \"%s\").",
			    impl, protohigh, protolow, flags);
		}
		Client_SetFlags(Client, flags);
	}

	return CONNECTED;
} /* IRC_PASS */


/**
 * IRC "NICK" command.
 * This function implements the IRC command "NICK" which is used to register
 * with the server, to change already registered nicknames and to introduce
 * new users which are connected to other servers.
 */
GLOBAL bool
IRC_NICK( CLIENT *Client, REQUEST *Req )
{
	CLIENT *intr_c, *target, *c;
	CONN_ID conn;
	char *nick, *user, *hostname, *modes, *info;
	int token, hops;

	assert( Client != NULL );
	assert( Req != NULL );

#ifndef STRICT_RFC
	/* Some IRC clients, for example BitchX, send the NICK and USER
	 * commands in the wrong order ... */
	if( Client_Type( Client ) == CLIENT_UNKNOWN
	    || Client_Type( Client ) == CLIENT_GOTPASS
	    || Client_Type( Client ) == CLIENT_GOTNICK
	    || Client_Type( Client ) == CLIENT_GOTUSER
	    || Client_Type( Client ) == CLIENT_USER
	    || ( Client_Type( Client ) == CLIENT_SERVER && Req->argc == 1 ))
#else
	if( Client_Type( Client ) == CLIENT_UNKNOWN
	    || Client_Type( Client ) == CLIENT_GOTPASS
	    || Client_Type( Client ) == CLIENT_GOTNICK
	    || Client_Type( Client ) == CLIENT_USER
	    || ( Client_Type( Client ) == CLIENT_SERVER && Req->argc == 1 ))
#endif
	{
		/* User registration or change of nickname */

		/* Wrong number of arguments? */
		if( Req->argc != 1 )
			return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG,
						   Client_ID( Client ),
						   Req->command );

		/* Search "target" client */
		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			target = Client_Search( Req->prefix );
			if( ! target )
				return IRC_WriteStrClient( Client,
							   ERR_NOSUCHNICK_MSG,
							   Client_ID( Client ),
							   Req->argv[0] );
		}
		else
		{
			/* Is this a restricted client? */
			if( Client_HasMode( Client, 'r' ))
				return IRC_WriteStrClient( Client,
							   ERR_RESTRICTED_MSG,
							   Client_ID( Client ));

			target = Client;
		}

#ifndef STRICT_RFC
		/* If the clients tries to change to its own nickname we won't
		 * do anything. This is how the original ircd behaves and some
		 * clients (for example Snak) expect it to be like this.
		 * But I doubt that this is "really the right thing" ... */
		if( strcmp( Client_ID( target ), Req->argv[0] ) == 0 )
			return CONNECTED;
#endif

		/* Check that the new nickname is available. Special case:
		 * the client only changes from/to upper to lower case. */
		if( strcasecmp( Client_ID( target ), Req->argv[0] ) != 0 )
		{
			if( ! Client_CheckNick( target, Req->argv[0] ))
				return CONNECTED;
		}

		if(( Client_Type( target ) != CLIENT_USER )
		   && ( Client_Type( target ) != CLIENT_SERVER ))
		{
			/* New client */
			Log( LOG_DEBUG, "Connection %d: got valid NICK command ...", 
			     Client_Conn( Client ));

			/* Register new nickname of this client */
			Client_SetID( target, Req->argv[0] );

			/* If we received a valid USER command already then
			 * register the new client! */
			if( Client_Type( Client ) == CLIENT_GOTUSER )
				return Hello_User( Client );
			else
				Client_SetType( Client, CLIENT_GOTNICK );
		}
		else
		{
			/* Nickname change */
			if (Client_Conn(target) > NONE) {
				/* Local client */
				Log(LOG_INFO,
				    "User \"%s\" changed nick (connection %d): \"%s\" -> \"%s\".",
				    Client_Mask(target), Client_Conn(target),
				    Client_ID(target), Req->argv[0]);
				Conn_UpdateIdle(Client_Conn(target));
			}
			else
			{
				/* Remote client */
				Log( LOG_DEBUG,
				     "User \"%s\" changed nick: \"%s\" -> \"%s\".",
				     Client_Mask( target ), Client_ID( target ),
				     Req->argv[0] );
			}

			/* Inform all users and servers (which have to know)
			 * of this nickname change */
			if( Client_Type( Client ) == CLIENT_USER )
				IRC_WriteStrClientPrefix( Client, Client,
							  "NICK :%s",
							  Req->argv[0] );
			IRC_WriteStrServersPrefix( Client, target,
						   "NICK :%s", Req->argv[0] );
			IRC_WriteStrRelatedPrefix( target, target, false,
						   "NICK :%s", Req->argv[0] );

			/* Register old nickname for WHOWAS queries */
			Client_RegisterWhowas( target );

			/* Save new nickname */
			Client_SetID( target, Req->argv[0] );

			IRC_SetPenalty( target, 2 );
		}

		return CONNECTED;
	} else if(Client_Type(Client) == CLIENT_SERVER ||
		  Client_Type(Client) == CLIENT_SERVICE) {
		/* Server or service introduces new client */

		/* Bad number of parameters? */
		if (Req->argc != 2 && Req->argc != 7)
			return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
						  Client_ID(Client), Req->command);

		if (Req->argc >= 7) {
			/* RFC 2813 compatible syntax */
			nick = Req->argv[0];
			hops = atoi(Req->argv[1]);
			user = Req->argv[2];
			hostname = Req->argv[3];
			token = atoi(Req->argv[4]);
			modes = Req->argv[5] + 1;
			info = Req->argv[6];
		} else {
			/* RFC 1459 compatible syntax */
			nick = Req->argv[0];
			hops = 1;
			user = Req->argv[0];
			hostname = Client_ID(Client);
			token = atoi(Req->argv[1]);
			modes = "";
			info = Req->argv[0];

			conn = Client_Conn(Client);
			if (conn != NONE &&
			    !(Conn_Options(conn) & CONN_RFC1459)) {
				Log(LOG_INFO,
				    "Switching connection %d (\"%s\") to RFC 1459 compatibility mode.",
				    conn, Client_ID(Client));
				Conn_SetOption(conn, CONN_RFC1459);
			}
		}

		/* Nick ueberpruefen */
		c = Client_Search(nick);
		if(c) {
			/* Der neue Nick ist auf diesem Server bereits registriert:
			 * sowohl der neue, als auch der alte Client muessen nun
			 * disconnectiert werden. */
			Log( LOG_ERR, "Server %s introduces already registered nick \"%s\"!", Client_ID( Client ), Req->argv[0] );
			Kill_Nick( Req->argv[0], "Nick collision" );
			return CONNECTED;
		}

		/* Server, zu dem der Client connectiert ist, suchen */
		intr_c = Client_GetFromToken(Client, token);
		if( ! intr_c )
		{
			Log( LOG_ERR, "Server %s introduces nick \"%s\" on unknown server!?", Client_ID( Client ), Req->argv[0] );
			Kill_Nick( Req->argv[0], "Unknown server" );
			return CONNECTED;
		}

		/* Neue Client-Struktur anlegen */
		c = Client_NewRemoteUser(intr_c, nick, hops, user, hostname,
					 token, modes, info, true);
		if( ! c )
		{
			/* Eine neue Client-Struktur konnte nicht angelegt werden.
			 * Der Client muss disconnectiert werden, damit der Netz-
			 * status konsistent bleibt. */
			Log( LOG_ALERT, "Can't create client structure! (on connection %d)", Client_Conn( Client ));
			Kill_Nick( Req->argv[0], "Server error" );
			return CONNECTED;
		}

		modes = Client_Modes( c );
		if( *modes ) Log( LOG_DEBUG, "User \"%s\" (+%s) registered (via %s, on %s, %d hop%s).", Client_Mask( c ), modes, Client_ID( Client ), Client_ID( intr_c ), Client_Hops( c ), Client_Hops( c ) > 1 ? "s": "" );
		else Log( LOG_DEBUG, "User \"%s\" registered (via %s, on %s, %d hop%s).", Client_Mask( c ), Client_ID( Client ), Client_ID( intr_c ), Client_Hops( c ), Client_Hops( c ) > 1 ? "s": "" );

		/* Andere Server, ausser dem Introducer, informieren */
		IRC_WriteStrServersPrefix( Client, Client, "NICK %s %d %s %s %d %s :%s", Req->argv[0], atoi( Req->argv[1] ) + 1, Req->argv[2], Req->argv[3], Client_MyToken( intr_c ), Req->argv[5], Req->argv[6] );

		return CONNECTED;
	}
	else return IRC_WriteStrClient( Client, ERR_ALREADYREGISTRED_MSG, Client_ID( Client ));
} /* IRC_NICK */


/**
 * Handler for the IRC command "USER".
 */
GLOBAL bool
IRC_USER(CLIENT * Client, REQUEST * Req)
{
	CLIENT *c;
#ifdef IDENTAUTH
	char *ptr;
#endif

	assert(Client != NULL);
	assert(Req != NULL);

	if (Client_Type(Client) == CLIENT_GOTNICK ||
#ifndef STRICT_RFC
	    Client_Type(Client) == CLIENT_UNKNOWN ||
#endif
	    Client_Type(Client) == CLIENT_GOTPASS)
	{
		/* New connection */
		if (Req->argc != 4)
			return IRC_WriteStrClient(Client,
						  ERR_NEEDMOREPARAMS_MSG,
						  Client_ID(Client),
						  Req->command);

		/* User name */
#ifdef IDENTAUTH
		ptr = Client_User(Client);
		if (!ptr || !*ptr || *ptr == '~')
			Client_SetUser(Client, Req->argv[0], false);
#else
		Client_SetUser(Client, Req->argv[0], false);
#endif

		/* "Real name" or user info text: Don't set it to the empty
		 * string, the original ircd can't deal with such "real names"
		 * (e. g. "USER user * * :") ... */
		if (*Req->argv[3])
			Client_SetInfo(Client, Req->argv[3]);
		else
			Client_SetInfo(Client, "-");

		LogDebug("Connection %d: got valid USER command ...",
		    Client_Conn(Client));
		if (Client_Type(Client) == CLIENT_GOTNICK)
			return Hello_User(Client);
		else
			Client_SetType(Client, CLIENT_GOTUSER);
		return CONNECTED;

	} else if (Client_Type(Client) == CLIENT_SERVER ||
		   Client_Type(Client) == CLIENT_SERVICE) {
		/* Server/service updating an user */
		if (Req->argc != 4)
			return IRC_WriteStrClient(Client,
						  ERR_NEEDMOREPARAMS_MSG,
						  Client_ID(Client),
						  Req->command);
		c = Client_Search(Req->prefix);
		if (!c)
			return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
						  Client_ID(Client),
						  Req->prefix);

		Client_SetUser(c, Req->argv[0], true);
		Client_SetHostname(c, Req->argv[1]);
		Client_SetInfo(c, Req->argv[3]);

		LogDebug("Connection %d: got valid USER command for \"%s\".",
			 Client_Conn(Client), Client_Mask(c));
		return CONNECTED;
	} else if (Client_Type(Client) == CLIENT_USER) {
		/* Already registered connection */
		return IRC_WriteStrClient(Client, ERR_ALREADYREGISTRED_MSG,
					  Client_ID(Client));
	} else {
		/* Unexpected/invalid connection state? */
		return IRC_WriteStrClient(Client, ERR_NOTREGISTERED_MSG,
					  Client_ID(Client));
	}
} /* IRC_USER */


/**
 * Service registration.
 * ngIRCd does not support services at the moment, so this function is a
 * dummy that returns ERR_ERRONEUSNICKNAME on each call.
 */
GLOBAL bool
IRC_SERVICE(CLIENT *Client, REQUEST *Req)
{
	assert(Client != NULL);
	assert(Req != NULL);

	if (Client_Type(Client) != CLIENT_GOTPASS)
		return IRC_WriteStrClient(Client, ERR_ALREADYREGISTRED_MSG,
					  Client_ID(Client));

	if (Req->argc != 6)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	return IRC_WriteStrClient(Client, ERR_ERRONEUSNICKNAME_MSG,
				  Client_ID(Client), Req->argv[0]);
} /* IRC_SERVICE */


GLOBAL bool
IRC_QUIT( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target;
	char quitmsg[LINE_LEN];

	assert( Client != NULL );
	assert( Req != NULL );

	/* Wrong number of arguments? */
	if( Req->argc > 1 )
		return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if (Req->argc == 1)
		strlcpy(quitmsg, Req->argv[0], sizeof quitmsg);

	if ( Client_Type( Client ) == CLIENT_SERVER )
	{
		/* Server */
		target = Client_Search( Req->prefix );
		if( ! target )
		{
			/* Den Client kennen wir nicht (mehr), also nichts zu tun. */
			Log( LOG_WARNING, "Got QUIT from %s for unknown client!?", Client_ID( Client ));
			return CONNECTED;
		}

		Client_Destroy( target, "Got QUIT command.", Req->argc == 1 ? quitmsg : NULL, true);

		return CONNECTED;
	}
	else
	{
		if (Req->argc == 1 && quitmsg[0] != '\"') {
			/* " " to avoid confusion */
			strlcpy(quitmsg, "\"", sizeof quitmsg);
			strlcat(quitmsg, Req->argv[0], sizeof quitmsg-1);
			strlcat(quitmsg, "\"", sizeof quitmsg );
		}

		/* User, Service, oder noch nicht registriert */
		Conn_Close( Client_Conn( Client ), "Got QUIT command.", Req->argc == 1 ? quitmsg : NULL, true);

		return DISCONNECTED;
	}
} /* IRC_QUIT */


GLOBAL bool
IRC_PING(CLIENT *Client, REQUEST *Req)
{
	CLIENT *target, *from;

	assert(Client != NULL);
	assert(Req != NULL);

	/* Wrong number of arguments? */
	if (Req->argc < 1)
		return IRC_WriteStrClient(Client, ERR_NOORIGIN_MSG,
					  Client_ID(Client));
#ifdef STRICT_RFC
	/* Don't ignore additional arguments when in "strict" mode */
	if (Req->argc > 2)
		 return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					   Client_ID(Client), Req->command);
#endif

	if (Req->argc > 1) {
		/* A target has been specified ... */
		target = Client_Search(Req->argv[1]);

		if (!target || Client_Type(target) != CLIENT_SERVER)
			return IRC_WriteStrClient(Client, ERR_NOSUCHSERVER_MSG,
					Client_ID(Client), Req->argv[1]);

		if (target != Client_ThisServer()) {
			/* Ok, we have to forward the PING */
			if (Client_Type(Client) == CLIENT_SERVER)
				from = Client_Search(Req->prefix);
			else
				from = Client;
			if (!from)
				return IRC_WriteStrClient(Client,
						ERR_NOSUCHSERVER_MSG,
						Client_ID(Client), Req->prefix);

			return IRC_WriteStrClientPrefix(target, from,
					"PING %s :%s", Req->argv[0],
					Req->argv[1] );
		}
	}

	if (Client_Type(Client) == CLIENT_SERVER) {
		if (Req->prefix)
			from = Client_Search(Req->prefix);
		else
			from = Client;
	} else
		from = Client_ThisServer();
	if (!from)
		return IRC_WriteStrClient(Client, ERR_NOSUCHSERVER_MSG,
					Client_ID(Client), Req->prefix);

	Log(LOG_DEBUG, "Connection %d: got PING, sending PONG ...",
	    Client_Conn(Client));

#ifdef STRICT_RFC
	return IRC_WriteStrClient(Client, "PONG %s :%s",
		Client_ID(from), Client_ID(Client));
#else
	/* Some clients depend on the argument being returned in the PONG
	 * reply (not mentioned in any RFC, though) */
	return IRC_WriteStrClient(Client, "PONG %s :%s",
		Client_ID(from), Req->argv[0]);
#endif
} /* IRC_PING */


GLOBAL bool
IRC_PONG(CLIENT *Client, REQUEST *Req)
{
	CLIENT *target, *from;
	char *s;

	assert(Client != NULL);
	assert(Req != NULL);

	/* Wrong number of arguments? */
	if (Req->argc < 1)
		return IRC_WriteStrClient(Client, ERR_NOORIGIN_MSG,
					  Client_ID(Client));
	if (Req->argc > 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	/* Forward? */
	if (Req->argc == 2 && Client_Type(Client) == CLIENT_SERVER) {
		target = Client_Search(Req->argv[0]);
		if (!target)
			return IRC_WriteStrClient(Client, ERR_NOSUCHSERVER_MSG,
					Client_ID(Client), Req->argv[0]);

		from = Client_Search(Req->prefix);

		if (target != Client_ThisServer() && target != from) {
			/* Ok, we have to forward the message. */
			if (!from)
				return IRC_WriteStrClient(Client,
						ERR_NOSUCHSERVER_MSG,
						Client_ID(Client), Req->prefix);

			if (Client_Type(Client_NextHop(target)) != CLIENT_SERVER)
				s = Client_ID(from);
			else
				s = Req->argv[0];
			return IRC_WriteStrClientPrefix(target, from,
				 "PONG %s :%s", s, Req->argv[1]);
		}
	}

	/* The connection timestamp has already been updated when the data has
	 * been read from so socket, so we don't need to update it here. */

	if (Client_Conn(Client) > NONE)
		Log(LOG_DEBUG,
			"Connection %d: received PONG. Lag: %ld seconds.",
			Client_Conn(Client),
			time(NULL) - Conn_LastPing(Client_Conn(Client)));
	else
		 Log(LOG_DEBUG,
			"Connection %d: received PONG.", Client_Conn(Client));

	return CONNECTED;
} /* IRC_PONG */


static bool
Hello_User(CLIENT * Client)
{
	assert(Client != NULL);

	/* Check password ... */
	if (strcmp(Client_Password(Client), Conf_ServerPwd) != 0) {
		/* Bad password! */
		Log(LOG_ERR,
		    "User \"%s\" rejected (connection %d): Bad password!",
		    Client_Mask(Client), Client_Conn(Client));
		Conn_Close(Client_Conn(Client), NULL, "Bad password", true);
		return DISCONNECTED;
	}

	Log(LOG_NOTICE, "User \"%s\" registered (connection %d).",
	    Client_Mask(Client), Client_Conn(Client));

	/* Inform other servers */
	IRC_WriteStrServers(NULL, "NICK %s 1 %s %s 1 +%s :%s",
			    Client_ID(Client), Client_User(Client),
			    Client_Hostname(Client), Client_Modes(Client),
			    Client_Info(Client));

	if (!IRC_WriteStrClient
	    (Client, RPL_WELCOME_MSG, Client_ID(Client), Client_Mask(Client)))
		return false;
	if (!IRC_WriteStrClient
	    (Client, RPL_YOURHOST_MSG, Client_ID(Client),
	     Client_ID(Client_ThisServer()), PACKAGE_VERSION, TARGET_CPU,
	     TARGET_VENDOR, TARGET_OS))
		return false;
	if (!IRC_WriteStrClient
	    (Client, RPL_CREATED_MSG, Client_ID(Client), NGIRCd_StartStr))
		return false;
	if (!IRC_WriteStrClient
	    (Client, RPL_MYINFO_MSG, Client_ID(Client),
	     Client_ID(Client_ThisServer()), PACKAGE_VERSION, USERMODES,
	     CHANMODES))
		return false;

	/* Features supported by this server (005 numeric, ISUPPORT),
	 * see <http://www.irc.org/tech_docs/005.html> for details. */
	if (!IRC_Send_ISUPPORT(Client))
		return DISCONNECTED;

	Client_SetType(Client, CLIENT_USER);

	if (!IRC_Send_LUSERS(Client))
		return DISCONNECTED;
	if (!IRC_Show_MOTD(Client))
		return DISCONNECTED;

	/* Suspend the client for a second ... */
	IRC_SetPenalty(Client, 1);

	return CONNECTED;
} /* Hello_User */


static void
Kill_Nick( char *Nick, char *Reason )
{
	REQUEST r;

	assert( Nick != NULL );
	assert( Reason != NULL );

	r.prefix = (char *)Client_ThisServer( );
	r.argv[0] = Nick;
	r.argv[1] = Reason;
	r.argc = 2;

	Log( LOG_ERR, "User(s) with nick \"%s\" will be disconnected: %s", Nick, Reason );
	IRC_KILL( Client_ThisServer( ), &r );
} /* Kill_Nick */


/* -eof- */
