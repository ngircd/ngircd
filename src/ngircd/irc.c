/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2004 Alexander Barton <alex@barton.de>
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

static char UNUSED id[] = "$Id: irc.c,v 1.131 2006/07/23 14:55:40 alex Exp $";

#include "imp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ngircd.h"
#include "resolve.h"
#include "conn-func.h"
#include "conf.h"
#include "client.h"
#include "channel.h"
#include "defines.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"
#include "parse.h"

#include "exp.h"
#include "irc.h"


static char *Option_String PARAMS(( CONN_ID Idx ));


GLOBAL bool
IRC_ERROR( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Req->argc < 1 ) Log( LOG_NOTICE, "Got ERROR from \"%s\"!", Client_Mask( Client ));
	else Log( LOG_NOTICE, "Got ERROR from \"%s\": %s!", Client_Mask( Client ), Req->argv[0] );

	return CONNECTED;
} /* IRC_ERROR */


/**
 * Kill client on request.
 * This function implements the IRC command "KILL" wich is used to selectively
 * disconnect clients. It can be used by IRC operators and servers, for example
 * to "solve" nick collisions after netsplits.
 * Please note that this function is also called internally, without a real
 * KILL command beeing received over the network! Client is Client_ThisServer()
 * in this case. */
GLOBAL bool
IRC_KILL( CLIENT *Client, REQUEST *Req )
{
	CLIENT *prefix, *c;
	char reason[COMMAND_LEN], *msg;
	CONN_ID my_conn, conn;

	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_SERVER ) &&
	   ( ! Client_OperByMe( Client )))
	{
		/* The originator of the KILL is neither an IRC operator of
		 * this server nor a server. */
		return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG,
					   Client_ID( Client ));
	}

	if( Req->argc != 2 )
	{
		/* This command requires exactly 2 parameters! */
		return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG,
					   Client_ID( Client ), Req->command );
	}

	if( Req->prefix ) prefix = Client_Search( Req->prefix );
	else prefix = Client;
	if( ! prefix )
	{
		Log( LOG_WARNING, "Got KILL with invalid prefix: \"%s\"!",
		     Req->prefix );
		prefix = Client_ThisServer( );
	}

	if( Client != Client_ThisServer( ))
	{
		/* This is a "real" KILL received from the network. */
		Log( LOG_NOTICE|LOG_snotice, "Got KILL command from \"%s\" for \"%s\": %s",
		     Client_Mask( prefix ), Req->argv[0], Req->argv[1] );
	}

	/* Build reason string */
	if( Client_Type( Client ) == CLIENT_USER )
	{
		/* Prefix the "reason" if the originator is a regular user,
		 * so users can't spoof KILLs of servers. */
		snprintf( reason, sizeof( reason ), "KILLed by %s: %s",
			  Client_ID( Client ), Req->argv[1] );
	}
	else
		strlcpy( reason, Req->argv[1], sizeof( reason ));

	/* Inform other servers */
	IRC_WriteStrServersPrefix( Client, prefix, "KILL %s :%s",
				   Req->argv[0], reason );

	/* Save ID of this connection */
	my_conn = Client_Conn( Client );

	/* Do we host such a client? */
	c = Client_Search( Req->argv[0] );
	if( c )
	{
		if(( Client_Type( c ) != CLIENT_USER ) &&
		   ( Client_Type( c ) != CLIENT_GOTNICK ))
		{
			/* Target of this KILL is not a regular user, this is
			 * invalid! So we ignore this case if we received a
			 * regular KILL from the network and try to kill the
			 * client/connection anyway (but log an error!) if the
			 * origin is the local server. */

			if( Client != Client_ThisServer( ))
			{
				/* Invalid KILL received from remote */
				if( Client_Type( c ) == CLIENT_SERVER )
					msg = ERR_CANTKILLSERVER_MSG;
				else
					msg = ERR_NOPRIVILEGES_MSG;
				return IRC_WriteStrClient( Client, msg,
					Client_ID( Client ));
			}

			Log( LOG_ERR, "Got KILL for invalid client type: %d, \"%s\"!",
			     Client_Type( c ), Req->argv[0] );
		}

		/* Kill client NOW! */
		conn = Client_Conn( c );
		Client_Destroy( c, NULL, reason, false );
		if( conn > NONE )
			Conn_Close( conn, NULL, reason, true );
	}
	else
		Log( LOG_NOTICE, "Client with nick \"%s\" is unknown here.", Req->argv[0] );

	/* Are we still connected or were we killed, too? */
	if(( my_conn > NONE ) && ( Conn_GetClient( my_conn )))
		return CONNECTED;
	else
		return DISCONNECTED;
} /* IRC_KILL */


