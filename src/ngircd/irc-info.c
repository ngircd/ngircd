/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2005 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * IRC info commands
 */


#include "portab.h"

static char UNUSED id[] = "$Id: irc-info.c,v 1.33.2.2 2006/12/02 14:26:53 fw Exp $";

#include "imp.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "ngircd.h"
#include "cvs-version.h"
#include "conn-func.h"
#include "conn-zip.h"
#include "client.h"
#include "channel.h"
#include "resolve.h"
#include "conf.h"
#include "defines.h"
#include "log.h"
#include "messages.h"
#include "tool.h"
#include "parse.h"
#include "irc-write.h"

#include "exp.h"
#include "irc-info.h"


GLOBAL bool
IRC_ADMIN(CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *prefix;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Ziel suchen */
	if( Req->argc == 1 ) target = Client_Search( Req->argv[0] );
	else target = Client_ThisServer( );

	/* Prefix ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) prefix = Client_Search( Req->prefix );
	else prefix = Client;
	if( ! prefix ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* An anderen Server weiterleiten? */
	if( target != Client_ThisServer( ))
	{
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( prefix, ERR_NOSUCHSERVER_MSG, Client_ID( prefix ), Req->argv[0] );

		/* forwarden */
		IRC_WriteStrClientPrefix( target, prefix, "ADMIN %s", Req->argv[0] );
		return CONNECTED;
	}

	/* mit Versionsinfo antworten */
	if( ! IRC_WriteStrClient( Client, RPL_ADMINME_MSG, Client_ID( prefix ), Conf_ServerName )) return DISCONNECTED;
	if( ! IRC_WriteStrClient( Client, RPL_ADMINLOC1_MSG, Client_ID( prefix ), Conf_ServerAdmin1 )) return DISCONNECTED;
	if( ! IRC_WriteStrClient( Client, RPL_ADMINLOC2_MSG, Client_ID( prefix ), Conf_ServerAdmin2 )) return DISCONNECTED;
	if( ! IRC_WriteStrClient( Client, RPL_ADMINEMAIL_MSG, Client_ID( prefix ), Conf_ServerAdminMail )) return DISCONNECTED;

	IRC_SetPenalty( Client, 1 );
	return CONNECTED;
} /* IRC_ADMIN */


GLOBAL bool
IRC_ISON( CLIENT *Client, REQUEST *Req )
{
	char rpl[COMMAND_LEN];
	CLIENT *c;
	char *ptr;
	int i;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	strlcpy( rpl, RPL_ISON_MSG, sizeof rpl );
	for( i = 0; i < Req->argc; i++ )
	{
		ptr = strtok( Req->argv[i], " " );
		while( ptr )
		{
			ngt_TrimStr( ptr );
			c = Client_Search( ptr );
			if( c && ( Client_Type( c ) == CLIENT_USER ))
			{
				/* Dieser Nick ist "online" */
				strlcat( rpl, ptr, sizeof( rpl ));
				strlcat( rpl, " ", sizeof( rpl ));
			}
			ptr = strtok( NULL, " " );
		}
	}
	ngt_TrimLastChr(rpl, ' ');

	return IRC_WriteStrClient( Client, rpl, Client_ID( Client ) );
} /* IRC_ISON */


GLOBAL bool
IRC_LINKS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *from, *c;
	char *mask;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Server-Mask ermitteln */
	if( Req->argc > 0 ) mask = Req->argv[Req->argc - 1];
	else mask = "*";

	/* Absender ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* An anderen Server forwarden? */
	if( Req->argc == 2 )
	{
		target = Client_Search( Req->argv[0] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[0] );
		else if( target != Client_ThisServer( )) return IRC_WriteStrClientPrefix( target, from, "LINKS %s %s", Req->argv[0], Req->argv[1] );
	}

	/* Wer ist der Absender? */
	if( Client_Type( Client ) == CLIENT_SERVER ) target = Client_Search( Req->prefix );
	else target = Client;
	if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	c = Client_First( );
	while( c )
	{
		if( Client_Type( c ) == CLIENT_SERVER )
		{
			if( ! IRC_WriteStrClient( target, RPL_LINKS_MSG, Client_ID( target ), Client_ID( c ), Client_ID( Client_TopServer( c ) ? Client_TopServer( c ) : Client_ThisServer( )), Client_Hops( c ), Client_Info( c ))) return DISCONNECTED;
		}
		c = Client_Next( c );
	}
	
	IRC_SetPenalty( target, 1 );
	return IRC_WriteStrClient( target, RPL_ENDOFLINKS_MSG, Client_ID( target ), mask );
} /* IRC_LINKS */


