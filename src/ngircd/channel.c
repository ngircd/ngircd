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

#define __channel_c__

#include "portab.h"

/**
 * @file
 * Channel management
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <time.h>

#include "conn-func.h"

#include "channel.h"

#include "irc-write.h"
#include "conf.h"
#include "hash.h"
#include "log.h"
#include "messages.h"
#include "match.h"
#include "parse.h"
#include "irc-mode.h"

#define REMOVE_PART 0
#define REMOVE_QUIT 1
#define REMOVE_KICK 2

static CHANNEL *My_Channels;
static CL2CHAN *My_Cl2Chan;

static CL2CHAN *Get_Cl2Chan PARAMS(( CHANNEL *Chan, CLIENT *Client ));
static CL2CHAN *Add_Client PARAMS(( CHANNEL *Chan, CLIENT *Client ));
static bool Remove_Client PARAMS(( int Type, CHANNEL *Chan, CLIENT *Client, CLIENT *Origin, const char *Reason, bool InformServer ));
static CL2CHAN *Get_First_Cl2Chan PARAMS(( CLIENT *Client, CHANNEL *Chan ));
static CL2CHAN *Get_Next_Cl2Chan PARAMS(( CL2CHAN *Start, CLIENT *Client, CHANNEL *Chan ));
static void Delete_Channel PARAMS(( CHANNEL *Chan ));
static void Free_Channel PARAMS(( CHANNEL *Chan ));
static void Set_KeyFile PARAMS((CHANNEL *Chan, const char *KeyFile));


GLOBAL void
Channel_Init( void )
{
	My_Channels = NULL;
	My_Cl2Chan = NULL;
} /* Channel_Init */


GLOBAL struct list_head *
Channel_GetListBans(CHANNEL *c)
{
	assert(c != NULL);
	return &c->list_bans;
}


GLOBAL struct list_head *
Channel_GetListExcepts(CHANNEL *c)
{
	assert(c != NULL);
	return &c->list_excepts;
}


GLOBAL struct list_head *
Channel_GetListInvites(CHANNEL *c)
{
	assert(c != NULL);
	return &c->list_invites;
}


/**
 * Generate predefined persistent channels and &SERVER
 */
GLOBAL void
Channel_InitPredefined( void )
{
	CHANNEL *new_chan;
	REQUEST Req;
	const struct Conf_Channel *conf_chan;
	char *c;
	char modes[COMMAND_LEN], name[CHANNEL_NAME_LEN];
	size_t i, n, channel_count = array_length(&Conf_Channels, sizeof(*conf_chan));

	conf_chan = array_start(&Conf_Channels);

	assert(channel_count == 0 || conf_chan != NULL);

	for (i = 0; i < channel_count; i++, conf_chan++) {
		if (!conf_chan->name[0])
			continue;
		if (!Channel_IsValidName(conf_chan->name)) {
			Log(LOG_ERR,
			    "Can't create pre-defined channel: invalid name: \"%s\"",
			    conf_chan->name);
			continue;
		}

		new_chan = Channel_Search(conf_chan->name);
		if (new_chan) {
			Log(LOG_INFO,
			    "Can't create pre-defined channel \"%s\": name already in use.",
			    conf_chan->name);
			Set_KeyFile(new_chan, conf_chan->keyfile);
			continue;
		}

		new_chan = Channel_Create(conf_chan->name);
		if (!new_chan) {
			Log(LOG_ERR, "Can't create pre-defined channel \"%s\"!",
							conf_chan->name);
			continue;
		}
		Channel_ModeAdd(new_chan, 'P');

		if (conf_chan->topic[0])
			Channel_SetTopic(new_chan, NULL, conf_chan->topic);

		/* Evaluate modes strings with fake requests */
		if (conf_chan->modes_num) {
			/* Prepare fake request structure */
			strlcpy(name, conf_chan->name, sizeof(name));
			LogDebug("Evaluating predefined channel modes for \"%s\" ...", name);
			Req.argv[0] = name;
			Req.prefix = Client_ID(Client_ThisServer());
			Req.command = "MODE";

			/* Iterate over channel modes strings */
			for (n = 0; n < conf_chan->modes_num; n++) {
				Req.argc = 1;
				strlcpy(modes, conf_chan->modes[n], sizeof(modes));
				LogDebug("Evaluate \"MODE %s %s\".", name, modes);
				c = strtok(modes, " ");
				while (c && Req.argc < 15) {
					Req.argv[Req.argc++] = c;
					c = strtok(0, " ");
				}

				if (Req.argc > 1) {
					/* Handling of legacy "Key" and "MaxUsers" settings:
					 * Enforce setting the respective mode(s), to support
					 * the legacy "Mode = kl" notation, which was valid but
					 * is an invalid MODE string: key and limit are missing!
					 * So set them manually when "k" or "l" are detected in
					 * the first MODE parameter ... */
					if (Req.argc > 1 && strchr(Req.argv[1], 'k')) {
						Channel_SetKey(new_chan, conf_chan->key);
						Channel_ModeAdd(new_chan, 'k');
					}
					if (strchr(Req.argv[1], 'l')) {
						Channel_SetMaxUsers(new_chan, conf_chan->maxusers);
						Channel_ModeAdd(new_chan, 'l');
					}

					IRC_MODE(Client_ThisServer(), &Req);
				}

				/* Original channel modes strings are no longer needed */
				free(conf_chan->modes[n]);
			}
		}

		Set_KeyFile(new_chan, conf_chan->keyfile);

		Log(LOG_INFO,
		    "Created pre-defined channel \"%s\", mode \"%s\" (%s, user limit %d).",
		    new_chan->name, new_chan->modes,
		    new_chan->key[0] ? "channel key set" : "no channel key",
		    new_chan->maxusers);
	}

	/* Make sure the local &SERVER channel exists */
	if (!Channel_Search("&SERVER")) {
		new_chan = Channel_Create("&SERVER");
		if (new_chan) {
			Channel_SetModes(new_chan, "mnPt");
			Channel_SetTopic(new_chan, Client_ThisServer(),
					 "Server Messages");
		} else
			Log(LOG_ERR, "Failed to create \"&SERVER\" channel!");
	} else
		LogDebug("Required channel \"&SERVER\" already exists, ok.");
} /* Channel_InitPredefined */


