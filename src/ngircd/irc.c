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
 * $Id: irc.c,v 1.84 2002/03/03 17:15:11 alex Exp $
 *
 * irc.c: IRC-Befehle
 *
 * $Log: irc.c,v $
 * Revision 1.84  2002/03/03 17:15:11  alex
 * - Source in weitere Module fuer IRC-Befehle aufgesplitted.
 *
 * Revision 1.83  2002/02/28 00:48:26  alex
 * - Forwarding von TOPIC an andere Server gefixed. Hoffentlich ;-)
 *
 * Revision 1.82  2002/02/27 23:26:36  alex
 * - einige Funktionen in irc-xxx-Module ausgegliedert.
 *
 * Revision 1.81  2002/02/27 20:55:44  alex
 * - Channel-Topics werden nun auch korrekt von anderen Server angenommen.
 *
 * Revision 1.80  2002/02/27 20:33:13  alex
 * - Channel-Topics implementiert.
 *
 * Revision 1.79  2002/02/27 18:57:21  alex
 * - PRIVMSG zeugt nun bei Texten an User an, wenn diese "away" sind.
 *
 * Revision 1.78  2002/02/27 18:23:45  alex
 * - IRC-Befehl "AWAY" implementert.
 *
 * Revision 1.77  2002/02/27 17:05:41  alex
 * - PRIVMSG beachtet nun die Channel-Modes "n" und "m".
 *
 * Revision 1.76  2002/02/27 16:04:14  alex
 * - Bug bei belegtem Nickname bei User-Registrierung (NICK-Befehl) behoben.
 *
 * Revision 1.75  2002/02/27 15:23:27  alex
 * - NAMES beachtet nun das "invisible" Flag ("i") von Usern.
 *
 * Revision 1.74  2002/02/27 03:44:53  alex
 * - gerade eben in SQUIT eingefuehrten Bug behoben: entfernte Server werden nun
 *   nur noch geloescht, die Verbindung, von der SQUIT kam, bleibt wieder offen.
 *
 * Revision 1.73  2002/02/27 03:08:05  alex
 * - Log-Meldungen bei SQUIT erneut ueberarbeitet ...
 *
 * Revision 1.72  2002/02/27 02:26:58  alex
 * - SQUIT wird auf jeden Fall geforwarded, zudem besseres Logging.
 *
 * Revision 1.71  2002/02/27 00:50:05  alex
 * - einige unnoetige Client_NextHop()-Aufrufe entfernt.
 * - NAMES korrigiert und komplett implementiert.
 *
 * Revision 1.70  2002/02/26 22:06:40  alex
 * - Nick-Aenderungen werden nun wieder korrekt ins Logfile geschrieben.
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ngircd.h"
#include "channel.h"
#include "client.h"
#include "conf.h"
#include "conn.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"
#include "tool.h"

#include <exp.h>
#include "irc.h"


GLOBAL BOOLEAN IRC_MOTD( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_USER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	return IRC_Show_MOTD( Client );
} /* IRC_MOTD */


