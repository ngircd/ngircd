/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton (alex@barton.de)
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
 * IRC channel commands
 */

#include "imp.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "conn.h"
#include "channel.h"
#include "conn-func.h"
#include "lists.h"
#include "log.h"
#include "match.h"
#include "messages.h"
#include "parse.h"
#include "irc.h"
#include "irc-info.h"
#include "irc-write.h"
#include "conf.h"

#include "exp.h"
#include "irc-channel.h"


/**
 * Part from all channels.
 *
 * RFC 2812, (3.2.1 Join message Command):
 *  Note that this message accepts a special argument ("0"), which is a
 *  special request to leave all channels the user is currently a member of.
 *  The server will process this message as if the user had sent a PART
 *  command (See Section 3.2.2) for each channel he is a member of.
 *
 * @param client	Client that initiated the part request
 * @param target	Client that should part all joined channels
 * @returns		CONNECTED or DISCONNECTED
 */
static bool
part_from_all_channels(CLIENT* client, CLIENT *target)
{
	CL2CHAN *cl2chan;
	CHANNEL *chan;

	while ((cl2chan = Channel_FirstChannelOf(target))) {
		chan = Channel_GetChannel(cl2chan);
		assert( chan != NULL );
		Channel_Part(target, client, Channel_Name(chan), Client_ID(target));
	}
	return CONNECTED;
} /* part_from_all_channels */


/**
 * Check weather a local client is allowed to join an already existing
 * channel or not.
 *
 * @param Client	Client that sent the JOIN command
 * @param chan		Channel to check
 * @param channame	Name of the channel
 * @param key		Provided channel key (or NULL)
 * @returns		true if client is allowed to join, false otherwise
 */
static bool
join_allowed(CLIENT *Client, CHANNEL *chan, const char *channame,
	     const char *key)
{
	bool is_invited, is_banned, is_exception;
	const char *channel_modes;

	/* Allow IRC operators to overwrite channel limits */
	if (strchr(Client_Modes(Client), 'o'))
		return true;

	is_banned = Lists_Check(Channel_GetListBans(chan), Client);
	is_exception = Lists_Check(Channel_GetListExcepts(chan), Client);
	is_invited = Lists_Check(Channel_GetListInvites(chan), Client);

	if (is_banned && !is_invited && !is_exception) {
		/* Client is banned from channel (and not on invite list) */
		IRC_WriteStrClient(Client, ERR_BANNEDFROMCHAN_MSG,
				   Client_ID(Client), channame);
		return false;
	}

	channel_modes = Channel_Modes(chan);
	if (strchr(channel_modes, 'i') && !is_invited) {
		/* Channel is "invite-only" and client is not on invite list */
		IRC_WriteStrClient(Client, ERR_INVITEONLYCHAN_MSG,
				   Client_ID(Client), channame);
		return false;
	}

	if (!Channel_CheckKey(chan, Client, key ? key : "")) {
		/* Channel is protected by a channel key and the client
		 * didn't specify the correct one */
		IRC_WriteStrClient(Client, ERR_BADCHANNELKEY_MSG,
				   Client_ID(Client), channame);
		return false;
	}

	if (strchr(channel_modes, 'l') &&
	    (Channel_MaxUsers(chan) <= Channel_MemberCount(chan))) {
		/* There are more clints joined to this channel than allowed */
		IRC_WriteStrClient(Client, ERR_CHANNELISFULL_MSG,
				   Client_ID(Client), channame);
		return false;
	}

	if (strchr(channel_modes, 'z') && !Conn_UsesSSL(Client_Conn(Client))) {
		/* Only "secure" clients are allowed, but clients doesn't
		 * use SSL encryption */
		IRC_WriteStrClient(Client, ERR_SECURECHANNEL_MSG,
				   Client_ID(Client), channame);
		return false;
	}

	if (strchr(channel_modes, 'O') && !Client_OperByMe(Client)) {
		/* Only IRC operators are allowed! */
		IRC_WriteStrClient(Client, ERR_OPONLYCHANNEL_MSG,
				   Client_ID(Client), channame);
		return false;
	}

	if (strchr(channel_modes, 'R') && !strchr(Client_Modes(Client), 'R')) {
		/* Only registered users are allowed! */
		IRC_WriteStrClient(Client, ERR_REGONLYCHANNEL_MSG,
				   Client_ID(Client), channame);
		return false;
	}

	return true;
} /* join_allowed */


