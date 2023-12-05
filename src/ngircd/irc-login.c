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
 * Login and logout
 */

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "conn-func.h"
#include "conf.h"
#include "channel.h"
#include "log.h"
#include "login.h"
#include "messages.h"
#include "parse.h"
#include "irc.h"
#include "irc-macros.h"
#include "irc-write.h"

#include "irc-login.h"

static void Change_Nick PARAMS((CLIENT * Origin, CLIENT * Target, char *NewNick,
				bool InformClient));

/**
 * Handler for the IRC "PASS" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
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
		return IRC_WriteErrClient(Client, ERR_UNKNOWNCOMMAND_MSG,
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
		return IRC_WriteErrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);
	} else {
		/* Registered connection, PASS command is not allowed! */
		return IRC_WriteErrClient(Client, ERR_ALREADYREGISTRED_MSG,
					  Client_ID(Client));
	}

	Conn_SetPassword(Client_Conn(Client), Req->argv[0]);

	/* Protocol version */
	if (Req->argc >= 2 && strlen(Req->argv[1]) >= 4) {
		char c2, c4;

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
			    "Peer on connection %d announces itself as %s-%s using protocol %d.%d/IRC+ (flags: \"%s\").",
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
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_NICK( CLIENT *Client, REQUEST *Req )
{
	CLIENT *intr_c, *target, *c;
	CHANNEL *chan;
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
		_IRC_ARGC_EQ_OR_RETURN_(Client, Req, 1)

		/* Search "target" client */
		if (Client_Type(Client) == CLIENT_SERVER) {
			_IRC_REQUIRE_PREFIX_OR_RETURN_(Client, Req)
			target = Client_Search(Req->prefix);
			if (!target)
				return IRC_WriteErrClient(Client,
							  ERR_NOSUCHNICK_MSG,
							  Client_ID(Client),
							  Req->argv[0]);
		} else {
			/* Is this a restricted client? */
			if (Client_HasMode(Client, 'r'))
				return IRC_WriteErrClient(Client,
							  ERR_RESTRICTED_MSG,
							  Client_ID(Client));
			target = Client;
		}

#ifndef STRICT_RFC
		/* If the clients tries to change to its own nickname we won't
		 * do anything. This is how the original ircd behaves and some
		 * clients (for example Snak) expect it to be like this.
		 * But I doubt that this is "really the right thing" ... */
		if (strcmp(Client_ID(target), Req->argv[0]) == 0)
			return CONNECTED;
#endif

		/* Check that the new nickname is available. Special case:
		 * the client only changes from/to upper to lower case. */
		if (strcasecmp(Client_ID(target), Req->argv[0]) != 0) {
			if (!Client_CheckNick(target, Req->argv[0]))
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
#ifdef HAVE_ARC4RANDOM
				Conn_SetAuthPing(Client_Conn(Client), arc4random());
#else
				Conn_SetAuthPing(Client_Conn(Client), rand());
#endif
				Conn_WriteStr(Client_Conn(Client), "PING :%ld",
					Conn_GetAuthPing(Client_Conn(Client)));
				LogDebug("Connection %d: sent AUTH PING %ld ...",
					Client_Conn(Client),
					Conn_GetAuthPing(Client_Conn(Client)));
			}
#endif

			/* If we received a valid USER command already then
			 * register the new client! */
			if( Client_Type( Client ) == CLIENT_GOTUSER )
				return Login_User( Client );
			else
				Client_SetType( Client, CLIENT_GOTNICK );
		} else {
			/* Nickname change */

			/* Check that the user isn't on any channels set +N */
			if(Client_Type(Client) == CLIENT_USER &&
			   !Client_HasMode(Client, 'o')) {
				chan = Channel_First();
				while (chan) {
					if(Channel_HasMode(chan, 'N') &&
					   Channel_IsMemberOf(chan, Client))
						return IRC_WriteErrClient(Client,
									  ERR_NONICKCHANGE_MSG,
									  Client_ID(Client),
									  Channel_Name(chan));
					chan = Channel_Next(chan);
				}
			}

			Change_Nick(Client, target, Req->argv[0],
				    Client_Type(Client) == CLIENT_USER ? true : false);
			IRC_SetPenalty(target, 2);
		}

		return CONNECTED;
	} else if(Client_Type(Client) == CLIENT_SERVER ||
		  Client_Type(Client) == CLIENT_SERVICE) {
		/* Server or service introduces new client */

		/* Bad number of parameters? */
		if (Req->argc != 2 && Req->argc != 7)
			return IRC_WriteErrClient(Client, ERR_NEEDMOREPARAMS_MSG,
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
			Log(LOG_ERR,
			    "Server %s introduces already registered nick \"%s\"!",
			    Client_ID(Client), Req->argv[0]);
			return IRC_KillClient(Client, NULL, Req->argv[0],
					      "Nick collision");
		}

		/* Find the Server this client is connected to */
		intr_c = Client_GetFromToken(Client, token);
		if (!intr_c) {
			Log(LOG_ERR,
			    "Server %s introduces nick \"%s\" on unknown server!?",
			    Client_ID(Client), Req->argv[0]);
			return IRC_KillClient(Client, NULL, Req->argv[0],
					      "Unknown server");
		}

		c = Client_NewRemoteUser(intr_c, nick, hops, user, hostname,
					 token, modes, info, true);
		if (!c) {
			/* Out of memory, we need to disconnect client to keep
			 * network state consistent! */
			Log(LOG_ALERT,
			    "Can't create client structure! (on connection %d)",
			    Client_Conn(Client));
			return IRC_KillClient(Client, NULL, Req->argv[0],
					      "Server error");
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
			Client_Introduce(Client, c, CLIENT_USER);

		return CONNECTED;
	}
	else
		return IRC_WriteErrClient(Client, ERR_ALREADYREGISTRED_MSG,
					  Client_ID(Client));
} /* IRC_NICK */