GLOBAL bool
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


GLOBAL bool
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


GLOBAL bool
IRC_TRACE( CLIENT *Client, REQUEST *Req )
{
	CLIENT *from, *target, *c;
	CONN_ID idx, idx2;
	char user[CLIENT_USER_LEN];

	assert( Client != NULL );
	assert( Req != NULL );

	/* Bad number of arguments? */
	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NORECIPIENT_MSG, Client_ID( Client ), Req->command );

	/* Search sender */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* Search target */
	if( Req->argc == 1 ) target = Client_Search( Req->argv[0] );
	else target = Client_ThisServer( );
	
	/* Forward command to other server? */
	if( target != Client_ThisServer( ))
	{
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[0] );

		/* Send RPL_TRACELINK back to initiator */
		idx = Client_Conn( Client ); assert( idx > NONE );
		idx2 = Client_Conn( Client_NextHop( target )); assert( idx2 > NONE );
		if( ! IRC_WriteStrClient( from, RPL_TRACELINK_MSG, Client_ID( from ), PACKAGE_NAME, PACKAGE_VERSION, Client_ID( target ), Client_ID( Client_NextHop( target )), Option_String( idx2 ), time( NULL ) - Conn_StartTime( idx2 ), Conn_SendQ( idx ), Conn_SendQ( idx2 ))) return DISCONNECTED;

		/* Forward command */
		IRC_WriteStrClientPrefix( target, from, "TRACE %s", Req->argv[0] );
		return CONNECTED;
	}

	/* Infos about all connected servers */
	c = Client_First( );
	while( c )
	{
		if( Client_Conn( c ) > NONE )
		{
			/* Local client */
			if( Client_Type( c ) == CLIENT_SERVER )
			{
				/* Server link */
				strlcpy( user, Client_User( c ), sizeof( user ));
				if( user[0] == '~' ) strlcpy( user, "unknown", sizeof( user ));
				if( ! IRC_WriteStrClient( from, RPL_TRACESERVER_MSG, Client_ID( from ), Client_ID( c ), user, Client_Hostname( c ), Client_Mask( Client_ThisServer( )), Option_String( Client_Conn( c )))) return DISCONNECTED;
			}
			if(( Client_Type( c ) == CLIENT_USER ) && ( strchr( Client_Modes( c ), 'o' )))
			{
				/* IRC Operator */
				if( ! IRC_WriteStrClient( from, RPL_TRACEOPERATOR_MSG, Client_ID( from ), Client_ID( c ))) return DISCONNECTED;
			}
		}
		c = Client_Next( c );
	}

	IRC_SetPenalty( Client, 3 );
	return IRC_WriteStrClient( from, RPL_TRACEEND_MSG, Client_ID( from ), Conf_ServerName, PACKAGE_NAME, PACKAGE_VERSION, NGIRCd_DebugLevel );
} /* IRC_TRACE */


GLOBAL bool
IRC_HELP( CLIENT *Client, REQUEST *Req )
{
	COMMAND *cmd;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Bad number of arguments? */
	if( Req->argc > 0 ) return IRC_WriteStrClient( Client, ERR_NORECIPIENT_MSG, Client_ID( Client ), Req->command );

	cmd = Parse_GetCommandStruct( );
	while( cmd->name )
	{
		if( ! IRC_WriteStrClient( Client, "NOTICE %s :%s", Client_ID( Client ), cmd->name )) return DISCONNECTED;
		cmd++;
	}
	
	IRC_SetPenalty( Client, 2 );
	return CONNECTED;
} /* IRC_HELP */


static char *
Option_String( CONN_ID Idx )
{
	static char option_txt[8];
	UINT16 options;

	options = Conn_Options(Idx);

	strcpy(option_txt, "F");	/* No idea what this means, but the
					 * original ircd sends it ... */
#ifdef ZLIB
	if(options & CONN_ZIP)		/* zlib compression supported. */
		strcat(option_txt, "z");
#endif

	return option_txt;
} /* Option_String */


/* -eof- */