GLOBAL BOOLEAN IRC_PRIVMSG( CLIENT *Client, REQUEST *Req )
{
	BOOLEAN is_member, has_voice, is_op, ok;
	CLIENT *cl, *from;
	CHANNEL *chan;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc == 0 ) return IRC_WriteStrClient( Client, ERR_NORECIPIENT_MSG, Client_ID( Client ), Req->command );
	if( Req->argc == 1 ) return IRC_WriteStrClient( Client, ERR_NOTEXTTOSEND_MSG, Client_ID( Client ));
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_GetFromID( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	cl = Client_Search( Req->argv[0] );
	if( cl )
	{
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
	if( chan )
	{
		/* Okay, Ziel ist ein Channel */
		is_member = has_voice = is_op = FALSE;
		if( Channel_IsMemberOf( chan, from ))
		{
			is_member = TRUE;
			if( strchr( Channel_UserModes( chan, from ), 'v' )) has_voice = TRUE;
			if( strchr( Channel_UserModes( chan, from ), 'o' )) is_op = TRUE;
		}
		
		/* pruefen, ob Client in Channel schreiben darf */
		ok = TRUE;
		if( strchr( Channel_Modes( chan ), 'n' ) && ( ! is_member )) ok = FALSE;
		if( strchr( Channel_Modes( chan ), 'm' ) && ( ! is_op ) && ( ! has_voice )) ok = FALSE;

		if( ! ok ) return IRC_WriteStrClient( from, ERR_CANNOTSENDTOCHAN_MSG, Client_ID( from ), Req->argv[0] );

		/* Text senden */
		if( Client_Conn( from ) > NONE ) Conn_UpdateIdle( Client_Conn( from ));
		return IRC_WriteStrChannelPrefix( Client, chan, from, TRUE, "PRIVMSG %s :%s", Req->argv[0], Req->argv[1] );
	}

	return IRC_WriteStrClient( from, ERR_NOSUCHNICK_MSG, Client_ID( from ), Req->argv[0] );
} /* IRC_PRIVMSG */


GLOBAL BOOLEAN IRC_NOTICE( CLIENT *Client, REQUEST *Req )
{
	CLIENT *to, *from;

	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return CONNECTED;

	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_GetFromID( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	to = Client_Search( Req->argv[0] );
	if( to )
	{
		/* Okay, Ziel ist ein User */
		return IRC_WriteStrClientPrefix( to, from, "NOTICE %s :%s", Client_ID( to ), Req->argv[1] );
	}
	else return CONNECTED;
} /* IRC_NOTICE */


GLOBAL BOOLEAN IRC_NAMES( CLIENT *Client, REQUEST *Req )
{
	CHAR rpl[COMMAND_LEN], *ptr;
	CLIENT *target, *from, *c;
	CHANNEL *chan;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* From aus Prefix ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_GetFromID( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->prefix );
	
	if( Req->argc == 2 )
	{
		/* an anderen Server forwarden */
		target = Client_GetFromID( Req->argv[1] );
		if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[1] );

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
	sprintf( rpl, RPL_NAMREPLY_MSG, Client_ID( from ), "*", "*" );
	while( c )
	{
		if(( Client_Type( c ) == CLIENT_USER ) && ( Channel_FirstChannelOf( c ) == NULL ) && ( ! strchr( Client_Modes( c ), 'i' )))
		{
			/* Okay, das ist ein User: anhaengen */
			if( rpl[strlen( rpl ) - 1] != ':' ) strcat( rpl, " " );
			strcat( rpl, Client_ID( c ));

			if( strlen( rpl ) > ( LINE_LEN - CLIENT_NICK_LEN - 4 ))
			{
				/* Zeile wird zu lang: senden! */
				if( ! IRC_WriteStrClient( from, rpl )) return DISCONNECTED;
				sprintf( rpl, RPL_NAMREPLY_MSG, Client_ID( from ), "*", "*" );
			}
		}

		/* naechster Client */
		c = Client_Next( c );
	}
	if( rpl[strlen( rpl ) - 1] != ':')
	{
		/* es wurden User gefunden */
		if( ! IRC_WriteStrClient( from, rpl )) return DISCONNECTED;
	}
	
	return IRC_WriteStrClient( from, RPL_ENDOFNAMES_MSG, Client_ID( from ), "*" );
} /* IRC_NAMES */


GLOBAL BOOLEAN IRC_ISON( CLIENT *Client, REQUEST *Req )
{
	CHAR rpl[COMMAND_LEN];
	CLIENT *c;
	CHAR *ptr;
	INT i;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_USER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	strcpy( rpl, RPL_ISON_MSG );
	for( i = 0; i < Req->argc; i++ )
	{
		ptr = strtok( Req->argv[i], " " );
		while( ptr )
		{
			ngt_TrimStr( ptr );
			c = Client_GetFromID( ptr );
			if( c && ( Client_Type( c ) == CLIENT_USER ))
			{
				/* Dieser Nick ist "online" */
				strcat( rpl, ptr );
				strcat( rpl, " " );
			}
			ptr = strtok( NULL, " " );
		}
	}
	if( rpl[strlen( rpl ) - 1] == ' ' ) rpl[strlen( rpl ) - 1] = '\0';

	return IRC_WriteStrClient( Client, rpl, Client_ID( Client ) );
} /* IRC_ISON */


GLOBAL BOOLEAN IRC_WHOIS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *from, *target, *c;
	CHAR str[LINE_LEN + 1], *ptr = NULL;
	CL2CHAN *cl2chan;
	CHANNEL *chan;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 1 ) || ( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Client suchen */
	c = Client_GetFromID( Req->argv[Req->argc - 1] );
	if(( ! c ) || ( Client_Type( c ) != CLIENT_USER )) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->argv[Req->argc - 1] );

	/* Empfaenger des WHOIS suchen */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_GetFromID( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );
	
	/* Forwarden an anderen Server? */
	if( Req->argc > 1 )
	{
		/* angegebenen Ziel-Server suchen */
		target = Client_GetFromID( Req->argv[1] );
		if( ! target ) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[1] );
		ptr = Req->argv[1];
	}
	else target = Client_ThisServer( );

	assert( target != NULL );
	
	if(( Client_NextHop( target ) != Client_ThisServer( )) && ( Client_Type( Client_NextHop( target )) == CLIENT_SERVER )) return IRC_WriteStrClientPrefix( target, from, "WHOIS %s :%s", Req->argv[0], ptr );
	
	/* Nick, User und Name */
	if( ! IRC_WriteStrClient( from, RPL_WHOISUSER_MSG, Client_ID( from ), Client_ID( c ), Client_User( c ), Client_Hostname( c ), Client_Info( c ))) return DISCONNECTED;

	/* Server */
	if( ! IRC_WriteStrClient( from, RPL_WHOISSERVER_MSG, Client_ID( from ), Client_ID( c ), Client_ID( Client_Introducer( c )), Client_Info( Client_Introducer( c )))) return DISCONNECTED;

	/* Channels */
	sprintf( str, RPL_WHOISCHANNELS_MSG, Client_ID( from ), Client_ID( c ));
	cl2chan = Channel_FirstChannelOf( c );
	while( cl2chan )
	{
		chan = Channel_GetChannel( cl2chan );
		assert( chan != NULL );
		
		/* Channel-Name anhaengen */
		if( str[strlen( str ) - 1] != ':' ) strcat( str, " " );
		if( strchr( Channel_UserModes( chan, c ), 'v' )) strcat( str, "+" );
		if( strchr( Channel_UserModes( chan, c ), 'o' )) strcat( str, "@" );
		strcat( str, Channel_Name( chan ));

		if( strlen( str ) > ( LINE_LEN - CHANNEL_NAME_LEN - 4 ))
		{
			/* Zeile wird zu lang: senden! */
			if( ! IRC_WriteStrClient( Client, str )) return DISCONNECTED;
			sprintf( str, RPL_WHOISCHANNELS_MSG, Client_ID( from ), Client_ID( c ));
		}

		/* naechstes Mitglied suchen */
		cl2chan = Channel_NextChannelOf( c, cl2chan );
	}
	if( str[strlen( str ) - 1] != ':')
	{
		/* Es sind noch Daten da, die gesendet werden muessen */
		if( ! IRC_WriteStrClient( Client, str )) return DISCONNECTED;
	}
	
	/* IRC-Operator? */
	if( Client_HasMode( c, 'o' ))
	{
		if( ! IRC_WriteStrClient( from, RPL_WHOISOPERATOR_MSG, Client_ID( from ), Client_ID( c ))) return DISCONNECTED;
	}

	/* Idle (nur lokale Clients) */
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


