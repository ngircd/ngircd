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
 * IRC commands
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "ngircd.h"
#include "conn-func.h"
#include "conf.h"
#include "channel.h"
#ifdef ICONV
# include "conn-encoding.h"
#endif
#include "irc-macros.h"
#include "irc-write.h"
#include "log.h"
#include "match.h"
#include "messages.h"
#include "parse.h"
#include "op.h"

#include "irc.h"

static char *Option_String PARAMS((CONN_ID Idx));
static bool Send_Message PARAMS((CLIENT *Client, REQUEST *Req, int ForceType,
				 bool SendErrors));
static bool Send_Message_Mask PARAMS((CLIENT *from, char *command,
				      char *targetMask, char *message,
				      bool SendErrors));
static bool Help PARAMS((CLIENT *Client, const char *Topic));

/**
 * Check if a list limit is reached and inform client accordingly.
 *
 * @param From The client.
 * @param Count Reply item count.
 * @param Limit Reply limit.
 * @param Name Name of the list.
 * @return true if list limit has been reached; false otherwise.
 */
GLOBAL bool
IRC_CheckListTooBig(CLIENT *From, const int Count, const int Limit,
		    const char *Name)
{
	assert(From != NULL);
	assert(Count >= 0);
	assert(Limit > 0);
	assert(Name != NULL);

	if (Count < Limit)
		return false;

	(void)IRC_WriteStrClient(From,
				 "NOTICE %s :%s list limit (%d) reached!",
				 Client_ID(From), Name, Limit);
	IRC_SetPenalty(From, 2);
	return true;
}

/**
 * Handler for the IRC "ERROR" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
*/
GLOBAL bool
IRC_ERROR(CLIENT *Client, REQUEST *Req)
{
	char *msg;

	assert( Client != NULL );
	assert( Req != NULL );

	if (Client_Type(Client) != CLIENT_GOTPASS
	    && Client_Type(Client) != CLIENT_GOTPASS_2813
	    && Client_Type(Client) != CLIENT_UNKNOWNSERVER
	    && Client_Type(Client) != CLIENT_SERVER
	    && Client_Type(Client) != CLIENT_SERVICE) {
		LogDebug("Ignored ERROR command from \"%s\" ...",
			 Client_Mask(Client));
		IRC_SetPenalty(Client, 2);
		return CONNECTED;
	}

	if (Req->argc < 1) {
		msg = "Got ERROR command";
		Log(LOG_NOTICE, "Got ERROR from \"%s\"!",
		    Client_Mask(Client));
	} else {
		msg = Req->argv[0];
		Log(LOG_NOTICE, "Got ERROR from \"%s\": \"%s\"!",
		    Client_Mask(Client), msg);
	}

	if (Client_Conn(Client) != NONE) {
		Conn_Close(Client_Conn(Client), NULL, msg, false);
		return DISCONNECTED;
	}

	return CONNECTED;
} /* IRC_ERROR */

/**
 * Handler for the IRC "KILL" command.
 *
 * This function implements the IRC command "KILL" which is used to selectively
 * disconnect clients. It can be used by IRC operators and servers, for example
 * to "solve" nick collisions after netsplits. See RFC 2812 section 3.7.1.
 *
 * Please note that this function is also called internally, without a real
 * KILL command being received over the network! Client is Client_ThisServer()
 * in this case, and the prefix in Req is NULL.
 *
 * @param Client The client from which this command has been received or
 * Client_ThisServer() when generated internally.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_KILL(CLIENT *Client, REQUEST *Req)
{
	CLIENT *prefix;
	char reason[COMMAND_LEN];

	assert (Client != NULL);
	assert (Req != NULL);

	if (Client_Type(Client) != CLIENT_SERVER && !Op_Check(Client, Req))
		return Op_NoPrivileges(Client, Req);

	/* Get prefix (origin); use the client if no prefix is given. */
	if (Req->prefix)
		prefix = Client_Search(Req->prefix);
	else
		prefix = Client;

	/* Log a warning message and use this server as origin when the
	 * prefix (origin) is invalid. And this is the reason why we don't
	 * use the _IRC_GET_SENDER_OR_RETURN_ macro above! */
	if (!prefix) {
		Log(LOG_WARNING, "Got KILL with invalid prefix: \"%s\"!",
		    Req->prefix );
		prefix = Client_ThisServer();
	}

	if (Client != Client_ThisServer())
		Log(LOG_NOTICE|LOG_snotice,
		    "Got KILL command from \"%s\" for \"%s\": \"%s\".",
		    Client_Mask(prefix), Req->argv[0], Req->argv[1]);

	/* Build reason string: Prefix the "reason" if the originator is a
	 * regular user, so users can't spoof KILLs of servers. */
	if (Client_Type(Client) == CLIENT_USER)
		snprintf(reason, sizeof(reason), "KILLed by %s: %s",
			 Client_ID(Client), Req->argv[1]);
	else
		strlcpy(reason, Req->argv[1], sizeof(reason));

	return IRC_KillClient(Client, prefix, Req->argv[0], reason);
}

