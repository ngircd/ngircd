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
 * IRC commands
 */


#include "portab.h"

static char UNUSED id[] = "$Id: irc.c,v 1.112 2002/12/27 13:35:19 alex Exp $";

#include "imp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "conn.h"
#include "client.h"
#include "channel.h"
#include "defines.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"
#include "parse.h"

#include "exp.h"
#include "irc.h"


GLOBAL BOOLEAN
IRC_ERROR( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Req->argc < 1 ) Log( LOG_NOTICE, "Got ERROR from \"%s\"!", Client_Mask( Client ));
	else Log( LOG_NOTICE, "Got ERROR from \"%s\": %s!", Client_Mask( Client ), Req->argv[0] );

	return CONNECTED;
} /* IRC_ERROR */


GLOBAL BOOLEAN
IRC_KILL( CLIENT *Client, REQUEST *Req )
{
	CLIENT *prefix, *c;
	CHAR reason[COMMAND_LEN];
	CONN_ID conn;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Is the user an IRC operator? */
	if(( Client_Type( Client ) != CLIENT_SERVER ) && ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));

	/* Bad number of parameters? */
	if(( Req->argc != 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Req->prefix ) prefix = Client_Search( Req->prefix );
	else prefix = Client;
	if( ! prefix )
	{
		Log( LOG_WARNING, "Got KILL with invalid prefix: \"%s\"!", Req->prefix );
		prefix = Client_ThisServer( );
	}

	Log( LOG_NOTICE|LOG_snotice, "Got KILL command from \"%s\" for \"%s\": %s", Client_Mask( prefix ), Req->argv[0], Req->argv[1] );

	/* Build reason string */
	if( Client_Type( Client ) == CLIENT_USER ) snprintf( reason, sizeof( reason ), "KILLed by %s: %s", Client_ID( Client ), Req->argv[1] );
	else strlcpy( reason, Req->argv[1], sizeof( reason ));

	/* Inform other servers */
	IRC_WriteStrServersPrefix( Client, prefix, "KILL %s :%s", Req->argv[0], reason );

	/* Do we host such a client? */
	c = Client_Search( Req->argv[0] );
	if( c )
	{
		/* Yes, there is such a client -- but is it a valid user? */
		if( Client_Type( c ) == CLIENT_SERVER ) IRC_WriteStrClient( Client, ERR_CANTKILLSERVER_MSG, Client_ID( Client ));
		else if( Client_Type( c ) != CLIENT_USER  )IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));
		else
		{
			/* Kill user NOW! */
			conn = Client_Conn( c );
			Client_Destroy( c, NULL, reason, FALSE );
			if( conn != NONE ) Conn_Close( Client_Conn( c ), NULL, reason, TRUE );
		}
	}
	else Log( LOG_NOTICE, "Client with nick \"%s\" is unknown here.", Req->argv[0] );

	/* Are we still connected or were we killed, too? */
	if( Client_Search( Req->argv[0] )) return CONNECTED;
	else return DISCONNECTED;
} /* IRC_KILL */


GLOBAL BOOLEAN
IRC_NOTICE( CLIENT *Client, REQUEST *Req )
{
	CLIENT *to, *from;

	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return CONNECTED;

	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	to = Client_Search( Req->argv[0] );
	if(( to ) && ( Client_Type( to ) == CLIENT_USER ))
	{
		/* Okay, Ziel ist ein User */
		return IRC_WriteStrClientPrefix( to, from, "NOTICE %s :%s", Client_ID( to ), Req->argv[1] );
	}
	else return CONNECTED;
} /* IRC_NOTICE */


GLOBAL BOOLEAN
IRC_PRIVMSG( CLIENT *Client, REQUEST *Req )
{
	CLIENT *cl, *from;
	CHANNEL *chan;
	
	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc == 0 ) return IRC_WriteStrClient( Client, ERR_NORECIPIENT_MSG, Client_ID( Client ), Req->command );
	if( Req->argc == 1 ) return IRC_WriteStrClient( Client, ERR_NOTEXTTOSEND_MSG, Client_ID( Client ));
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	cl = Client_Search( Req->argv[0] );
	if( cl )
	{
		/* Okay, Ziel ist ein Client. Aber ist es auch ein User? */
		if( Client_Type( cl ) != CLIENT_USER ) return IRC_WriteStrClient( from, ERR_NOSUCHNICK_MSG, Client_ID( from ), Req->argv[0] );

		/* Okay, Ziel ist ein User */
		if(( Client_Type( Client ) != CLIENT_SERVER ) && ( strchr( Client_Modes( cl ), 'a' )))
		{
			/* Ziel-User ist AWAY: Meldung verschicken */
			if( ! IRC_WriteStrClient( from, RPL_AWAY_MSG, Client_ID( from ), Client_ID( cl ), Client_Away( cl ))) return DISCONNECTED;
		}

		/* Text senden */
		if( Client_Conn( from ) > NONE ) Conn_UpdateIdle( Client_Conn( from ));
		return IRC_WriteStrClientPrefix( cl, from, "PRIVMSG %s :%s", Client_ID( cl ), Req->argv[1] );
	}

	chan = Channel_Search( Req->argv[0] );
	if( chan ) return Channel_Write( chan, from, Client, Req->argv[1] );

	return IRC_WriteStrClient( from, ERR_NOSUCHNICK_MSG, Client_ID( from ), Req->argv[0] );
} /* IRC_PRIVMSG */


/* -eof- */
