/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * IRC commands for mode changes (MODE, AWAY, ...)
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "conn.h"
#include "client.h"
#include "channel.h"
#include "irc-write.h"
#include "lists.h"
#include "log.h"
#include "parse.h"
#include "messages.h"
#include "resolve.h"
#include "conf.h"

#include "exp.h"
#include "irc-mode.h"


static bool Client_Mode PARAMS(( CLIENT *Client, REQUEST *Req, CLIENT *Origin, CLIENT *Target ));
static bool Channel_Mode PARAMS(( CLIENT *Client, REQUEST *Req, CLIENT *Origin, CHANNEL *Channel ));

static bool Add_Ban_Invite PARAMS((int what, CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, const char *Pattern));
static bool Del_Ban_Invite PARAMS((int what, CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, const char *Pattern));

static bool Send_ListChange PARAMS(( char *Mode, CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, const char *Mask ));


GLOBAL bool
IRC_MODE( CLIENT *Client, REQUEST *Req )
{
	CLIENT *cl, *origin;
	CHANNEL *chan;

	assert( Client != NULL );
	assert( Req != NULL );

	/* No parameters? */
	if( Req->argc < 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Origin for answers */
	if( Client_Type( Client ) == CLIENT_SERVER )
	{
		origin = Client_Search( Req->prefix );
		if( ! origin ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );
	}
	else origin = Client;

	/* Channel or user mode? */
	cl = NULL; chan = NULL;
	if (Client_IsValidNick(Req->argv[0]))
		cl = Client_Search(Req->argv[0]);
	if (Channel_IsValidName(Req->argv[0]))
		chan = Channel_Search(Req->argv[0]);

	if (cl)
		return Client_Mode(Client, Req, origin, cl);
	if (chan)
		return Channel_Mode(Client, Req, origin, chan);

	/* No target found! */
	return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
			Client_ID(Client), Req->argv[0]);
} /* IRC_MODE */


