/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: irc-channel.c,v 1.3 2002/03/25 17:08:54 alex Exp $
 *
 * irc-channel.c: IRC-Channel-Befehle
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <string.h>

#include "defines.h"
#include "irc.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"

#include "exp.h"
#include "irc-channel.h"


GLOBAL BOOLEAN IRC_JOIN( CLIENT *Client, REQUEST *Req )
{
	CHAR *channame, *flags, *topic, modes[8];
	BOOLEAN is_new_chan;
	CLIENT *target;
	CHANNEL *chan;

	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Wer ist der Absender? */
	if( Client_Type( Client ) == CLIENT_SERVER ) target = Client_Search( Req->prefix );
	else target = Client;
	if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* Channel-Namen durchgehen */
	channame = strtok( Req->argv[0], "," );
	while( channame )
	{
		/* wird der Channel neu angelegt? */
		flags = NULL;

		if( Channel_Search( channame )) is_new_chan = FALSE;
		else is_new_chan = TRUE;

		/* Hat ein Server Channel-User-Modes uebergeben? */
		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			/* Channel-Flags extrahieren */
			flags = strchr( channame, 0x7 );
			if( flags ) *flags++ = '\0';
		}

		/* neuer Channel udn lokaler Client? */
		if( is_new_chan && ( Client_Type( Client ) == CLIENT_USER ))
		{
			/* Dann soll der Client Channel-Operator werden! */
			flags = "o";
		}

		/* Channel joinen (und ggf. anlegen) */
		if( ! Channel_Join( target, channame ))
		{
			/* naechsten Namen ermitteln */
			channame = strtok( NULL, "," );
			continue;
		}
		chan = Channel_Search( channame );
		assert( chan != NULL );

		/* Modes setzen (wenn vorhanden) */
		while( flags && *flags )
		{
			Channel_UserModeAdd( chan, target, *flags );
			flags++;
		}

		/* Muessen Modes an andere Server gemeldet werden? */
		strcpy( &modes[1], Channel_UserModes( chan, target ));
		if( modes[1] ) modes[0] = 0x7;
		else modes[0] = '\0';

		/* An andere Server weiterleiten */
		IRC_WriteStrServersPrefix( Client, target, "JOIN :%s%s", channame, modes );

		/* im Channel bekannt machen */
		IRC_WriteStrChannelPrefix( Client, chan, target, FALSE, "JOIN :%s", channame );
		if( modes[1] )
		{
			/* Modes im Channel bekannt machen */
			IRC_WriteStrChannelPrefix( Client, chan, target, FALSE, "MODE %s %s :%s", channame, modes, Client_ID( target ));
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


GLOBAL BOOLEAN IRC_PART( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target;
	CHAR *chan;

	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

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


GLOBAL BOOLEAN IRC_TOPIC( CLIENT *Client, REQUEST *Req )
{
	CHANNEL *chan;
	CLIENT *from;
	CHAR *topic;

	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 1 ) || ( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* Welcher Channel? */
	chan = Channel_Search( Req->argv[0] );
	if( ! chan ) return IRC_WriteStrClient( from, ERR_NOTONCHANNEL_MSG, Client_ID( from ), Req->argv[0] );

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


/* -eof- */