/**
 * Handler for the IRC "NOTICE" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
*/
GLOBAL bool
IRC_NOTICE(CLIENT *Client, REQUEST *Req)
{
	return Send_Message(Client, Req, CLIENT_USER, false);
} /* IRC_NOTICE */

/**
 * Handler for the IRC "PRIVMSG" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_PRIVMSG(CLIENT *Client, REQUEST *Req)
{
	return Send_Message(Client, Req, CLIENT_USER, true);
} /* IRC_PRIVMSG */

/**
 * Handler for the IRC "SQUERY" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_SQUERY(CLIENT *Client, REQUEST *Req)
{
	return Send_Message(Client, Req, CLIENT_SERVICE, true);
} /* IRC_SQUERY */

/*
 * Handler for the IRC "TRACE" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
 GLOBAL bool
IRC_TRACE(CLIENT *Client, REQUEST *Req)
{
	CLIENT *from, *target, *c;
	CONN_ID idx, idx2;
	char user[CLIENT_USER_LEN];

	assert(Client != NULL);
	assert(Req != NULL);

	_IRC_GET_SENDER_OR_RETURN_(from, Req, Client)
	_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 0, from)

	/* Forward command to other server? */
	if (target != Client_ThisServer()) {
		/* Send RPL_TRACELINK back to initiator */
		idx = Client_Conn(Client);
		assert(idx > NONE);
		idx2 = Client_Conn(Client_NextHop(target));
		assert(idx2 > NONE);

		if (!IRC_WriteStrClient(from, RPL_TRACELINK_MSG,
					Client_ID(from), PACKAGE_NAME,
					PACKAGE_VERSION, Client_ID(target),
					Client_ID(Client_NextHop(target)),
					Option_String(idx2),
					(long)(time(NULL) - Conn_StartTime(idx2)),
					Conn_SendQ(idx), Conn_SendQ(idx2)))
			return DISCONNECTED;

		/* Forward command */
		IRC_WriteStrClientPrefix(target, from, "TRACE %s", Req->argv[0]);
		return CONNECTED;
	}

	/* Infos about all connected servers */
	c = Client_First();
	while (c) {
		if (Client_Conn(c) > NONE) {
			/* Local client */
			if (Client_Type(c) == CLIENT_SERVER) {
				/* Server link */
				strlcpy(user, Client_User(c), sizeof(user));
				if (user[0] == '~')
					strlcpy(user, "unknown", sizeof(user));
				if (!IRC_WriteStrClient(from,
						RPL_TRACESERVER_MSG,
						Client_ID(from), Client_ID(c),
						user, Client_Hostname(c),
						Client_Mask(Client_ThisServer()),
						Option_String(Client_Conn(c))))
					return DISCONNECTED;
			}
			if (Client_Type(c) == CLIENT_USER
			    && Client_HasMode(c, 'o')) {
				/* IRC Operator */
				if (!IRC_WriteStrClient(from,
						RPL_TRACEOPERATOR_MSG,
						Client_ID(from), Client_ID(c)))
					return DISCONNECTED;
			}
		}
		c = Client_Next( c );
	}

	return IRC_WriteStrClient(from, RPL_TRACEEND_MSG, Client_ID(from),
				  Conf_ServerName, PACKAGE_NAME,
				  PACKAGE_VERSION, NGIRCd_DebugLevel);
} /* IRC_TRACE */