/**
 * Set user channel modes.
 *
 * @param chan		Channel
 * @param target	User to set modes for
 * @param flags		Channel modes to add
 */
static void
join_set_channelmodes(CHANNEL *chan, CLIENT *target, const char *flags)
{
	if (flags) {
		while (*flags) {
			Channel_UserModeAdd(chan, target, *flags);
			flags++;
		}
	}

	/* If channel persistent and client is ircop: make client chanop */
	if (strchr(Channel_Modes(chan), 'P') && strchr(Client_Modes(target), 'o'))
		Channel_UserModeAdd(chan, target, 'o');
} /* join_set_channelmodes */


/**
 * Forward JOIN command to a specific server
 *
 * This function diffentiates between servers using RFC 2813 mode that
 * support the JOIN command with appended ASCII 7 character and channel
 * modes, and servers using RFC 1459 protocol which require separate JOIN
 * and MODE commands.
 *
 * @param To		Forward JOIN (and MODE) command to this peer server
 * @param Prefix	Client used to prefix the genrated commands
 * @param Data		Parameters of JOIN command to forward, probably
 *			containing channel modes separated by ASCII 7.
 */
static void
cb_join_forward(CLIENT *To, CLIENT *Prefix, void *Data)
{
	CONN_ID conn;
	char str[COMMAND_LEN], *ptr = NULL;

	strlcpy(str, (char *)Data, sizeof(str));
	conn = Client_Conn(To);

	if (Conn_Options(conn) & CONN_RFC1459) {
		/* RFC 1459 compatibility mode, appended modes are NOT
		 * supported, so strip them off! */
		ptr = strchr(str, 0x7);
		if (ptr)
			*ptr++ = '\0';
	}

	IRC_WriteStrClientPrefix(To, Prefix, "JOIN %s", str);
	if (ptr && *ptr)
		IRC_WriteStrClientPrefix(To, Prefix, "MODE %s +%s %s", str, ptr,
					 Client_ID(Prefix));
} /* cb_join_forward */


/**
 * Forward JOIN command to all servers
 *
 * This function calls cb_join_forward(), which differentiates between
 * protocol implementations (e.g. RFC 2812, RFC 1459).
 *
 * @param Client	Client used to prefix the genrated commands
 * @param target	Forward JOIN (and MODE) command to this peer server
 * @param chan		Channel structure
 * @param channame	Channel name
 */
static void
join_forward(CLIENT *Client, CLIENT *target, CHANNEL *chan,
					const char *channame)
{
	char modes[CHANNEL_MODE_LEN], str[COMMAND_LEN];

	/* RFC 2813, 4.2.1: channel modes are separated from the channel
	 * name with ASCII 7, if any, and not spaces: */
	strlcpy(&modes[1], Channel_UserModes(chan, target), sizeof(modes) - 1);
	if (modes[1])
		modes[0] = 0x7;
	else
		modes[0] = '\0';

	/* forward to other servers (if it is not a local channel) */
	if (!Channel_IsLocal(chan)) {
		snprintf(str, sizeof(str), "%s%s", channame, modes);
		IRC_WriteStrServersPrefixFlag_CB(Client, target, '\0',
						 cb_join_forward, str);
	}

	/* tell users in this channel about the new client */
	IRC_WriteStrChannelPrefix(Client, chan, target, false,
				  "JOIN :%s",  channame);

	/* synchronize channel modes */
	if (modes[1]) {
		IRC_WriteStrChannelPrefix(Client, chan, target, false,
					  "MODE %s +%s %s", channame,
					  &modes[1], Client_ID(target));
	}
} /* join_forward */


/**
 * Aknowledge user JOIN request and send "channel info" numerics.
 *
 * @param Client	Client used to prefix the genrated commands
 * @param target	Forward commands/numerics to this user
 * @param chan		Channel structure
 * @param channame	Channel name
 */