static void
Free_Channel(CHANNEL *chan)
{
	array_free(&chan->topic);
	array_free(&chan->keyfile);
	Lists_Free(&chan->list_bans);
	Lists_Free(&chan->list_excepts);
	Lists_Free(&chan->list_invites);

	free(chan);
}


GLOBAL void
Channel_Exit( void )
{
	CHANNEL *c, *c_next;
	CL2CHAN *cl2chan, *cl2chan_next;

	/* free struct Channel */
	c = My_Channels;
	while (c) {
		c_next = c->next;
		Free_Channel(c);
		c = c_next;
	}

	/* Free Channel allocation table */
	cl2chan = My_Cl2Chan;
	while (cl2chan) {
		cl2chan_next = cl2chan->next;
		free(cl2chan);
		cl2chan = cl2chan_next;
	}
} /* Channel_Exit */


/**
 * Join Channel
 * This function lets a client join a channel.  First, the function
 * checks that the specified channel name is valid and that the client
 * isn't already a member.  If the specified channel doesn't exist,
 * a new channel is created.  Client is added to channel by function
 * Add_Client().
 */
GLOBAL bool
Channel_Join( CLIENT *Client, const char *Name )
{
	CHANNEL *chan;

	assert(Client != NULL);
	assert(Name != NULL);

	/* Check that the channel name is valid */
	if (! Channel_IsValidName(Name)) {
		IRC_WriteErrClient(Client, ERR_NOSUCHCHANNEL_MSG,
				   Client_ID(Client), Name);
		return false;
	}

	chan = Channel_Search(Name);
	if(chan) {
		/* Check if the client is already in the channel */
		if (Get_Cl2Chan(chan, Client))
			return false;
	} else {
		/* If the specified channel does not exist, the channel
		 * is now created */
		chan = Channel_Create(Name);
		if (!chan)
			return false;
	}

	/* Add user to Channel */
	if (! Add_Client(chan, Client))
		return false;

	return true;
} /* Channel_Join */


/**
 * Part client from channel.
 * This function lets a client part from a channel. First, the function checks
 * if the channel exists and the client is a member of it and sends out
 * appropriate error messages if not. The real work is done by the function
 * Remove_Client().
 */
GLOBAL bool
Channel_Part(CLIENT * Client, CLIENT * Origin, const char *Name, const char *Reason)
{
	CHANNEL *chan;

	assert(Client != NULL);
	assert(Name != NULL);
	assert(Reason != NULL);

	/* Check that specified channel exists */
	chan = Channel_Search(Name);
	if (!chan) {
		IRC_WriteErrClient(Client, ERR_NOSUCHCHANNEL_MSG,
				   Client_ID(Client), Name);
		return false;
	}

	/* Check that the client is in the channel */
	if (!Get_Cl2Chan(chan, Client)) {
		IRC_WriteErrClient(Client, ERR_NOTONCHANNEL_MSG,
				   Client_ID(Client), Name);
		return false;
	}

	if (Conf_MorePrivacy)
		Reason = "";

	/* Part client from channel */
	if (!Remove_Client(REMOVE_PART, chan, Client, Origin, Reason, true))
		return false;
	else
		return true;
} /* Channel_Part */


/**
 * Kick user from Channel
 */
