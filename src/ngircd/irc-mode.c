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
 * IRC commands for mode changes (MODE, AWAY, ...)
 */


#include "portab.h"

static char UNUSED id[] = "$Id: irc-mode.c,v 1.24.2.1 2003/01/02 18:03:05 alex Exp $";

#include "imp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conn.h"
#include "client.h"
#include "channel.h"
#include "defines.h"
#include "irc-write.h"
#include "lists.h"
#include "log.h"
#include "parse.h"
#include "messages.h"
#include "resolve.h"
#include "conf.h"

#include "exp.h"
#include "irc-mode.h"


LOCAL BOOLEAN Client_Mode PARAMS(( CLIENT *Client, REQUEST *Req, CLIENT *Origin, CLIENT *Target ));
LOCAL BOOLEAN Channel_Mode PARAMS(( CLIENT *Client, REQUEST *Req, CLIENT *Origin, CHANNEL *Channel ));

LOCAL BOOLEAN Add_Invite PARAMS(( CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, CHAR *Pattern ));
LOCAL BOOLEAN Add_Ban PARAMS(( CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, CHAR *Pattern ));

LOCAL BOOLEAN Del_Invite PARAMS(( CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, CHAR *Pattern ));
LOCAL BOOLEAN Del_Ban PARAMS(( CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, CHAR *Pattern ));

LOCAL BOOLEAN Send_ListChange PARAMS(( CHAR *Mode, CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, CHAR *Mask ));


GLOBAL BOOLEAN
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
	cl = chan = NULL;
	if( Client_IsValidNick( Req->argv[0] )) cl = Client_Search( Req->argv[0] );
	if( Channel_IsValidName( Req->argv[0] )) chan = Channel_Search( Req->argv[0] );

	if( cl ) return Client_Mode( Client, Req, origin, cl );
	if( chan ) return Channel_Mode( Client, Req, origin, chan );

	/* No target found! */
	return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->argv[0] );
} /* IRC_MODE */


LOCAL BOOLEAN
Client_Mode( CLIENT *Client, REQUEST *Req, CLIENT *Origin, CLIENT *Target )
{
	/* Handle client mode requests */

	CHAR the_modes[COMMAND_LEN], x[2], *mode_ptr;
	BOOLEAN ok, set;
	INT mode_arg;

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
	if( *mode_ptr == '+' ) set = TRUE;
	else if( *mode_ptr == '-' ) set = FALSE;
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
					if(( the_modes[strlen( the_modes ) - 1] == '+' ) || ( the_modes[strlen( the_modes ) - 1] == '-' ))
					{
						/* Adjust last action modifier in result */
						the_modes[strlen( the_modes ) - 1] = *mode_ptr;
					}
					else
					{
						/* Append modifier character to result string */
						x[0] = *mode_ptr; strcat( the_modes, x );
					}
					if( *mode_ptr == '+' ) set = TRUE;
					else set = FALSE;
				}
				continue;
		}
		
		/* Validate modes */
		x[0] = '\0';
		switch( *mode_ptr )
		{
			case 'a':
				/* Away */
				if( Client_Type( Client ) == CLIENT_SERVER )
				{
					x[0] = 'a';
					Client_SetAway( Client, DEFAULT_AWAY_MSG );
				}
				else ok = IRC_WriteStrClient( Origin, ERR_NOPRIVILEGES_MSG, Client_ID( Origin ));
				break;
			case 'i':
				/* Invisible */
				x[0] = 'i';
				break;
			case 'o':
				/* IRC operator (only unsetable!) */
				if(( ! set ) || ( Client_Type( Client ) == CLIENT_SERVER ))
				{
					Client_SetOperByMe( Target, FALSE );
					x[0] = 'o';
				}
				else ok = IRC_WriteStrClient( Origin, ERR_NOPRIVILEGES_MSG, Client_ID( Origin ));
				break;
			case 'r':
				/* Restricted (only setable) */
				if(( set ) || ( Client_Type( Client ) == CLIENT_SERVER )) x[0] = 'r';
				else ok = IRC_WriteStrClient( Origin, ERR_RESTRICTED_MSG, Client_ID( Origin ));
				break;
			case 's':
				/* Server messages */
				x[0] = 's';
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
			if( Client_ModeAdd( Target, x[0] )) strcat( the_modes, x );

		}
		else
		{
			/* Unset mode */
			if( Client_ModeDel( Target, x[0] )) strcat( the_modes, x );
		}		
	}
