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
 * $Id: irc-server.c,v 1.23 2002/11/30 15:04:57 alex Exp $
 *
 * irc-server.c: IRC-Befehle fuer Server-Links
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resolve.h"
#include "conf.h"
#include "conn.h"
#include "client.h"
#include "channel.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"
#include "parse.h"
#include "ngircd.h"

#include "exp.h"
#include "irc-server.h"


GLOBAL BOOLEAN
IRC_SERVER( CLIENT *Client, REQUEST *Req )
{
	CHAR str[LINE_LEN], *ptr;
	CLIENT *from, *c, *cl;
	CL2CHAN *cl2chan;
	INT max_hops, i;
	CHANNEL *chan;
	BOOLEAN ok;
	CONN_ID con;
	
	assert( Client != NULL );
	assert( Req != NULL );

	/* Fehler liefern, wenn kein lokaler Client */
	if( Client_Conn( Client ) <= NONE ) return IRC_WriteStrClient( Client, ERR_UNKNOWNCOMMAND_MSG, Client_ID( Client ), Req->command );

	if( Client_Type( Client ) == CLIENT_GOTPASSSERVER )
	{
		/* Verbindung soll als Server-Server-Verbindung registriert werden */
		Log( LOG_DEBUG, "Connection %d: got SERVER command (new server link) ...", Client_Conn( Client ));

		/* Falsche Anzahl Parameter? */
		if(( Req->argc != 2 ) && ( Req->argc != 3 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		/* Ist dieser Server bei uns konfiguriert? */
		for( i = 0; i < Conf_Server_Count; i++ ) if( strcasecmp( Req->argv[0], Conf_Server[i].name ) == 0 ) break;
		if( i >= Conf_Server_Count )
		{
			/* Server ist nicht konfiguriert! */
			Log( LOG_ERR, "Connection %d: Server \"%s\" not configured here!", Client_Conn( Client ), Req->argv[0] );
			Conn_Close( Client_Conn( Client ), NULL, "Server not configured here", TRUE );
			return DISCONNECTED;
		}
		if( strcmp( Client_Password( Client ), Conf_Server[i].pwd_in ) != 0 )
		{
			/* Falsches Passwort */
			Log( LOG_ERR, "Connection %d: Got bad password from server \"%s\"!", Client_Conn( Client ), Req->argv[0] );
			Conn_Close( Client_Conn( Client ), NULL, "Bad password", TRUE );
			return DISCONNECTED;
		}
		
		/* Ist ein Server mit dieser ID bereits registriert? */
		if( ! Client_CheckID( Client, Req->argv[0] )) return DISCONNECTED;

		/* Server-Strukturen fuellen ;-) */
		Client_SetID( Client, Req->argv[0] );
		Client_SetHops( Client, 1 );
		Client_SetInfo( Client, Req->argv[Req->argc - 1] );

		/* Meldet sich der Server bei uns an (d.h., bauen nicht wir
		 * selber die Verbindung zu einem anderen Server auf)? */
		con = Client_Conn( Client );
		if( Client_Token( Client ) != TOKEN_OUTBOUND )
		{
			/* Eingehende Verbindung: Unseren SERVER- und PASS-Befehl senden */
			ok = TRUE;
			if( ! IRC_WriteStrClient( Client, "PASS %s %s", Conf_Server[i].pwd_out, NGIRCd_ProtoID )) ok = FALSE;
			else ok = IRC_WriteStrClient( Client, "SERVER %s 1 :%s", Conf_ServerName, Conf_ServerInfo );
			if( ! ok )
			{
				Conn_Close( con, "Unexpected server behavior!", NULL, FALSE );
				return DISCONNECTED;
			}
			Client_SetIntroducer( Client, Client );
			Client_SetToken( Client, 1 );
		}
		else
		{
			/* Ausgehende verbindung, SERVER und PASS wurden von uns bereits
			 * an die Gegenseite uerbermittelt */
			Client_SetToken( Client, atoi( Req->argv[1] ));
		}

		Log( LOG_NOTICE|LOG_snotice, "Server \"%s\" registered (connection %d, 1 hop - direct link).", Client_ID( Client ), con );

		Client_SetType( Client, CLIENT_SERVER );
		Conn_SetServer( con, i );

#ifdef USE_ZLIB
		/* Kompression initialisieren, wenn erforderlich */
		if( strchr( Client_Flags( Client ), 'Z' ))
		{
			if( ! Conn_InitZip( con ))
			{
				/* Fehler! */
				Conn_Close( con, "Can't inizialize compression (zlib)!", NULL, FALSE );
				return DISCONNECTED;
			}
		}
#endif

		/* maximalen Hop Count ermitteln */
		max_hops = 0;
		c = Client_First( );
		while( c )
		{
			if( Client_Hops( c ) > max_hops ) max_hops = Client_Hops( c );
			c = Client_Next( c );
		}
		
		/* Alle bisherigen Server dem neuen Server bekannt machen,
		 * die bisherigen Server ueber den neuen informierenn */
		for( i = 0; i < ( max_hops + 1 ); i++ )
		{
			c = Client_First( );
			while( c )
			{
				if(( Client_Type( c ) == CLIENT_SERVER ) && ( c != Client ) && ( c != Client_ThisServer( )) && ( Client_Hops( c ) == i ))
				{
					if( Client_Conn( c ) > NONE )
					{
						/* Dem gefundenen Server gleich den neuen
						 * Server bekannt machen */
						if( ! IRC_WriteStrClient( c, "SERVER %s %d %d :%s", Client_ID( Client ), Client_Hops( Client ) + 1, Client_MyToken( Client ), Client_Info( Client ))) return DISCONNECTED;
					}
					
					/* Den neuen Server ueber den alten informieren */
					if( ! IRC_WriteStrClientPrefix( Client, Client_Hops( c ) == 1 ? Client_ThisServer( ) : Client_Introducer( c ), "SERVER %s %d %d :%s", Client_ID( c ), Client_Hops( c ) + 1, Client_MyToken( c ), Client_Info( c ))) return DISCONNECTED;
				}
				c = Client_Next( c );
			}
		}

		/* alle User dem neuen Server bekannt machen */
		c = Client_First( );
		while( c )
		{
			if( Client_Type( c ) == CLIENT_USER )
			{
				/* User an neuen Server melden */
				if( ! IRC_WriteStrClient( Client, "NICK %s %d %s %s %d +%s :%s", Client_ID( c ), Client_Hops( c ) + 1, Client_User( c ), Client_Hostname( c ), Client_MyToken( Client_Introducer( c )), Client_Modes( c ), Client_Info( c ))) return DISCONNECTED;
			}
			c = Client_Next( c );
		}

		/* Channels dem neuen Server bekannt machen */
		chan = Channel_First( );
		while( chan )
		{
#ifdef IRCPLUS
			/* Wenn unterstuetzt, CHANINFO senden */
			if( strchr( Client_Flags( Client ), 'C' ))
			{
				/* CHANINFO senden */
				if( ! IRC_WriteStrClient( Client, "CHANINFO %s +%s :%s", Channel_Name( chan ), Channel_Modes( chan ), Channel_Topic( chan ))) return DISCONNECTED;
			}
#endif

			/* alle Member suchen */
			cl2chan = Channel_FirstMember( chan );
			sprintf( str, "NJOIN %s :", Channel_Name( chan ));
			while( cl2chan )
			{
				cl = Channel_GetClient( cl2chan );
				assert( cl != NULL );

				/* Nick, ggf. mit Modes, anhaengen */
				if( str[strlen( str ) - 1] != ':' ) strcat( str, "," );
				if( strchr( Channel_UserModes( chan, cl ), 'v' )) strcat( str, "+" );
				if( strchr( Channel_UserModes( chan, cl ), 'o' )) strcat( str, "@" );
				strcat( str, Client_ID( cl ));

				if( strlen( str ) > ( LINE_LEN - CLIENT_NICK_LEN - 8 ))
				{
					/* Zeile senden */
					if( ! IRC_WriteStrClient( Client, "%s", str )) return DISCONNECTED;
					sprintf( str, "NJOIN %s :", Channel_Name( chan ));
				}
				
				cl2chan = Channel_NextMember( chan, cl2chan );
			}

			/* noch Daten da? */
			if( str[strlen( str ) - 1] != ':')
			{
				/* Ja; Also senden ... */
				if( ! IRC_WriteStrClient( Client, "%s", str )) return DISCONNECTED;
			}

			/* naechsten Channel suchen */
			chan = Channel_Next( chan );
		}
		
		return CONNECTED;
	}
	else if( Client_Type( Client ) == CLIENT_SERVER )
	{
		/* Neuer Server wird im Netz angekuendigt */

		/* Falsche Anzahl Parameter? */
		if( Req->argc != 4 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		/* Ist ein Server mit dieser ID bereits registriert? */
		if( ! Client_CheckID( Client, Req->argv[0] )) return DISCONNECTED;

		/* Ueberfluessige Hostnamen aus Info-Text entfernen */
		ptr = strchr( Req->argv[3] + 2, '[' );
		if( ! ptr ) ptr = Req->argv[3];

		from = Client_Search( Req->prefix );
		if( ! from )
		{
			/* Hm, Server, der diesen einfuehrt, ist nicht bekannt!? */
			Log( LOG_ALERT, "Unknown ID in prefix of SERVER: \"%s\"! (on connection %d)", Req->prefix, Client_Conn( Client ));
			Conn_Close( Client_Conn( Client ), NULL, "Unknown ID in prefix of SERVER", TRUE );
			return DISCONNECTED;
		}

		/* Neue Client-Struktur anlegen */
		c = Client_NewRemoteServer( Client, Req->argv[0], from, atoi( Req->argv[1] ), atoi( Req->argv[2] ), ptr, TRUE );
		if( ! c )
		{
			/* Neue Client-Struktur konnte nicht angelegt werden */
			Log( LOG_ALERT, "Can't create client structure for server! (on connection %d)", Client_Conn( Client ));
			Conn_Close( Client_Conn( Client ), NULL, "Can't allocate client structure for remote server", TRUE );
			return DISCONNECTED;
		}

		/* Log-Meldung zusammenbauen und ausgeben */
		if(( Client_Hops( c ) > 1 ) && ( Req->prefix[0] )) sprintf( str, "connected to %s, ", Client_ID( from ));
		else strcpy( str, "" );
		Log( LOG_NOTICE|LOG_snotice, "Server \"%s\" registered (via %s, %s%d hop%s).", Client_ID( c ), Client_ID( Client ), str, Client_Hops( c ), Client_Hops( c ) > 1 ? "s": "" );

		/* Andere Server informieren */
		IRC_WriteStrServersPrefix( Client, from, "SERVER %s %d %d :%s", Client_ID( c ), Client_Hops( c ) + 1, Client_MyToken( c ), Client_Info( c ));

		return CONNECTED;
	}
	else return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );
} /* IRC_SERVER */


GLOBAL BOOLEAN
IRC_NJOIN( CLIENT *Client, REQUEST *Req )
{
	CHAR str[COMMAND_LEN], *channame, *ptr, modes[8];
	BOOLEAN is_op, is_voiced;
	CHANNEL *chan;
	CLIENT *c;
	
	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	strncpy( str, Req->argv[1], COMMAND_LEN - 1 );
	str[COMMAND_LEN - 1] = '\0';

	channame = Req->argv[0];
	ptr = strtok( str, "," );
	while( ptr )
	{
		is_op = is_voiced = FALSE;
		
		/* Prefixe abschneiden */
		while(( *ptr == '@' ) || ( *ptr == '+' ))
		{
			if( *ptr == '@' ) is_op = TRUE;
			if( *ptr == '+' ) is_voiced = TRUE;
			ptr++;
		}

		c = Client_Search( ptr );
		if( c )
		{
			Channel_Join( c, channame );
			chan = Channel_Search( channame );
			assert( chan != NULL );
			
			if( is_op ) Channel_UserModeAdd( chan, c, 'o' );
			if( is_voiced ) Channel_UserModeAdd( chan, c, 'v' );

			/* im Channel bekannt machen */
			IRC_WriteStrChannelPrefix( Client, chan, c, FALSE, "JOIN :%s", channame );

			/* Channel-User-Modes setzen */
			strcpy( modes, Channel_UserModes( chan, c ));
			if( modes[0] )
			{
				/* Modes im Channel bekannt machen */
				IRC_WriteStrChannelPrefix( Client, chan, Client, FALSE, "MODE %s +%s %s", channame, modes, Client_ID( c ));
			}
		}
		else Log( LOG_ERR, "Got NJOIN for unknown nick \"%s\" for channel \"%s\"!", ptr, channame );
		
		/* naechsten Nick suchen */
		ptr = strtok( NULL, "," );
	}

	/* an andere Server weiterleiten */
	IRC_WriteStrServersPrefix( Client, Client_ThisServer( ), "NJOIN %s :%s", Req->argv[0], Req->argv[1] );

	return CONNECTED;
} /* IRC_NJOIN */


GLOBAL BOOLEAN
IRC_SQUIT( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target;
	CHAR msg[LINE_LEN + 64];

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	Log( LOG_DEBUG, "Got SQUIT from %s for \"%s\": \"%s\" ...", Client_ID( Client ), Req->argv[0], Req->argv[1] );

	target = Client_Search( Req->argv[0] );
	if( ! target )
	{
		/* Den Server kennen wir nicht (mehr), also nichts zu tun. */
		Log( LOG_WARNING, "Got SQUIT from %s for unknown server \"%s\"!?", Client_ID( Client ), Req->argv[0] );
		return CONNECTED;
	}

	if( Req->argv[1][0] )
	{
		if( strlen( Req->argv[1] ) > LINE_LEN ) Req->argv[1][LINE_LEN] = '\0';
		sprintf( msg, "%s (SQUIT from %s).", Req->argv[1], Client_ID( Client ));
	}
	else sprintf( msg, "Got SQUIT from %s.", Client_ID( Client ));

	if( Client_Conn( target ) > NONE )
	{
		/* dieser Server hat die Connection */
		if( Req->argv[1][0] ) Conn_Close( Client_Conn( target ), msg, Req->argv[1], TRUE );
		else Conn_Close( Client_Conn( target ), msg, NULL, TRUE );
		return DISCONNECTED;
	}
	else
	{
		/* Verbindung hielt anderer Server */
		Client_Destroy( target, msg, Req->argv[1], FALSE );
		return CONNECTED;
	}
} /* IRC_SQUIT */


/* -eof- */