GLOBAL bool
IRC_LUSERS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *from;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Absender ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* An anderen Server forwarden? */
	if( Req->argc == 2 )
	{
		target = Client_Search( Req->argv[1] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[1] );
		else if( target != Client_ThisServer( )) return IRC_WriteStrClientPrefix( target, from, "LUSERS %s %s", Req->argv[0], Req->argv[1] );
	}

	/* Wer ist der Absender? */
	if( Client_Type( Client ) == CLIENT_SERVER ) target = Client_Search( Req->prefix );
	else target = Client;
	if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	IRC_Send_LUSERS( target );

	IRC_SetPenalty( target, 1 );
	return CONNECTED;
} /* IRC_LUSERS */


GLOBAL bool
IRC_MOTD( CLIENT *Client, REQUEST *Req )
{
	CLIENT *from, *target;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* From aus Prefix ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	if( Req->argc == 1 )
	{
		/* an anderen Server forwarden */
		target = Client_Search( Req->argv[0] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[0] );

		if( target != Client_ThisServer( ))
		{
			/* Ok, anderer Server ist das Ziel: forwarden */
			return IRC_WriteStrClientPrefix( target, from, "MOTD %s", Req->argv[0] );
		}
	}

	IRC_SetPenalty( from, 3 );
	return IRC_Show_MOTD( from );
} /* IRC_MOTD */


GLOBAL bool
IRC_NAMES( CLIENT *Client, REQUEST *Req )
{
	char rpl[COMMAND_LEN], *ptr;
	CLIENT *target, *from, *c;
	CHANNEL *chan;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* From aus Prefix ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	if( Req->argc == 2 )
	{
		/* an anderen Server forwarden */
		target = Client_Search( Req->argv[1] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[1] );

		if( target != Client_ThisServer( ))
		{
			/* Ok, anderer Server ist das Ziel: forwarden */
			return IRC_WriteStrClientPrefix( target, from, "NAMES %s :%s", Req->argv[0], Req->argv[1] );
		}
	}

	if( Req->argc > 0 )
	{
		/* bestimmte Channels durchgehen */
		ptr = strtok( Req->argv[0], "," );
		while( ptr )
		{
			chan = Channel_Search( ptr );
			if( chan )
			{
				/* Namen ausgeben */
				if( ! IRC_Send_NAMES( from, chan )) return DISCONNECTED;
			}
			if( ! IRC_WriteStrClient( from, RPL_ENDOFNAMES_MSG, Client_ID( from ), ptr )) return DISCONNECTED;

			/* naechsten Namen ermitteln */
			ptr = strtok( NULL, "," );
		}
		return CONNECTED;
	}

	/* alle Channels durchgehen */
	chan = Channel_First( );
	while( chan )
	{
		/* Namen ausgeben */
		if( ! IRC_Send_NAMES( from, chan )) return DISCONNECTED;

		/* naechster Channel */
		chan = Channel_Next( chan );
	}

	/* Nun noch alle Clients ausgeben, die in keinem Channel sind */
	c = Client_First( );
	snprintf( rpl, sizeof( rpl ), RPL_NAMREPLY_MSG, Client_ID( from ), "*", "*" );
	while( c )
	{
		if(( Client_Type( c ) == CLIENT_USER ) && ( Channel_FirstChannelOf( c ) == NULL ) && ( ! strchr( Client_Modes( c ), 'i' )))
		{
			/* Okay, das ist ein User: anhaengen */
			if( rpl[strlen( rpl ) - 1] != ':' ) strlcat( rpl, " ", sizeof( rpl ));
			strlcat( rpl, Client_ID( c ), sizeof( rpl ));

			if( strlen( rpl ) > ( LINE_LEN - CLIENT_NICK_LEN - 4 ))
			{
				/* Zeile wird zu lang: senden! */
				if( ! IRC_WriteStrClient( from, "%s", rpl )) return DISCONNECTED;
				snprintf( rpl, sizeof( rpl ), RPL_NAMREPLY_MSG, Client_ID( from ), "*", "*" );
			}
		}

		/* naechster Client */
		c = Client_Next( c );
	}
	if( rpl[strlen( rpl ) - 1] != ':')
	{
		/* es wurden User gefunden */
		if( ! IRC_WriteStrClient( from, "%s", rpl )) return DISCONNECTED;
	}

	IRC_SetPenalty( from, 1 );
	return IRC_WriteStrClient( from, RPL_ENDOFNAMES_MSG, Client_ID( from ), "*" );
} /* IRC_NAMES */