client_exit:
	
	/* Are there changed modes? */
	if( the_modes[1] )
	{
		/* Remoce needless action modifier characters */
		if(( the_modes[strlen( the_modes ) - 1] == '+' ) || ( the_modes[strlen( the_modes ) - 1] == '-' )) the_modes[strlen( the_modes ) - 1] = '\0';

		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			/* Forward modes to other servers */
			IRC_WriteStrServersPrefix( Client, Origin, "MODE %s :%s", Client_ID( Target ), the_modes );
		}
		else
		{
			/* Send reply to client and inform other servers */
			ok = IRC_WriteStrClientPrefix( Client, Origin, "MODE %s %s", Client_ID( Target ), the_modes );
			IRC_WriteStrServersPrefix( Client, Origin, "MODE %s :%s", Client_ID( Target ), the_modes );
		}
		Log( LOG_DEBUG, "User \"%s\": Mode change, now \"%s\".", Client_Mask( Target ), Client_Modes( Target ));
	}
		
	return ok;
} /* Client_Mode */


LOCAL BOOLEAN
Channel_Mode( CLIENT *Client, REQUEST *Req, CLIENT *Origin, CHANNEL *Channel )
{
	/* Handle channel and channel-user modes */

	CHAR the_modes[COMMAND_LEN], the_args[COMMAND_LEN], x[2], argadd[CLIENT_PASS_LEN], *mode_ptr;
	BOOLEAN ok, set, modeok, skiponce;
	INT mode_arg, arg_arg;
	CLIENT *client;
	LONG l;

	/* Mode request: let's answer it :-) */
	if( Req->argc == 1 ) return IRC_WriteStrClient( Origin, RPL_CHANNELMODEIS_MSG, Client_ID( Origin ), Channel_Name( Channel ), Channel_Modes( Channel ));

	/* Is the user allowed to change modes? */
	if( Client_Type( Client ) == CLIENT_USER )
	{
		if( strchr( Channel_UserModes( Channel, Origin ), 'o' )) modeok = TRUE;
		else modeok = FALSE;
		if( Conf_OperCanMode )
		{
			/* auch IRC-Operatoren duerfen MODE verwenden */
			if( Client_OperByMe( Origin )) modeok = TRUE;
		}
	}
	else modeok = TRUE;

	mode_arg = 1;
	mode_ptr = Req->argv[mode_arg];
	if( Req->argc > mode_arg + 1 ) arg_arg = mode_arg + 1;
	else arg_arg = -1;

	/* Initial state: set or unset modes? */
	skiponce = FALSE;
	if( *mode_ptr == '-' ) set = FALSE;
	else if( *mode_ptr == '+' ) set = TRUE;
	else set = skiponce = TRUE;

	/* Prepare reply string */
	if( set ) strcpy( the_modes, "+" );
	else strcpy( the_modes, "-" );
	strcpy( the_args, " " );

	x[1] = '\0';
	ok = CONNECTED;
	while( mode_ptr )
	{
		if( ! skiponce ) mode_ptr++;
		if( ! *mode_ptr )
		{
			/* Try next argument if there's any */
			if( arg_arg > mode_arg ) mode_arg = arg_arg;
			else mode_arg++;
			if( mode_arg < Req->argc ) mode_ptr = Req->argv[mode_arg];
			else break;
			if( Req->argc > mode_arg + 1 ) arg_arg = mode_arg + 1;
			else arg_arg = -1;
		}
		skiponce = FALSE;

		switch( *mode_ptr )
		{
			case '+':
			case '-':
				if((( *mode_ptr == '+' ) && ( ! set )) || (( *mode_ptr == '-' ) && ( set )))
				{
					/* Action modifier ("+"/"-") must be changed ... */
					if(( the_modes[strlen( the_modes ) - 1] == '+' ) || ( the_modes[strlen( the_modes ) - 1] == '-' ))
					{
						/* Adjust last action modifier in result */
						the_modes[strlen( the_modes ) - 1] = *mode_ptr;
					}
					else
					{
						/* Append modifier character to result string */
						x[0] = *mode_ptr; strcat( the_modes, x );
					}
					if( *mode_ptr == '+' ) set = TRUE;
					else set = FALSE;
				}
				continue;
		}

		/* Are there arguments left? */
		if( arg_arg >= Req->argc ) arg_arg = -1;

		/* Validate modes */
		x[0] = '\0';
		argadd[0] = '\0';
		client = NULL;
		switch( *mode_ptr )
		{
			/* Channel modes */
			case 'i':
				/* Invite-Only */
				if( modeok ) x[0] = 'i';
				else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
				break;
			case 'm':
				/* Moderated */
				if( modeok ) x[0] = 'm';
				else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
				break;
			case 'n':
				/* kein Schreiben in den Channel von aussen */
				if( modeok ) x[0] = 'n';
				else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
				break;
			case 't':
				/* Topic Lock */
				if( modeok ) x[0] = 't';
				else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
				break;
			case 'P':
				/* Persistent channel */
				if( modeok )
				{
					if( set && ( ! Client_OperByMe( Client )))
					{
						/* Only IRC operators are allowed to set P mode */
						ok = IRC_WriteStrClient( Origin, ERR_NOPRIVILEGES_MSG, Client_ID( Origin ));
					}
					else x[0] = 'P';
				}
				else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
				break;

			/* Channel user modes */
			case 'o':
				/* Channel operator */
			case 'v':
				/* Voice */
				if( arg_arg > mode_arg )
				{
					if( modeok )
					{
						client = Client_Search( Req->argv[arg_arg] );
						if( client ) x[0] = *mode_ptr;
						else ok = IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->argv[arg_arg] );
					}
					else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
					Req->argv[arg_arg][0] = '\0';
					arg_arg++;
				}
				else ok = IRC_WriteStrClient( Origin, ERR_NEEDMOREPARAMS_MSG, Client_ID( Origin ), Req->command );
				break;
			case 'k':
				/* Channel key */
				if( ! set )
				{
					if( modeok ) x[0] = *mode_ptr;
					else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
					break;
				}
				if( arg_arg > mode_arg )
				{
					if( modeok )
					{
						Channel_ModeDel( Channel, 'k' );
						Channel_SetKey( Channel, Req->argv[arg_arg] );
						strcpy( argadd, Channel_Key( Channel ));
						x[0] = *mode_ptr;
					}
					else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
					Req->argv[arg_arg][0] = '\0';
					arg_arg++;
				}
				else ok = IRC_WriteStrClient( Origin, ERR_NEEDMOREPARAMS_MSG, Client_ID( Origin ), Req->command );
				break;
			case 'l':
				/* Member limit */
				if( ! set )
				{
					if( modeok ) x[0] = *mode_ptr;
					else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
					break;
				}
				if( arg_arg > mode_arg )
				{
					if( modeok )
					{
						l = atol( Req->argv[arg_arg] );
						if( l > 0 && l < 0xFFFF )
						{
							Channel_ModeDel( Channel, 'l' );
							Channel_SetMaxUsers( Channel, l );
							sprintf( argadd, "%ld", l );
							x[0] = *mode_ptr;
						}
					}
					else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
					Req->argv[arg_arg][0] = '\0';
					arg_arg++;
				}
				else ok = IRC_WriteStrClient( Origin, ERR_NEEDMOREPARAMS_MSG, Client_ID( Origin ), Req->command );
				break;

			/* Channel lists */
			case 'I':
				/* Invite lists */
				if( arg_arg > mode_arg )
				{
					/* modify list */
					if( modeok )
					{
						if( set ) Add_Invite( Origin, Client, Channel, Req->argv[arg_arg] );
						else Del_Invite( Origin, Client, Channel, Req->argv[arg_arg] );
					}
					else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
					Req->argv[arg_arg][0] = '\0';
					arg_arg++;
				}
				else Lists_ShowInvites( Origin, Channel );
				break;
			case 'b':
				/* Ban lists */
				if( arg_arg > mode_arg )
				{
					/* modify list */
					if( modeok )
					{
						if( set ) Add_Ban( Origin, Client, Channel, Req->argv[arg_arg] );
						else Del_Ban( Origin, Client, Channel, Req->argv[arg_arg] );
					}
					else ok = IRC_WriteStrClient( Origin, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Origin ), Channel_Name( Channel ));
					Req->argv[arg_arg][0] = '\0';
					arg_arg++;
				}
				else Lists_ShowBans( Origin, Channel );
				break;

			default:
				Log( LOG_DEBUG, "Unknown mode \"%c%c\" from \"%s\" on %s!?", set ? '+' : '-', *mode_ptr, Client_ID( Origin ), Channel_Name( Channel ));
				if( Client_Type( Client ) != CLIENT_SERVER ) ok = IRC_WriteStrClient( Origin, ERR_UMODEUNKNOWNFLAG2_MSG, Client_ID( Origin ), set ? '+' : '-', *mode_ptr );
				x[0] = '\0';
				goto chan_exit;
		}
		if( ! ok ) break;

		/* Is there a valid mode change? */
		if( ! x[0] ) continue;

		if( set )
		{
			/* Set mode */
			if( client )
			{
				/* Channel-User-Mode */
				if( Channel_UserModeAdd( Channel, client, x[0] ))
				{
					strcat( the_args, Client_ID( client ));
					strcat( the_args, " " ); strcat( the_modes, x );
					Log( LOG_DEBUG, "User \"%s\": Mode change on %s, now \"%s\"", Client_Mask( client ), Channel_Name( Channel ), Channel_UserModes( Channel, client ));
				}
			}
			else
			{
				/* Channel-Mode */
				if( Channel_ModeAdd( Channel, x[0] ))
				{
					strcat( the_modes, x );
					Log( LOG_DEBUG, "Channel %s: Mode change, now \"%s\".", Channel_Name( Channel ), Channel_Modes( Channel ));
				}
			}
		}
		else
		{
			/* Unset mode */
			if( client )
			{
				/* Channel-User-Mode */
				if( Channel_UserModeDel( Channel, client, x[0] ))
				{
					strcat( the_args, Client_ID( client ));
					strcat( the_args, " " ); strcat( the_modes, x );
					Log( LOG_DEBUG, "User \"%s\": Mode change on %s, now \"%s\"", Client_Mask( client ), Channel_Name( Channel ), Channel_UserModes( Channel, client ));
				}
			}
			else
			{
				/* Channel-Mode */
				if( Channel_ModeDel( Channel, x[0] ))
				{
					strcat( the_modes, x );
					Log( LOG_DEBUG, "Channel %s: Mode change, now \"%s\".", Channel_Name( Channel ), Channel_Modes( Channel ));
				}
			}
		}

		/* Are there additional arguments to add? */
		if( argadd[0] )
		{
			if( the_args[strlen( the_args ) - 1] != ' ' ) strcat( the_args, " " );
			strcat( the_args, argadd );
		}
	}