/**
 * Handler for the IRC "HELP" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_HELP(CLIENT *Client, REQUEST *Req)
{
	COMMAND *cmd;

	assert(Client != NULL);
	assert(Req != NULL);

	if ((Req->argc == 0 && array_bytes(&Conf_Helptext) > 0)
	    || (Req->argc >= 1 && strcasecmp(Req->argv[0], "Commands") != 0)) {
		/* Help text available and requested */
		if (Req->argc >= 1)
			return Help(Client, Req->argv[0]);

		if (!Help(Client, "Intro"))
			return DISCONNECTED;
		return CONNECTED;
	}

	cmd = Parse_GetCommandStruct();
	while(cmd->name) {
		if (!IRC_WriteStrClient(Client, "NOTICE %s :%s",
					Client_ID(Client), cmd->name))
			return DISCONNECTED;
		cmd++;
	}
	return CONNECTED;
} /* IRC_HELP */

/**
 * Kill an client identified by its nick name.
 *
 * Please note that after killig a client, its CLIENT cond CONNECTION
 * structures are invalid. So the caller must make sure on its own not to
 * access data of probably killed clients after calling this function!
 *
 * @param Client The client from which the command leading to the KILL has
 *		been received, or NULL. The KILL will no be forwarded in this
 *		direction. Only relevant when From is set, too.
 * @param From The client from which the command originated, or NULL for
		the local server.
 * @param Nick The nick name to kill.
 * @param Reason Text to send as reason to the client and other servers.
 */
GLOBAL bool
IRC_KillClient(CLIENT *Client, CLIENT *From, const char *Nick, const char *Reason)
{
	const char *msg;
	CONN_ID my_conn = NONE, conn;
	CLIENT *c;

	assert(Nick != NULL);
	assert(Reason != NULL);

	/* Do we know such a client in the network? */
	c = Client_Search(Nick);
	if (!c) {
		LogDebug("Client with nick \"%s\" is unknown, not forwarding.", Nick);
		return CONNECTED;
	}

	if (Client_Type(c) != CLIENT_USER && Client_Type(c) != CLIENT_GOTNICK
	    && Client_Type(c) != CLIENT_SERVICE) {
		/* Target of this KILL is not a regular user, this is
		 * invalid! So we ignore this case if we received a
		 * regular KILL from the network and try to kill the
		 * client/connection anyway (but log an error!) if the
		 * origin is the local server. */

		if (Client != Client_ThisServer()) {
			/* Invalid KILL received from remote */
			if (Client_Type(c) == CLIENT_SERVER)
				msg = ERR_CANTKILLSERVER_MSG;
			else
				msg = ERR_NOPRIVILEGES_MSG;
			return IRC_WriteErrClient(Client, msg, Client_ID(Client));
		}

		Log(LOG_ERR,
		    "Got KILL for invalid client type: %d, \"%s\"!",
		    Client_Type(c), Nick);
	}

	/* Inform other servers */
	IRC_WriteStrServersPrefix(From ? Client : NULL,
				  From ? From : Client_ThisServer(),
				  "KILL %s :%s", Nick, Reason);


	/* Save ID of this connection */
	if (Client)
		my_conn = Client_Conn(Client);

	/* Kill the client NOW:
	 *  - Close the local connection (if there is one),
	 *  - Destroy the CLIENT structure for remote clients.
	 * Note: Conn_Close() removes the CLIENT structure as well. */
	conn = Client_Conn(c);
	if(conn > NONE)
		Conn_Close(conn, NULL, Reason, true);
	else
		Client_Destroy(c, NULL, Reason, false);

	/* Are we still connected or were we killed, too? */
	if (my_conn > NONE && Conn_GetClient(my_conn))
		return CONNECTED;
	else
		return DISCONNECTED;
}

/**
 * Send help for a given topic to the client.
 *
 * @param Client The client requesting help.
 * @param Topic The help topic requested.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
Help(CLIENT *Client, const char *Topic)
{
	char *line;
	size_t helptext_len, len_str, idx_start, lines = 0;
	bool in_article = false;

	assert(Client != NULL);
	assert(Topic != NULL);

	helptext_len = array_bytes(&Conf_Helptext);
	line = array_start(&Conf_Helptext);
	while (helptext_len > 0) {
		len_str = strlen(line) + 1;
		assert(helptext_len >= len_str);
		helptext_len -= len_str;

		if (in_article) {
			/* The first character in each article text line must
			 * be a TAB (ASCII 9) character which will be stripped
			 * in the output. If it is not a TAB, the end of the
			 * article has been reached. */
			if (line[0] != '\t') {
				if (lines > 0)
					return CONNECTED;
				else
					break;
			}

			/* A single '.' character indicates an empty line */
			if (line[1] == '.' && line[2] == '\0')
				idx_start = 2;
			else
				idx_start = 1;

			if (!IRC_WriteStrClient(Client, "NOTICE %s :%s",
						Client_ID(Client),
						&line[idx_start]))
				return DISCONNECTED;
			lines++;

		} else {
			if (line[0] == '-' && line[1] == ' '
			    && strcasecmp(&line[2], Topic) == 0)
				in_article = true;
		}

		line += len_str;
	}

	/* Help topic not found (or empty)! */
	if (!IRC_WriteStrClient(Client, "NOTICE %s :No help for \"%s\" found!",
				Client_ID(Client), Topic))
		return DISCONNECTED;

	return CONNECTED;
}

