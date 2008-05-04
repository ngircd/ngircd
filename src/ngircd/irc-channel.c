/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * IRC channel commands
 */


#include "portab.h"

static char UNUSED id[] = "$Id: irc-channel.c,v 1.45 2008/02/24 18:57:38 fw Exp $";

#include "imp.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "conn.h"
#include "client.h"
#include "channel.h"
#include "lists.h"
#include "log.h"
#include "match.h"
#include "messages.h"
#include "parse.h"
#include "irc-info.h"
#include "irc-write.h"
#include "resolve.h"
#include "conf.h"

#include "exp.h"
#include "irc-channel.h"


/*
 * RFC 2812, (3.2.1 Join message Command):
 *  Note that this message
 *  accepts a special argument ("0"), which is a special request to leave all
 *  channels the user is currently a member of. The server will process this
 *  message as if the user had sent a PART command (See Section 3.2.2) for
 *  each channel he is a member of.
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
}


static bool
join_allowed(CLIENT *Client, CLIENT *target, CHANNEL *chan,
			const char *channame, const char *key)
{
	bool is_invited, is_banned;
	const char *channel_modes;

	/* Allow IRC operators to overwrite channel limits */
	if (strchr(Client_Modes(Client), 'o'))
		return true;

	is_banned = Lists_Check(Channel_GetListBans(chan), target);
	is_invited = Lists_Check(Channel_GetListInvites(chan), target);

	if (is_banned && !is_invited) {
		IRC_WriteStrClient(Client, ERR_BANNEDFROMCHAN_MSG, Client_ID(Client), channame);
		return false;
	}

	channel_modes = Channel_Modes(chan);
	if ((strchr(channel_modes, 'i')) && !is_invited) {
		/* Channel is "invite-only" (and Client wasn't invited) */
		IRC_WriteStrClient(Client, ERR_INVITEONLYCHAN_MSG, Client_ID(Client), channame);
		return false;
	}

	/* Is the channel protected by a key? */
	if (strchr(channel_modes, 'k') &&
		strcmp(Channel_Key(chan), key ? key : ""))
	{
		IRC_WriteStrClient(Client, ERR_BADCHANNELKEY_MSG, Client_ID(Client), channame);
		return false;
	}
	/* Are there already too many members? */
	if ((strchr(channel_modes, 'l')) && (Channel_MaxUsers(chan) <= Channel_MemberCount(chan))) {
		IRC_WriteStrClient(Client, ERR_CHANNELISFULL_MSG, Client_ID(Client), channame);
		return false;
	}
	return true;
}


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
}


static void
join_forward(CLIENT *Client, CLIENT *target, CHANNEL *chan,
					const char *channame)
{
	char modes[8];

	strlcpy(&modes[1], Channel_UserModes(chan, target), sizeof(modes) - 1);

	if (modes[1])
		modes[0] = 0x7;
	else
		modes[0] = '\0';
	/* forward to other servers */
	IRC_WriteStrServersPrefix(Client, target, "JOIN :%s%s", channame, modes);

	/* tell users in this channel about the new client */
	IRC_WriteStrChannelPrefix(Client, chan, target, false, "JOIN :%s", channame);
	if (modes[1])
		IRC_WriteStrChannelPrefix(Client, chan, target, false, "MODE %s +%s %s",
						channame, &modes[1], Client_ID(target));
}


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
	return IRC_WriteStrClient(Client, RPL_ENDOFNAMES_MSG, Client_ID(Client), Channel_Name(chan));
}