/**
 * Handler for the IRC "SVSNICK" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_SVSNICK(CLIENT *Client, REQUEST *Req)
{
	CLIENT *from, *target;

	assert(Client != NULL);
	assert(Req != NULL);

	_IRC_REQUIRE_PREFIX_OR_RETURN_(Client, Req)

	/* Search the originator */
	from = Client_Search(Req->prefix);
	if (!from)
		from = Client;

	/* Search the target */
	target = Client_Search(Req->argv[0]);
	if (!target || Client_Type(target) != CLIENT_USER)
		return IRC_WriteErrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->argv[0]);

	if (Client_Conn(target) <= NONE) {
		/* We have to forward the message to the server handling
		 * this user; this is required to make sure all servers
		 * in the network do follow the nick name change! */
		return IRC_WriteStrClientPrefix(Client_NextHop(target), from,
						"SVSNICK %s %s",
						Req->argv[0], Req->argv[1]);
	}

	/* Make sure that the new nickname is valid */
	if (!Client_CheckNick(from, Req->argv[1]))
		return CONNECTED;

	Change_Nick(from, target, Req->argv[1], true);
	return CONNECTED;
}

/**
 * Handler for the IRC "USER" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_USER(CLIENT * Client, REQUEST * Req)
{
	CLIENT *c;
	char *ptr;

	assert(Client != NULL);
	assert(Req != NULL);

	if (Client_Type(Client) == CLIENT_GOTNICK ||
#ifndef STRICT_RFC
	    Client_Type(Client) == CLIENT_UNKNOWN ||
#endif
	    Client_Type(Client) == CLIENT_GOTPASS)
	{
		/* New connection */
		_IRC_ARGC_EQ_OR_RETURN_(Client, Req, 4)

		/* User name: only alphanumeric characters and limited
		   punctuation is allowed.*/
		ptr = Req->argv[0];
		while (*ptr) {
			if (!isalnum((int)*ptr) &&
			    *ptr != '+' && *ptr != '-' && *ptr != '@' &&
			    *ptr != '.' && *ptr != '_') {
				Conn_Close(Client_Conn(Client), NULL,
					   "Invalid user name", true);
				return DISCONNECTED;
			}
			ptr++;
		}

		/* Save the received username for authentication, and use
		 * it up to the first '@' as default user name (like ircd2.11,
		 * bahamut, ircd-seven, ...), prefixed with '~', if needed: */
		Client_SetOrigUser(Client, Req->argv[0]);
		ptr = strchr(Req->argv[0], '@');
		if (ptr)
			*ptr = '\0';
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
			return Login_User(Client);
		else
			Client_SetType(Client, CLIENT_GOTUSER);
		return CONNECTED;

	} else if (Client_Type(Client) == CLIENT_SERVER ||
		   Client_Type(Client) == CLIENT_SERVICE) {
		/* Server/service updating an user */
		_IRC_ARGC_EQ_OR_RETURN_(Client, Req, 4)
		_IRC_REQUIRE_PREFIX_OR_RETURN_(Client, Req)

		c = Client_Search(Req->prefix);
		if (!c)
			return IRC_WriteErrClient(Client, ERR_NOSUCHNICK_MSG,
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
			Client_Introduce(Client, c, CLIENT_USER);

		return CONNECTED;
	} else if (Client_Type(Client) == CLIENT_USER) {
		/* Already registered connection */
		return IRC_WriteErrClient(Client, ERR_ALREADYREGISTRED_MSG,
					  Client_ID(Client));
	} else {
		/* Unexpected/invalid connection state? */
		return IRC_WriteErrClient(Client, ERR_NOTREGISTERED_MSG,
					  Client_ID(Client));
	}
} /* IRC_USER */