chan_exit:

	/* Are there changed modes? */
	if( the_modes[1] )
	{
		/* Clean up mode string */
		if(( the_modes[strlen( the_modes ) - 1] == '+' ) || ( the_modes[strlen( the_modes ) - 1] == '-' )) the_modes[strlen( the_modes ) - 1] = '\0';

		/* Clean up argument string if there are none */
		if( ! the_args[1] ) the_args[0] = '\0';

		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			/* Forward mode changes to channel users and other servers */
			IRC_WriteStrServersPrefix( Client, Origin, "MODE %s %s%s", Channel_Name( Channel ), the_modes, the_args );
			IRC_WriteStrChannelPrefix( Client, Channel, Origin, FALSE, "MODE %s %s%s", Channel_Name( Channel ), the_modes, the_args );
		}
		else
		{
			/* Send reply to client and inform other servers and channel users */
			ok = IRC_WriteStrClientPrefix( Client, Origin, "MODE %s %s%s", Channel_Name( Channel ), the_modes, the_args );
			IRC_WriteStrServersPrefix( Client, Origin, "MODE %s %s%s", Channel_Name( Channel ), the_modes, the_args );
			IRC_WriteStrChannelPrefix( Client, Channel, Origin, FALSE, "MODE %s %s%s", Channel_Name( Channel ), the_modes, the_args );
		}
	}

	return CONNECTED;
} /* Channel_Mode */


