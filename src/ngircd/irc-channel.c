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

static char UNUSED id[] = "$Id: irc-channel.c,v 1.23 2003/01/01 13:29:40 alex Exp $";

#include "imp.h"
#include <assert.h>
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


GLOBAL BOOLEAN
IRC_JOIN( CLIENT *Client, REQUEST *Req )
{
	CHAR *channame, *key, *flags, *topic, modes[8];
	BOOLEAN is_new_chan, is_invited, is_banned;
	CLIENT *target;
	CHANNEL *chan;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Bad number of arguments? */
	if(( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Who is the sender? */
	if( Client_Type( Client ) == CLIENT_SERVER ) target = Client_Search( Req->prefix );
	else target = Client;
	if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* Are channel keys given? */
	if( Req->argc > 1 ) key = Req->argv[1];
	else key = NULL;

	/* Channel-Namen durchgehen */
	chan = NULL;
	channame = strtok( Req->argv[0], "," );
	while( channame )
	{
		chan = flags = NULL;

		/* wird der Channel neu angelegt? */
		if( Channel_Search( channame )) is_new_chan = FALSE;
		else is_new_chan = TRUE;

		/* Hat ein Server Channel-User-Modes uebergeben? */
		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			/* Channel-Flags extrahieren */
			flags = strchr( channame, 0x7 );
			if( flags )
			{
				*flags = '\0';
				flags++;
			}
		}

		/* Lokaler Client? */
		if( Client_Type( Client ) == CLIENT_USER )
		{
			/* Test if the user has reached his maximum channel count */
			if( Client_Type( Client ) == CLIENT_USER )
			{
				if(( Conf_MaxJoins > 0 ) && ( Channel_CountForUser( Client ) >= Conf_MaxJoins ))
				{
					IRC_WriteStrClient( Client, ERR_TOOMANYCHANNELS_MSG, Client_ID( Client ), channame );
					return CONNECTED;
				}
			}

			/* Existiert der Channel bereits, oder wird er im Moment neu erzeugt? */
			if( is_new_chan )
			{
				/* Erster User im Channel: Operator-Flag setzen */
				flags = "o";
			}
			else
			{
				/* Existierenden Channel suchen */
				chan = Channel_Search( channame );
				assert( chan != NULL );

				is_banned = Lists_CheckBanned( target, chan );
				is_invited = Lists_CheckInvited( target, chan );

				/* Testen, ob Client gebanned ist */
				if(( is_banned == TRUE ) &&  ( is_invited == FALSE ))
				{
					/* Client ist gebanned (und nicht invited): */
					IRC_WriteStrClient( Client, ERR_BANNEDFROMCHAN_MSG, Client_ID( Client ), channame );

					/* Try next name, if any */
					channame = strtok( NULL, "," );
					continue;
				}

				/* Ist der Channel "invite-only"? */
				if(( strchr( Channel_Modes( chan ), 'i' )) && ( is_invited == FALSE ))
				{
					/* Channel ist "invite-only" und Client wurde nicht invited: */
					IRC_WriteStrClient( Client, ERR_INVITEONLYCHAN_MSG, Client_ID( Client ), channame );

					/* Try next name, if any */
					channame = strtok( NULL, "," );
					continue;
				}

				/* Is the channel protected by a key? */
				if(( strchr( Channel_Modes( chan ), 'k' )) && ( strcmp( Channel_Key( chan ), key ? key : "" ) != 0 ))
				{
					/* Bad channel key! */
					IRC_WriteStrClient( Client, ERR_BADCHANNELKEY_MSG, Client_ID( Client ), channame );

					/* Try next name, if any */
					channame = strtok( NULL, "," );
					continue;
				}

				/* Are there already too many members? */
				if(( strchr( Channel_Modes( chan ), 'l' )) && ( Channel_MaxUsers( chan ) <= Channel_MemberCount( chan )))
				{
					/* Bad channel key! */
					IRC_WriteStrClient( Client, ERR_CHANNELISFULL_MSG, Client_ID( Client ), channame );

					/* Try next name, if any */
					channame = strtok( NULL, "," );
					continue;
				}
			}
		}

		/* Channel joinen (und ggf. anlegen) */
		if( ! Channel_Join( target, channame ))
		{
			/* naechsten Namen ermitteln */
			channame = strtok( NULL, "," );
			continue;
		}
		if( ! chan ) chan = Channel_Search( channame );
		assert( chan != NULL );

		/* Modes setzen (wenn vorhanden) */
		while( flags && *flags )
		{
			Channel_UserModeAdd( chan, target, *flags );
			flags++;
		}

		/* Wenn persistenter Channel und IRC-Operator: zum Channel-OP machen */
		if(( strchr( Channel_Modes( chan ), 'P' )) && ( strchr( Client_Modes( target ), 'o' ))) Channel_UserModeAdd( chan, target, 'o' );

		/* Muessen Modes an andere Server gemeldet werden? */
		strlcpy( &modes[1], Channel_UserModes( chan, target ), sizeof( modes ) - 1 );
		if( modes[1] ) modes[0] = 0x7;
		else modes[0] = '\0';

		/* An andere Server weiterleiten */
		IRC_WriteStrServersPrefix( Client, target, "JOIN :%s%s", channame, modes );

		/* im Channel bekannt machen */
		IRC_WriteStrChannelPrefix( Client, chan, target, FALSE, "JOIN :%s", channame );
		if( modes[1] )
		{
			/* Modes im Channel bekannt machen */
			IRC_WriteStrChannelPrefix( Client, chan, target, FALSE, "MODE %s +%s %s", channame, &modes[1], Client_ID( target ));
		}

		if( Client_Type( Client ) == CLIENT_USER )
		{
			/* an Client bestaetigen */
			IRC_WriteStrClientPrefix( Client, target, "JOIN :%s", channame );

			/* Topic an Client schicken */
			topic = Channel_Topic( chan );
			if( *topic ) IRC_WriteStrClient( Client, RPL_TOPIC_MSG, Client_ID( Client ), channame, topic );

			/* Mitglieder an Client Melden */
			IRC_Send_NAMES( Client, chan );
			IRC_WriteStrClient( Client, RPL_ENDOFNAMES_MSG, Client_ID( Client ), Channel_Name( chan ));
		}

		/* naechsten Namen ermitteln */
		channame = strtok( NULL, "," );
	}
	return CONNECTED;
} /* IRC_JOIN */


GLOBAL BOOLEAN
IRC_PART( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target;
	CHAR *chan;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Wer ist der Absender? */
	if( Client_Type( Client ) == CLIENT_SERVER ) target = Client_Search( Req->prefix );
	else target = Client;
	if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* Channel-Namen durchgehen */
	chan = strtok( Req->argv[0], "," );
	while( chan )
	{
		if( ! Channel_Part( target, Client, chan, Req->argc > 1 ? Req->argv[1] : Client_ID( target )))
		{
			/* naechsten Namen ermitteln */
			chan = strtok( NULL, "," );
			continue;
		}

		/* naechsten Namen ermitteln */
		chan = strtok( NULL, "," );
	}
	return CONNECTED;
} /* IRC_PART */


GLOBAL BOOLEAN
IRC_TOPIC( CLIENT *Client, REQUEST *Req )
{
	CHANNEL *chan;
	CLIENT *from;
	CHAR *topic;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 1 ) || ( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

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
		/* Topic erfragen */
		topic = Channel_Topic( chan );
		if( *topic ) return IRC_WriteStrClient( from, RPL_TOPIC_MSG, Client_ID( from ), Channel_Name( chan ), topic );
		else return IRC_WriteStrClient( from, RPL_NOTOPIC_MSG, Client_ID( from ), Channel_Name( chan ));
	}

	if( strchr( Channel_Modes( chan ), 't' ))
	{
		/* Topic Lock. Ist der User ein Channel Operator? */
		if( ! strchr( Channel_UserModes( chan, from ), 'o' )) return IRC_WriteStrClient( from, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( from ), Channel_Name( chan ));
	}

	/* Topic setzen */
	Channel_SetTopic( chan, Req->argv[1] );
	Log( LOG_DEBUG, "User \"%s\" set topic on \"%s\": %s", Client_Mask( from ), Channel_Name( chan ), Req->argv[1][0] ? Req->argv[1] : "<none>" );

	/* im Channel bekannt machen und an Server weiterleiten */
	IRC_WriteStrServersPrefix( Client, from, "TOPIC %s :%s", Req->argv[0], Req->argv[1] );
	IRC_WriteStrChannelPrefix( Client, chan, from, FALSE, "TOPIC %s :%s", Req->argv[0], Req->argv[1] );

	if( Client_Type( Client ) == CLIENT_USER ) return IRC_WriteStrClientPrefix( Client, Client, "TOPIC %s :%s", Req->argv[0], Req->argv[1] );
	else return CONNECTED;
} /* IRC_TOPIC */


