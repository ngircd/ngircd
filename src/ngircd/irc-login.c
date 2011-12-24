/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2011 Alexander Barton (alex@barton.de) and Contributors.
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
 * Login and logout
 */

#include "imp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>

#include "ngircd.h"
#include "conn-func.h"
#include "class.h"
#include "conf.h"
#include "channel.h"
#include "io.h"
#include "log.h"
#include "messages.h"
#include "pam.h"
#include "parse.h"
#include "irc.h"
#include "irc-info.h"
#include "irc-write.h"

#include "exp.h"
#include "irc-login.h"


static bool Hello_User PARAMS(( CLIENT *Client ));
static bool Hello_User_PostAuth PARAMS(( CLIENT *Client ));
static void Kill_Nick PARAMS(( char *Nick, char *Reason ));
static void Introduce_Client PARAMS((CLIENT *To, CLIENT *Client, int Type));
static void Reject_Client PARAMS((CLIENT *Client));

static void cb_introduceClient PARAMS((CLIENT *Client, CLIENT *Prefix,
				       void *i));

#ifdef PAM
static void cb_Read_Auth_Result PARAMS((int r_fd, UNUSED short events));
#endif

/**
 * Handler for the IRC "PASS" command.
 *
 * See RFC 2813 section 4.1.1, and RFC 2812 section 3.1.1.
 *
 * @param Client	The client from which this command has been received.
 * @param Req		Request structure with prefix and all parameters.
 * @returns		CONNECTED or DISCONNECTED.
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
		LogDebug("Connection %d: got PASS command (RFC 1459) ...",
			 Client_Conn(Client));
	} else if ((Client_Type(Client) == CLIENT_UNKNOWN ||
		    Client_Type(Client) == CLIENT_UNKNOWNSERVER) &&
		   (Req->argc == 3 || Req->argc == 4)) {
		/* Not yet registered "unknown" connection or outgoing server
		 * link, PASS with three or four argument: server using the
		 * RFC 2813 section 4.1.1 syntax. */
		LogDebug("Connection %d: got PASS command (RFC 2813, new server link) ...",
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

		Client_SetType(Client, CLIENT_GOTPASS_2813);
	} else {
		protohigh = protolow = 0;
		Client_SetType(Client, CLIENT_GOTPASS);
	}

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
			serverver = ptr ? ptr + 1 : "?";
			flags = strchr(ptr ? serverver : impl, ':');
			if (flags) {
				*flags = '\0';
				flags++;
			} else
				flags = "";
			Log(LOG_INFO,
			    "Peer on conenction %d announces itself as %s-%s using protocol %d.%d/IRC+ (flags: \"%s\").",
			    Client_Conn(Client), impl, serverver,
			    protohigh, protolow, flags);
		} else {
			/* The peer seems to be a server supporting the
			 * "original" IRC protocol (RFC 2813). */
			if (strchr(orig_flags, 'Z'))
				flags = "Z";
			else
				flags = "";
			Log(LOG_INFO,
			    "Peer on connection %d announces itself as \"%s\" using protocol %d.%d (flags: \"%s\").",
			    Client_Conn(Client), impl,
			    protohigh, protolow, flags);
		}
		Client_SetFlags(Client, flags);
	}

	return CONNECTED;
} /* IRC_PASS */