GLOBAL void
Channel_Kick(CLIENT *Peer, CLIENT *Target, CLIENT *Origin, const char *Name,
	     const char *Reason )
{
	CHANNEL *chan;
	bool can_kick = false;

	assert(Peer != NULL);
	assert(Target != NULL);
	assert(Origin != NULL);
	assert(Name != NULL);
	assert(Reason != NULL);

	/* Check that channel exists */
	chan = Channel_Search( Name );
	if (!chan) {
		IRC_WriteErrClient(Origin, ERR_NOSUCHCHANNEL_MSG,
				   Client_ID(Origin), Name);
		return;
	}

	if (Client_Type(Peer) != CLIENT_SERVER &&
	    Client_Type(Origin) != CLIENT_SERVICE) {
		/* Check that user is on the specified channel */
		if (!Channel_IsMemberOf(chan, Origin)) {
			IRC_WriteErrClient(Origin, ERR_NOTONCHANNEL_MSG,
					   Client_ID(Origin), Name);
			return;
		}
	}

	/* Check that the client to be kicked is on the specified channel */
	if (!Channel_IsMemberOf(chan, Target)) {
		IRC_WriteErrClient(Origin, ERR_USERNOTINCHANNEL_MSG,
				   Client_ID(Origin), Client_ID(Target), Name );
		return;
	}

	if(Client_Type(Peer) == CLIENT_USER) {
		/* Channel mode 'Q' and user mode 'q' on target: nobody but
		 * IRC Operators and servers can kick the target user */
		if ((Channel_HasMode(chan, 'Q')
		     || Client_HasMode(Target, 'q')
		     || Client_Type(Target) == CLIENT_SERVICE)
		    && !Client_HasMode(Origin, 'o')) {
			IRC_WriteErrClient(Origin, ERR_KICKDENY_MSG,
					   Client_ID(Origin), Name,
					   Client_ID(Target));
			return;
		}

		/* Check if client has the rights to kick target */

		/* Owner can kick everyone */
		if (Channel_UserHasMode(chan, Peer, 'q'))
			can_kick = true;

		/* Admin can't kick owner */
		else if (Channel_UserHasMode(chan, Peer, 'a') &&
		    !Channel_UserHasMode(chan, Target, 'q'))
			can_kick = true;

		/* Op can't kick owner | admin */
		else if (Channel_UserHasMode(chan, Peer, 'o') &&
		    !Channel_UserHasMode(chan, Target, 'q') &&
		    !Channel_UserHasMode(chan, Target, 'a'))
			can_kick = true;

		/* Half Op can't kick owner | admin | op */
		else if (Channel_UserHasMode(chan, Peer, 'h') &&
		    !Channel_UserHasMode(chan, Target, 'q') &&
		    !Channel_UserHasMode(chan, Target, 'a') &&
		    !Channel_UserHasMode(chan, Target, 'o'))
			can_kick = true;

		/* IRC operators & IRCd with OperCanMode enabled
		 * can kick anyways regardless of privilege */
		else if(Client_HasMode(Origin, 'o') && Conf_OperCanMode)
		    can_kick = true;

		if(!can_kick) {
			IRC_WriteErrClient(Origin, ERR_CHANOPPRIVTOOLOW_MSG,
					   Client_ID(Origin), Name);
			return;
		}
	}

	/* Kick Client from channel */
	Remove_Client( REMOVE_KICK, chan, Target, Origin, Reason, true);
} /* Channel_Kick */


GLOBAL void
Channel_Quit( CLIENT *Client, const char *Reason )
{
	CHANNEL *c, *next_c;

	assert( Client != NULL );
	assert( Reason != NULL );

	if (Conf_MorePrivacy)
		Reason = "";

	IRC_WriteStrRelatedPrefix( Client, Client, false, "QUIT :%s", Reason );

	c = My_Channels;
	while( c )
	{
		next_c = c->next;
		Remove_Client( REMOVE_QUIT, c, Client, Client, Reason, false );
		c = next_c;
	}
} /* Channel_Quit */


/**
 * Get number of channels this server knows and that are "visible" to
 * the given client. If no client is given, all channels will be counted.
 *
 * @param Client The client to check or NULL.
 * @return Number of channels visible to the client.
 */
GLOBAL unsigned long
Channel_CountVisible (CLIENT *Client)
{
	CHANNEL *c;
	unsigned long count = 0;

	c = My_Channels;
	while(c) {
		if (Client) {
			if (!Channel_HasMode(c, 's')
			    || Channel_IsMemberOf(c, Client))
				count++;
		} else
			count++;
		c = c->next;
	}
	return count;
}


GLOBAL unsigned long
Channel_MemberCount( CHANNEL *Chan )
{
	CL2CHAN *cl2chan;
	unsigned long count = 0;

	assert( Chan != NULL );

	cl2chan = My_Cl2Chan;
	while( cl2chan )
	{
		if( cl2chan->channel == Chan ) count++;
		cl2chan = cl2chan->next;
	}
	return count;
} /* Channel_MemberCount */


GLOBAL int
Channel_CountForUser( CLIENT *Client )
{
	/* Count number of channels a user is member of. */

	CL2CHAN *cl2chan;
	int count = 0;

	assert( Client != NULL );

	cl2chan = My_Cl2Chan;
	while( cl2chan )
	{
		if( cl2chan->client == Client ) count++;
		cl2chan = cl2chan->next;
	}

	return count;
} /* Channel_CountForUser */


GLOBAL const char *
Channel_Name( const CHANNEL *Chan )
{
	assert( Chan != NULL );
	return Chan->name;
} /* Channel_Name */


GLOBAL char *
Channel_Modes( CHANNEL *Chan )
{
	assert( Chan != NULL );
	return Chan->modes;
} /* Channel_Modes */