GLOBAL bool
IRC_STATS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *from, *target, *cl;
	CONN_ID con;
	char query;
	COMMAND *cmd;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* From aus Prefix ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	if( Req->argc == 2 )
	{
		/* an anderen Server forwarden */
		target = Client_Search( Req->argv[1] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[1] );

		if( target != Client_ThisServer( ))
		{
			/* Ok, anderer Server ist das Ziel: forwarden */
			return IRC_WriteStrClientPrefix( target, from, "STATS %s %s", Req->argv[0], Req->argv[1] );
		}
	}

	if( Req->argc > 0 ) query = Req->argv[0][0] ? Req->argv[0][0] : '*';
	else query = '*';

	switch ( query )
	{
		case 'l':	/* Links */
		case 'L':
			con = Conn_First( );
			while( con != NONE )
			{
				cl = Conn_GetClient( con );
				if( cl && (( Client_Type( cl ) == CLIENT_SERVER ) || ( cl == Client )))
				{
					/* Server link or our own connection */
#ifdef ZLIB
					if( Conn_Options( con ) & CONN_ZIP )
					{
						if( ! IRC_WriteStrClient( from, RPL_STATSLINKINFOZIP_MSG, Client_ID( from ), Client_Mask( cl ), Conn_SendQ( con ), Conn_SendMsg( con ), Zip_SendBytes( con ), Conn_SendBytes( con ), Conn_RecvMsg( con ), Zip_RecvBytes( con ), Conn_RecvBytes( con ), (long)( time( NULL ) - Conn_StartTime( con )))) return DISCONNECTED;
					}
					else
#endif
					{
						if( ! IRC_WriteStrClient( from, RPL_STATSLINKINFO_MSG, Client_ID( from ), Client_Mask( cl ), Conn_SendQ( con ), Conn_SendMsg( con ), Conn_SendBytes( con ), Conn_RecvMsg( con ), Conn_RecvBytes( con ), (long)( time( NULL ) - Conn_StartTime( con )))) return DISCONNECTED;
					}
				}
				con = Conn_Next( con );
			}
			break;
		case 'm':	/* IRC-Befehle */
		case 'M':
			cmd = Parse_GetCommandStruct( );
			while( cmd->name )
			{
				if( cmd->lcount > 0 || cmd->rcount > 0 )
				{
					if( ! IRC_WriteStrClient( from, RPL_STATSCOMMANDS_MSG, Client_ID( from ), cmd->name, cmd->lcount, cmd->bytes, cmd->rcount )) return DISCONNECTED;
				}
				cmd++;
			}
			break;
	}

	IRC_SetPenalty( from, 2 );
	return IRC_WriteStrClient( from, RPL_ENDOFSTATS_MSG, Client_ID( from ), query );
} /* IRC_STATS */


GLOBAL bool
IRC_TIME( CLIENT *Client, REQUEST *Req )
{
	CLIENT *from, *target;
	char t_str[64];
	time_t t;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* From aus Prefix ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	if( Req->argc == 1 )
	{
		/* an anderen Server forwarden */
		target = Client_Search( Req->argv[0] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[0] );

		if( target != Client_ThisServer( ))
		{
			/* Ok, anderer Server ist das Ziel: forwarden */
			return IRC_WriteStrClientPrefix( target, from, "TIME %s", Req->argv[0] );
		}
	}

	t = time( NULL );
	(void)strftime( t_str, 60, "%A %B %d %Y -- %H:%M %Z", localtime( &t ));
	return IRC_WriteStrClient( from, RPL_TIME_MSG, Client_ID( from ), Client_ID( Client_ThisServer( )), t_str );
} /* IRC_TIME */