static bool
Client_Mode( CLIENT *Client, REQUEST *Req, CLIENT *Origin, CLIENT *Target )
{
	/* Handle client mode requests */

	char the_modes[COMMAND_LEN], x[2], *mode_ptr;
	bool ok, set;
	int mode_arg;
	size_t len;

	/* Is the client allowed to request or change the modes? */
	if( Client_Type( Client ) == CLIENT_USER )
	{
		/* Users are only allowed to manipulate their own modes! */
		if( Target != Client ) return IRC_WriteStrClient( Client, ERR_USERSDONTMATCH_MSG, Client_ID( Client ));
	}

	/* Mode request: let's answer it :-) */
	if( Req->argc == 1 ) return IRC_WriteStrClient( Origin, RPL_UMODEIS_MSG, Client_ID( Origin ), Client_Modes( Target ));

	mode_arg = 1;
	mode_ptr = Req->argv[mode_arg];

	/* Initial state: set or unset modes? */
	if( *mode_ptr == '+' ) set = true;
	else if( *mode_ptr == '-' ) set = false;
	else return IRC_WriteStrClient( Origin, ERR_UMODEUNKNOWNFLAG_MSG, Client_ID( Origin ));

	/* Prepare reply string */
	if( set ) strcpy( the_modes, "+" );
	else strcpy( the_modes, "-" );

	x[1] = '\0';
	ok = CONNECTED;
	while( mode_ptr )
	{
		mode_ptr++;
		if( ! *mode_ptr )
		{
			/* Try next argument if there's any */
			mode_arg++;
			if( mode_arg < Req->argc ) mode_ptr = Req->argv[mode_arg];
			else break;
		}
		
		switch( *mode_ptr )
		{
			case '+':
			case '-':
				if((( *mode_ptr == '+' ) && ( ! set )) || (( *mode_ptr == '-' ) && ( set )))
				{
					/* Action modifier ("+"/"-") must be changed ... */
					len = strlen( the_modes ) - 1;
					if(( the_modes[len] == '+' ) || ( the_modes[len] == '-' ))
					{
						/* Adjust last action modifier in result */
						the_modes[len] = *mode_ptr;
					}
					else
					{
						/* Append modifier character to result string */
						x[0] = *mode_ptr;
						strlcat( the_modes, x, sizeof( the_modes ));
					}
					if( *mode_ptr == '+' ) set = true;
					else set = false;
				}
				continue;
		}
		
		/* Validate modes */
		x[0] = '\0';
		switch( *mode_ptr )
		{
			case 'i': /* Invisible */
			case 's': /* Server messages */
			case 'w': /* Wallops messages */
				x[0] = *mode_ptr;
				break;

			case 'a': /* Away */
				if( Client_Type( Client ) == CLIENT_SERVER )
				{
					x[0] = 'a';
					Client_SetAway( Origin, DEFAULT_AWAY_MSG );
				}
				else ok = IRC_WriteStrClient( Origin, ERR_NOPRIVILEGES_MSG, Client_ID( Origin ));
				break;

			case 'c': /* Receive connect notices
				   * (only settable by IRC operators!) */
				if(!set || Client_OperByMe(Origin)
				   || Client_Type(Client) == CLIENT_SERVER)
					x[0] = 'c';
				else
					ok = IRC_WriteStrClient(Origin,
							ERR_NOPRIVILEGES_MSG,
							Client_ID(Origin));
				break;
			case 'o': /* IRC operator (only unsettable!) */
				if(( ! set ) || ( Client_Type( Client ) == CLIENT_SERVER ))
				{
					Client_SetOperByMe( Target, false );
					x[0] = 'o';
				}
				else ok = IRC_WriteStrClient( Origin, ERR_NOPRIVILEGES_MSG, Client_ID( Origin ));
				break;

			case 'r': /* Restricted (only settable) */
				if(( set ) || ( Client_Type( Client ) == CLIENT_SERVER )) x[0] = 'r';
				else ok = IRC_WriteStrClient( Origin, ERR_RESTRICTED_MSG, Client_ID( Origin ));
				break;

			default:
				Log( LOG_DEBUG, "Unknown mode \"%c%c\" from \"%s\"!?", set ? '+' : '-', *mode_ptr, Client_ID( Origin ));
				if( Client_Type( Client ) != CLIENT_SERVER ) ok = IRC_WriteStrClient( Origin, ERR_UMODEUNKNOWNFLAG2_MSG, Client_ID( Origin ), set ? '+' : '-', *mode_ptr );
				x[0] = '\0';
				goto client_exit;
		}
		if( ! ok ) break;

		/* Is there a valid mode change? */
		if( ! x[0] ) continue;

		if( set )
		{
			/* Set mode */
			if( Client_ModeAdd( Target, x[0] )) strlcat( the_modes, x, sizeof( the_modes ));

		}
		else
		{
			/* Unset mode */
			if( Client_ModeDel( Target, x[0] )) strlcat( the_modes, x, sizeof( the_modes ));
		}		
	}
client_exit:
	
	/* Are there changed modes? */
	if( the_modes[1] )
	{
		/* Remoce needless action modifier characters */
		len = strlen( the_modes ) - 1;
		if(( the_modes[len] == '+' ) || ( the_modes[len] == '-' )) the_modes[len] = '\0';

		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			/* Forward modes to other servers */
			IRC_WriteStrServersPrefix( Client, Origin, "MODE %s :%s", Client_ID( Target ), the_modes );
		}
		else
		{
			/* Send reply to client and inform other servers */
			ok = IRC_WriteStrClientPrefix( Client, Origin, "MODE %s :%s", Client_ID( Target ), the_modes );
			IRC_WriteStrServersPrefix( Client, Origin, "MODE %s :%s", Client_ID( Target ), the_modes );
		}
		LogDebug("%s \"%s\": Mode change, now \"%s\".",
			 Client_TypeText(Target), Client_Mask(Target),
			 Client_Modes(Target));
	}
	
	IRC_SetPenalty( Client, 1 );	
	return ok;
} /* Client_Mode */