static bool
join_send_topic(CLIENT *Client, CLIENT *target, CHANNEL *chan,
					const char *channame)
{
	const char *topic;

	if (Client_Type(Client) != CLIENT_USER)
		return true;
	/* acknowledge join */
	if (!IRC_WriteStrClientPrefix(Client, target, "JOIN :%s", channame))
		return false;

	/* Send topic to client, if any */
	topic = Channel_Topic(chan);
	assert(topic != NULL);
	if (*topic) {
		if (!IRC_WriteStrClient(Client, RPL_TOPIC_MSG,
			Client_ID(Client), channame, topic))
				return false;
#ifndef STRICT_RFC
		if (!IRC_WriteStrClient(Client, RPL_TOPICSETBY_MSG,
			Client_ID(Client), channame,
			Channel_TopicWho(chan),
			Channel_TopicTime(chan)))
				return false;
#endif
	}
	/* send list of channel members to client */
	if (!IRC_Send_NAMES(Client, chan))
		return false;
	return IRC_WriteStrClient(Client, RPL_ENDOFNAMES_MSG, Client_ID(Client),
				  Channel_Name(chan));
} /* join_send_topic */


/**
 * Handler for the IRC "JOIN" command.
 *
 * See RFC 2812, 3.2.1 "Join message"; RFC 2813, 4.2.1 "Join message".
 *
 * @param Client The client from which this command has been received
 * @param Req Request structure with prefix and all parameters
 * @returns CONNECTED or DISCONNECTED
 */
GLOBAL bool
IRC_JOIN( CLIENT *Client, REQUEST *Req )
{
	char *channame, *key = NULL, *flags, *lastkey = NULL, *lastchan = NULL;
	CLIENT *target;
	CHANNEL *chan;

	assert (Client != NULL);
	assert (Req != NULL);

	/* Bad number of arguments? */
	if (Req->argc < 1 || Req->argc > 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	/* Who is the sender? */
	if (Client_Type(Client) == CLIENT_SERVER)
		target = Client_Search(Req->prefix);
	else
		target = Client;

	if (!target)
		return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->prefix);

	/* Is argument "0"? */
	if (Req->argc == 1 && !strncmp("0", Req->argv[0], 2))
		return part_from_all_channels(Client, target);

	/* Are channel keys given? */
	if (Req->argc > 1)
		key = strtok_r(Req->argv[1], ",", &lastkey);

	channame = Req->argv[0];
	channame = strtok_r(channame, ",", &lastchan);

	/* Make sure that "channame" is not the empty string ("JOIN :") */
	if (! channame)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	while (channame) {
		flags = NULL;

		/* Did the server include channel-user-modes? */
		if (Client_Type(Client) == CLIENT_SERVER) {
			flags = strchr(channame, 0x7);
			if (flags) {
				*flags = '\0';
				flags++;
			}
		}

		chan = Channel_Search(channame);
		if (!chan && Conf_PredefChannelsOnly) {
			 /* channel must be created, but forbidden by config */
			IRC_WriteStrClient(Client, ERR_BANNEDFROMCHAN_MSG,
					   Client_ID(Client), channame);
			goto join_next;
		}

		/* Local client? */
		if (Client_Type(Client) == CLIENT_USER) {
			if (chan) {
				/* Already existing channel: already member? */
				if (Channel_IsMemberOf(chan, Client))
				    goto join_next;
			}

			/* Test if the user has reached the channel limit */
			if ((Conf_MaxJoins > 0) &&
			    (Channel_CountForUser(Client) >= Conf_MaxJoins)) {
				if (!IRC_WriteStrClient(Client,
						ERR_TOOMANYCHANNELS_MSG,
						Client_ID(Client), channame))
					return DISCONNECTED;
				goto join_next;
			}

			if (chan) {
				/* Already existing channel: check if the
				 * client is allowed to join */
				if (!join_allowed(Client, chan, channame, key))
					goto join_next;
			} else {
				/* New channel: first user will become channel
				 * operator unless this is a modeless channel */
				if (*channame != '+')
					flags = "o";
			}

			/* Local client: update idle time */
			Conn_UpdateIdle(Client_Conn(Client));
		} else {
			/* Remote server: we don't need to know whether the
			 * client is invited or not, but we have to make sure
			 * that the "one shot" entries (generated by INVITE
			 * commands) in this list become deleted when a user
			 * joins a channel this way. */
			if (chan)
				(void)Lists_Check(Channel_GetListInvites(chan),
						  target);
		}

		/* Join channel (and create channel if it doesn't exist) */
		if (!Channel_Join(target, channame))
			goto join_next;

		if (!chan) { /* channel is new; it has been created above */
			chan = Channel_Search(channame);
			assert(chan != NULL);
			if (Channel_IsModeless(chan)) {
				Channel_ModeAdd(chan, 't'); /* /TOPIC not allowed */
				Channel_ModeAdd(chan, 'n'); /* no external msgs */
			}
		}
		assert(chan != NULL);

		join_set_channelmodes(chan, target, flags);

		join_forward(Client, target, chan, channame);

		if (!join_send_topic(Client, target, chan, channame))
			break; /* write error */

	join_next:
		/* next channel? */
		channame = strtok_r(NULL, ",", &lastchan);
		if (channame && key)
			key = strtok_r(NULL, ",", &lastkey);
	}
	return CONNECTED;
} /* IRC_JOIN */