GLOBAL bool
IRC_USERHOST( CLIENT *Client, REQUEST *Req )
{
	char rpl[COMMAND_LEN];
	CLIENT *c;
	int max, i;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Req->argc > 5 ) max = 5;
	else max = Req->argc;

	strlcpy( rpl, RPL_USERHOST_MSG, sizeof rpl );
	for( i = 0; i < max; i++ )
	{
		c = Client_Search( Req->argv[i] );
		if( c && ( Client_Type( c ) == CLIENT_USER ))
		{
			/* Dieser Nick ist "online" */
			strlcat( rpl, Client_ID( c ), sizeof( rpl ));
			if( Client_HasMode( c, 'o' )) strlcat( rpl, "*", sizeof( rpl ));
			strlcat( rpl, "=", sizeof( rpl ));
			if( Client_HasMode( c, 'a' )) strlcat( rpl, "-", sizeof( rpl ));
			else strlcat( rpl, "+", sizeof( rpl ));
			strlcat( rpl, Client_User( c ), sizeof( rpl ));
			strlcat( rpl, "@", sizeof( rpl ));
			strlcat( rpl, Client_Hostname( c ), sizeof( rpl ));
			strlcat( rpl, " ", sizeof( rpl ));
		}
	}
	ngt_TrimLastChr( rpl, ' ');

	return IRC_WriteStrClient( Client, rpl, Client_ID( Client ) );
} /* IRC_USERHOST */


GLOBAL bool
IRC_VERSION( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *prefix;
#ifdef CVSDATE
	char ver[12], vertxt[30];
#endif

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Ziel suchen */
	if( Req->argc == 1 ) target = Client_Search( Req->argv[0] );
	else target = Client_ThisServer( );

	/* Prefix ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) prefix = Client_Search( Req->prefix );
	else prefix = Client;
	if( ! prefix ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* An anderen Server weiterleiten? */
	if( target != Client_ThisServer( ))
	{
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( prefix, ERR_NOSUCHSERVER_MSG, Client_ID( prefix ), Req->argv[0] );

		/* forwarden */
		IRC_WriteStrClientPrefix( target, prefix, "VERSION %s", Req->argv[0] );
		return CONNECTED;
	}

	/* mit Versionsinfo antworten */
	IRC_SetPenalty( Client, 1 );
#ifdef CVSDATE
	strlcpy( ver, CVSDATE, sizeof( ver ));
	strncpy( ver + 4, ver + 5, 2 );
	strncpy( ver + 6, ver + 8, 3 );
	snprintf( vertxt, sizeof( vertxt ), "%s(%s)", PACKAGE_VERSION, ver );
	return IRC_WriteStrClient( Client, RPL_VERSION_MSG, Client_ID( prefix ), PACKAGE_NAME, vertxt, NGIRCd_DebugLevel, Conf_ServerName, NGIRCd_VersionAddition );
#else
	return IRC_WriteStrClient( Client, RPL_VERSION_MSG, Client_ID( prefix ), PACKAGE_NAME, PACKAGE_VERSION, NGIRCd_DebugLevel, Conf_ServerName, NGIRCd_VersionAddition );
#endif
} /* IRC_VERSION */