static bool
Channel_Mode_Answer_Request(CLIENT *Origin, CHANNEL *Channel)
{
	char the_modes[COMMAND_LEN], the_args[COMMAND_LEN], argadd[CLIENT_PASS_LEN];
	const char *mode_ptr;

	/* Member or not? -- That's the question! */
	if (!Channel_IsMemberOf(Channel, Origin))
		return IRC_WriteStrClient(Origin, RPL_CHANNELMODEIS_MSG,
			Client_ID(Origin), Channel_Name(Channel), Channel_Modes(Channel));

	/* The sender is a member: generate extended reply */
	strlcpy(the_modes, Channel_Modes(Channel), sizeof(the_modes));
	mode_ptr = the_modes;
	the_args[0] = '\0';

	while(*mode_ptr) {
		switch(*mode_ptr) {
		case 'l':
			snprintf(argadd, sizeof(argadd), " %lu", Channel_MaxUsers(Channel));
			strlcat(the_args, argadd, sizeof(the_args));
			break;
		case 'k':
			strlcat(the_args, " ", sizeof(the_args));
			strlcat(the_args, Channel_Key(Channel), sizeof(the_args));
			break;
		}
		mode_ptr++;
	}
	if (the_args[0])
		strlcat(the_modes, the_args, sizeof(the_modes));

	return IRC_WriteStrClient(Origin, RPL_CHANNELMODEIS_MSG,
		Client_ID(Origin), Channel_Name(Channel), the_modes);
}


/**
 * Handle channel mode and channel-user mode changes
 */