/**
 * Handler for the IRC "PART" command.
 *
 * See RFC 2812, 3.2.2: "Part message".
 *
 * @param Client	The client from which this command has been received
 * @param Req		Request structure with prefix and all parameters
 * @returns		CONNECTED or DISCONNECTED
 */
GLOBAL bool
IRC_PART(CLIENT * Client, REQUEST * Req)
{
	CLIENT *target;
	char *chan;

	assert(Client != NULL);
	assert(Req != NULL);

	if (Req->argc < 1 || Req->argc > 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	/* Get the sender */
	if (Client_Type(Client) == CLIENT_SERVER)
		target = Client_Search(Req->prefix);
	else
		target = Client;
	if (!target)
		return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->prefix);

	/* Loop over all the given channel names */
	chan = strtok(Req->argv[0], ",");

	/* Make sure that "chan" is not the empty string ("PART :") */
	if (! chan)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	while (chan) {
		Channel_Part(target, Client, chan,
			     Req->argc > 1 ? Req->argv[1] : Client_ID(target));
		chan = strtok(NULL, ",");
	}

	/* Update idle time, if local client */
	if (Client_Conn(Client) > NONE)
		Conn_UpdateIdle(Client_Conn(Client));

	return CONNECTED;
} /* IRC_PART */


/**
 * Handler for the IRC "TOPIC" command.
 *
 * See RFC 2812, 3.2.4 "Topic message".
 *
 * @param Client	The client from which this command has been received
 * @param Req		Request structure with prefix and all parameters
 * @returns		CONNECTED or DISCONNECTED
 */