/**
 * Handler for the IRC "NICK" command.
 *
 * See RFC 2812, 3.1.2 "Nick message", and RFC 2813, 4.1.3 "Nick".
 *
 * This function implements the IRC command "NICK" which is used to register
 * with the server, to change already registered nicknames and to introduce
 * new users which are connected to other servers.
 *
 * @param Client	The client from which this command has been received.
 * @param Req		Request structure with prefix and all parameters.
 * @returns		CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_NICK( CLIENT *Client, REQUEST *Req )
{
	CLIENT *intr_c, *target, *c;
	char *nick, *user, *hostname, *modes, *info;
	int token, hops;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Some IRC clients, for example BitchX, send the NICK and USER
	 * commands in the wrong order ... */
	if(Client_Type(Client) == CLIENT_UNKNOWN
	    || Client_Type(Client) == CLIENT_GOTPASS
	    || Client_Type(Client) == CLIENT_GOTNICK
#ifndef STRICT_RFC
	    || Client_Type(Client) == CLIENT_GOTUSER
#endif
	    || Client_Type(Client) == CLIENT_USER
	    || Client_Type(Client) == CLIENT_SERVICE
	    || (Client_Type(Client) == CLIENT_SERVER && Req->argc == 1))
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

		if (Client_Type(target) != CLIENT_USER &&
		    Client_Type(target) != CLIENT_SERVICE &&
		    Client_Type(target) != CLIENT_SERVER) {
			/* New client */
			LogDebug("Connection %d: got valid NICK command ...",
			     Client_Conn( Client ));

			/* Register new nickname of this client */
			Client_SetID( target, Req->argv[0] );

#ifndef STRICT_RFC
			if (Conf_AuthPing) {
				Conn_SetAuthPing(Client_Conn(Client), rand());
				IRC_WriteStrClient(Client, "PING :%ld",
					Conn_GetAuthPing(Client_Conn(Client)));
				LogDebug("Connection %d: sent AUTH PING %ld ...",
					Client_Conn(Client),
					Conn_GetAuthPing(Client_Conn(Client)));
			}
#endif

			/* If we received a valid USER command already then
			 * register the new client! */
			if( Client_Type( Client ) == CLIENT_GOTUSER )
				return Hello_User( Client );
			else
				Client_SetType( Client, CLIENT_GOTNICK );
		} else {
			/* Nickname change */
			if (Client_Conn(target) > NONE) {
				/* Local client */
				Log(LOG_INFO,
				    "%s \"%s\" changed nick (connection %d): \"%s\" -> \"%s\".",
				    Client_TypeText(target), Client_Mask(target),
				    Client_Conn(target), Client_ID(target),
				    Req->argv[0]);
				Conn_UpdateIdle(Client_Conn(target));
			} else {
				/* Remote client */
				LogDebug("%s \"%s\" changed nick: \"%s\" -> \"%s\".",
					 Client_TypeText(target),
					 Client_Mask(target), Client_ID(target),
					 Req->argv[0]);
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
		}

		c = Client_Search(nick);
		if(c) {
			/*
			 * the new nick is already present on this server:
			 * the new and the old one have to be disconnected now.
			 */
			Log( LOG_ERR, "Server %s introduces already registered nick \"%s\"!", Client_ID( Client ), Req->argv[0] );
			Kill_Nick( Req->argv[0], "Nick collision" );
			return CONNECTED;
		}

		/* Find the Server this client is connected to */
		intr_c = Client_GetFromToken(Client, token);
		if( ! intr_c )
		{
			Log( LOG_ERR, "Server %s introduces nick \"%s\" on unknown server!?", Client_ID( Client ), Req->argv[0] );
			Kill_Nick( Req->argv[0], "Unknown server" );
			return CONNECTED;
		}

		c = Client_NewRemoteUser(intr_c, nick, hops, user, hostname,
					 token, modes, info, true);
		if( ! c )
		{
			/* out of memory, need to disconnect client to keep network state consistent */
			Log( LOG_ALERT, "Can't create client structure! (on connection %d)", Client_Conn( Client ));
			Kill_Nick( Req->argv[0], "Server error" );
			return CONNECTED;
		}

		/* RFC 2813: client is now fully registered, inform all the
		 * other servers about the new user.
		 * RFC 1459: announce the new client only after receiving the
		 * USER command, first we need more information! */
		if (Req->argc < 7) {
			LogDebug("Client \"%s\" is being registered (RFC 1459) ...",
				 Client_Mask(c));
			Client_SetType(c, CLIENT_GOTNICK);
		} else
			Introduce_Client(Client, c, CLIENT_USER);

		return CONNECTED;
	}
	else return IRC_WriteStrClient( Client, ERR_ALREADYREGISTRED_MSG, Client_ID( Client ));
} /* IRC_NICK */