GLOBAL BOOLEAN IRC_WHO( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_USER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );
	
	return CONNECTED;
} /* IRC_WHO */


GLOBAL BOOLEAN IRC_USERHOST( CLIENT *Client, REQUEST *Req )
{
	CHAR rpl[COMMAND_LEN];
	CLIENT *c;
	INT max, i;

	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_USER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Req->argc > 5 ) max = 5;
	else max = Req->argc;
	
	strcpy( rpl, RPL_USERHOST_MSG );
	for( i = 0; i < max; i++ )
	{
		c = Client_GetFromID( Req->argv[i] );
		if( c && ( Client_Type( c ) == CLIENT_USER ))
		{
			/* Dieser Nick ist "online" */
			strcat( rpl, Client_ID( c ));
			if( Client_HasMode( c, 'o' )) strcat( rpl, "*" );
			strcat( rpl, "=" );
			if( Client_HasMode( c, 'a' )) strcat( rpl, "-" );
			else strcat( rpl, "+" );
			strcat( rpl, Client_User( c ));
			strcat( rpl, "@" );
			strcat( rpl, Client_Hostname( c ));
			strcat( rpl, " " );
		}
	}
	if( rpl[strlen( rpl ) - 1] == ' ' ) rpl[strlen( rpl ) - 1] = '\0';

	return IRC_WriteStrClient( Client, rpl, Client_ID( Client ) );
} /* IRC_USERHOST */