/**
 * Handler for the IRC "SERVICE" command.
 *
 * At the moment ngIRCd doesn't support directly linked services, so this
 * function returns ERR_ERRONEUSNICKNAME when the SERVICE command has not been
 * received from a peer server.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
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
		return IRC_WriteErrClient(Client, ERR_ALREADYREGISTRED_MSG,
					  Client_ID(Client));

	if (Client_Type(Client) != CLIENT_SERVER)
		return IRC_WriteErrClient(Client, ERR_ERRONEUSNICKNAME_MSG,
				  Client_ID(Client), Req->argv[0]);

	nick = Req->argv[0];
	user = NULL; host = NULL;
	token = atoi(Req->argv[1]);
	hops = atoi(Req->argv[4]);
	info = Req->argv[5];

	/* Validate service name ("nickname") */
	c = Client_Search(nick);
	if(c) {
		/* Nickname collision: disconnect (KILL) both clients! */
		Log(LOG_ERR,
		    "Server %s introduces already registered service \"%s\"!",
		    Client_ID(Client), nick);
		return IRC_KillClient(Client, NULL, nick, "Nick collision");
	}

	/* Get the server to which the service is connected */
	intr_c = Client_GetFromToken(Client, token);
	if (! intr_c) {
		Log(LOG_ERR,
		    "Server %s introduces service \"%s\" on unknown server!?",
		    Client_ID(Client), nick);
		return IRC_KillClient(Client, NULL, nick, "Unknown server");
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
		Log(LOG_ALERT,
		    "Can't create client structure! (on connection %d)",
		    Client_Conn(Client));
		return IRC_KillClient(Client, NULL, nick, "Server error");
	}

	Client_Introduce(Client, c, CLIENT_SERVICE);
	return CONNECTED;
} /* IRC_SERVICE */

/**
 * Handler for the IRC "WEBIRC" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_WEBIRC(CLIENT *Client, REQUEST *Req)
{
	if (!Conf_WebircPwd[0] || strcmp(Req->argv[0], Conf_WebircPwd) != 0)
		return IRC_WriteErrClient(Client, ERR_PASSWDMISMATCH_MSG,
					  Client_ID(Client));

	LogDebug("Connection %d: got valid WEBIRC command: user=%s, host=%s, ip=%s",
		 Client_Conn(Client), Req->argv[1], Req->argv[2], Req->argv[3]);

	Client_SetUser(Client, Req->argv[1], true);
	Client_SetOrigUser(Client, Req->argv[1]);
	if (Conf_DNS)
		Client_SetHostname(Client, Req->argv[2]);
	else
		Client_SetHostname(Client, Req->argv[3]);
	Client_SetIPAText(Client, Req->argv[3]);

	return CONNECTED;
} /* IRC_WEBIRC */