/**
 * Handler for the IRC "USER" command.
 *
 * See RFC 2812, 3.1.3 "User message".
 *
 * @param Client	The client from which this command has been received.
 * @param Req		Request structure with prefix and all parameters.
 * @returns		CONNECTED or DISCONNECTED.
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
		Client_SetOrigUser(Client, Req->argv[0]);

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
		Client_SetOrigUser(c, Req->argv[0]);
		Client_SetHostname(c, Req->argv[1]);
		Client_SetInfo(c, Req->argv[3]);

		LogDebug("Connection %d: got valid USER command for \"%s\".",
			 Client_Conn(Client), Client_Mask(c));

		/* RFC 1459 style user registration?
		 * Introduce client to network: */
		if (Client_Type(c) == CLIENT_GOTNICK)
			Introduce_Client(Client, c, CLIENT_USER);

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
 * Handler for the IRC "SERVICE" command.
 *
 * This function implements IRC Services registration using the SERVICE command
 * defined in RFC 2812 3.1.6 and RFC 2813 4.1.4.
 *
 * At the moment ngIRCd doesn't support directly linked services, so this
 * function returns ERR_ERRONEUSNICKNAME when the SERVICE command has not been
 * received from a peer server.
 *
 * @param Client	The client from which this command has been received.
 * @param Req		Request structure with prefix and all parameters.
 * @returns		CONNECTED or DISCONNECTED..
 */
GLOBAL bool
IRC_SERVICE(CLIENT *Client, REQUEST *Req)
{
	CLIENT *c, *intr_c;
	char *nick, *user, *host, *info, *modes, *ptr;
	int token, hops;

	assert(Client != NULL);
	assert(Req != NULL);

	if (Client_Type(Client) != CLIENT_GOTPASS &&
	    Client_Type(Client) != CLIENT_SERVER)
		return IRC_WriteStrClient(Client, ERR_ALREADYREGISTRED_MSG,
					  Client_ID(Client));

	if (Req->argc != 6)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	if (Client_Type(Client) != CLIENT_SERVER)
		return IRC_WriteStrClient(Client, ERR_ERRONEUSNICKNAME_MSG,
				  Client_ID(Client), Req->argv[0]);

	/* Bad number of parameters? */
	if (Req->argc != 6)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	nick = Req->argv[0];
	user = NULL; host = NULL;
	token = atoi(Req->argv[1]);
	hops = atoi(Req->argv[4]);
	info = Req->argv[5];

	/* Validate service name ("nick name") */
	c = Client_Search(nick);
	if(c) {
		/* Nick name collission: disconnect (KILL) both clients! */
		Log(LOG_ERR, "Server %s introduces already registered service \"%s\"!",
		    Client_ID(Client), nick);
		Kill_Nick(nick, "Nick collision");
		return CONNECTED;
	}

	/* Get the server to which the service is connected */
	intr_c = Client_GetFromToken(Client, token);
	if (! intr_c) {
		Log(LOG_ERR, "Server %s introduces service \"%s\" on unknown server!?",
		    Client_ID(Client), nick);
		Kill_Nick(nick, "Unknown server");
		return CONNECTED;
	}

	/* Get user and host name */
	ptr = strchr(nick, '@');
	if (ptr) {
		*ptr = '\0';
		host = ++ptr;
	}
	if (!host)
		host = Client_Hostname(intr_c);
	ptr = strchr(nick, '!');
	if (ptr) {
		*ptr = '\0';
		user = ++ptr;
	}
	if (!user)
		user = nick;

	/* According to RFC 2812/2813 parameter 4 <type> "is currently reserved
	 * for future usage"; but we use it to transfer the modes and check
	 * that the first character is a '+' sign and ignore it otherwise. */
	modes = (Req->argv[3][0] == '+') ? ++Req->argv[3] : "";

	c = Client_NewRemoteUser(intr_c, nick, hops, user, host,
				 token, modes, info, true);
	if (! c) {
		/* Couldn't create client structure, so KILL the service to
		 * keep network status consistent ... */
		Log(LOG_ALERT, "Can't create client structure! (on connection %d)",
		    Client_Conn(Client));
		Kill_Nick(nick, "Server error");
		return CONNECTED;
	}

	Introduce_Client(Client, c, CLIENT_SERVICE);
	return CONNECTED;
} /* IRC_SERVICE */