GLOBAL bool
IRC_WHO( CLIENT *Client, REQUEST *Req )
{
	bool ok, only_ops;
	char flags[8];
	const char *ptr;
	CL2CHAN *cl2chan;
	CHANNEL *chan, *cn;
	CLIENT *c;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	only_ops = false;
	chan = NULL;

	if( Req->argc == 2 )
	{
		/* Nur OPs anzeigen? */
		if( strcmp( Req->argv[1], "o" ) == 0 ) only_ops = true;
#ifdef STRICT_RFC
		else return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );
#endif
	}

	if( Req->argc >= 1 )
	{
		/* wurde ein Channel oder Nick-Mask angegeben? */
		chan = Channel_Search( Req->argv[0] );
	}

	if( chan )
	{
		/* User eines Channels ausgeben */
		if( ! IRC_Send_WHO( Client, chan, only_ops )) return DISCONNECTED;
	}

	c = Client_First( );
	while( c )
	{
		if(( Client_Type( c ) == CLIENT_USER ) && ( ! strchr( Client_Modes( c ), 'i' )))
		{
			ok = false;
			if( Req->argc == 0 ) ok = true;
			else
			{
				if( strcasecmp( Req->argv[0], Client_ID( c )) == 0 ) ok = true;
				else if( strcmp( Req->argv[0], "0" ) == 0 ) ok = true;
			}

			if( ok && (( ! only_ops ) || ( strchr( Client_Modes( c ), 'o' ))))
			{
				/* Get flags */
				strcpy( flags, "H" );
				if( strchr( Client_Modes( c ), 'o' )) strlcat( flags, "*", sizeof( flags ));

				/* Search suitable channel */
				cl2chan = Channel_FirstChannelOf( c );
				while( cl2chan )
				{
					cn = Channel_GetChannel( cl2chan );
					if( Channel_IsMemberOf( cn, Client ) ||
					    ! strchr( Channel_Modes( cn ), 's' ))
					{
						ptr = Channel_Name( cn );
						break;
					}
					cl2chan = Channel_NextChannelOf( c, cl2chan );
				}
				if( ! cl2chan ) ptr = "*";

				if( ! IRC_WriteStrClient( Client, RPL_WHOREPLY_MSG, Client_ID( Client ), ptr, Client_User( c ), Client_Hostname( c ), Client_ID( Client_Introducer( c )), Client_ID( c ), flags, Client_Hops( c ), Client_Info( c ))) return DISCONNECTED;
			}
		}

		/* naechster Client */
		c = Client_Next( c );
	}

	if( chan ) return IRC_WriteStrClient( Client, RPL_ENDOFWHO_MSG, Client_ID( Client ), Channel_Name( chan ));
	else if( Req->argc == 0 ) return IRC_WriteStrClient( Client, RPL_ENDOFWHO_MSG, Client_ID( Client ), "*" );
	else return IRC_WriteStrClient( Client, RPL_ENDOFWHO_MSG, Client_ID( Client ), Req->argv[0] );
} /* IRC_WHO */


GLOBAL bool
IRC_WHOIS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *from, *target, *c;
	char str[LINE_LEN + 1];
	CL2CHAN *cl2chan;
	CHANNEL *chan;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Bad number of parameters? */
	if(( Req->argc < 1 ) || ( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Search client */
	c = Client_Search( Req->argv[Req->argc - 1] );
	if(( ! c ) || ( Client_Type( c ) != CLIENT_USER )) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->argv[Req->argc - 1] );

	/* Search sender of the WHOIS */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* Forward to other server? */
	if( Req->argc > 1 )
	{
		/* Search target server (can be specified as nick of that server!) */
		target = Client_Search( Req->argv[0] );
		if( ! target ) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[0] );
	}
	else target = Client_ThisServer( );

	assert( target != NULL );

	if(( Client_NextHop( target ) != Client_ThisServer( )) && ( Client_Type( Client_NextHop( target )) == CLIENT_SERVER )) return IRC_WriteStrClientPrefix( target, from, "WHOIS %s :%s", Req->argv[0], Req->argv[1] );

	/* Nick, user and name */
	if( ! IRC_WriteStrClient( from, RPL_WHOISUSER_MSG, Client_ID( from ), Client_ID( c ), Client_User( c ), Client_Hostname( c ), Client_Info( c ))) return DISCONNECTED;

	/* Server */
	if( ! IRC_WriteStrClient( from, RPL_WHOISSERVER_MSG, Client_ID( from ), Client_ID( c ), Client_ID( Client_Introducer( c )), Client_Info( Client_Introducer( c )))) return DISCONNECTED;

	/* Channels */
	snprintf( str, sizeof( str ), RPL_WHOISCHANNELS_MSG, Client_ID( from ), Client_ID( c ));
	cl2chan = Channel_FirstChannelOf( c );
	while( cl2chan )
	{
		chan = Channel_GetChannel( cl2chan );
		assert( chan != NULL );

		/* next */
		cl2chan = Channel_NextChannelOf( c, cl2chan );

		/* Secret channel? */
		if( strchr( Channel_Modes( chan ), 's' ) && ! Channel_IsMemberOf( chan, Client )) continue;

		/* Concatenate channel names */
		if( str[strlen( str ) - 1] != ':' ) strlcat( str, " ", sizeof( str ));
		if( strchr( Channel_UserModes( chan, c ), 'o' )) strlcat( str, "@", sizeof( str ));
		else if( strchr( Channel_UserModes( chan, c ), 'v' )) strlcat( str, "+", sizeof( str ));
		strlcat( str, Channel_Name( chan ), sizeof( str ));

		if( strlen( str ) > ( LINE_LEN - CHANNEL_NAME_LEN - 4 ))
		{
			/* Line becomes too long: send it! */
			if( ! IRC_WriteStrClient( Client, "%s", str )) return DISCONNECTED;
			snprintf( str, sizeof( str ), RPL_WHOISCHANNELS_MSG, Client_ID( from ), Client_ID( c ));
		}
	}
	if( str[strlen( str ) - 1] != ':')
	{
		/* There is data left to send: */
		if( ! IRC_WriteStrClient( Client, "%s", str )) return DISCONNECTED;
	}

	/* IRC-Operator? */
	if( Client_HasMode( c, 'o' ))
	{
		if( ! IRC_WriteStrClient( from, RPL_WHOISOPERATOR_MSG, Client_ID( from ), Client_ID( c ))) return DISCONNECTED;
	}

	/* Idle (only local clients) */
	if( Client_Conn( c ) > NONE )
	{
		if( ! IRC_WriteStrClient( from, RPL_WHOISIDLE_MSG, Client_ID( from ), Client_ID( c ), Conn_GetIdle( Client_Conn ( c )))) return DISCONNECTED;
	}

	/* Away? */
	if( Client_HasMode( c, 'a' ))
	{
		if( ! IRC_WriteStrClient( from, RPL_AWAY_MSG, Client_ID( from ), Client_ID( c ), Client_Away( c ))) return DISCONNECTED;
	}

	/* End of Whois */
	return IRC_WriteStrClient( from, RPL_ENDOFWHOIS_MSG, Client_ID( from ), Client_ID( c ));
} /* IRC_WHOIS */