GLOBAL bool
Channel_HasMode( CHANNEL *Chan, char Mode )
{
	assert( Chan != NULL );
	return strchr( Chan->modes, Mode ) != NULL;
} /* Channel_HasMode */


GLOBAL char *
Channel_Key( CHANNEL *Chan )
{
	assert( Chan != NULL );
	return Chan->key;
} /* Channel_Key */


GLOBAL unsigned long
Channel_MaxUsers( CHANNEL *Chan )
{
	assert( Chan != NULL );
	return Chan->maxusers;
} /* Channel_MaxUsers */


GLOBAL CHANNEL *
Channel_First( void )
{
	return My_Channels;
} /* Channel_First */


GLOBAL CHANNEL *
Channel_Next( CHANNEL *Chan )
{
	assert( Chan != NULL );
	return Chan->next;
} /* Channel_Next */


GLOBAL CHANNEL *
Channel_Search( const char *Name )
{
	/* Search channel structure */

	CHANNEL *c;
	UINT32 search_hash;

	assert( Name != NULL );

	search_hash = Hash( Name );
	c = My_Channels;
	while( c )
	{
		if( search_hash == c->hash )
		{
			/* hash hit */
			if( strcasecmp( Name, c->name ) == 0 ) return c;
		}
		c = c->next;
	}
	return NULL;
} /* Channel_Search */


GLOBAL CL2CHAN *
Channel_FirstMember( CHANNEL *Chan )
{
	assert( Chan != NULL );
	return Get_First_Cl2Chan( NULL, Chan );
} /* Channel_FirstMember */


GLOBAL CL2CHAN *
Channel_NextMember( CHANNEL *Chan, CL2CHAN *Cl2Chan )
{
	assert( Chan != NULL );
	assert( Cl2Chan != NULL );
	return Get_Next_Cl2Chan( Cl2Chan->next, NULL, Chan );
} /* Channel_NextMember */


GLOBAL CL2CHAN *
Channel_FirstChannelOf( CLIENT *Client )
{
	assert( Client != NULL );
	return Get_First_Cl2Chan( Client, NULL );
} /* Channel_FirstChannelOf */


GLOBAL CL2CHAN *
Channel_NextChannelOf( CLIENT *Client, CL2CHAN *Cl2Chan )
{
	assert( Client != NULL );
	assert( Cl2Chan != NULL );
	return Get_Next_Cl2Chan( Cl2Chan->next, Client, NULL );
} /* Channel_NextChannelOf */


GLOBAL CLIENT *
Channel_GetClient( CL2CHAN *Cl2Chan )
{
	assert( Cl2Chan != NULL );
	return Cl2Chan->client;
} /* Channel_GetClient */


GLOBAL CHANNEL *
Channel_GetChannel( CL2CHAN *Cl2Chan )
{
	assert( Cl2Chan != NULL );
	return Cl2Chan->channel;
} /* Channel_GetChannel */


GLOBAL bool
Channel_IsValidName( const char *Name )
{
	assert( Name != NULL );

#ifdef STRICT_RFC
	if (strlen(Name) <= 1)
		return false;
#endif
	if (strchr("#&+", Name[0]) == NULL)
		return false;
	if (strlen(Name) >= CHANNEL_NAME_LEN)
		return false;

	return Name[strcspn(Name, " ,:\007")] == 0;
} /* Channel_IsValidName */


GLOBAL bool
Channel_ModeAdd( CHANNEL *Chan, char Mode )
{
	/* set Mode.
	 * If the channel already had this mode, return false.
	 * If the channel mode was newly set return true.
	 */

	char x[2];

	assert( Chan != NULL );

	x[0] = Mode; x[1] = '\0';
	if( ! Channel_HasMode( Chan, x[0] ))
	{
		/* Channel does not have this mode yet, set it */
		strlcat( Chan->modes, x, sizeof( Chan->modes ));
		return true;
	}
	else return false;
} /* Channel_ModeAdd */


GLOBAL bool
Channel_ModeDel( CHANNEL *Chan, char Mode )
{
	/* Delete mode.
	 * if the mode was removed return true.
	 * if the channel did not have the mode, return false.
	*/
	char *p;

	assert( Chan != NULL );

	p = strchr( Chan->modes, Mode );
	if( ! p ) return false;

	/* Channel has mode -> delete */
	while( *p )
	{
		*p = *(p + 1);
		p++;
	}
	return true;
} /* Channel_ModeDel */


GLOBAL bool
Channel_UserModeAdd( CHANNEL *Chan, CLIENT *Client, char Mode )
{
	/* Set Channel-User-Mode.
	 * if mode was newly set, return true.
	 * if the User already had this channel-mode, return false.
	 */

	CL2CHAN *cl2chan;
	char x[2];

	assert( Chan != NULL );
	assert( Client != NULL );

	cl2chan = Get_Cl2Chan( Chan, Client );
	assert( cl2chan != NULL );

	x[0] = Mode; x[1] = '\0';
	if( ! strchr( cl2chan->modes, x[0] ))
	{
		/* mode not set, -> set it */
		strlcat( cl2chan->modes, x, sizeof( cl2chan->modes ));
		return true;
	}
	else return false;
} /* Channel_UserModeAdd */