GLOBAL BOOLEAN
IRC_AWAY( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if(( Req->argc == 1 ) && (Req->argv[0][0] ))
	{
		/* AWAY setzen */
		Client_SetAway( Client, Req->argv[0] );
		Client_ModeAdd( Client, 'a' );
		IRC_WriteStrServersPrefix( Client, Client, "MODE %s :+a", Client_ID( Client ));
		return IRC_WriteStrClient( Client, RPL_NOWAWAY_MSG, Client_ID( Client ));
	}
	else
	{
		/* AWAY loeschen */
		Client_ModeDel( Client, 'a' );
		IRC_WriteStrServersPrefix( Client, Client, "MODE %s :-a", Client_ID( Client ));
		return IRC_WriteStrClient( Client, RPL_UNAWAY_MSG, Client_ID( Client ));
	}
} /* IRC_AWAY */


LOCAL BOOLEAN
Add_Invite( CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, CHAR *Pattern )
{
	CHAR *mask;

	assert( Client != NULL );
	assert( Channel != NULL );
	assert( Pattern != NULL );

	mask = Lists_MakeMask( Pattern );

	if( ! Lists_AddInvited( Prefix, mask, Channel, FALSE )) return CONNECTED;
	return Send_ListChange( "+I", Prefix, Client, Channel, mask );
} /* Add_Invite */