/**
 * IRC "WHOWAS" function.
 * This function implements the IRC command "WHOWHAS". It handles local
 * requests and request that should be forwarded to other servers.
 */
GLOBAL bool
IRC_WHOWAS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *prefix;
	WHOWAS *whowas;
	int max, last, count, i;
	char t_str[60];
	
	assert( Client != NULL );
	assert( Req != NULL );

	/* Wrong number of parameters? */
	if(( Req->argc < 1 ) || ( Req->argc > 3 ))
		return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG,
					   Client_ID( Client ), Req->command );

	/* Search taget */
	if( Req->argc == 3 )
		target = Client_Search( Req->argv[2] );
	else
		target = Client_ThisServer( );

	/* Get prefix */
	if( Client_Type( Client ) == CLIENT_SERVER )
		prefix = Client_Search( Req->prefix );
	else
		prefix = Client;

	if( ! prefix )
		return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG,
					   Client_ID( Client ), Req->prefix );

	/* Forward to other server? */
	if( target != Client_ThisServer( ))
	{
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER ))
			return IRC_WriteStrClient( prefix, ERR_NOSUCHSERVER_MSG,
						   Client_ID( prefix ),
						   Req->argv[2] );

		/* Forward */
		IRC_WriteStrClientPrefix( target, prefix, "WHOWAS %s %s %s",
					  Req->argv[0], Req->argv[1],
					  Req->argv[2] );
		return CONNECTED;
	}
	
	whowas = Client_GetWhowas( );
	last = Client_GetLastWhowasIndex( );
	if( last < 0 ) last = 0;
	
	if( Req->argc > 1 )
	{
		max = atoi( Req->argv[1] );
		if( max < 1 ) max = MAX_WHOWAS;
	}
	else
		max = DEFAULT_WHOWAS;
	
	i = last;
	count = 0;
	do
	{
		/* Used entry? */
		if( whowas[i].time > 0 &&
		    strcasecmp( Req->argv[0], whowas[i].id ) == 0 )
		{
			(void)strftime( t_str, sizeof(t_str),
					"%a %b %d %H:%M:%S %Y",
					localtime( &whowas[i].time ));
		
			if( ! IRC_WriteStrClient( prefix, RPL_WHOWASUSER_MSG,
						  Client_ID( prefix ),
						  whowas[i].id,
						  whowas[i].user,
						  whowas[i].host, 
						  whowas[i].info ))
				return DISCONNECTED;
		
			if( ! IRC_WriteStrClient( prefix, RPL_WHOISSERVER_MSG,
						  Client_ID( prefix ),
						  whowas[i].id,
						  whowas[i].server, t_str ))
				return DISCONNECTED;
		
			count++;
			if( count >= max ) break;
		}
		
		/* previos entry */
		i--;

		/* "underflow", wrap around */
		if( i < 0 ) i = MAX_WHOWAS - 1;
	} while( i != last );
	
	return IRC_WriteStrClient( prefix, RPL_ENDOFWHOWAS_MSG,
				   Client_ID( prefix ), Req->argv[0] );
} /* IRC_WHOWAS */