static bool
Channel_Mode(CLIENT *Client, REQUEST *Req, CLIENT *Origin, CHANNEL *Channel)
{
	char the_modes[COMMAND_LEN], the_args[COMMAND_LEN], x[2],
	    argadd[CLIENT_PASS_LEN], *mode_ptr;
	bool connected, set, modeok = true, skiponce, use_servermode = false, retval;
	int mode_arg, arg_arg;
	CLIENT *client;
	long l;
	size_t len;

	if (Channel_IsModeless(Channel))
		return IRC_WriteStrClient(Client, ERR_NOCHANMODES_MSG,
				Client_ID(Client), Channel_Name(Channel));

	/* Mode request: let's answer it :-) */
	if (Req->argc <= 1)
		return Channel_Mode_Answer_Request(Origin, Channel);

	/* Is the user allowed to change modes? */
	if (Client_Type(Client) == CLIENT_USER) {
		/* Is the originating user on that channel? */
		if (!Channel_IsMemberOf(Channel, Origin))
			return IRC_WriteStrClient(Origin, ERR_NOTONCHANNEL_MSG,
				Client_ID(Origin), Channel_Name(Channel));
		modeok = false;
		/* channel operator? */
		if (strchr(Channel_UserModes(Channel, Origin), 'o'))
			modeok = true;
		else if (Conf_OperCanMode) {
			/* IRC-Operators can use MODE as well */
			if (Client_OperByMe(Origin)) {
				modeok = true;
				if (Conf_OperServerMode)
					use_servermode = true; /* Change Origin to Server */
			}
		}
	}

	mode_arg = 1;
	mode_ptr = Req->argv[mode_arg];
	if (Req->argc > mode_arg + 1)
		arg_arg = mode_arg + 1;
	else
		arg_arg = -1;

	/* Initial state: set or unset modes? */
	skiponce = false;
	switch (*mode_ptr) {
	case '-':
		set = false;
		break;
	case '+':
		set = true;
		break;
	default:
		set = true;
		skiponce = true;
	}

	/* Prepare reply string */
	strcpy(the_modes, set ? "+" : "-");
	the_args[0] = '\0';

	x[1] = '\0';
	connected = CONNECTED;
	while (mode_ptr) {
		if (!skiponce)
			mode_ptr++;
		if (!*mode_ptr) {
			/* Try next argument if there's any */
			if (arg_arg > mode_arg)
				mode_arg = arg_arg;
			else
				mode_arg++;

			if (mode_arg >= Req->argc)
				break;
			mode_ptr = Req->argv[mode_arg];

			if (Req->argc > mode_arg + 1)
				arg_arg = mode_arg + 1;
			else
				arg_arg = -1;
		}
		skiponce = false;

		switch (*mode_ptr) {
		case '+':
		case '-':
			if (((*mode_ptr == '+') && !set)
			    || ((*mode_ptr == '-') && set)) {
				/* Action modifier ("+"/"-") must be changed ... */
				len = strlen(the_modes) - 1;
				if (the_modes[len] == '+' || the_modes[len] == '-') {
					/* Adjust last action modifier in result */
					the_modes[len] = *mode_ptr;
				} else {
					/* Append modifier character to result string */
					x[0] = *mode_ptr;
					strlcat(the_modes, x, sizeof(the_modes));
				}
				set = *mode_ptr == '+';
			}
			continue;
		}

		/* Are there arguments left? */
		if (arg_arg >= Req->argc)
			arg_arg = -1;

		/* Validate modes */
		x[0] = '\0';
		argadd[0] = '\0';
		client = NULL;
		switch (*mode_ptr) {
		/* --- Channel modes --- */
		case 'i': /* Invite only */
		case 'm': /* Moderated */
		case 'n': /* Only members can write */
		case 's': /* Secret channel */
		case 't': /* Topic locked */
		case 'z': /* Secure connections only */
			if (modeok)
				x[0] = *mode_ptr;
			else
				connected = IRC_WriteStrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin), Channel_Name(Channel));
			break;
		case 'k': /* Channel key */
			if (!set) {
				if (modeok)
					x[0] = *mode_ptr;
				else
					connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				break;
			}
			if (arg_arg > mode_arg) {
				if (modeok) {
					Channel_ModeDel(Channel, 'k');
					Channel_SetKey(Channel,
						       Req->argv[arg_arg]);
					strlcpy(argadd, Channel_Key(Channel),
						sizeof(argadd));
					x[0] = *mode_ptr;
				} else {
					connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				}
				Req->argv[arg_arg][0] = '\0';
				arg_arg++;
			} else {
				connected = IRC_WriteStrClient(Origin,
					ERR_NEEDMOREPARAMS_MSG,
					Client_ID(Origin), Req->command);
				goto chan_exit;
			}
			break;
		case 'l': /* Member limit */
			if (!set) {
				if (modeok)
					x[0] = *mode_ptr;
				else
					connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				break;
			}
			if (arg_arg > mode_arg) {
				if (modeok) {
					l = atol(Req->argv[arg_arg]);
					if (l > 0 && l < 0xFFFF) {
						Channel_ModeDel(Channel, 'l');
						Channel_SetMaxUsers(Channel, l);
						snprintf(argadd, sizeof(argadd),
							 "%ld", l);
						x[0] = *mode_ptr;
					}
				} else {
					connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				}
				Req->argv[arg_arg][0] = '\0';
				arg_arg++;
			} else {
				connected = IRC_WriteStrClient(Origin,
					ERR_NEEDMOREPARAMS_MSG,
					Client_ID(Origin), Req->command);
				goto chan_exit;
			}
			break;
		case 'P': /* Persistent channel */
			if (modeok) {
				/* Only IRC operators are allowed to
				 * set the 'P' channel mode! */
				if (set && !(Client_OperByMe(Client)
				    || Client_Type(Client) == CLIENT_SERVER))
					connected = IRC_WriteStrClient(Origin,
						ERR_NOPRIVILEGES_MSG,
						Client_ID(Origin));
				else
					x[0] = 'P';
			} else
				connected = IRC_WriteStrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin),
					Channel_Name(Channel));
			break;
		/* --- Channel user modes --- */
		case 'o': /* Channel operator */
		case 'v': /* Voice */
			if (arg_arg > mode_arg) {
				if (modeok) {
					client = Client_Search(Req->argv[arg_arg]);
					if (client)
						x[0] = *mode_ptr;
					else
						connected = IRC_WriteStrClient(Client,
							ERR_NOSUCHNICK_MSG,
							Client_ID(Client),
							Req->argv[arg_arg]);
				} else {
					connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				}
				Req->argv[arg_arg][0] = '\0';
				arg_arg++;
			} else {
				connected = IRC_WriteStrClient(Origin,
					ERR_NEEDMOREPARAMS_MSG,
					Client_ID(Origin), Req->command);
				goto chan_exit;
			}
			break;
		/* --- Channel lists --- */
		case 'I': /* Invite lists */
		case 'b': /* Ban lists */
			if (arg_arg > mode_arg) {
				/* modify list */
				if (modeok) {
					connected = set
					   ? Add_Ban_Invite(*mode_ptr, Origin,
						Client, Channel,
						Req->argv[arg_arg])
					   : Del_Ban_Invite(*mode_ptr, Origin,
						Client, Channel,
						Req->argv[arg_arg]);
				} else {
					connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				}
				Req->argv[arg_arg][0] = '\0';
				arg_arg++;
			} else {
				if (*mode_ptr == 'I')
					Channel_ShowInvites(Origin, Channel);
				else
					Channel_ShowBans(Origin, Channel);
			}
			break;
		default:
			Log(LOG_DEBUG,
			    "Unknown mode \"%c%c\" from \"%s\" on %s!?",
			    set ? '+' : '-', *mode_ptr, Client_ID(Origin),
			    Channel_Name(Channel));
			if (Client_Type(Client) != CLIENT_SERVER)
				connected = IRC_WriteStrClient(Origin,
					ERR_UMODEUNKNOWNFLAG2_MSG,
					Client_ID(Origin),
					set ? '+' : '-', *mode_ptr);
			x[0] = '\0';
			goto chan_exit;
		}	/* switch() */

		if (!connected)
			break;

		/* Is there a valid mode change? */
		if (!x[0])
			continue;

		/* Validate target client */
		if (client && (!Channel_IsMemberOf(Channel, client))) {
			if (!IRC_WriteStrClient
			    (Origin, ERR_USERNOTINCHANNEL_MSG,
			     Client_ID(Origin), Client_ID(client),
			     Channel_Name(Channel)))
				break;

			continue;
		}

		if (client) {
			/* Channel-User-Mode */
			retval = set
			       ? Channel_UserModeAdd(Channel, client, x[0])
			       : Channel_UserModeDel(Channel, client, x[0]);
			if (retval) {
				strlcat(the_args, " ", sizeof(the_args));
				strlcat(the_args, Client_ID(client),
					sizeof(the_args));
				strlcat(the_modes, x, sizeof(the_modes));
				LogDebug
				    ("User \"%s\": Mode change on %s, now \"%s\"",
				     Client_Mask(client), Channel_Name(Channel),
				     Channel_UserModes(Channel, client));
			}
		} else {
			/* Channel-Mode */
			retval = set
			       ? Channel_ModeAdd(Channel, x[0])
			       : Channel_ModeDel(Channel, x[0]);
			if (retval) {
				strlcat(the_modes, x, sizeof(the_modes));
				LogDebug("Channel %s: Mode change, now \"%s\".",
					 Channel_Name(Channel),
					 Channel_Modes(Channel));
			}
		}

		/* Are there additional arguments to add? */
		if (argadd[0]) {
			strlcat(the_args, " ", sizeof(the_args));
			strlcat(the_args, argadd, sizeof(the_args));
		}
	}

      chan_exit:
	/* Are there changed modes? */
	if (the_modes[1]) {
		/* Clean up mode string */
		len = strlen(the_modes) - 1;
		if ((the_modes[len] == '+') || (the_modes[len] == '-'))
			the_modes[len] = '\0';

		if (Client_Type(Client) == CLIENT_SERVER) {
			/* MODE requests for local channels from other servers
			 * are definitely invalid! */
			if (Channel_IsLocal(Channel)) {
				Log(LOG_ALERT, "Got remote MODE command for local channel!? Ignored.");
				return CONNECTED;
			}

			/* Forward mode changes to channel users and all the
			 * other remote servers: */
			IRC_WriteStrServersPrefix(Client, Origin,
				"MODE %s %s%s", Channel_Name(Channel),
				the_modes, the_args);
			IRC_WriteStrChannelPrefix(Client, Channel, Origin,
				false, "MODE %s %s%s", Channel_Name(Channel),
				the_modes, the_args);
		} else {
			if (use_servermode)
				Origin = Client_ThisServer();
			/* Send reply to client and inform other servers and channel users */
			connected = IRC_WriteStrClientPrefix(Client, Origin,
					"MODE %s %s%s", Channel_Name(Channel),
					the_modes, the_args);
			/* Only forward requests for non-local channels */
			if (!Channel_IsLocal(Channel))
				IRC_WriteStrServersPrefix(Client, Origin,
					"MODE %s %s%s", Channel_Name(Channel),
					the_modes, the_args);
			IRC_WriteStrChannelPrefix(Client, Channel, Origin,
				false, "MODE %s %s%s", Channel_Name(Channel),
				the_modes, the_args);
		}
	}

	IRC_SetPenalty(Client, 1);
	return connected;
} /* Channel_Mode */