GLOBAL bool
Channel_UserModeDel( CHANNEL *Chan, CLIENT *Client, char Mode )
{
	/* Delete Channel-User-Mode.
	 * If Mode was removed, return true.
	 * If User did not have the Channel-Mode, return false.
	 */

	CL2CHAN *cl2chan;
	char *p;

	assert( Chan != NULL );
	assert( Client != NULL );

	cl2chan = Get_Cl2Chan( Chan, Client );
	assert( cl2chan != NULL );

	p = strchr( cl2chan->modes, Mode );
	if( ! p ) return false;

	/* Client has Mode -> delete */
	while( *p )
	{
		*p = *(p + 1);
		p++;
	}
	return true;
} /* Channel_UserModeDel */


GLOBAL char *
Channel_UserModes( CHANNEL *Chan, CLIENT *Client )
{
	/* return Users' Channel-Modes */

	CL2CHAN *cl2chan;

	assert( Chan != NULL );
	assert( Client != NULL );

	cl2chan = Get_Cl2Chan( Chan, Client );
	assert( cl2chan != NULL );

	return cl2chan->modes;
} /* Channel_UserModes */


/**
 * Test if a user has a given channel user mode.
 *
 * @param Chan The channel to check.
 * @param Client The client to check.
 * @param Mode The channel user mode to test for.
 * @return true if the user has the given channel user mode set.
 */
GLOBAL bool
Channel_UserHasMode( CHANNEL *Chan, CLIENT *Client, char Mode )
{
	char *channel_user_modes;

	assert(Chan != NULL);
	assert(Client != NULL);
	assert(Mode > 0);

	channel_user_modes = Channel_UserModes(Chan, Client);
	if (!channel_user_modes || !*channel_user_modes)
		return false;

	return strchr(channel_user_modes, Mode) != NULL;
} /* Channel_UserHasMode */


GLOBAL bool
Channel_IsMemberOf( CHANNEL *Chan, CLIENT *Client )
{
	/* Test if Client is on Channel Chan */

	assert( Chan != NULL );
	assert( Client != NULL );
	return Get_Cl2Chan(Chan, Client) != NULL;
} /* Channel_IsMemberOf */


GLOBAL char *
Channel_Topic( CHANNEL *Chan )
{
	char *ret;
	assert( Chan != NULL );
	ret = array_start(&Chan->topic);
	return ret ? ret : "";
} /* Channel_Topic */


#ifndef STRICT_RFC

GLOBAL unsigned int
Channel_TopicTime(CHANNEL *Chan)
{
	assert(Chan != NULL);
	return (unsigned int) Chan->topic_time;
} /* Channel_TopicTime */


GLOBAL char *
Channel_TopicWho(CHANNEL *Chan)
{
	assert(Chan != NULL);
	return Chan->topic_who;
} /* Channel_TopicWho */


GLOBAL unsigned int
Channel_CreationTime(CHANNEL *Chan)
{
	assert(Chan != NULL);
	return (unsigned int) Chan->creation_time;
} /* Channel_CreationTime */

#endif


GLOBAL void
Channel_SetTopic(CHANNEL *Chan, CLIENT *Client, const char *Topic)
{
	size_t len;
	assert( Chan != NULL );
	assert( Topic != NULL );

	len = strlen(Topic);
	if (len < array_bytes(&Chan->topic))
		array_free(&Chan->topic);

	if (len >= COMMAND_LEN || !array_copyb(&Chan->topic, Topic, len+1))
		Log(LOG_WARNING, "could not set new Topic \"%s\" on %s: %s",
					Topic, Chan->name, strerror(errno));
#ifndef STRICT_RFC
	Chan->topic_time = time(NULL);
	if (Client != NULL && Client_Type(Client) != CLIENT_SERVER)
		strlcpy(Chan->topic_who, Client_ID(Client),
			sizeof Chan->topic_who);
	else
		strlcpy(Chan->topic_who, DEFAULT_TOPIC_ID,
			sizeof Chan->topic_who);
#else
	(void) Client;
#endif
} /* Channel_SetTopic */


GLOBAL void
Channel_SetModes( CHANNEL *Chan, const char *Modes )
{
	assert( Chan != NULL );
	assert( Modes != NULL );

	strlcpy( Chan->modes, Modes, sizeof( Chan->modes ));
} /* Channel_SetModes */


GLOBAL void
Channel_SetKey( CHANNEL *Chan, const char *Key )
{
	assert( Chan != NULL );
	assert( Key != NULL );

	strlcpy( Chan->key, Key, sizeof( Chan->key ));
	LogDebug("Channel %s: Key is now \"%s\".", Chan->name, Chan->key );
} /* Channel_SetKey */


GLOBAL void
Channel_SetMaxUsers(CHANNEL *Chan, unsigned long Count)
{
	assert( Chan != NULL );

	Chan->maxusers = Count;
	LogDebug("Channel %s: Member limit is now %lu.", Chan->name, Chan->maxusers );
} /* Channel_SetMaxUsers */