GLOBAL BOOLEAN IRC_ERROR( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Req->argc < 1 ) Log( LOG_NOTICE, "Got ERROR from \"%s\"!", Client_Mask( Client ));
	else Log( LOG_NOTICE, "Got ERROR from \"%s\": %s!", Client_Mask( Client ), Req->argv[0] );

	return CONNECTED;
} /* IRC_ERROR */


GLOBAL BOOLEAN IRC_LUSERS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *from;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Absender ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_GetFromID( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* An anderen Server forwarden? */
	if( Req->argc == 2 )
	{
		target = Client_GetFromID( Req->argv[1] );
		if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[1] );
		else if( target != Client_ThisServer( )) return IRC_WriteStrClientPrefix( target, from, "LUSERS %s %s", Req->argv[0], Req->argv[1] );
	}

	/* Wer ist der Absender? */
	if( Client_Type( Client ) == CLIENT_SERVER ) target = Client_GetFromID( Req->prefix );
	else target = Client;
	if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );
	
	IRC_Send_LUSERS( target );

	return CONNECTED;
} /* IRC_LUSERS */


GLOBAL BOOLEAN IRC_LINKS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *from, *c;
	CHAR *mask;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Server-Mask ermitteln */
	if( Req->argc > 0 ) mask = Req->argv[Req->argc - 1];
	else mask = "*";

	/* Absender ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_GetFromID( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );
	
	/* An anderen Server forwarden? */
	if( Req->argc == 2 )
	{
		target = Client_GetFromID( Req->argv[0] );
		if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[0] );
		else if( target != Client_ThisServer( )) return IRC_WriteStrClientPrefix( target, from, "LINKS %s %s", Req->argv[0], Req->argv[1] );
	}

	/* Wer ist der Absender? */
	if( Client_Type( Client ) == CLIENT_SERVER ) target = Client_GetFromID( Req->prefix );
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
	
	return IRC_WriteStrClient( target, RPL_ENDOFLINKS_MSG, Client_ID( target ), mask );
} /* IRC_LINKS */


GLOBAL BOOLEAN IRC_VERSION( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *prefix;
	
	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Ziel suchen */
	if( Req->argc == 1 ) target = Client_GetFromID( Req->argv[0] );
	else target = Client_ThisServer( );

	/* Prefix ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) prefix = Client_GetFromID( Req->prefix );
	else prefix = Client;
	if( ! prefix ) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->prefix );
	
	/* An anderen Server weiterleiten? */
	if( target != Client_ThisServer( ))
	{
		if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[0] );

		/* forwarden */
		IRC_WriteStrClientPrefix( target, prefix, "VERSION %s", Req->argv[0] );
		return CONNECTED;
	}

	/* mit Versionsinfo antworten */
	return IRC_WriteStrClient( Client, RPL_VERSION_MSG, Client_ID( prefix ), NGIRCd_DebugLevel, Conf_ServerName, NGIRCd_VersionAddition( ));
} /* IRC_VERSION */


GLOBAL BOOLEAN IRC_KILL( CLIENT *Client, REQUEST *Req )
{
	CLIENT *prefix, *c;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_SERVER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc != 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	prefix = Client_GetFromID( Req->prefix );
	if( ! prefix )
	{
		Log( LOG_WARNING, "Got KILL with invalid prefix: \"%s\"!", Req->prefix );
		prefix = Client_ThisServer( );
	}
	
	Log( LOG_NOTICE, "Got KILL command from \"%s\" for \"%s\": %s", Client_Mask( prefix ), Req->argv[0], Req->argv[1] );
	
	/* andere Server benachrichtigen */
	IRC_WriteStrServersPrefix( Client, prefix, "KILL %s :%s", Req->argv[0], Req->argv[1] );

	/* haben wir selber einen solchen Client? */
	c = Client_GetFromID( Req->argv[0] );
	if( c && ( Client_Conn( c ) != NONE )) Conn_Close( Client_Conn( c ), NULL, Req->argv[1], TRUE );
	
	return CONNECTED;
} /* IRC_KILL */


