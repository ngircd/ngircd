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
 * $Id: irc-op.c,v 1.5 2002/06/11 13:59:07 alex Exp $
 *
 * irc-op.c: Befehle zur Channel-Verwaltung
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "conn.h"
#include "client.h"
#include "channel.h"
#include "defines.h"
#include "irc-write.h"
#include "lists.h"
#include "log.h"
#include "messages.h"
#include "parse.h"

#include "exp.h"
#include "irc-op.h"


GLOBAL BOOLEAN
IRC_KICK( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *from;
	
	assert( Client != NULL );
	assert( Req != NULL );

	/* Valider Client? */
	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 2) || ( Req->argc > 3 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );
	
	/* Ziel-User suchen */
	target = Client_Search( Req->argv[1] );
	if( ! target ) return IRC_WriteStrClient( from, ERR_NOSUCHNICK_MSG, Client_ID( from ), Req->argv[1] );

	Channel_Kick( target, from, Req->argv[0], Req->argc == 3 ? Req->argv[2] : Client_ID( from ));
	return CONNECTED;
} /* IRC_KICK */	


GLOBAL BOOLEAN
IRC_BAN( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	/* Valider Client? */
	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Keine Parameter? */
	if( Req->argc < 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	return CONNECTED;
} /* IRC_BAN */	


GLOBAL BOOLEAN
IRC_INVITE( CLIENT *Client, REQUEST *Req )
{
	CHANNEL *chan;
	CLIENT *target, *from;
	BOOLEAN remember = FALSE;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Valider Client? */
	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );
	
	/* User suchen */
	target = Client_Search( Req->argv[0] );
	if(( ! target ) || ( Client_Type( target ) != CLIENT_USER )) return IRC_WriteStrClient( from, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->argv[0] );

	chan = Channel_Search( Req->argv[1] );

	if( chan )
	{
		/* Der Channel existiert bereits; ist der User Mitglied? */
		if( ! Channel_IsMemberOf( chan, from )) return IRC_WriteStrClient( from, ERR_NOTONCHANNEL_MSG, Client_ID( Client ), Req->argv[1] );

		/* Ist der Channel "invite-only"? */
		if( strchr( Channel_Modes( chan ), 'i' ))
		{
			/* Ja. Der User muss Channel-Operator sein! */
			if( ! strchr( Channel_UserModes( chan, from ), 'o' )) return IRC_WriteStrClient( from, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( from ), Channel_Name( chan ));
			remember = TRUE;
		}

		/* Ist der Ziel-User bereits Mitglied? */
		if( Channel_IsMemberOf( chan, target )) return IRC_WriteStrClient( from, ERR_USERONCHANNEL_MSG, Client_ID( from ), Req->argv[0], Req->argv[1] );

	}

	Log( LOG_DEBUG, "User \"%s\" invites \"%s\" to \"%s\" ...", Client_Mask( from ), Req->argv[0], Req->argv[1] );
	IRC_WriteStrClientPrefix( target, from, "INVITE %s %s", Req->argv[0], Req->argv[1] );
	if( remember ) Lists_AddInvited( Client_Mask( target ), chan, TRUE );

	if( Client_Conn( target ) > NONE )
	{
		/* lokaler Zeil-Client, Status-Code melden */
		if( ! IRC_WriteStrClientPrefix( from, from, RPL_INVITING_MSG, Client_ID( from ), Req->argv[0], Req->argv[1] )) return DISCONNECTED;
	}
	
	return CONNECTED;
} /* IRC_INVITE */


/* -eof- */