/**
 * Check if a client is allowed to send to a specific channel.
 *
 * @param Chan The channel to check.
 * @param From The client that wants to send.
 * @return true if the client is allowed to send, false otherwise.
 */
static bool
Can_Send_To_Channel(CHANNEL *Chan, CLIENT *From)
{
	bool is_member, has_voice, is_halfop, is_op, is_chanadmin, is_owner;

	is_member = has_voice = is_halfop = is_op = is_chanadmin = is_owner = false;

	/* The server itself always can send messages :-) */
	if (Client_ThisServer() == From)
		return true;

	if (Channel_IsMemberOf(Chan, From)) {
		is_member = true;
		if (Channel_UserHasMode(Chan, From, 'v'))
			has_voice = true;
		if (Channel_UserHasMode(Chan, From, 'h'))
			is_halfop = true;
		if (Channel_UserHasMode(Chan, From, 'o'))
			is_op = true;
		if (Channel_UserHasMode(Chan, From, 'a'))
			is_chanadmin = true;
		if (Channel_UserHasMode(Chan, From, 'q'))
			is_owner = true;
	}

	/*
	 * Is the client allowed to write to channel?
	 *
	 * If channel mode n set: non-members cannot send to channel.
	 * If channel mode m set: need voice.
	 */
	if (Channel_HasMode(Chan, 'n') && !is_member)
		return false;

	if (Channel_HasMode(Chan, 'M') && !Client_HasMode(From, 'R')
	    && !Client_HasMode(From, 'o'))
		return false;

	if (has_voice || is_halfop || is_op || is_chanadmin || is_owner)
		return true;

	if (Channel_HasMode(Chan, 'm'))
		return false;

	if (Lists_Check(&Chan->list_excepts, From))
		return true;

	return !Lists_Check(&Chan->list_bans, From);
}


GLOBAL bool
Channel_Write(CHANNEL *Chan, CLIENT *From, CLIENT *Client, const char *Command,
	      bool SendErrors, const char *Text)
{
	if (!Can_Send_To_Channel(Chan, From)) {
		if (! SendErrors)
			return CONNECTED;	/* no error, see RFC 2812 */
		if (Channel_HasMode(Chan, 'M'))
			return IRC_WriteErrClient(From, ERR_NEEDREGGEDNICK_MSG,
						  Client_ID(From), Channel_Name(Chan));
		else
			return IRC_WriteErrClient(From, ERR_CANNOTSENDTOCHAN_MSG,
					  Client_ID(From), Channel_Name(Chan));
	}

	if (Client_Conn(From) > NONE)
		Conn_UpdateIdle(Client_Conn(From));

	IRC_WriteStrChannelPrefix(Client, Chan, From, true, "%s %s :%s",
				  Command, Channel_Name(Chan), Text);
	return CONNECTED;
}


GLOBAL CHANNEL *
Channel_Create( const char *Name )
{
	/* Create new CHANNEL structure and add it to linked list */
	CHANNEL *c;

	assert( Name != NULL );

	c = (CHANNEL *)malloc( sizeof( CHANNEL ));
	if( ! c )
	{
		Log( LOG_EMERG, "Can't allocate memory! [New_Chan]" );
		return NULL;
	}
	memset( c, 0, sizeof( CHANNEL ));
	strlcpy( c->name, Name, sizeof( c->name ));
	c->hash = Hash( c->name );
	c->next = My_Channels;
#ifndef STRICT_RFC
	c->creation_time = time(NULL);
#endif
	My_Channels = c;
	LogDebug("Created new channel structure for \"%s\".", Name);
	return c;
} /* Channel_Create */


static CL2CHAN *
Get_Cl2Chan( CHANNEL *Chan, CLIENT *Client )
{
	CL2CHAN *cl2chan;

	assert( Chan != NULL );
	assert( Client != NULL );

	cl2chan = My_Cl2Chan;
	while( cl2chan )
	{
		if(( cl2chan->channel == Chan ) && ( cl2chan->client == Client )) return cl2chan;
		cl2chan = cl2chan->next;
	}
	return NULL;
} /* Get_Cl2Chan */


static CL2CHAN *
Add_Client( CHANNEL *Chan, CLIENT *Client )
{
	CL2CHAN *cl2chan;

	assert( Chan != NULL );
	assert( Client != NULL );

	/* Create new CL2CHAN structure */
	cl2chan = (CL2CHAN *)malloc( sizeof( CL2CHAN ));
	if( ! cl2chan )
	{
		Log( LOG_EMERG, "Can't allocate memory! [Add_Client]" );
		return NULL;
	}
	cl2chan->channel = Chan;
	cl2chan->client = Client;
	strcpy( cl2chan->modes, "" );

	/* concatenate */
	cl2chan->next = My_Cl2Chan;
	My_Cl2Chan = cl2chan;

	LogDebug("User \"%s\" joined channel \"%s\".", Client_Mask(Client), Chan->name);

	return cl2chan;
} /* Add_Client */