GLOBAL bool
IRC_AWAY( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if(( Req->argc == 1 ) && (Req->argv[0][0] ))
	{
		Client_SetAway( Client, Req->argv[0] );
		Client_ModeAdd( Client, 'a' );
		IRC_WriteStrServersPrefix( Client, Client, "MODE %s :+a", Client_ID( Client ));
		return IRC_WriteStrClient( Client, RPL_NOWAWAY_MSG, Client_ID( Client ));
	}
	else
	{
		Client_ModeDel( Client, 'a' );
		IRC_WriteStrServersPrefix( Client, Client, "MODE %s :-a", Client_ID( Client ));
		return IRC_WriteStrClient( Client, RPL_UNAWAY_MSG, Client_ID( Client ));
	}
} /* IRC_AWAY */


static bool
Add_Ban_Invite(int what, CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, const char *Pattern)
{
	const char *mask;
	bool already;
	bool ret;

	assert( Client != NULL );
	assert( Channel != NULL );
	assert( Pattern != NULL );
	assert(what == 'I' || what == 'b');

	mask = Lists_MakeMask(Pattern);

	already = Lists_CheckDupeMask(Channel_GetListInvites(Channel), mask);
	if (!already) {
		if (what == 'I')
			ret = Channel_AddInvite(Channel, mask, false);
		else
			ret = Channel_AddBan(Channel, mask);
		if (!ret)
			return CONNECTED;
	}
	if (already && (Client_Type(Prefix) == CLIENT_SERVER))
		return CONNECTED;

	if (what == 'I')
		return Send_ListChange("+I", Prefix, Client, Channel, mask);
	return Send_ListChange("+b", Prefix, Client, Channel, mask);
}