GLOBAL bool
IRC_Send_LUSERS( CLIENT *Client )
{
	unsigned long cnt;
#ifndef STRICT_RFC
	unsigned long max;
#endif

	assert( Client != NULL );

	/* Users, services and serevers in the network */
	if( ! IRC_WriteStrClient( Client, RPL_LUSERCLIENT_MSG, Client_ID( Client ), Client_UserCount( ), Client_ServiceCount( ), Client_ServerCount( ))) return DISCONNECTED;

	/* Number of IRC operators */
	cnt = Client_OperCount( );
	if( cnt > 0 )
	{
		if( ! IRC_WriteStrClient( Client, RPL_LUSEROP_MSG, Client_ID( Client ), cnt )) return DISCONNECTED;
	}

	/* Unknown connections */
	cnt = Client_UnknownCount( );
	if( cnt > 0 )
	{
		if( ! IRC_WriteStrClient( Client, RPL_LUSERUNKNOWN_MSG, Client_ID( Client ), cnt )) return DISCONNECTED;
	}

	/* Number of created channels */
	if( ! IRC_WriteStrClient( Client, RPL_LUSERCHANNELS_MSG, Client_ID( Client ), Channel_Count( ))) return DISCONNECTED;

	/* Number of local users, services and servers */
	if( ! IRC_WriteStrClient( Client, RPL_LUSERME_MSG, Client_ID( Client ), Client_MyUserCount( ), Client_MyServiceCount( ), Client_MyServerCount( ))) return DISCONNECTED;

#ifndef STRICT_RFC
	/* Maximum number of local users */
	cnt = Client_MyUserCount();
	max = Client_MyMaxUserCount();
	if (! IRC_WriteStrClient(Client, RPL_LOCALUSERS_MSG, Client_ID(Client),
			cnt, max, cnt, max))
		return DISCONNECTED;
	/* Maximum number of users in the network */
	cnt = Client_UserCount();
	max = Client_MaxUserCount();
	if(! IRC_WriteStrClient(Client, RPL_NETUSERS_MSG, Client_ID(Client),
			cnt, max, cnt, max))
		return DISCONNECTED;
#endif
	
	return CONNECTED;
} /* IRC_Send_LUSERS */


static bool Show_MOTD_Start(CLIENT *Client)
{
	return IRC_WriteStrClient(Client, RPL_MOTDSTART_MSG,
		Client_ID( Client ), Client_ID( Client_ThisServer( )));
}

static bool Show_MOTD_Sendline(CLIENT *Client, const char *msg)
{
	return IRC_WriteStrClient(Client, RPL_MOTD_MSG, Client_ID( Client ), msg);
}

static bool Show_MOTD_End(CLIENT *Client)
{
	return IRC_WriteStrClient( Client, RPL_ENDOFMOTD_MSG, Client_ID( Client ));
}


GLOBAL bool
IRC_Show_MOTD( CLIENT *Client )
{
	char line[127];
	FILE *fd;

	assert( Client != NULL );

	if (Conf_MotdPhrase[0]) {
		if (!Show_MOTD_Start(Client))
			return DISCONNECTED;
		if (!Show_MOTD_Sendline(Client, Conf_MotdPhrase))
			return DISCONNECTED;

		return Show_MOTD_End(Client);
	}

	fd = fopen( Conf_MotdFile, "r" );
	if( ! fd ) {
		Log( LOG_WARNING, "Can't read MOTD file \"%s\": %s", Conf_MotdFile, strerror( errno ));
		return IRC_WriteStrClient( Client, ERR_NOMOTD_MSG, Client_ID( Client ) );
	}

	if (!Show_MOTD_Start( Client )) {
		fclose(fd);
		return false;
	}

	while (fgets( line, (int)sizeof line, fd )) {
		ngt_TrimLastChr( line, '\n');

		if( ! Show_MOTD_Sendline( Client, line)) {
			fclose( fd );
			return false;
		}
	}
	fclose(fd);
	return Show_MOTD_End(Client);
} /* IRC_Show_MOTD */