/**
 * Get pointer to a static string representing the connection "options".
 *
 * @param Idx Connection index.
 * @return Pointer to static (global) string buffer.
 */
static char *
#if defined(SSL_SUPPORT) || defined(ZLIB)
Option_String(CONN_ID Idx)
{
	static char option_txt[8];
	UINT16 options;

	assert(Idx != NONE);

	options = Conn_Options(Idx);
	strcpy(option_txt, "F");	/* No idea what this means, but the
					 * original ircd sends it ... */
#ifdef SSL_SUPPORT
	if(options & CONN_SSL)		/* SSL encrypted link */
		strlcat(option_txt, "s", sizeof(option_txt));
#endif
#ifdef ZLIB
	if(options & CONN_ZIP)		/* zlib compression enabled */
		strlcat(option_txt, "z", sizeof(option_txt));
#endif

	return option_txt;
#else
Option_String(UNUSED CONN_ID Idx)
{
	return "";
#endif
} /* Option_String */

/**
 * Send a message to target(s).
 *
 * This function is used by IRC_{PRIVMSG|NOTICE|SQUERY} to actually
 * send the message(s).
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @param ForceType Required type of the destination of the message(s).
 * @param SendErrors Whether to report errors back to the client or not.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
Send_Message(CLIENT * Client, REQUEST * Req, int ForceType, bool SendErrors)
{
	CLIENT *cl, *from;
	CL2CHAN *cl2chan;
	CHANNEL *chan;
	char *currentTarget = Req->argv[0];
	char *strtok_last = NULL;
	char *message = NULL;
	char *targets[MAX_HNDL_TARGETS];
	int i, target_nr = 0;

	assert(Client != NULL);
	assert(Req != NULL);

	if (Req->argc == 0) {
		if (!SendErrors)
			return CONNECTED;
		return IRC_WriteErrClient(Client, ERR_NORECIPIENT_MSG,
					  Client_ID(Client), Req->command);
	}
	if (Req->argc == 1) {
		if (!SendErrors)
			return CONNECTED;
		return IRC_WriteErrClient(Client, ERR_NOTEXTTOSEND_MSG,
					  Client_ID(Client));
	}
	if (Req->argc > 2) {
		if (!SendErrors)
			return CONNECTED;
		return IRC_WriteErrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);
	}

	if (Client_Type(Client) == CLIENT_SERVER && Req->prefix)
		from = Client_Search(Req->prefix);
	else
		from = Client;
	if (!from)
		return IRC_WriteErrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->prefix);

#ifdef ICONV
	if (Client_Conn(Client) > NONE)
		message = Conn_EncodingFrom(Client_Conn(Client), Req->argv[1]);
	else
#endif
		message = Req->argv[1];

	if (message[0] == '\0') {
		if (!SendErrors)
			return CONNECTED;
		return IRC_WriteErrClient(Client, ERR_NOTEXTTOSEND_MSG,
					  Client_ID(Client));
	}

	/* handle msgtarget = msgto *("," msgto) */
	currentTarget = strtok_r(currentTarget, ",", &strtok_last);
	ngt_UpperStr(Req->command);

	/* Please note that "currentTarget" is NULL when the target contains
	 * the separator character only, e. g. "," or ",,,," etc.! */
	while (currentTarget) {
		/* Make sure that there hasn't been such a target already: */
		targets[target_nr++] = currentTarget;
		for(i = 0; i < target_nr - 1; i++) {
			if (strcasecmp(currentTarget, targets[i]) == 0)
				goto send_next_target;
		}

		/* Check for and handle valid <msgto> of form:
		 * RFC 2812 2.3.1:
		 *   msgto =  channel / ( user [ "%" host ] "@" servername )
		 *   msgto =/ ( user "%" host ) / targetmask
		 *   msgto =/ nickname / ( nickname "!" user "@" host )
		 */
		if (strchr(currentTarget, '!') == NULL)
			/* nickname */
			cl = Client_Search(currentTarget);
		else
			cl = NULL;

		if (cl == NULL) {
			/* If currentTarget isn't a nickname check for:
			 * user ["%" host] "@" servername
			 * user "%" host
			 * nickname "!" user "@" host
			 */
			char target[COMMAND_LEN];
			char * nick = NULL;
			char * user = NULL;
			char * host = NULL;
			char * server = NULL;

			strlcpy(target, currentTarget, COMMAND_LEN);
			server = strchr(target, '@');
			if (server) {
				*server = '\0';
				server++;
			}
			host = strchr(target, '%');
			if (host) {
				*host = '\0';
				host++;
			}
			user = strchr(target, '!');
			if (user) {
				/* msgto form: nick!user@host */
				*user = '\0';
				user++;
				nick = target;
				host = server; /* not "@server" but "@host" */
			} else {
				user = target;
			}

			for (cl = Client_First(); cl != NULL; cl = Client_Next(cl)) {
				if (Client_Type(cl) != CLIENT_USER &&
				    Client_Type(cl) != CLIENT_SERVICE)
					continue;
				if (nick != NULL && host != NULL) {
					if (strcasecmp(nick, Client_ID(cl)) == 0 &&
					    strcasecmp(user, Client_User(cl)) == 0 &&
					    strcasecmp(host, Client_HostnameDisplayed(cl)) == 0)
						break;
					else
						continue;
				}
				if (strcasecmp(user, Client_User(cl)) != 0)
					continue;
				if (host != NULL && strcasecmp(host,
						Client_HostnameDisplayed(cl)) != 0)
					continue;
				if (server != NULL && strcasecmp(server,
						Client_ID(Client_Introducer(cl))) != 0)
					continue;
				break;
			}
		}

		if (cl) {
			/* Target is a user, enforce type */
#ifndef STRICT_RFC
			if (Client_Type(cl) != ForceType &&
			    !(ForceType == CLIENT_USER &&
			      (Client_Type(cl) == CLIENT_USER ||
			       Client_Type(cl) == CLIENT_SERVICE))) {
#else
			if (Client_Type(cl) != ForceType) {
#endif
				if (SendErrors && !IRC_WriteErrClient(
				    from, ERR_NOSUCHNICK_MSG,Client_ID(from),
				    currentTarget))
					return DISCONNECTED;
				goto send_next_target;
			}

#ifndef STRICT_RFC
			if (ForceType == CLIENT_SERVICE &&
			    (Conn_Options(Client_Conn(Client_NextHop(cl)))
			     & CONN_RFC1459)) {
				/* SQUERY command but RFC 1459 link: convert
				 * request to PRIVMSG command */
				Req->command = "PRIVMSG";
			}
#endif
			if (Client_HasMode(cl, 'b') &&
			    !Client_HasMode(from, 'R') &&
			    !Client_HasMode(from, 'o') &&
			    !(Client_Type(from) == CLIENT_SERVER) &&
			    !(Client_Type(from) == CLIENT_SERVICE)) {
				if (SendErrors && !IRC_WriteErrClient(from,
						ERR_NONONREG_MSG,
						Client_ID(from), Client_ID(cl)))
					return DISCONNECTED;
				goto send_next_target;
			}

			if (Client_HasMode(cl, 'C') &&
			    !Client_HasMode(from, 'o') &&
			    !(Client_Type(from) == CLIENT_SERVER) &&
			    !(Client_Type(from) == CLIENT_SERVICE)) {
				cl2chan = Channel_FirstChannelOf(cl);
				while (cl2chan) {
					chan = Channel_GetChannel(cl2chan);
					if (Channel_IsMemberOf(chan, from))
						break;
					cl2chan = Channel_NextChannelOf(cl, cl2chan);
				}
				if (!cl2chan) {
					if (SendErrors && !IRC_WriteErrClient(
					    from, ERR_NOTONSAMECHANNEL_MSG,
					    Client_ID(from), Client_ID(cl)))
						return DISCONNECTED;
					goto send_next_target;
				}
			}

			if (SendErrors && (Client_Type(Client) != CLIENT_SERVER)
			    && Client_HasMode(cl, 'a')) {
				/* Target is away */
				if (!IRC_WriteStrClient(from, RPL_AWAY_MSG,
							Client_ID(from),
							Client_ID(cl),
							Client_Away(cl)))
					return DISCONNECTED;
			}
			if (Client_Conn(from) > NONE) {
				Conn_UpdateIdle(Client_Conn(from));
			}
			if (!IRC_WriteStrClientPrefix(cl, from, "%s %s :%s",
						      Req->command, Client_ID(cl),
						      message))
				return DISCONNECTED;
		} else if (ForceType != CLIENT_SERVICE
			   && (chan = Channel_Search(currentTarget))) {
			/* Target is a channel */
			if (!Channel_Write(chan, from, Client, Req->command,
					   SendErrors, message))
					return DISCONNECTED;
		} else if (ForceType != CLIENT_SERVICE
			   && strchr("$#", currentTarget[0])
			   && strchr(currentTarget, '.')) {
			/* $#: server/host mask, RFC 2812, sec. 3.3.1 */
			if (!Send_Message_Mask(from, Req->command, currentTarget,
					       message, SendErrors))
				return DISCONNECTED;
		} else {
			if (!SendErrors)
				return CONNECTED;
			if (!IRC_WriteErrClient(from, ERR_NOSUCHNICK_MSG,
						Client_ID(from), currentTarget))
				return DISCONNECTED;
		}

	send_next_target:
		currentTarget = strtok_r(NULL, ",", &strtok_last);
		if (!currentTarget)
			break;

		Conn_SetPenalty(Client_Conn(Client), 1);

		if (target_nr >= MAX_HNDL_TARGETS) {
			/* Too many targets given! */
			return IRC_WriteErrClient(Client,
						  ERR_TOOMANYTARGETS_MSG,
						  currentTarget);
		}
	}

	return CONNECTED;
} /* Send_Message */

/**
 * Send a message to "target mask" target(s).
 *
 * See RFC 2812, sec. 3.3.1 for details.
 *
 * @param from The client from which this command has been received.
 * @param command The command to use (PRIVMSG, NOTICE, ...).
 * @param targetMask The "target mask" (will be verified by this function).
 * @param message The message to send.
 * @param SendErrors Whether to report errors back to the client or not.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
Send_Message_Mask(CLIENT * from, char * command, char * targetMask,
		  char * message, bool SendErrors)
{
	CLIENT *cl;
	bool client_match;
	char *mask = targetMask + 1;
	const char *check_wildcards;

	cl = NULL;

	if (!Client_HasMode(from, 'o')) {
		if (!SendErrors)
			return true;
		return IRC_WriteErrClient(from, ERR_NOPRIVILEGES_MSG,
					  Client_ID(from));
	}

	/*
	 * RFC 2812, sec. 3.3.1 requires that targetMask have at least one
	 * dot (".") and no wildcards ("*", "?") following the last one.
	 */
	check_wildcards = strrchr(targetMask, '.');
	if (!check_wildcards || check_wildcards[strcspn(check_wildcards, "*?")]) {
		if (!SendErrors)
			return true;
		return IRC_WriteErrClient(from, ERR_WILDTOPLEVEL_MSG,
					  targetMask);
	}

	if (targetMask[0] == '#') {
		/* #: hostmask, see RFC 2812, sec. 3.3.1 */
		for (cl = Client_First(); cl != NULL; cl = Client_Next(cl)) {
			if (Client_Type(cl) != CLIENT_USER)
				continue;
			client_match = MatchCaseInsensitive(mask, Client_Hostname(cl));
			if (client_match)
				if (!IRC_WriteStrClientPrefix(cl, from, "%s %s :%s",
						command, Client_ID(cl), message))
					return false;
		}
	} else {
		/* $: server mask, see RFC 2812, sec. 3.3.1 */
		assert(targetMask[0] == '$');
		for (cl = Client_First(); cl != NULL; cl = Client_Next(cl)) {
			if (Client_Type(cl) != CLIENT_USER)
				continue;
			client_match = MatchCaseInsensitive(mask,
					Client_ID(Client_Introducer(cl)));
			if (client_match)
				if (!IRC_WriteStrClientPrefix(cl, from, "%s %s :%s",
						command, Client_ID(cl), message))
					return false;
		}
	}
	return CONNECTED;
} /* Send_Message_Mask */

/* -eof- */