/**
 * Handler for the IRC "WEBIRC" command.
 *
 * See doc/Protocol.txt, section II.4:
 * "Update webchat/proxy client information".
 *
 * @param Client	The client from which this command has been received.
 * @param Req		Request structure with prefix and all parameters.
 * @returns		CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_WEBIRC(CLIENT *Client, REQUEST *Req)
{
	/* Exactly 4 parameters are requited */
	if (Req->argc != 4)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	if (!Conf_WebircPwd[0] || strcmp(Req->argv[0], Conf_WebircPwd) != 0)
		return IRC_WriteStrClient(Client, ERR_PASSWDMISMATCH_MSG,
					  Client_ID(Client));

	LogDebug("Connection %d: got valid WEBIRC command: user=%s, host=%s, ip=%s",
		 Client_Conn(Client), Req->argv[1], Req->argv[2], Req->argv[3]);

	Client_SetUser(Client, Req->argv[1], true);
	Client_SetOrigUser(Client, Req->argv[1]);
	Client_SetHostname(Client, Req->argv[2]);
	return CONNECTED;
} /* IRC_WEBIRC */


/**
 * Handler for the IRC "QUIT" command.
 *
 * See RFC 2812, 3.1.7 "Quit", and RFC 2813, 4.1.5 "Quit".
 *
 * @param Client	The client from which this command has been received.
 * @param Req		Request structure with prefix and all parameters.
 * @returns		CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_QUIT( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target;
	char quitmsg[LINE_LEN];

	assert(Client != NULL);
	assert(Req != NULL);

	/* Wrong number of arguments? */
	if (Req->argc > 1)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	if (Req->argc == 1)
		strlcpy(quitmsg, Req->argv[0], sizeof quitmsg);

	if (Client_Type(Client) == CLIENT_SERVER) {
		/* Server */
		target = Client_Search(Req->prefix);
		if (!target) {
			Log(LOG_WARNING,
			    "Got QUIT from %s for unknown client!?",
			    Client_ID(Client));
			return CONNECTED;
		}

		if (target != Client) {
			Client_Destroy(target, "Got QUIT command.",
				       Req->argc == 1 ? quitmsg : NULL, true);
			return CONNECTED;
		} else {
			Conn_Close(Client_Conn(Client), "Got QUIT command.",
				   Req->argc == 1 ? quitmsg : NULL, true);
			return DISCONNECTED;
		}
	} else {
		if (Req->argc == 1 && quitmsg[0] != '\"') {
			/* " " to avoid confusion */
			strlcpy(quitmsg, "\"", sizeof quitmsg);
			strlcat(quitmsg, Req->argv[0], sizeof quitmsg-1);
			strlcat(quitmsg, "\"", sizeof quitmsg );
		}

		/* User, Service, or not yet registered */
		Conn_Close(Client_Conn(Client), "Got QUIT command.",
			   Req->argc == 1 ? quitmsg : NULL, true);

		return DISCONNECTED;
	}
} /* IRC_QUIT */


#ifndef STRICT_RFC

/**
 * Handler for HTTP command, e.g. GET and POST
 *
 * We handle these commands here to avoid the quite long timeout when
 * some user tries to access this IRC daemon using an web browser ...
 *
 * @param Client	The client from which this command has been received.
 * @param Req		Request structure with prefix and all parameters.
 * @returns		CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_QUIT_HTTP( CLIENT *Client, REQUEST *Req )
{
	Req->argc = 1;
	Req->argv[0] = "Oops, HTTP request received? This is IRC!";
	return IRC_QUIT(Client, Req);
} /* IRC_QUIT_HTTP */

#endif