GLOBAL BOOLEAN
IRC_LIST( CLIENT *Client, REQUEST *Req )
{
	CHAR *pattern;
	CHANNEL *chan;
	CLIENT *from, *target;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Req->argc > 0 ) pattern = strtok( Req->argv[0], "," );
	else pattern = "*";

	/* From aus Prefix ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->prefix );

	if( Req->argc == 2 )
	{
		/* an anderen Server forwarden */
		target = Client_Search( Req->argv[1] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[1] );

		if( target != Client_ThisServer( ))
		{
			/* Ok, anderer Server ist das Ziel: forwarden */
			return IRC_WriteStrClientPrefix( target, from, "LIST %s :%s", from, Req->argv[1] );
		}
	}
	
	while( pattern )
	{
		/* alle Channel durchgehen */
		chan = Channel_First( );
		while( chan )
		{
			/* Passt die Suchmaske auf diesen Channel? */
			if( Match( pattern, Channel_Name( chan )))
			{
				/* Treffer! */
				if( ! IRC_WriteStrClient( from, RPL_LIST_MSG, from, Channel_Name( chan ), Channel_MemberCount( chan ), Channel_Topic( chan ))) return DISCONNECTED;
			}
			chan = Channel_Next( chan );
		}
		
		/* naechsten Namen ermitteln */
		if( Req->argc > 0 ) pattern = strtok( NULL, "," );
		else pattern = NULL;
	}
	
	return IRC_WriteStrClient( from, RPL_LISTEND_MSG, from );
} /* IRC_LIST */