LOCAL BOOLEAN
Add_Ban( CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, CHAR *Pattern )
{
	CHAR *mask;

	assert( Client != NULL );
	assert( Channel != NULL );
	assert( Pattern != NULL );

	mask = Lists_MakeMask( Pattern );

	if( ! Lists_AddBanned( Prefix, mask, Channel )) return CONNECTED;
	return Send_ListChange( "+b", Prefix, Client, Channel, mask );
} /* Add_Ban */


LOCAL BOOLEAN
Del_Invite( CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, CHAR *Pattern )
{
	CHAR *mask;

	assert( Client != NULL );
	assert( Channel != NULL );
	assert( Pattern != NULL );

	mask = Lists_MakeMask( Pattern );
	Lists_DelInvited( mask, Channel );
	return Send_ListChange( "-I", Prefix, Client, Channel, mask );
} /* Del_Invite */


LOCAL BOOLEAN
Del_Ban( CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, CHAR *Pattern )
{
	CHAR *mask;

	assert( Client != NULL );
	assert( Channel != NULL );
	assert( Pattern != NULL );

	mask = Lists_MakeMask( Pattern );
	Lists_DelBanned( mask, Channel );
	return Send_ListChange( "-b", Prefix, Client, Channel, mask );
} /* Del_Ban */


LOCAL BOOLEAN
Send_ListChange( CHAR *Mode, CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel, CHAR *Mask )
{
	/* Bestaetigung an Client schicken & andere Server sowie Channel-User informieren */

	BOOLEAN ok;

	if( Client_Type( Client ) == CLIENT_USER )
	{
		/* Bestaetigung an Client */
		ok = IRC_WriteStrClientPrefix( Client, Prefix, "MODE %s %s %s", Channel_Name( Channel ), Mode, Mask );
	}
	else ok = TRUE;

	/* an andere Server */
	IRC_WriteStrServersPrefix( Client, Prefix, "MODE %s %s %s", Channel_Name( Channel ), Mode, Mask );

	/* und lokale User im Channel */
	IRC_WriteStrChannelPrefix( Client, Channel, Prefix, FALSE, "MODE %s %s %s", Channel_Name( Channel ), Mode, Mask );
	
	return ok;
} /* Send_ListChange */


/* -eof- */