GLOBAL bool
IRC_TOPIC( CLIENT *Client, REQUEST *Req )
{
	CHANNEL *chan;
	CLIENT *from;
	char *topic;
	bool r, is_oper;

	assert( Client != NULL );
	assert( Req != NULL );

	if (Req->argc < 1 || Req->argc > 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	if (Client_Type(Client) == CLIENT_SERVER)
		from = Client_Search(Req->prefix);
	else
		from = Client;

	if (!from)
		return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->prefix);

	chan = Channel_Search(Req->argv[0]);
	if (!chan)
		return IRC_WriteStrClient(from, ERR_NOSUCHCHANNEL_MSG,
					  Client_ID(from), Req->argv[0]);

	/* Only IRC opers and channel members allowed */
	is_oper = Client_OperByMe(from);
	if (!Channel_IsMemberOf(chan, from) && !is_oper)
		return IRC_WriteStrClient(from, ERR_NOTONCHANNEL_MSG,
					  Client_ID(from), Req->argv[0]);

	if (Req->argc == 1) {
		/* Request actual topic */
		topic = Channel_Topic(chan);
		if (*topic) {
			r = IRC_WriteStrClient(from, RPL_TOPIC_MSG,
					       Client_ID(Client),
					       Channel_Name(chan), topic);
#ifndef STRICT_RFC
			if (!r)
				return r;
			r = IRC_WriteStrClient(from, RPL_TOPICSETBY_MSG,
					       Client_ID(Client),
					       Channel_Name(chan),
					       Channel_TopicWho(chan),
					       Channel_TopicTime(chan));
#endif
			return r;
		}
		else
			return IRC_WriteStrClient(from, RPL_NOTOPIC_MSG,
						  Client_ID(from),
						  Channel_Name(chan));
	}

	if (strchr(Channel_Modes(chan), 't')) {
		/* Topic Lock. Is the user a channel op or IRC operator? */
		if(!strchr(Channel_UserModes(chan, from), 'h') &&
		   !strchr(Channel_UserModes(chan, from), 'o') &&
		   !strchr(Channel_UserModes(chan, from), 'a') &&
		   !strchr(Channel_UserModes(chan, from), 'q') &&
		   !is_oper)
			return IRC_WriteStrClient(from, ERR_CHANOPRIVSNEEDED_MSG,
						  Client_ID(from),
						  Channel_Name(chan));
	}

	/* Set new topic */
	Channel_SetTopic(chan, from, Req->argv[1]);
	LogDebug("%s \"%s\" set topic on \"%s\": %s",
		 Client_TypeText(from), Client_Mask(from), Channel_Name(chan),
		 Req->argv[1][0] ? Req->argv[1] : "<none>");

	if (Conf_OperServerMode)
		from = Client_ThisServer();

	/* Update channel and forward new topic to other servers */
	if (!Channel_IsLocal(chan))
		IRC_WriteStrServersPrefix(Client, from, "TOPIC %s :%s",
					  Req->argv[0], Req->argv[1]);
	IRC_WriteStrChannelPrefix(Client, chan, from, false, "TOPIC %s :%s",
				  Req->argv[0], Req->argv[1]);

	if (Client_Type(Client) == CLIENT_USER)
		return IRC_WriteStrClientPrefix(Client, Client, "TOPIC %s :%s",
						Req->argv[0], Req->argv[1]);
	else
		return CONNECTED;
} /* IRC_TOPIC */


/**
 * Handler for the IRC "LIST" command.
 *
 * See RFC 2812, 3.2.6 "List message".
 *
 * This implementation handles the local case as well as the forwarding of the
 * LIST command to other servers in the IRC network.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_LIST( CLIENT *Client, REQUEST *Req )
{
	char *pattern;
	CHANNEL *chan;
	CLIENT *from, *target;
	int count = 0;

	assert(Client != NULL);
	assert(Req != NULL);

	/* Bad number of prameters? */
	if (Req->argc > 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	if (Req->argc > 0)
		pattern = strtok(Req->argv[0], ",");
	else
		pattern = "*";

	/* Get sender from prefix, if any */
	if (Client_Type(Client) == CLIENT_SERVER)
		from = Client_Search(Req->prefix);
	else
		from = Client;

	if (!from)
		return IRC_WriteStrClient(Client, ERR_NOSUCHSERVER_MSG,
					  Client_ID(Client), Req->prefix);

	if (Req->argc == 2) {
		/* Forward to other server? */
		target = Client_Search(Req->argv[1]);
		if (! target || Client_Type(target) != CLIENT_SERVER)
			return IRC_WriteStrClient(from, ERR_NOSUCHSERVER_MSG,
						  Client_ID(Client),
						  Req->argv[1]);

		if (target != Client_ThisServer()) {
			/* Target is indeed an other server, forward it! */
			return IRC_WriteStrClientPrefix(target, from,
							"LIST %s :%s",
							Req->argv[0],
							Req->argv[1]);
		}
	}

	while (pattern) {
		/* Loop through all the channels */
		if (Req->argc > 0)
			ngt_LowerStr(pattern);
		chan = Channel_First();
		while (chan) {
			/* Check search pattern */
			if (MatchCaseInsensitive(pattern, Channel_Name(chan))) {
				/* Gotcha! */
				if (!strchr(Channel_Modes(chan), 's')
				    || Channel_IsMemberOf(chan, from)) {
					if (IRC_CheckListTooBig(from, count,
								 MAX_RPL_LIST,
								 "LIST"))
						break;
					if (!IRC_WriteStrClient(from,
					     RPL_LIST_MSG, Client_ID(from),
					     Channel_Name(chan),
					     Channel_MemberCount(chan),
					     Channel_Topic( chan )))
						return DISCONNECTED;
					count++;
				}
			}
			chan = Channel_Next(chan);
		}

		/* Get next name ... */
		if(Req->argc > 0)
			pattern = strtok(NULL, ",");
		else
			pattern = NULL;
	}

	IRC_SetPenalty(from, 2);
	return IRC_WriteStrClient(from, RPL_LISTEND_MSG, Client_ID(from));
} /* IRC_LIST */