GLOBAL BOOLEAN
IRC_CHANINFO( CLIENT *Client, REQUEST *Req )
{
	CLIENT *from;
	CHANNEL *chan;
	CHAR *ptr;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 1 ) || ( Req->argc > 3 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* From-Server suchen */
	from = Client_Search( Req->prefix );
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* Channel suchen bzw. erzeugen */
	chan = Channel_Search( Req->argv[0] );
	if( ! chan ) chan = Channel_Create( Req->argv[0] );
	if( ! chan ) return CONNECTED;

	if( Req->argv[1][0] == '+' )
	{
		ptr = Channel_Modes( chan );
		if( ! *ptr )
		{
			/* OK, es sind noch keine Modes gesetzt */
			Channel_SetModes( chan, &Req->argv[1][1] );
			IRC_WriteStrChannelPrefix( Client, chan, from, FALSE, "MODE %s +%s", Req->argv[0], &Req->argv[1][1] );
		}
	}
	else Log( LOG_WARNING, "CHANNELINFO: invalid MODE format ignored!" );

	if( Req->argc == 3 )
	{
		/* Es wurde auch ein Topic mit uebermittelt */
		ptr = Channel_Topic( chan );
		if( ! *ptr )
		{
			/* OK, es ist bisher kein Topic gesetzt */
			Channel_SetTopic( chan, Req->argv[2] );
			IRC_WriteStrChannelPrefix( Client, chan, from, FALSE, "TOPIC %s :%s", Req->argv[0], Req->argv[2] );
		}
	}

	/* an andere Server forwarden */
	IRC_WriteStrServersPrefixFlag( Client, from, 'C', "CHANINFO %s %s :%s", Req->argv[0], Req->argv[1], Req->argv[2] );
	return CONNECTED;
} /* IRC_CHANINFO */


/* -eof- */