/**
 * Handler for the IRC "QUIT" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_QUIT( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target;
	char quitmsg[COMMAND_LEN];

	assert(Client != NULL);
	assert(Req != NULL);

	if (Req->argc == 1)
		strlcpy(quitmsg, Req->argv[0], sizeof quitmsg);

	if (Client_Type(Client) == CLIENT_SERVER) {
		/* Server */
		_IRC_REQUIRE_PREFIX_OR_RETURN_(Client, Req)

		target = Client_Search(Req->prefix);
		if (!target) {
			Log(LOG_WARNING,
			    "Got QUIT from %s for unknown client!?",
			    Client_ID(Client));
			return CONNECTED;
		}

		if (target != Client) {
			Client_Destroy(target, "Got QUIT command",
				       Req->argc == 1 ? quitmsg : NULL, true);
			return CONNECTED;
		} else {
			Conn_Close(Client_Conn(Client), "Got QUIT command",
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
		Conn_Close(Client_Conn(Client), "Got QUIT command",
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
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
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
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_PING(CLIENT *Client, REQUEST *Req)
{
	CLIENT *target, *from;

	assert(Client != NULL);
	assert(Req != NULL);

	if (Req->argc < 1)
		return IRC_WriteErrClient(Client, ERR_NOORIGIN_MSG,
					  Client_ID(Client));
#ifdef STRICT_RFC
	/* Don't ignore additional arguments when in "strict" mode */
	_IRC_ARGC_LE_OR_RETURN_(Client, Req, 2)
#endif

	if (Req->argc > 1) {
		/* A target has been specified ... */
		target = Client_Search(Req->argv[1]);

		if (!target || Client_Type(target) != CLIENT_SERVER)
			return IRC_WriteErrClient(Client, ERR_NOSUCHSERVER_MSG,
					Client_ID(Client), Req->argv[1]);

		if (target != Client_ThisServer()) {
			/* Ok, we have to forward the PING */
			if (Client_Type(Client) == CLIENT_SERVER) {
				_IRC_REQUIRE_PREFIX_OR_RETURN_(Client, Req)
				from = Client_Search(Req->prefix);
			} else
				from = Client;
			if (!from)
				return IRC_WriteErrClient(Client,
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
		return IRC_WriteErrClient(Client, ERR_NOSUCHSERVER_MSG,
					Client_ID(Client), Req->prefix);

	LogDebug("Connection %d: got PING, sending PONG ...",
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
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
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
			return IRC_WriteErrClient(Client, ERR_NOORIGIN_MSG,
						  Client_ID(Client));
		else
			return CONNECTED;
	}
	if (Client_Type(Client) == CLIENT_USER) {
		_IRC_ARGC_LE_OR_RETURN_(Client, Req, 2)
	}

	/* Forward? */
	if (Req->argc == 2 && Client_Type(Client) == CLIENT_SERVER) {
		_IRC_REQUIRE_PREFIX_OR_RETURN_(Client, Req)

		target = Client_Search(Req->argv[0]);
		if (!target)
			return IRC_WriteErrClient(Client, ERR_NOSUCHSERVER_MSG,
					Client_ID(Client), Req->argv[0]);

		from = Client_Search(Req->prefix);

		if (target != Client_ThisServer() && target != from) {
			/* Ok, we have to forward the message. */
			if (!from)
				return IRC_WriteErrClient(Client,
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
		if (auth_ping == atol(Req->argv[0])) {
			Conn_SetAuthPing(conn, 0);
			if (Client_Type(Client) == CLIENT_WAITAUTHPING)
				Login_User(Client);
		} else
			if (!IRC_WriteStrClient(Client,
					"NOTICE %s :To connect, type /QUOTE PONG %ld",
					Client_ID(Client), auth_ping))
				return DISCONNECTED;
	}
#endif

	if (Client_Type(Client) == CLIENT_SERVER && Conn_LastPing(conn) == 0) {
		Log(LOG_INFO,
		    "Synchronization with \"%s\" done (connection %d): %ld second%s [%ld users, %ld channels].",
		    Client_ID(Client), conn,
		    (long)(time(NULL) - Conn_GetSignon(conn)),
		    time(NULL) - Conn_GetSignon(conn) == 1 ? "" : "s",
		    Client_UserCount(), Channel_CountVisible(NULL));
	} else {
		if (Conn_LastPing(conn) > 1)
			LogDebug("Connection %d: received PONG. Lag: %ld seconds.",
				 conn, (long)(time(NULL) - Conn_LastPing(conn)));
		else
			LogDebug("Got unexpected PONG on connection %d. Ignored.",
				 conn);
	}

	/* We got a PONG, so signal that none is pending on this connection. */
	Conn_UpdatePing(conn, 1);
	return CONNECTED;
} /* IRC_PONG */

/**
 * Change the nickname of a client.
 *
 * @param Origin The client which caused the nickname change.
 * @param Target The client of which the nickname should be changed.
 * @param NewNick The new nickname.
 */
static void
Change_Nick(CLIENT *Origin, CLIENT *Target, char *NewNick, bool InformClient)
{
	if (Client_Conn(Target) > NONE) {
		/* Local client */
		Log(LOG_INFO,
		    "%s \"%s\" changed nick (connection %d): \"%s\" -> \"%s\".",
		    Client_TypeText(Target), Client_Mask(Target),
		    Client_Conn(Target), Client_ID(Target), NewNick);
		Conn_UpdateIdle(Client_Conn(Target));
	} else {
		/* Remote client */
		LogDebug("%s \"%s\" changed nick: \"%s\" -> \"%s\".",
			 Client_TypeText(Target),
			 Client_Mask(Target), Client_ID(Target), NewNick);
	}

	/* Inform all servers and users (which have to know) of the new name */
	if (InformClient) {
		IRC_WriteStrClientPrefix(Target, Target, "NICK :%s", NewNick);
		IRC_WriteStrServersPrefix(NULL, Target, "NICK :%s", NewNick);
	} else
		IRC_WriteStrServersPrefix(Origin, Target, "NICK :%s", NewNick);
	IRC_WriteStrRelatedPrefix(Target, Target, false, "NICK :%s", NewNick);

	/* Register old nickname for WHOWAS queries */
	Client_RegisterWhowas(Target);

	/* Save new nickname */
	Client_SetID(Target, NewNick);
}

/* -eof- */