static bool
Del_Ban_Invite(int what, CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, const char *Pattern)
{
	const char *mask;
	struct list_head *list;

	assert( Client != NULL );
	assert( Channel != NULL );
	assert( Pattern != NULL );
	assert(what == 'I' || what == 'b');

	mask = Lists_MakeMask( Pattern );

	if (what == 'I')
		list = Channel_GetListInvites(Channel);
	else
		list = Channel_GetListBans(Channel);

	Lists_Del(list, mask);
	if (what == 'I')
		return Send_ListChange( "-I", Prefix, Client, Channel, mask );
	return Send_ListChange( "-b", Prefix, Client, Channel, mask );
}


static bool
Send_ListChange( char *Mode, CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, const char *Mask )
{
	bool ok;

	if( Client_Type( Client ) == CLIENT_USER )
	{
		/* send confirmation to client */
		ok = IRC_WriteStrClientPrefix( Client, Prefix, "MODE %s %s %s", Channel_Name( Channel ), Mode, Mask );
	}
	else ok = true;

	/* to other servers */
	IRC_WriteStrServersPrefix( Client, Prefix, "MODE %s %s %s", Channel_Name( Channel ), Mode, Mask );

	/* and local users in channel */
	IRC_WriteStrChannelPrefix( Client, Channel, Prefix, false, "MODE %s %s %s", Channel_Name( Channel ), Mode, Mask );
	
	return ok;
} /* Send_ListChange */


/* -eof- */