GLOBAL bool
IRC_Send_NAMES( CLIENT *Client, CHANNEL *Chan )
{
	bool is_visible, is_member;
	char str[LINE_LEN + 1];
	CL2CHAN *cl2chan;
	CLIENT *cl;

	assert( Client != NULL );
	assert( Chan != NULL );

	if( Channel_IsMemberOf( Chan, Client )) is_member = true;
	else is_member = false;

	/* Secret channel? */
	if( ! is_member && strchr( Channel_Modes( Chan ), 's' )) return CONNECTED;

	/* Alle Mitglieder suchen */
	snprintf( str, sizeof( str ), RPL_NAMREPLY_MSG, Client_ID( Client ), "=", Channel_Name( Chan ));
	cl2chan = Channel_FirstMember( Chan );
	while( cl2chan )
	{
		cl = Channel_GetClient( cl2chan );

		if( strchr( Client_Modes( cl ), 'i' )) is_visible = false;
		else is_visible = true;

		if( is_member || is_visible )
		{
			/* Nick anhaengen */
			if( str[strlen( str ) - 1] != ':' ) strlcat( str, " ", sizeof( str ));
			if( strchr( Channel_UserModes( Chan, cl ), 'o' )) strlcat( str, "@", sizeof( str ));
			else if( strchr( Channel_UserModes( Chan, cl ), 'v' )) strlcat( str, "+", sizeof( str ));
			strlcat( str, Client_ID( cl ), sizeof( str ));

			if( strlen( str ) > ( LINE_LEN - CLIENT_NICK_LEN - 4 ))
			{
				/* Zeile wird zu lang: senden! */
				if( ! IRC_WriteStrClient( Client, "%s", str )) return DISCONNECTED;
				snprintf( str, sizeof( str ), RPL_NAMREPLY_MSG, Client_ID( Client ), "=", Channel_Name( Chan ));
			}
		}

		/* naechstes Mitglied suchen */
		cl2chan = Channel_NextMember( Chan, cl2chan );
	}
	if( str[strlen( str ) - 1] != ':')
	{
		/* Es sind noch Daten da, die gesendet werden muessen */
		if( ! IRC_WriteStrClient( Client, "%s", str )) return DISCONNECTED;
	}

	return CONNECTED;
} /* IRC_Send_NAMES */


GLOBAL bool
IRC_Send_WHO( CLIENT *Client, CHANNEL *Chan, bool OnlyOps )
{
	bool is_visible, is_member;
	CL2CHAN *cl2chan;
	char flags[8];
	CLIENT *c;

	assert( Client != NULL );
	assert( Chan != NULL );

	if( Channel_IsMemberOf( Chan, Client )) is_member = true;
	else is_member = false;

	/* Secret channel? */
	if( ! is_member && strchr( Channel_Modes( Chan ), 's' )) return CONNECTED;

	/* Alle Mitglieder suchen */
	cl2chan = Channel_FirstMember( Chan );
	while( cl2chan )
	{
		c = Channel_GetClient( cl2chan );

		if( strchr( Client_Modes( c ), 'i' )) is_visible = false;
		else is_visible = true;

		if( is_member || is_visible )
		{
			/* Flags zusammenbasteln */
			strcpy( flags, "H" );
			if( strchr( Client_Modes( c ), 'o' )) strlcat( flags, "*", sizeof( flags ));
			if( strchr( Channel_UserModes( Chan, c ), 'o' )) strlcat( flags, "@", sizeof( flags ));
			else if( strchr( Channel_UserModes( Chan, c ), 'v' )) strlcat( flags, "+", sizeof( flags ));

			/* ausgeben */
			if(( ! OnlyOps ) || ( strchr( Client_Modes( c ), 'o' )))
			{
				if( ! IRC_WriteStrClient( Client, RPL_WHOREPLY_MSG, Client_ID( Client ), Channel_Name( Chan ), Client_User( c ), Client_Hostname( c ), Client_ID( Client_Introducer( c )), Client_ID( c ), flags, Client_Hops( c ), Client_Info( c ))) return DISCONNECTED;
			}
		}

		/* naechstes Mitglied suchen */
		cl2chan = Channel_NextMember( Chan, cl2chan );
	}
	return CONNECTED;
} /* IRC_Send_WHO */


/* -eof- */
