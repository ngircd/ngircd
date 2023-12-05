/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2015 Alexander Barton (alex@barton.de) and Contributors.
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
 * Handlers for IRC numerics sent to the server
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "conn-func.h"
#include "conf.h"
#include "channel.h"
#include "class.h"
#include "irc-write.h"
#include "lists.h"
#include "log.h"
#include "parse.h"

#include "numeric.h"

/**
 * Announce a channel and its users in the network.
 */
static bool
Announce_Channel(CLIENT *Client, CHANNEL *Chan)
{
	CL2CHAN *cl2chan;
	CLIENT *cl;
	char str[COMMAND_LEN], *ptr;
	bool njoin, xop;

	/* Check features of remote server */
	njoin = Conn_Options(Client_Conn(Client)) & CONN_RFC1459 ? false : true;
	xop = Client_HasFlag(Client, 'X') ? true : false;

	/* Get all the members of this channel */
	cl2chan = Channel_FirstMember(Chan);
	snprintf(str, sizeof(str), "NJOIN %s :", Channel_Name(Chan));
	while (cl2chan) {
		cl = Channel_GetClient(cl2chan);
		assert(cl != NULL);

		if (njoin) {
			/* RFC 2813: send NJOIN with nicknames and modes
			 * (if user is channel operator or has voice) */
			if (str[strlen(str) - 1] != ':')
				strlcat(str, ",", sizeof(str));

			/* Prepare user prefix (ChanOp, voiced, ...) */
			if (xop && Channel_UserHasMode(Chan, cl, 'q'))
				strlcat(str, "~", sizeof(str));
			if (xop && Channel_UserHasMode(Chan, cl, 'a'))
				strlcat(str, "&", sizeof(str));
			if (Channel_UserHasMode(Chan, cl, 'o'))
				strlcat(str, "@", sizeof(str));
			if (xop && Channel_UserHasMode(Chan, cl, 'h'))
				strlcat(str, "%", sizeof(str));
			if (Channel_UserHasMode(Chan, cl, 'v'))
				strlcat(str, "+", sizeof(str));

			strlcat(str, Client_ID(cl), sizeof(str));

			/* Send the data if the buffer is "full" */
			if (strlen(str) > (sizeof(str) - CLIENT_NICK_LEN - 8)) {
				if (!IRC_WriteStrClient(Client, "%s", str))
					return DISCONNECTED;
				snprintf(str, sizeof(str), "NJOIN %s :",
					 Channel_Name(Chan));
			}
		} else {
			/* RFC 1459: no NJOIN, send JOIN and MODE */
			if (!IRC_WriteStrClientPrefix(Client, cl, "JOIN %s",
						Channel_Name(Chan)))
				return DISCONNECTED;
			ptr = Channel_UserModes(Chan, cl);
			while (*ptr) {
				if (!IRC_WriteStrClientPrefix(Client, cl,
						   "MODE %s +%c %s",
						   Channel_Name(Chan), ptr[0],
						   Client_ID(cl)))
					return DISCONNECTED;
				ptr++;
			}
		}

		cl2chan = Channel_NextMember(Chan, cl2chan);
	}

	/* Data left in the buffer? */
	if (str[strlen(str) - 1] != ':') {
		/* Yes, send it ... */
		if (!IRC_WriteStrClient(Client, "%s", str))
			return DISCONNECTED;
	}

	return CONNECTED;
} /* Announce_Channel */

/**
 * Announce new server in the network
 * @param Client New server
 * @param Server Existing server in the network
 */
static bool
Announce_Server(CLIENT * Client, CLIENT * Server)
{
	CLIENT *c;

	if (Client_Conn(Server) > NONE) {
		/* Announce the new server to the one already registered
		 * which is directly connected to the local server */
		if (!IRC_WriteStrClient
		    (Server, "SERVER %s %d %d :%s", Client_ID(Client),
		     Client_Hops(Client) + 1, Client_MyToken(Client),
		     Client_Info(Client)))
			return DISCONNECTED;
	}

	if (Client_Hops(Server) == 1)
		c = Client_ThisServer();
	else
		c = Client_TopServer(Server);

	/* Inform new server about the one already registered in the network */
	return IRC_WriteStrClientPrefix(Client, c, "SERVER %s %d %d :%s",
		Client_ID(Server), Client_Hops(Server) + 1,
		Client_MyToken(Server), Client_Info(Server));
} /* Announce_Server */