static bool
Remove_Client( int Type, CHANNEL *Chan, CLIENT *Client, CLIENT *Origin, const char *Reason, bool InformServer )
{
	CL2CHAN *cl2chan, *last_cl2chan;
	CHANNEL *c;

	assert( Chan != NULL );
	assert( Client != NULL );
	assert( Origin != NULL );
	assert( Reason != NULL );

	/* Do not inform other servers if the channel is local to this server,
	 * regardless of what the caller requested! */
	if(InformServer)
		InformServer = !Channel_IsLocal(Chan);

	last_cl2chan = NULL;
	cl2chan = My_Cl2Chan;
	while( cl2chan )
	{
		if(( cl2chan->channel == Chan ) && ( cl2chan->client == Client )) break;
		last_cl2chan = cl2chan;
		cl2chan = cl2chan->next;
	}
	if( ! cl2chan ) return false;

	c = cl2chan->channel;
	assert( c != NULL );

	/* maintain cl2chan list */
	if( last_cl2chan ) last_cl2chan->next = cl2chan->next;
	else My_Cl2Chan = cl2chan->next;
	free( cl2chan );

	switch( Type )
	{
		case REMOVE_QUIT:
			/* QUIT: other servers have already been notified,
			 * see Client_Destroy(); so only inform other clients
			 * in same channel. */
			assert( InformServer == false );
			LogDebug("User \"%s\" left channel \"%s\" (%s).",
					Client_Mask( Client ), c->name, Reason );
			break;
		case REMOVE_KICK:
			/* User was KICKed: inform other servers (public
			 * channels) and all users in the channel */
			if( InformServer )
				IRC_WriteStrServersPrefix( Client_NextHop( Origin ),
					Origin, "KICK %s %s :%s", c->name, Client_ID( Client ), Reason);
			IRC_WriteStrChannelPrefix(Client, c, Origin, false, "KICK %s %s :%s",
							c->name, Client_ID( Client ), Reason );
			if ((Client_Conn(Client) > NONE) &&
					(Client_Type(Client) == CLIENT_USER))
			{
				IRC_WriteStrClientPrefix(Client, Origin, "KICK %s %s :%s",
								c->name, Client_ID( Client ), Reason);
			}
			LogDebug("User \"%s\" has been kicked off \"%s\" by \"%s\": %s.",
				Client_Mask( Client ), c->name, Client_ID(Origin), Reason);
			break;
		default: /* PART */
			if (Conf_MorePrivacy)
				Reason = "";

			if (InformServer)
				IRC_WriteStrServersPrefix(Origin, Client, "PART %s :%s", c->name, Reason);

			IRC_WriteStrChannelPrefix(Origin, c, Client, false, "PART %s :%s",
									c->name, Reason);

			if ((Client_Conn(Origin) > NONE) &&
					(Client_Type(Origin) == CLIENT_USER))
			{
				IRC_WriteStrClientPrefix( Origin, Client, "PART %s :%s", c->name, Reason);
				LogDebug("User \"%s\" left channel \"%s\" (%s).",
					Client_Mask(Client), c->name, Reason);
			}
	}

	/* When channel is empty and is not pre-defined, delete */
	if( ! Channel_HasMode( Chan, 'P' ))
	{
		if( ! Get_First_Cl2Chan( NULL, Chan )) Delete_Channel( Chan );
	}

	return true;
} /* Remove_Client */


GLOBAL bool
Channel_AddBan(CHANNEL *c, const char *mask, const char *who )
{
	struct list_head *h = Channel_GetListBans(c);
	LogDebug("Adding \"%s\" to \"%s\" ban list", mask, Channel_Name(c));
	return Lists_Add(h, mask, time(NULL), who, false);
}


GLOBAL bool
Channel_AddExcept(CHANNEL *c, const char *mask, const char *who )
{
	struct list_head *h = Channel_GetListExcepts(c);
	LogDebug("Adding \"%s\" to \"%s\" exception list", mask, Channel_Name(c));
	return Lists_Add(h, mask, time(NULL), who, false);
}


GLOBAL bool
Channel_AddInvite(CHANNEL *c, const char *mask, bool onlyonce, const char *who )
{
	struct list_head *h = Channel_GetListInvites(c);
	LogDebug("Adding \"%s\" to \"%s\" invite list", mask, Channel_Name(c));
	return Lists_Add(h, mask, time(NULL), who, onlyonce);
}


static bool
ShowChannelList(struct list_head *head, CLIENT *Client, CHANNEL *Channel,
		char *msg, char *msg_end)
{
	struct list_elem *e;

	assert (Client != NULL);
	assert (Channel != NULL);

	e = Lists_GetFirst(head);
	while (e) {
		if (!IRC_WriteStrClient(Client, msg, Client_ID(Client),
					Channel_Name(Channel),
					Lists_GetMask(e),
					Lists_GetReason(e),
					Lists_GetValidity(e)))
			return DISCONNECTED;
		e = Lists_GetNext(e);
	}

	return IRC_WriteStrClient(Client, msg_end, Client_ID(Client),
				  Channel_Name(Channel));
}