GLOBAL bool
IRC_JOIN( CLIENT *Client, REQUEST *Req )
{
	char *channame, *channame_ptr, *key, *key_ptr, *flags;
	CLIENT *target;
	CHANNEL *chan;

	assert( Client != NULL );
	assert( Req != NULL );

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
		return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG, Client_ID(Client), Req->prefix);

	/* Is argument "0"? */
	if (Req->argc == 1 && !strncmp("0", Req->argv[0], 2))
		return part_from_all_channels(Client, target);

	/* Are channel keys given? */
	if (Req->argc > 1) {
		key = Req->argv[1];
		key_ptr = strchr(key, ',');
		if (key_ptr) *key_ptr = '\0';
	} else {
		key = key_ptr = NULL;
	}
	channame = Req->argv[0];
	channame_ptr = strchr(channame, ',');
	if (channame_ptr) *channame_ptr = '\0';

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
			 /* channel must be created, but server does not allow this */
			IRC_WriteStrClient(Client, ERR_BANNEDFROMCHAN_MSG, Client_ID(Client), channame);
			break;
		}

		/* Local client? */
		if (Client_Type(Client) == CLIENT_USER) {
			/* Test if the user has reached his maximum channel count */
			if ((Conf_MaxJoins > 0) && (Channel_CountForUser(Client) >= Conf_MaxJoins))
				return IRC_WriteStrClient(Client, ERR_TOOMANYCHANNELS_MSG,
							Client_ID(Client), channame);
			if (!chan) {
				/*
				 * New Channel: first user will be channel operator
				 * unless this is a modeless channel.
				 */
				if (*channame != '+')
					flags = "o";
			} else
				if (!join_allowed(Client, target, chan, channame, key))
					break;
		} else {
			/* Remote server: we don't need to know whether the
			 * client is invited or not, but we have to make sure
			 * that the "one shot" entries (generated by INVITE
			 * commands) in this list become deleted when a user
			 * joins a channel this way. */
			if (chan) (void)Lists_Check(Channel_GetListInvites(chan), target);
		}

		/* Join channel (and create channel if it doesn't exist) */
		if (!Channel_Join(target, channame))
			break;

		if (!chan) { /* channel is new; it has been created above */
			chan = Channel_Search(channame);
			assert(chan != NULL);
			if (*channame == '+') { /* modeless channel... */
				Channel_ModeAdd(chan, 't'); /* /TOPIC not allowed */
				Channel_ModeAdd(chan, 'n'); /* no external msgs */
			}
		}
		assert(chan != NULL);

		join_set_channelmodes(chan, target, flags);

		join_forward(Client, target, chan, channame);

		if (!join_send_topic(Client, target, chan, channame))
			break; /* write error */

		/* next channel? */
		channame = channame_ptr;
		if (channame) {
			channame++;
			channame_ptr = strchr(channame, ',');
			if (channame_ptr) *channame_ptr = '\0';

			if (key_ptr) {
				key = ++key_ptr;
				key_ptr = strchr(key, ',');
				if (key_ptr) *key_ptr = '\0';
			}
		}
	}
	return CONNECTED;
} /* IRC_JOIN */


/**
 * Handler for the IRC "PART" command.
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
	while (chan) {
		Channel_Part(target, Client, chan,
			     Req->argc > 1 ? Req->argv[1] : Client_ID(target));
		chan = strtok(NULL, ",");
	}
	return CONNECTED;
} /* IRC_PART */