/**
 * Handler for the IRC "PING" command.
 *
 * See RFC 2812, 3.7.2 "Ping message".
 *
 * @param Client	The client from which this command has been received.
 * @param Req		Request structure with prefix and all parameters.
 * @returns		CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_PING(CLIENT *Client, REQUEST *Req)
{
	CLIENT *target, *from;

	assert(Client != NULL);
	assert(Req != NULL);

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


/**
 * Handler for the IRC "PONG" command.
 *
 * See RFC 2812, 3.7.3 "Pong message".
 *
 * @param Client	The client from which this command has been received.
 * @param Req		Request structure with prefix and all parameters.
 * @returns		CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_PONG(CLIENT *Client, REQUEST *Req)
{
	CLIENT *target, *from;
	CONN_ID conn;
#ifndef STRICT_RFC
	long auth_ping;
#endif
	char *s;

	assert(Client != NULL);
	assert(Req != NULL);

	/* Wrong number of arguments? */
	if (Req->argc < 1) {
		if (Client_Type(Client) == CLIENT_USER)
			return IRC_WriteStrClient(Client, ERR_NOORIGIN_MSG,
						  Client_ID(Client));
		else
			return CONNECTED;
	}
	if (Req->argc > 2) {
		if (Client_Type(Client) == CLIENT_USER)
			return IRC_WriteStrClient(Client,
						  ERR_NEEDMOREPARAMS_MSG,
						  Client_ID(Client),
						  Req->command);
		else
			return CONNECTED;
	}

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

	conn = Client_Conn(Client);

#ifndef STRICT_RFC
	/* Check authentication PING-PONG ... */
	auth_ping = Conn_GetAuthPing(conn);
	if (auth_ping) {
		LogDebug("AUTH PONG: waiting for token \"%ld\", got \"%s\" ...",
			 auth_ping, Req->argv[0]);
		if (auth_ping == atoi(Req->argv[0])) {
			Conn_SetAuthPing(conn, 0);
			if (Client_Type(Client) == CLIENT_WAITAUTHPING)
				Hello_User(Client);
		} else
			if (!IRC_WriteStrClient(Client,
					"To connect, type /QUOTE PONG %ld",
					auth_ping))
				return DISCONNECTED;
	}
#endif

#ifdef DEBUG
	if (conn > NONE)
		Log(LOG_DEBUG,
			"Connection %d: received PONG. Lag: %ld seconds.", conn,
			time(NULL) - Conn_LastPing(Client_Conn(Client)));
	else
		 Log(LOG_DEBUG,
			"Connection %d: received PONG.", conn);
#endif
	return CONNECTED;
} /* IRC_PONG */


/**
 * Initiate client registration.
 *
 * This function is called after the daemon received the required NICK and
 * USER commands of a new client. If the daemon is compiled with support for
 * PAM, the authentication sub-processs is forked; otherwise the global server
 * password is checked.
 *
 * @param Client	The client logging in.
 * @returns		CONNECTED or DISCONNECTED.
 */
static bool
Hello_User(CLIENT * Client)
{
#ifdef PAM
	int pipefd[2], result;
	pid_t pid;
#endif
	CONN_ID conn;

	assert(Client != NULL);
	conn = Client_Conn(Client);

#ifndef STRICT_RFC
	if (Conf_AuthPing) {
		/* Did we receive the "auth PONG" already? */
		if (Conn_GetAuthPing(conn)) {
			Client_SetType(Client, CLIENT_WAITAUTHPING);
			LogDebug("Connection %d: Waiting for AUTH PONG ...", conn);
			return CONNECTED;
		}
	}
#endif

	if (Class_IsMember(CLASS_GLINE, Client) ||
	    Class_IsMember(CLASS_KLINE, Client)) {
		Reject_Client(Client);
		return DISCONNECTED;
	}

#ifdef PAM
	if (!Conf_PAM) {
		/* Don't do any PAM authentication at all, instead emulate
		 * the beahiour of the daemon compiled without PAM support:
		 * because there can't be any "server password", all
		 * passwords supplied are classified as "wrong". */
		if(Client_Password(Client)[0] == '\0')
			return Hello_User_PostAuth(Client);
		Reject_Client(Client);
		return DISCONNECTED;
	}

	/* Fork child process for PAM authentication; and make sure that the
	 * process timeout is set higher than the login timeout! */
	pid = Proc_Fork(Conn_GetProcStat(conn), pipefd,
			cb_Read_Auth_Result, Conf_PongTimeout + 1);
	if (pid > 0) {
		LogDebug("Authenticator for connection %d created (PID %d).",
			 conn, pid);
		return CONNECTED;
	} else {
		/* Sub process */
		Log_Init_Subprocess("Auth");
		Conn_CloseAllSockets(NONE);
		result = PAM_Authenticate(Client);
		if (write(pipefd[1], &result, sizeof(result)) != sizeof(result))
			Log_Subprocess(LOG_ERR,
				       "Failed to pipe result to parent!");
		Log_Exit_Subprocess("Auth");
		exit(0);
	}
#else
	/* Check global server password ... */
	if (strcmp(Client_Password(Client), Conf_ServerPwd) != 0) {
		/* Bad password! */
		Reject_Client(Client);
		return DISCONNECTED;
	}
	return Hello_User_PostAuth(Client);
#endif
}