GLOBAL bool
Channel_ShowBans( CLIENT *Client, CHANNEL *Channel )
{
	struct list_head *h;

	assert( Channel != NULL );

	h = Channel_GetListBans(Channel);
	return ShowChannelList(h, Client, Channel, RPL_BANLIST_MSG,
			       RPL_ENDOFBANLIST_MSG);
}


GLOBAL bool
Channel_ShowExcepts( CLIENT *Client, CHANNEL *Channel )
{
	struct list_head *h;

	assert( Channel != NULL );

	h = Channel_GetListExcepts(Channel);
	return ShowChannelList(h, Client, Channel, RPL_EXCEPTLIST_MSG,
			       RPL_ENDOFEXCEPTLIST_MSG);
}


GLOBAL bool
Channel_ShowInvites( CLIENT *Client, CHANNEL *Channel )
{
	struct list_head *h;

	assert( Channel != NULL );

	h = Channel_GetListInvites(Channel);
	return ShowChannelList(h, Client, Channel, RPL_INVITELIST_MSG,
			       RPL_ENDOFINVITELIST_MSG);
}


/**
 * Log a message to the local &SERVER channel, if it exists.
 */
GLOBAL void
Channel_LogServer(const char *msg)
{
	CHANNEL *sc;
	CLIENT *c;

	assert(msg != NULL);

	sc = Channel_Search("&SERVER");
	if (!sc)
		return;

	c = Client_ThisServer();
	Channel_Write(sc, c, c, "PRIVMSG", false, msg);
} /* Channel_LogServer */


GLOBAL bool
Channel_CheckKey(CHANNEL *Chan, CLIENT *Client, const char *Key)
{
	char *file_name, line[COMMAND_LEN], *nick, *pass;
	FILE *fd;

	assert(Chan != NULL);
	assert(Client != NULL);
	assert(Key != NULL);

	if (!Channel_HasMode(Chan, 'k'))
		return true;
	if (*Key == '\0')
		return false;
	if (strcmp(Chan->key, Key) == 0)
		return true;

	file_name = array_start(&Chan->keyfile);
	if (!file_name)
		return false;
	fd = fopen(file_name, "r");
	if (!fd) {
		Log(LOG_ERR, "Can't open channel key file \"%s\" for %s: %s",
		    file_name, Chan->name, strerror(errno));
		return false;
	}

	while (fgets(line, (int)sizeof(line), fd) != NULL) {
		ngt_TrimStr(line);
		if (! (nick = strchr(line, ':')))
			continue;
		*nick++ = '\0';
		if (!Match(line, Client_User(Client)))
			continue;
		if (! (pass = strchr(nick, ':')))
			continue;
		*pass++ = '\0';
		if (!Match(nick, Client_ID(Client)))
			continue;
		if (strcmp(Key, pass) != 0)
			continue;

		fclose(fd);
		return true;
	}
	fclose(fd);
	return false;
} /* Channel_CheckKey */


static CL2CHAN *
Get_First_Cl2Chan( CLIENT *Client, CHANNEL *Chan )
{
	return Get_Next_Cl2Chan( My_Cl2Chan, Client, Chan );
} /* Get_First_Cl2Chan */


static CL2CHAN *
Get_Next_Cl2Chan( CL2CHAN *Start, CLIENT *Client, CHANNEL *Channel )
{
	CL2CHAN *cl2chan;

	assert( Client != NULL || Channel != NULL );

	cl2chan = Start;
	while( cl2chan )
	{
		if(( Client ) && ( cl2chan->client == Client )) return cl2chan;
		if(( Channel ) && ( cl2chan->channel == Channel )) return cl2chan;
		cl2chan = cl2chan->next;
	}
	return NULL;
} /* Get_Next_Cl2Chan */


/**
 * Remove a channel and free all of its data structures.
 */
static void
Delete_Channel(CHANNEL *Chan)
{
	CHANNEL *chan, *last_chan;

	last_chan = NULL;
	chan = My_Channels;
	while (chan) {
		if (chan == Chan)
			break;
		last_chan = chan;
		chan = chan->next;
	}

	assert(chan != NULL);
	if (!chan)
		return;

	/* maintain channel list */
	if (last_chan)
		last_chan->next = chan->next;
	else
		My_Channels = chan->next;

	LogDebug("Freed channel structure for \"%s\".", Chan->name);
	Free_Channel(Chan);
} /* Delete_Channel */


static void
Set_KeyFile(CHANNEL *Chan, const char *KeyFile)
{
	size_t len;

	assert(Chan != NULL);
	assert(KeyFile != NULL);

	len = strlen(KeyFile);
	if (len < array_bytes(&Chan->keyfile)) {
		Log(LOG_INFO, "Channel key file of %s removed.", Chan->name);
		array_free(&Chan->keyfile);
	}

	if (len < 1)
		return;

	if (!array_copyb(&Chan->keyfile, KeyFile, len+1))
		Log(LOG_WARNING,
		    "Could not set new channel key file \"%s\" for %s: %s",
		    KeyFile, Chan->name, strerror(errno));
	else
		Log(LOG_INFO|LOG_snotice,
		    "New local channel key file \"%s\" for %s activated.",
		    KeyFile, Chan->name);
} /* Set_KeyFile */


/* -eof- */
