/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2014 Alexander Barton (alex@barton.de) and Contributors.
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
 * Channel operator commands
 */

#include <assert.h>
#include <string.h>

#include "conn.h"
#include "channel.h"
#include "irc-macros.h"
#include "irc-write.h"
#include "lists.h"
#include "log.h"
#include "messages.h"
#include "parse.h"

#include "irc-op.h"

/* Local functions */

static bool
try_kick(CLIENT *peer, CLIENT* from, const char *nick, const char *channel,
	 const char *reason)
{
	CLIENT *target = Client_Search(nick);

	if (!target)
		return IRC_WriteErrClient(from, ERR_NOSUCHNICK_MSG,
					  Client_ID(from), nick);

	Channel_Kick(peer, target, from, channel, reason);
	return true;
}

/* Global functions */

/**
 * Handler for the IRC command "KICK".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_KICK(CLIENT *Client, REQUEST *Req)
{
	CLIENT *from;
	char *itemList = Req->argv[0];
	const char* currentNick, *currentChannel, *reason;
	unsigned int channelCount = 1;
	unsigned int nickCount = 1;

	assert( Client != NULL );
	assert( Req != NULL );

	_IRC_GET_SENDER_OR_RETURN_(from, Req, Client)

	while (*itemList) {
		if (*itemList == ',') {
			*itemList = '\0';
			channelCount++;
		}
		itemList++;
	}

	itemList = Req->argv[1];
	while (*itemList) {
		if (*itemList == ',') {
			*itemList = '\0';
			nickCount++;
		}
		itemList++;
	}

	reason = Req->argc == 3 ? Req->argv[2] : Client_ID(from);
	currentNick = Req->argv[1];
	currentChannel = Req->argv[0];
	if (channelCount == 1) {
		while (nickCount > 0) {
			if (!try_kick(Client, from, currentNick,
				      currentChannel, reason))
				return false;

			while (*currentNick)
				currentNick++;

			currentNick++;
			nickCount--;
		}
	} else if (channelCount == nickCount) {
		while (nickCount > 0) {
			if (!try_kick(Client, from, currentNick,
				      currentChannel, reason))
				return false;

			while (*currentNick)
				currentNick++;

			while (*currentChannel)
				currentChannel++;

			currentNick++;
			currentChannel++;
			nickCount--;
		}
	} else {
		return IRC_WriteErrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					Client_ID(Client), Req->command);
	}
	return true;
} /* IRC_KICK */

/**
 * Handler for the IRC command "INVITE".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_INVITE(CLIENT *Client, REQUEST *Req)
{
	CHANNEL *chan;
	CLIENT *target, *from;
	const char *colon_if_necessary;
	bool remember = false;

	assert( Client != NULL );
	assert( Req != NULL );

	_IRC_GET_SENDER_OR_RETURN_(from, Req, Client)

	/* Search user */
	target = Client_Search(Req->argv[0]);
	if (!target || (Client_Type(target) != CLIENT_USER))
		return IRC_WriteErrClient(from, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->argv[0]);

	if (Req->argv[1][0] == '&') {
		/* Local channel. Make sure the target user is on this server! */
		if (Client_Conn(target) == NONE)
			return IRC_WriteErrClient(from, ERR_USERNOTONSERV_MSG,
						  Client_ID(Client),
						  Req->argv[0]);
	}

	chan = Channel_Search(Req->argv[1]);
	if (chan) {
		/* Channel exists. Is the user a valid member of the channel? */
		if (!Channel_IsMemberOf(chan, from))
			return IRC_WriteErrClient(from, ERR_NOTONCHANNEL_MSG,
						  Client_ID(Client),
						  Req->argv[1]);

		/* Is the channel "invite-disallow"? */
		if (Channel_HasMode(chan, 'V'))
			return IRC_WriteErrClient(from, ERR_NOINVITE_MSG,
						  Client_ID(from),
						  Channel_Name(chan));

		/* Is the channel "invite-only"? */
		if (Channel_HasMode(chan, 'i')) {
			/* Yes. The user issuing the INVITE command must be
			 * channel owner/admin/operator/halfop! */
			if (!Channel_UserHasMode(chan, from, 'q') &&
			    !Channel_UserHasMode(chan, from, 'a') &&
			    !Channel_UserHasMode(chan, from, 'o') &&
			    !Channel_UserHasMode(chan, from, 'h'))
				return IRC_WriteErrClient(from,
							  ERR_CHANOPRIVSNEEDED_MSG,
							  Client_ID(from),
							  Channel_Name(chan));
			remember = true;
		}

		/* Is the target user already member of the channel? */
		if (Channel_IsMemberOf(chan, target))
			return IRC_WriteErrClient(from, ERR_USERONCHANNEL_MSG,
						  Client_ID(from),
						  Req->argv[0], Req->argv[1]);

		/* If the target user is banned on that channel: remember invite */
		if (Lists_Check(Channel_GetListBans(chan), target))
			remember = true;

		if (remember) {
			/* We must remember this invite */
			if (!Channel_AddInvite(chan, Client_MaskCloaked(target),
						true, Client_ID(from)))
				return CONNECTED;
		}
	}

	LogDebug("User \"%s\" invites \"%s\" to \"%s\" ...", Client_Mask(from),
		 Req->argv[0], Req->argv[1]);

	/*
	 * RFC 2812 states:
	 * 'There is no requirement that the channel [..] must exist or be a
	 * valid channel'. The problem with this is that this allows the
	 * "channel" to contain spaces, in which case we must prefix its name
	 * with a colon to make it clear that it is only a single argument.
	 */
	colon_if_necessary = strchr(Req->argv[1], ' ') ? ":":"";
	/* Inform target client */
	IRC_WriteStrClientPrefix(target, from, "INVITE %s %s%s", Req->argv[0],
				  colon_if_necessary, Req->argv[1]);

	if (Client_Conn(target) > NONE) {
		/* The target user is local, so we have to send the status code */
		if (!IRC_WriteStrClient(from, RPL_INVITING_MSG,
					Client_ID(from), Req->argv[0],
					colon_if_necessary, Req->argv[1]))
			return DISCONNECTED;

		if (Client_HasMode(target, 'a') &&
			!IRC_WriteStrClient(from, RPL_AWAY_MSG, Client_ID(from),
					Client_ID(target), Client_Away(target)))
				return DISCONNECTED;
	}
	return CONNECTED;
} /* IRC_INVITE */

/* -eof- */
