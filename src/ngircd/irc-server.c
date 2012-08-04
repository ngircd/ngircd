/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2007 Alexander Barton (alex@barton.de)
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
 * IRC commands for server links
 */

#include "imp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "defines.h"
#include "conn.h"
#include "conn-func.h"
#include "conn-zip.h"
#include "conf.h"
#include "channel.h"
#include "irc-write.h"
#include "lists.h"
#include "log.h"
#include "messages.h"
#include "parse.h"
#include "numeric.h"
#include "ngircd.h"
#include "irc-info.h"
#include "op.h"

#include "exp.h"
#include "irc-server.h"


/**
 * Handler for the IRC command "SERVER".
 * See RFC 2813 section 4.1.2.
 */
GLOBAL bool
IRC_SERVER( CLIENT *Client, REQUEST *Req )
{
	char str[LINE_LEN];
	CLIENT *from, *c;
	int i;
	CONN_ID con;
	
	assert( Client != NULL );
	assert( Req != NULL );

	/* Return an error if this is not a local client */
	if (Client_Conn(Client) <= NONE)
		return IRC_WriteStrClient(Client, ERR_UNKNOWNCOMMAND_MSG,
					  Client_ID(Client), Req->command);

	if (Client_Type(Client) == CLIENT_GOTPASS ||
	    Client_Type(Client) == CLIENT_GOTPASS_2813) {
		/* We got a PASS command from the peer, and now a SERVER
		 * command: the peer tries to register itself as a server. */
		LogDebug("Connection %d: got SERVER command (new server link) ...",
			Client_Conn(Client));

		if(( Req->argc != 2 ) && ( Req->argc != 3 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		/* Ist this server configured on out side? */
		for( i = 0; i < MAX_SERVERS; i++ ) if( strcasecmp( Req->argv[0], Conf_Server[i].name ) == 0 ) break;
		if( i >= MAX_SERVERS )
		{
			Log( LOG_ERR, "Connection %d: Server \"%s\" not configured here!", Client_Conn( Client ), Req->argv[0] );
			Conn_Close( Client_Conn( Client ), NULL, "Server not configured here", true);
			return DISCONNECTED;
		}
		if( strcmp( Client_Password( Client ), Conf_Server[i].pwd_in ) != 0 )
		{
			/* wrong password */
			Log( LOG_ERR, "Connection %d: Got bad password from server \"%s\"!", Client_Conn( Client ), Req->argv[0] );
			Conn_Close( Client_Conn( Client ), NULL, "Bad password", true);
			return DISCONNECTED;
		}
		
		/* Is there a registered server with this ID? */
		if( ! Client_CheckID( Client, Req->argv[0] )) return DISCONNECTED;

		Client_SetID( Client, Req->argv[0] );
		Client_SetHops( Client, 1 );
		Client_SetInfo( Client, Req->argv[Req->argc - 1] );

		/* Is this server registering on our side, or are we connecting to
		 * a remote server? */
		con = Client_Conn(Client);
		if (Client_Token(Client) != TOKEN_OUTBOUND) {
			/* Incoming connection, send user/pass */
			if (!IRC_WriteStrClient(Client, "PASS %s %s",
						Conf_Server[i].pwd_out,
						NGIRCd_ProtoID)
			    || !IRC_WriteStrClient(Client, "SERVER %s 1 :%s",
						   Conf_ServerName,
						   Conf_ServerInfo)) {
				    Conn_Close(con, "Unexpected server behavior!",
					       NULL, false);
				    return DISCONNECTED;
			}
			Client_SetIntroducer(Client, Client);
			Client_SetToken(Client, 1);
		} else {
			/* outgoing connect, we already sent a SERVER and PASS
			 * command to the peer */
			Client_SetToken(Client, atoi(Req->argv[1]));
		}

		/* Mark this connection as belonging to an configured server */
		Conf_SetServer(i, con);

		/* Check protocol level */
		if (Client_Type(Client) == CLIENT_GOTPASS) {
			/* We got a "simple" PASS command, so the peer is
			 * using the protocol as defined in RFC 1459. */
			if (! (Conn_Options(con) & CONN_RFC1459))
				Log(LOG_INFO,
				    "Switching connection %d (\"%s\") to RFC 1459 compatibility mode.",
				    con, Client_ID(Client));
			Conn_SetOption(con, CONN_RFC1459);
		}

		Client_SetType(Client, CLIENT_UNKNOWNSERVER);

#ifdef ZLIB
		if (strchr(Client_Flags(Client), 'Z') && !Zip_InitConn(con)) {
			Conn_Close( con, "Can't inizialize compression (zlib)!", NULL, false );
			return DISCONNECTED;
		}
#endif

#ifdef IRCPLUS
		if (strchr(Client_Flags(Client), 'H')) {
			LogDebug("Peer supports IRC+ extended server handshake ...");
			if (!IRC_Send_ISUPPORT(Client))
				return DISCONNECTED;
			return IRC_WriteStrClient(Client, RPL_ENDOFMOTD_MSG,
						  Client_ID(Client));
		} else {
#endif
			if (Conf_MaxNickLength != CLIENT_NICK_LEN_DEFAULT)
				Log(LOG_CRIT,
				    "Attention: this server uses a non-standard nick length, but the peer doesn't support the IRC+ extended server handshake!");
#ifdef IRCPLUS
		}
#endif

		return IRC_Num_ENDOFMOTD(Client, Req);
	}
	else if( Client_Type( Client ) == CLIENT_SERVER )
	{
		/* New server is being introduced to the network */

		if( Req->argc != 4 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		/* check for existing server with same ID */
		if( ! Client_CheckID( Client, Req->argv[0] )) return DISCONNECTED;

		from = Client_Search( Req->prefix );
		if( ! from )
		{
			/* Uh, Server, that introduced the new server is unknown?! */
			Log( LOG_ALERT, "Unknown ID in prefix of SERVER: \"%s\"! (on connection %d)", Req->prefix, Client_Conn( Client ));
			Conn_Close( Client_Conn( Client ), NULL, "Unknown ID in prefix of SERVER", true);
			return DISCONNECTED;
		}

		c = Client_NewRemoteServer(Client, Req->argv[0], from, atoi(Req->argv[1]), atoi(Req->argv[2]), Req->argv[3], true);
		if (!c) {
			Log( LOG_ALERT, "Can't create client structure for server! (on connection %d)", Client_Conn( Client ));
			Conn_Close( Client_Conn( Client ), NULL, "Can't allocate client structure for remote server", true);
			return DISCONNECTED;
		}

		if(( Client_Hops( c ) > 1 ) && ( Req->prefix[0] )) snprintf( str, sizeof( str ), "connected to %s, ", Client_ID( from ));
		else strcpy( str, "" );
		Log( LOG_NOTICE|LOG_snotice, "Server \"%s\" registered (via %s, %s%d hop%s).", Client_ID( c ), Client_ID( Client ), str, Client_Hops( c ), Client_Hops( c ) > 1 ? "s": "" );

		/* notify other servers */
		IRC_WriteStrServersPrefix( Client, from, "SERVER %s %d %d :%s", Client_ID( c ), Client_Hops( c ) + 1, Client_MyToken( c ), Client_Info( c ));

		return CONNECTED;
	} else
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);
} /* IRC_SERVER */


GLOBAL bool
IRC_NJOIN( CLIENT *Client, REQUEST *Req )
{
	char nick_in[COMMAND_LEN], nick_out[COMMAND_LEN], *channame, *ptr, modes[8];
	bool is_owner, is_chanadmin, is_op, is_halfop, is_voiced;
	CHANNEL *chan;
	CLIENT *c;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( Req->argc != 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	strlcpy( nick_in, Req->argv[1], sizeof( nick_in ));
	strcpy( nick_out, "" );

	channame = Req->argv[0];
	ptr = strtok( nick_in, "," );
	while( ptr )
	{
		is_op = is_voiced = false;
		
		/* cut off prefixes */
		while(( *ptr == '~') || ( *ptr == '&' ) || ( *ptr == '@' ) ||
			( *ptr == '%') || ( *ptr == '+' ))
		{	
			if( *ptr == '~' ) is_owner = true;
			if( *ptr == '&' ) is_chanadmin = true;
			if( *ptr == '@' ) is_op = true;
			if( *ptr == 'h' ) is_halfop = true;
			if( *ptr == '+' ) is_voiced = true;
			ptr++;
		}

		c = Client_Search( ptr );
		if( c )
		{
			Channel_Join( c, channame );
			chan = Channel_Search( channame );
			assert( chan != NULL );
			
			if( is_owner ) Channel_UserModeAdd( chan, c, 'q' );
			if( is_chanadmin ) Channel_UserModeAdd( chan, c, 'a' );
			if( is_op ) Channel_UserModeAdd( chan, c, 'o' );
			if( is_halfop ) Channel_UserModeAdd( chan, c, 'h' );
			if( is_voiced ) Channel_UserModeAdd( chan, c, 'v' );

			/* announce to channel... */
			IRC_WriteStrChannelPrefix( Client, chan, c, false, "JOIN :%s", channame );

			/* set Channel-User-Modes */
			strlcpy( modes, Channel_UserModes( chan, c ), sizeof( modes ));
			if( modes[0] )
			{
				/* send modes to channel */
				IRC_WriteStrChannelPrefix( Client, chan, Client, false, "MODE %s +%s %s", channame, modes, Client_ID( c ));
			}

			if( nick_out[0] != '\0' ) strlcat( nick_out, ",", sizeof( nick_out ));
			if( is_owner ) strlcat( nick_out, "~", sizeof( nick_out ));
			if( is_chanadmin ) strlcat( nick_out, "&", sizeof( nick_out ));
			if( is_op ) strlcat( nick_out, "@", sizeof( nick_out ));
			if( is_halfop ) strlcat( nick_out, "%", sizeof( nick_out ));
			if( is_voiced ) strlcat( nick_out, "+", sizeof( nick_out ));
			strlcat( nick_out, ptr, sizeof( nick_out ));
		}
		else Log( LOG_ERR, "Got NJOIN for unknown nick \"%s\" for channel \"%s\"!", ptr, channame );
		
		/* search for next Nick */
		ptr = strtok( NULL, "," );
	}

	/* forward to other servers */
	if( nick_out[0] != '\0' ) IRC_WriteStrServersPrefix( Client, Client_ThisServer( ), "NJOIN %s :%s", Req->argv[0], nick_out );

	return CONNECTED;
} /* IRC_NJOIN */


/**
 * Handler for the IRC command "SQUIT".
 * See RFC 2813 section 4.1.2 and RFC 2812 section 3.1.8.
 */
GLOBAL bool
IRC_SQUIT(CLIENT * Client, REQUEST * Req)
{
	char msg[COMMAND_LEN], logmsg[COMMAND_LEN];
	CLIENT *from, *target;
	CONN_ID con;
	int loglevel;

	assert(Client != NULL);
	assert(Req != NULL);

	if (Client_Type(Client) != CLIENT_SERVER
	    && !Client_HasMode(Client, 'o'))
		return Op_NoPrivileges(Client, Req);

	/* Bad number of arguments? */
	if (Req->argc != 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	if (Client_Type(Client) == CLIENT_SERVER && Req->prefix) {
		from = Client_Search(Req->prefix);
		if (Client_Type(from) != CLIENT_SERVER
		    && !Op_Check(Client, Req))
			return Op_NoPrivileges(Client, Req);
	} else
		from = Client;
	if (!from)
		return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->prefix);

	if (Client_Type(Client) == CLIENT_USER)
		loglevel = LOG_NOTICE | LOG_snotice;
	else
		loglevel = LOG_DEBUG;
	Log(loglevel, "Got SQUIT from %s for \"%s\": \"%s\" ...",
	    Client_ID(from), Req->argv[0], Req->argv[1]);

	target = Client_Search(Req->argv[0]);
	if (Client_Type(Client) != CLIENT_SERVER &&
	    target == Client_ThisServer())
		return Op_NoPrivileges(Client, Req);
	if (!target) {
		/* The server is (already) unknown */
		Log(LOG_WARNING,
		    "Got SQUIT from %s for unknown server \"%s\"!?",
		    Client_ID(Client), Req->argv[0]);
		return CONNECTED;
	}

	con = Client_Conn(target);

	if (Req->argv[1][0])
		if (Client_NextHop(from) != Client || con > NONE)
			snprintf(msg, sizeof(msg), "%s (SQUIT from %s)",
				 Req->argv[1], Client_ID(from));
		else
			strlcpy(msg, Req->argv[1], sizeof(msg));
	else
		snprintf(msg, sizeof(msg), "Got SQUIT from %s",
			 Client_ID(from));

	if (con > NONE) {
		/* We are directly connected to the target server, so we
		 * have to tear down the connection and to inform all the
		 * other remaining servers in the network */
		IRC_SendWallops(Client_ThisServer(), Client_ThisServer(),
				"Received SQUIT %s from %s: %s",
				Req->argv[0], Client_ID(from),
				Req->argv[1][0] ? Req->argv[1] : "-");
		Conn_Close(con, NULL, msg, true);
		if (con == Client_Conn(Client))
			return DISCONNECTED;
	} else {
		/* This server is not directly connected, so the SQUIT must
		 * be forwarded ... */
		if (Client_Type(from) != CLIENT_SERVER) {
			/* The origin is not an IRC server, so don't evaluate
			 * this SQUIT but simply forward it */
			IRC_WriteStrClientPrefix(Client_NextHop(target),
			    from, "SQUIT %s :%s", Req->argv[0], Req->argv[1]);
		} else {
			/* SQUIT has been generated by another server, so
			 * remove the target server from the network! */
			logmsg[0] = '\0';
			if (!strchr(msg, '('))
				snprintf(logmsg, sizeof(logmsg),
					 "%s (SQUIT from %s)", Req->argv[1],
					 Client_ID(from));
			Client_Destroy(target, logmsg[0] ? logmsg : msg,
				       msg, false);
		}
	}
	return CONNECTED;
} /* IRC_SQUIT */

/* -eof- */