#ifdef PAM

/**
 * Read result of the authenticatior sub-process from pipe
 *
 * @param r_fd		File descriptor of the pipe.
 * @param events	(ignored IO specification)
 */
static void
cb_Read_Auth_Result(int r_fd, UNUSED short events)
{
	CONN_ID conn;
	CLIENT *client;
	int result;
	size_t len;
	PROC_STAT *proc;

	LogDebug("Auth: Got callback on fd %d, events %d", r_fd, events);
	conn = Conn_GetFromProc(r_fd);
	if (conn == NONE) {
		/* Ops, none found? Probably the connection has already
		 * been closed!? We'll ignore that ... */
		io_close(r_fd);
		LogDebug("Auth: Got callback for unknown connection!?");
		return;
	}
	proc = Conn_GetProcStat(conn);
	client = Conn_GetClient(conn);

	/* Read result from pipe */
	len = Proc_Read(proc, &result, sizeof(result));
	Proc_Close(proc);
	if (len == 0)
		return;

	if (len != sizeof(result)) {
		Log(LOG_CRIT, "Auth: Got malformed result!");
		Reject_Client(client);
		return;
	}

	if (result == true) {
		Client_SetUser(client, Client_OrigUser(client), true);
		(void)Hello_User_PostAuth(client);
	} else
		Reject_Client(client);
}

#endif


/**
 * Reject a client because of wrong password.
 *
 * This function is called either when the global server password or a password
 * checked using PAM has been wrong.
 *
 * @param Client	The client to reject.
 */
static void
Reject_Client(CLIENT *Client)
{
	Log(LOG_ERR,
	    "User \"%s\" rejected (connection %d): Access denied!",
	    Client_Mask(Client), Client_Conn(Client));
	Conn_Close(Client_Conn(Client), NULL,
		   "Access denied! Bad password?", true);
}


/**
 * Finish client registration.
 *
 * Introduce the new client to the network and send all "hello messages"
 * to it after authentication has been succeeded.
 *
 * @param Client	The client logging in.
 * @returns		CONNECTED or DISCONNECTED.
 */
static bool
Hello_User_PostAuth(CLIENT *Client)
{
	Introduce_Client(NULL, Client, CLIENT_USER);

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

	if (!IRC_Send_LUSERS(Client))
		return DISCONNECTED;
	if (!IRC_Show_MOTD(Client))
		return DISCONNECTED;

	/* Suspend the client for a second ... */
	IRC_SetPenalty(Client, 1);

	return CONNECTED;
}


/**
 * Kill all users with a specific nick name in the network.
 *
 * @param Nick		Nick name.
 * @param Reason	Reason for the KILL.
 */
static void
Kill_Nick(char *Nick, char *Reason)
{
	REQUEST r;

	assert (Nick != NULL);
	assert (Reason != NULL);

	r.prefix = NULL;
	r.argv[0] = Nick;
	r.argv[1] = Reason;
	r.argc = 2;

	Log(LOG_ERR, "User(s) with nick \"%s\" will be disconnected: %s",
	    Nick, Reason);

	IRC_KILL(Client_ThisServer(), &r);
} /* Kill_Nick */