#ifdef IRCPLUS

/**
 * Send a specific list to a remote server.
 */
static bool
Send_List(CLIENT *Client, CHANNEL *Chan, struct list_head *Head, char Type)
{
	struct list_elem *elem;

	elem = Lists_GetFirst(Head);
	while (elem) {
		if (!IRC_WriteStrClient(Client, "MODE %s +%c %s",
					Channel_Name(Chan), Type,
					Lists_GetMask(elem))) {
			return DISCONNECTED;
		}
		elem = Lists_GetNext(elem);
	}
	return CONNECTED;
}

/**
 * Synchronize invite, ban, except, and G-Line lists between servers.
 *
 * @param Client New server.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
Synchronize_Lists(CLIENT * Client)
{
	CHANNEL *c;
	struct list_head *head;
	struct list_elem *elem;
	time_t t;

	assert(Client != NULL);

	/* g-lines */
	head = Class_GetList(CLASS_GLINE);
	elem = Lists_GetFirst(head);
	while (elem) {
		t = Lists_GetValidity(elem) - time(NULL);
		if (!IRC_WriteStrClient(Client, "GLINE %s %ld :%s",
					Lists_GetMask(elem),
					t > 0 ? (long)t : 0,
					Lists_GetReason(elem)))
			return DISCONNECTED;
		elem = Lists_GetNext(elem);
	}

	c = Channel_First();
	while (c) {
		if (!Send_List(Client, c, Channel_GetListExcepts(c), 'e'))
			return DISCONNECTED;
		if (!Send_List(Client, c, Channel_GetListBans(c), 'b'))
			return DISCONNECTED;
		if (!Send_List(Client, c, Channel_GetListInvites(c), 'I'))
			return DISCONNECTED;
		c = Channel_Next(c);
	}
	return CONNECTED;
}

/**
 * Send CHANINFO commands to a new server (inform it about existing channels).
 * @param Client New server
 * @param Chan Channel
 */
static bool
Send_CHANINFO(CLIENT * Client, CHANNEL * Chan)
{
	char *modes, *topic, *key;
	bool has_k, has_l;

	Log(LOG_DEBUG, "Sending CHANINFO commands for \"%s\" ...",
	    Channel_Name(Chan));

	modes = Channel_Modes(Chan);
	topic = Channel_Topic(Chan);

	if (!*modes && !*topic)
		return CONNECTED;

	has_k = Channel_HasMode(Chan, 'k');
	has_l = Channel_HasMode(Chan, 'l');

	/* send CHANINFO */
	if (!has_k && !has_l) {
		if (!*topic) {
			/* "CHANINFO <chan> +<modes>" */
			return IRC_WriteStrClient(Client, "CHANINFO %s +%s",
						  Channel_Name(Chan), modes);
		}
		/* "CHANINFO <chan> +<modes> :<topic>" */
		return IRC_WriteStrClient(Client, "CHANINFO %s +%s :%s",
					  Channel_Name(Chan), modes, topic);
	}
	/* "CHANINFO <chan> +<modes> <key> <limit> :<topic>" */
	key = Channel_Key(Chan);
	return IRC_WriteStrClient(Client, "CHANINFO %s +%s %s %lu :%s",
				  Channel_Name(Chan), modes,
				  has_k ? (key && *key ? key : "*") : "*",
				  has_l ? Channel_MaxUsers(Chan) : 0, topic);
} /* Send_CHANINFO */

#endif /* IRCPLUS */