/**
 * Handler for the IRC+ command "CHANINFO".
 *
 * See doc/Protocol.txt, section II.3:
 * "Exchange channel-modes, topics, and persistent channels".
 *
 * @param Client	The client from which this command has been received
 * @param Req		Request structure with prefix and all parameters
 * @returns		CONNECTED or DISCONNECTED
 */
GLOBAL bool
IRC_CHANINFO( CLIENT *Client, REQUEST *Req )
{
	char modes_add[COMMAND_LEN], l[16], *ptr;
	CLIENT *from;
	CHANNEL *chan;
	int arg_topic;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Bad number of parameters? */
	if (Req->argc < 2 || Req->argc == 4 || Req->argc > 5)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	/* Compatibility kludge */
	if( Req->argc == 5 ) arg_topic = 4;
	else if( Req->argc == 3 ) arg_topic = 2;
	else arg_topic = -1;

	/* Search origin */
	from = Client_Search( Req->prefix );
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* Search or create channel */
	chan = Channel_Search( Req->argv[0] );
	if( ! chan ) chan = Channel_Create( Req->argv[0] );
	if( ! chan ) return CONNECTED;

	if( Req->argv[1][0] == '+' )
	{
		ptr = Channel_Modes( chan );
		if( ! *ptr )
		{
			/* OK, this channel doesn't have modes jet, set the received ones: */
			Channel_SetModes( chan, &Req->argv[1][1] );

			if( Req->argc == 5 )
			{
				if( strchr( Channel_Modes( chan ), 'k' )) Channel_SetKey( chan, Req->argv[2] );
				if( strchr( Channel_Modes( chan ), 'l' )) Channel_SetMaxUsers( chan, atol( Req->argv[3] ));
			}
			else
			{
				/* Delete modes which we never want to inherit */
				Channel_ModeDel( chan, 'l' );
				Channel_ModeDel( chan, 'k' );
			}

			strcpy( modes_add, "" );
			ptr = Channel_Modes( chan );
			while( *ptr )
			{
				if( *ptr == 'l' )
				{
					snprintf( l, sizeof( l ), " %lu", Channel_MaxUsers( chan ));
					strlcat( modes_add, l, sizeof( modes_add ));
				}
				if( *ptr == 'k' )
				{
					strlcat( modes_add, " ", sizeof( modes_add ));
					strlcat( modes_add, Channel_Key( chan ), sizeof( modes_add ));
				}
	     			ptr++;
			}

			/* Inform members of this channel */
			IRC_WriteStrChannelPrefix( Client, chan, from, false, "MODE %s +%s%s", Req->argv[0], Channel_Modes( chan ), modes_add );
		}
	}
	else Log( LOG_WARNING, "CHANINFO: invalid MODE format ignored!" );

	if( arg_topic > 0 )
	{
		/* We got a topic */
		ptr = Channel_Topic( chan );
		if(( ! *ptr ) && ( Req->argv[arg_topic][0] ))
		{
			/* OK, there is no topic jet */
			Channel_SetTopic(chan, Client, Req->argv[arg_topic]);
			IRC_WriteStrChannelPrefix(Client, chan, from, false,
			     "TOPIC %s :%s", Req->argv[0], Channel_Topic(chan));
		}
	}

	/* Forward CHANINFO to other serevrs */
	if( Req->argc == 5 ) IRC_WriteStrServersPrefixFlag( Client, from, 'C', "CHANINFO %s %s %s %s :%s", Req->argv[0], Req->argv[1], Req->argv[2], Req->argv[3], Req->argv[4] );
	else if( Req->argc == 3 ) IRC_WriteStrServersPrefixFlag( Client, from, 'C', "CHANINFO %s %s :%s", Req->argv[0], Req->argv[1], Req->argv[2] );
	else IRC_WriteStrServersPrefixFlag( Client, from, 'C', "CHANINFO %s %s", Req->argv[0], Req->argv[1] );

	return CONNECTED;
} /* IRC_CHANINFO */


/* -eof- */