/**
 * Introduce a new user or service client in the network.
 *
 * @param From		Remote server introducing the client or NULL (local).
 * @param Client	New client.
 * @param Type		Type of the client (CLIENT_USER or CLIENT_SERVICE).
 */
static void
Introduce_Client(CLIENT *From, CLIENT *Client, int Type)
{
	/* Set client type (user or service) */
	Client_SetType(Client, Type);

	if (From) {
		if (Conf_IsService(Conf_GetServer(Client_Conn(From)),
				   Client_ID(Client)))
			Client_SetType(Client, CLIENT_SERVICE);
		LogDebug("%s \"%s\" (+%s) registered (via %s, on %s, %d hop%s).",
			 Client_TypeText(Client), Client_Mask(Client),
			 Client_Modes(Client), Client_ID(From),
			 Client_ID(Client_Introducer(Client)),
			 Client_Hops(Client), Client_Hops(Client) > 1 ? "s": "");
	} else {
		Log(LOG_NOTICE, "%s \"%s\" registered (connection %d).",
		    Client_TypeText(Client), Client_Mask(Client),
		    Client_Conn(Client));
		Log_ServerNotice('c', "Client connecting: %s (%s@%s) [%s] - %s",
			         Client_ID(Client), Client_User(Client),
				 Client_Hostname(Client),
				 Conn_IPA(Client_Conn(Client)),
				 Client_TypeText(Client));
	}

	/* Inform other servers */
	IRC_WriteStrServersPrefixFlag_CB(From,
				From != NULL ? From : Client_ThisServer(),
				'\0', cb_introduceClient, (void *)Client);
} /* Introduce_Client */


/**
 * Introduce a new user or service client to a remote server.
 *
 * This function differentiates between RFC1459 and RFC2813 server links and
 * generates the appropriate commands to register the new user or service.
 *
 * @param To		The remote server to inform.
 * @param Prefix	Prefix for the generated commands.
 * @param data		CLIENT structure of the new client.
 */
static void
cb_introduceClient(CLIENT *To, CLIENT *Prefix, void *data)
{
	CLIENT *c = (CLIENT *)data;
	CONN_ID conn;
	char *modes, *user, *host;

	modes = Client_Modes(c);
	user = Client_User(c) ? Client_User(c) : "-";
	host = Client_Hostname(c) ? Client_Hostname(c) : "-";

	conn = Client_Conn(To);
	if (Conn_Options(conn) & CONN_RFC1459) {
		/* RFC 1459 mode: separate NICK and USER commands */
		Conn_WriteStr(conn, "NICK %s :%d", Client_ID(c),
			      Client_Hops(c) + 1);
		Conn_WriteStr(conn, ":%s USER %s %s %s :%s",
			      Client_ID(c), user, host,
			      Client_ID(Client_Introducer(c)), Client_Info(c));
		if (modes[0])
			Conn_WriteStr(conn, ":%s MODE %s +%s",
				      Client_ID(c), Client_ID(c), modes);
	} else {
		/* RFC 2813 mode: one combined NICK or SERVICE command */
		if (Client_Type(c) == CLIENT_SERVICE
		    && strchr(Client_Flags(To), 'S'))
			IRC_WriteStrClientPrefix(To, Prefix,
					 "SERVICE %s %d * +%s %d :%s",
					 Client_Mask(c),
					 Client_MyToken(Client_Introducer(c)),
					 Client_Modes(c), Client_Hops(c) + 1,
					 Client_Info(c));
		else
			IRC_WriteStrClientPrefix(To, Prefix,
					 "NICK %s %d %s %s %d +%s :%s",
					 Client_ID(c), Client_Hops(c) + 1,
					 user, host,
					 Client_MyToken(Client_Introducer(c)),
					 modes, Client_Info(c));
	}
} /* cb_introduceClient */


/* -eof- */