/**
 * Handle ENDOFMOTD (376) numeric and login remote server.
 * The peer is either an IRC server (no IRC+ protocol), or we got the
 * ENDOFMOTD numeric from an IRC+ server. We have to register the new server.
 */
GLOBAL bool
IRC_Num_ENDOFMOTD(CLIENT * Client, UNUSED REQUEST * Req)
{
	int max_hops, i;
	CLIENT *c;
	CHANNEL *chan;

	Client_SetType(Client, CLIENT_SERVER);

	Log(LOG_NOTICE | LOG_snotice,
	    "Server \"%s\" registered (connection %d, 1 hop - direct link).",
	    Client_ID(Client), Client_Conn(Client));

	/* Get highest hop count */
	max_hops = 0;
	c = Client_First();
	while (c) {
		if (Client_Hops(c) > max_hops)
			max_hops = Client_Hops(c);
		c = Client_Next(c);
	}

	/* Inform the new server about all other servers, and announce the
	 * new server to all the already registered ones. Important: we have
	 * to do this "in order" and can't introduce servers of which the
	 * "toplevel server" isn't known already. */
	for (i = 0; i < (max_hops + 1); i++) {
		for (c = Client_First(); c != NULL; c = Client_Next(c)) {
			if (Client_Type(c) != CLIENT_SERVER)
				continue;	/* not a server */
			if (Client_Hops(c) != i)
				continue;	/* not actual "nesting level" */
			if (c == Client || c == Client_ThisServer())
				continue;	/* that's us or the peer! */

			if (!Announce_Server(Client, c))
				return DISCONNECTED;
		}
	}

	/* Announce all the users to the new server */
	c = Client_First();
	while (c) {
		if (Client_Type(c) == CLIENT_USER ||
		    Client_Type(c) == CLIENT_SERVICE) {
			if (!Client_Announce(Client, Client_ThisServer(), c))
				return DISCONNECTED;
		}
		c = Client_Next(c);
	}

	/* Announce all channels to the new server */
	chan = Channel_First();
	while (chan) {
		if (Channel_IsLocal(chan)) {
			chan = Channel_Next(chan);
			continue;
		}
#ifdef IRCPLUS
		/* Send CHANINFO if the peer supports it */
		if (Client_HasFlag(Client, 'C')) {
			if (!Send_CHANINFO(Client, chan))
				return DISCONNECTED;
		}
#endif

		if (!Announce_Channel(Client, chan))
			return DISCONNECTED;

		/* Get next channel ... */
		chan = Channel_Next(chan);
	}

#ifdef IRCPLUS
	if (Client_HasFlag(Client, 'L')) {
		LogDebug("Synchronizing INVITE- and BAN-lists ...");
		if (!Synchronize_Lists(Client))
			return DISCONNECTED;
	}
#endif

	if (!IRC_WriteStrClient(Client, "PING :%s",
	    Client_ID(Client_ThisServer())))
		return DISCONNECTED;

	return CONNECTED;
} /* IRC_Num_ENDOFMOTD */

/**
 * Handle ISUPPORT (005) numeric.
 */
GLOBAL bool
IRC_Num_ISUPPORT(CLIENT * Client, REQUEST * Req)
{
	int i;
	char *key, *value;

	for (i = 1; i < Req->argc - 1; i++) {
		key = Req->argv[i];
		value = strchr(key, '=');
		if (value)
			*value++ = '\0';
		else
			value = "";

		if (strcmp("NICKLEN", key) == 0) {
			if ((unsigned int)atol(value) == Conf_MaxNickLength - 1)
				continue;

			/* Nickname length settings are different! */
			Log(LOG_ERR,
			    "Peer uses incompatible nickname length (%d/%d)! Disconnecting ...",
			    Conf_MaxNickLength - 1, atoi(value));
			Conn_Close(Client_Conn(Client),
				   "Incompatible nickname length",
				   NULL, false);
			return DISCONNECTED;
		}
	}

	return CONNECTED;
} /* IRC_Num_ISUPPORT */

/* -eof- */