GLOBAL BOOLEAN IRC_Show_MOTD( CLIENT *Client )
{
	BOOLEAN ok;
	CHAR line[127];
	FILE *fd;
	
	assert( Client != NULL );

	fd = fopen( Conf_MotdFile, "r" );
	if( ! fd )
	{
		Log( LOG_WARNING, "Can't read MOTD file \"%s\": %s", Conf_MotdFile, strerror( errno ));
		return IRC_WriteStrClient( Client, ERR_NOMOTD_MSG, Client_ID( Client ) );
	}
	
	IRC_WriteStrClient( Client, RPL_MOTDSTART_MSG, Client_ID( Client ), Client_ID( Client_ThisServer( )));
	while( TRUE )
	{
		if( ! fgets( line, 126, fd )) break;
		if( line[strlen( line ) - 1] == '\n' ) line[strlen( line ) - 1] = '\0';
		if( ! IRC_WriteStrClient( Client, RPL_MOTD_MSG, Client_ID( Client ), line ))
		{
			fclose( fd );
			return FALSE;
		}
	}
	ok = IRC_WriteStrClient( Client, RPL_ENDOFMOTD_MSG, Client_ID( Client ) );

	fclose( fd );
	
	return ok;
} /* IRC_Show_MOTD */


GLOBAL BOOLEAN IRC_Send_NAMES( CLIENT *Client, CHANNEL *Chan )
{
	BOOLEAN is_visible, is_member;
	CHAR str[LINE_LEN + 1];
	CL2CHAN *cl2chan;
	CLIENT *cl;
	
	assert( Client != NULL );
	assert( Chan != NULL );

	if( Channel_IsMemberOf( Chan, Client )) is_member = TRUE;
	else is_member = FALSE;
			 
	/* Alle Mitglieder suchen */
	sprintf( str, RPL_NAMREPLY_MSG, Client_ID( Client ), "=", Channel_Name( Chan ));
	cl2chan = Channel_FirstMember( Chan );
	while( cl2chan )
	{
		cl = Channel_GetClient( cl2chan );

		if( strchr( Client_Modes( cl ), 'i' )) is_visible = FALSE;
		else is_visible = TRUE;

		if( is_member || is_visible )
		{
			/* Nick anhaengen */
			if( str[strlen( str ) - 1] != ':' ) strcat( str, " " );
			if( strchr( Channel_UserModes( Chan, cl ), 'v' )) strcat( str, "+" );
			if( strchr( Channel_UserModes( Chan, cl ), 'o' )) strcat( str, "@" );
			strcat( str, Client_ID( cl ));
	
			if( strlen( str ) > ( LINE_LEN - CLIENT_NICK_LEN - 4 ))
			{
				/* Zeile wird zu lang: senden! */
				if( ! IRC_WriteStrClient( Client, str )) return DISCONNECTED;
				sprintf( str, RPL_NAMREPLY_MSG, Client_ID( Client ), "=", Channel_Name( Chan ));
			}
		}

		/* naechstes Mitglied suchen */
		cl2chan = Channel_NextMember( Chan, cl2chan );
	}
	if( str[strlen( str ) - 1] != ':')
	{
		/* Es sind noch Daten da, die gesendet werden muessen */
		if( ! IRC_WriteStrClient( Client, str )) return DISCONNECTED;
	}

	return CONNECTED;
} /* IRC_Send_NAMES */


GLOBAL BOOLEAN IRC_Send_LUSERS( CLIENT *Client )
{
	INT cnt;

	assert( Client != NULL );

	/* Users, Services und Serevr im Netz */
	if( ! IRC_WriteStrClient( Client, RPL_LUSERCLIENT_MSG, Client_ID( Client ), Client_UserCount( ), Client_ServiceCount( ), Client_ServerCount( ))) return DISCONNECTED;

	/* IRC-Operatoren im Netz */
	cnt = Client_OperCount( );
	if( cnt > 0 )
	{
		if( ! IRC_WriteStrClient( Client, RPL_LUSEROP_MSG, Client_ID( Client ), cnt )) return DISCONNECTED;
	}

	/* Unbekannt Verbindungen */
	cnt = Client_UnknownCount( );
	if( cnt > 0 )
	{
		if( ! IRC_WriteStrClient( Client, RPL_LUSERUNKNOWN_MSG, Client_ID( Client ), cnt )) return DISCONNECTED;
	}

	/* Channels im Netz */
	if( ! IRC_WriteStrClient( Client, RPL_LUSERCHANNELS_MSG, Client_ID( Client ), Channel_Count( ))) return DISCONNECTED;

	/* Channels im Netz */
	if( ! IRC_WriteStrClient( Client, RPL_LUSERME_MSG, Client_ID( Client ), Client_MyUserCount( ), Client_MyServiceCount( ), Client_MyServerCount( ))) return DISCONNECTED;
	
	return CONNECTED;
} /* IRC_Send_LUSERS */


/* -eof- */