GLOBAL bool
IRC_TOPIC( CLIENT *Client, REQUEST *Req )
{
	CHANNEL *chan;
	CLIENT *from;
	char *topic;
	bool r;

	assert( Client != NULL );
	assert( Req != NULL );

	if ((Req->argc < 1) || (Req->argc > 2))
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG, Client_ID(Client), Req->command);

	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* Welcher Channel? */
	chan = Channel_Search( Req->argv[0] );
	if( ! chan ) return IRC_WriteStrClient( from, ERR_NOSUCHCHANNEL_MSG, Client_ID( from ), Req->argv[0] );

	/* Ist der User Mitglied in dem Channel? */
	if( ! Channel_IsMemberOf( chan, from )) return IRC_WriteStrClient( from, ERR_NOTONCHANNEL_MSG, Client_ID( from ), Req->argv[0] );

	if( Req->argc == 1 )
	{
		/* Request actual topic */
		topic = Channel_Topic(chan);
		if (*topic) {
			r = IRC_WriteStrClient(from, RPL_TOPIC_MSG,
				Client_ID(Client), Channel_Name(chan), topic);
#ifndef STRICT_RFC
			r = IRC_WriteStrClient(from, RPL_TOPICSETBY_MSG,
				Client_ID(Client), Channel_Name(chan),
				Channel_TopicWho(chan),
				Channel_TopicTime(chan));
#endif
			return r;
		}
		else
			 return IRC_WriteStrClient(from, RPL_NOTOPIC_MSG,
					Client_ID(from), Channel_Name(chan));
	}

	if( strchr( Channel_Modes( chan ), 't' ))
	{
		/* Topic Lock. Ist der User ein Channel Operator? */
		if( ! strchr( Channel_UserModes( chan, from ), 'o' )) return IRC_WriteStrClient( from, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( from ), Channel_Name( chan ));
	}

	/* Set new topic */
	Channel_SetTopic(chan, from, Req->argv[1]);
	Log(LOG_DEBUG, "User \"%s\" set topic on \"%s\": %s",
		Client_Mask(from), Channel_Name(chan),
		Req->argv[1][0] ? Req->argv[1] : "<none>");

	/* im Channel bekannt machen und an Server weiterleiten */
	IRC_WriteStrServersPrefix( Client, from, "TOPIC %s :%s", Req->argv[0], Req->argv[1] );
	IRC_WriteStrChannelPrefix( Client, chan, from, false, "TOPIC %s :%s", Req->argv[0], Req->argv[1] );

	if( Client_Type( Client ) == CLIENT_USER ) return IRC_WriteStrClientPrefix( Client, Client, "TOPIC %s :%s", Req->argv[0], Req->argv[1] );
	else return CONNECTED;
} /* IRC_TOPIC */


/**
 * Handler for the IRC "LIST" command.
 * This implementation handles the local case as well as the forwarding of the
 * LIST command to other servers in the IRC network.
 */
GLOBAL bool
IRC_LIST( CLIENT *Client, REQUEST *Req )
{
	char *pattern;
	CHANNEL *chan;
	CLIENT *from, *target;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Bad number of prameters? */
	if( Req->argc > 2 )
		return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG,
			Client_ID( Client ), Req->command );

	if( Req->argc > 0 )
		pattern = strtok( Req->argv[0], "," );
	else
		pattern = "*";

	/* Get sender from prefix, if any */
	if( Client_Type( Client ) == CLIENT_SERVER )
		from = Client_Search( Req->prefix );
	else
		from = Client;

	if( ! from )
		return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG,
				Client_ID( Client ), Req->prefix );

	if( Req->argc == 2 )
	{
		/* Forward to other server? */
		target = Client_Search( Req->argv[1] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER ))
			return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG,
					Client_ID( Client ), Req->argv[1] );

		if( target != Client_ThisServer( ))
		{
			/* Target is indeed an other server, forward it! */
			return IRC_WriteStrClientPrefix( target, from,
					"LIST %s :%s", Client_ID( from ),
					Req->argv[1] );
		}
	}
	
	while( pattern )
	{
		/* Loop through all the channels */
		chan = Channel_First( );
		while( chan )
		{
			/* Check search pattern */
			if( Match( pattern, Channel_Name( chan )))
			{
				/* Gotcha! */
				if( ! strchr( Channel_Modes( chan ), 's' ) ||
				    Channel_IsMemberOf( chan, from ))
				{
					if( ! IRC_WriteStrClient( from,
					    RPL_LIST_MSG, Client_ID( from ),
					    Channel_Name( chan ),
					    Channel_MemberCount( chan ),
					    Channel_Topic( chan )))
						return DISCONNECTED;
				}
			}
			chan = Channel_Next( chan );
		}
		
		/* Get next name ... */
		if( Req->argc > 0 )
			pattern = strtok( NULL, "," );
		else
			pattern = NULL;
	}
	
	return IRC_WriteStrClient( from, RPL_LISTEND_MSG, Client_ID( from ));
} /* IRC_LIST */


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
	if(( Req->argc < 2 ) || ( Req->argc == 4 ) || ( Req->argc > 5 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

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
