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
 * $Id: irc-login.c,v 1.1 2002/02/27 23:26:21 alex Exp $
 *
 * irc-login.c: Anmeldung und Abmeldung im IRC
 *
 * $Log: irc-login.c,v $
 * Revision 1.1  2002/02/27 23:26:21  alex
 * - Modul aus irc.c bzw. irc.h ausgegliedert.
 *
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ngircd.h"
#include "conf.h"
#include "irc.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"

#include <exp.h>
#include "irc-login.h"


LOCAL BOOLEAN Hello_User( CLIENT *Client );
LOCAL VOID Kill_Nick( CHAR *Nick, CHAR *Reason );


GLOBAL BOOLEAN IRC_PASS( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	/* Fehler liefern, wenn kein lokaler Client */
	if( Client_Conn( Client ) <= NONE ) return IRC_WriteStrClient( Client, ERR_UNKNOWNCOMMAND_MSG, Client_ID( Client ), Req->command );
	
	if(( Client_Type( Client ) == CLIENT_UNKNOWN ) && ( Req->argc == 1))
	{
		/* noch nicht registrierte unbekannte Verbindung */
		Log( LOG_DEBUG, "Connection %d: got PASS command ...", Client_Conn( Client ));

		/* Passwort speichern */
		Client_SetPassword( Client, Req->argv[0] );

		Client_SetType( Client, CLIENT_GOTPASS );
		return CONNECTED;
	}
	else if((( Client_Type( Client ) == CLIENT_UNKNOWN ) || ( Client_Type( Client ) == CLIENT_UNKNOWNSERVER )) && (( Req->argc == 3 ) || ( Req->argc == 4 )))
	{
		/* noch nicht registrierte Server-Verbindung */
		Log( LOG_DEBUG, "Connection %d: got PASS command (new server link) ...", Client_Conn( Client ));

		/* Passwort speichern */
		Client_SetPassword( Client, Req->argv[0] );

		Client_SetType( Client, CLIENT_GOTPASSSERVER );
		return CONNECTED;
	}
	else if(( Client_Type( Client ) == CLIENT_UNKNOWN  ) || ( Client_Type( Client ) == CLIENT_UNKNOWNSERVER ))
	{
		/* Falsche Anzahl Parameter? */
		return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );
	}
	else return IRC_WriteStrClient( Client, ERR_ALREADYREGISTRED_MSG, Client_ID( Client ));
} /* IRC_PASS */


GLOBAL BOOLEAN IRC_SERVER( CLIENT *Client, REQUEST *Req )
{
	CHAR str[LINE_LEN], *ptr;
	CLIENT *from, *c, *cl;
	CL2CHAN *cl2chan;
	INT max_hops, i;
	CHANNEL *chan;
	BOOLEAN ok;
	
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
		if( strcmp( Client_Password( Client ), Conf_Server[i].pwd ) != 0 )
		{
			/* Falsches Passwort */
			Log( LOG_ERR, "Connection %d: Bad password for server \"%s\"!", Client_Conn( Client ), Req->argv[0] );
			Conn_Close( Client_Conn( Client ), NULL, "Bad password", TRUE );
			return DISCONNECTED;
		}
		
		/* Ist ein Server mit dieser ID bereits registriert? */
		if( ! Client_CheckID( Client, Req->argv[0] )) return DISCONNECTED;

		/* Server-Strukturen fuellen ;-) */
		Client_SetID( Client, Req->argv[0] );
		Client_SetHops( Client, 1 );
		Client_SetInfo( Client, Req->argv[Req->argc - 1] );
		
		/* Meldet sich der Server bei uns an? */
		if( Req->argc == 2 )
		{
			/* Unseren SERVER- und PASS-Befehl senden */
			ok = TRUE;
			if( ! IRC_WriteStrClient( Client, "PASS %s "PASSSERVERADD, Conf_Server[i].pwd )) ok = FALSE;
			else ok = IRC_WriteStrClient( Client, "SERVER %s 1 :%s", Conf_ServerName, Conf_ServerInfo );
			if( ! ok )
			{
				Conn_Close( Client_Conn( Client ), "Unexpected server behavior!", NULL, FALSE );
				return DISCONNECTED;
			}
			Client_SetIntroducer( Client, Client );
			Client_SetToken( Client, 1 );
		}
		else  Client_SetToken( Client, atoi( Req->argv[1] ));

		Log( LOG_NOTICE, "Server \"%s\" registered (connection %d, 1 hop - direct link).", Client_ID( Client ), Client_Conn( Client ));

		Client_SetType( Client, CLIENT_SERVER );

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
			sprintf( str, "NJOIN %s :", Channel_Name( chan ));

			/* alle Member suchen */
			cl2chan = Channel_FirstMember( chan );
			while( cl2chan )
			{
				cl = Channel_GetClient( cl2chan );
				assert( cl != NULL );

				/* Nick, ggf. mit Modes, anhaengen */
				if( str[strlen( str ) - 1] != ':' ) strcat( str, "," );
				if( strchr( Channel_UserModes( chan, cl ), 'v' )) strcat( str, "+" );
				if( strchr( Channel_UserModes( chan, cl ), 'o' )) strcat( str, "@" );
				strcat( str, Client_ID( cl ));

				if( strlen( str ) > ( LINE_LEN - CLIENT_NICK_LEN - 4 ))
				{
					/* Zeile senden */
					if( ! IRC_WriteStrClient( Client, str )) return DISCONNECTED;
					sprintf( str, "NJOIN %s :", Channel_Name( chan ));
				}
				
				cl2chan = Channel_NextMember( chan, cl2chan );
			}

			/* noch Daten da? */
			if( str[strlen( str ) - 1] != ':')
			{
				/* Ja; Also senden ... */
				if( ! IRC_WriteStrClient( Client, str )) return DISCONNECTED;
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

		from = Client_GetFromID( Req->prefix );
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
		Log( LOG_NOTICE, "Server \"%s\" registered (via %s, %s%d hop%s).", Client_ID( c ), Client_ID( Client ), str, Client_Hops( c ), Client_Hops( c ) > 1 ? "s": "" );

		/* Andere Server informieren */
		IRC_WriteStrServersPrefix( Client, from, "SERVER %s %d %d :%s", Client_ID( c ), Client_Hops( c ) + 1, Client_MyToken( c ), Client_Info( c ));

		return CONNECTED;
	}
	else return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );
} /* IRC_SERVER */


GLOBAL BOOLEAN IRC_NJOIN( CLIENT *Client, REQUEST *Req )
{
	CHAR *channame, *ptr, modes[8];
	BOOLEAN is_op, is_voiced;
	CHANNEL *chan;
	CLIENT *c;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_SERVER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTEREDSERVER_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	channame = Req->argv[0];
	ptr = strtok( Req->argv[1], "," );
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

		c = Client_GetFromID( ptr );
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


GLOBAL BOOLEAN IRC_NICK( CLIENT *Client, REQUEST *Req )
{
	CLIENT *intr_c, *target, *c;
	CHAR *modes;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Zumindest BitchX sendet NICK-USER in der falschen Reihenfolge. */
#ifndef STRICT_RFC
	if( Client_Type( Client ) == CLIENT_UNKNOWN || Client_Type( Client ) == CLIENT_GOTPASS || Client_Type( Client ) == CLIENT_GOTNICK || Client_Type( Client ) == CLIENT_GOTUSER || Client_Type( Client ) == CLIENT_USER || ( Client_Type( Client ) == CLIENT_SERVER && Req->argc == 1 ))
#else
	if( Client_Type( Client ) == CLIENT_UNKNOWN || Client_Type( Client ) == CLIENT_GOTPASS || Client_Type( Client ) == CLIENT_GOTNICK || Client_Type( Client ) == CLIENT_USER || ( Client_Type( Client ) == CLIENT_SERVER && Req->argc == 1 ))
#endif
	{
		/* User-Registrierung bzw. Nick-Aenderung */

		/* Falsche Anzahl Parameter? */
		if( Req->argc != 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		/* "Ziel-Client" ermitteln */
		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			target = Client_GetFromID( Req->prefix );
			if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->argv[0] );
		}
		else
		{
			/* Ist der Client "restricted"? */
			if( Client_HasMode( Client, 'r' )) return IRC_WriteStrClient( Client, ERR_RESTRICTED_MSG, Client_ID( Client ));
			target = Client;
		}

#ifndef STRICT_RFC
		/* Wenn der Client zu seinem eigenen Nick wechseln will, so machen
		 * wir nichts. So macht es das Original und mind. Snak hat probleme,
		 * wenn wir es nicht so machen. Ob es so okay ist? Hm ... */
		if( strcmp( Client_ID( target ), Req->argv[0] ) == 0 ) return CONNECTED;
#endif
		
		/* pruefen, ob Nick bereits vergeben. Speziallfall: der Client
		 * will nur die Gross- und Kleinschreibung aendern. Das darf
		 * er natuerlich machen :-) */
		if( strcasecmp( Client_ID( target ), Req->argv[0] ) != 0 )
		{
			if( ! Client_CheckNick( target, Req->argv[0] )) return CONNECTED;
		}

		/* Nick-Aenderung: allen mitteilen! */
		
		if( Client_Type( Client ) == CLIENT_USER ) IRC_WriteStrClientPrefix( Client, Client, "NICK :%s", Req->argv[0] );
		IRC_WriteStrServersPrefix( Client, target, "NICK :%s", Req->argv[0] );
		IRC_WriteStrRelatedPrefix( target, target, FALSE, "NICK :%s", Req->argv[0] );

		if(( Client_Type( target ) != CLIENT_USER ) && ( Client_Type( target ) != CLIENT_SERVER ))
		{
			/* Neuer Client */
			Log( LOG_DEBUG, "Connection %d: got valid NICK command ...", Client_Conn( Client ));

			/* Client-Nick registrieren */
			Client_SetID( target, Req->argv[0] );

			/* schon ein USER da? Dann registrieren! */
			if( Client_Type( Client ) == CLIENT_GOTUSER ) return Hello_User( Client );
			else Client_SetType( Client, CLIENT_GOTNICK );
		}
		else
		{
			/* Nick-Aenderung */
			Log( LOG_INFO, "User \"%s\" changed nick: \"%s\" -> \"%s\".", Client_Mask( target ), Client_ID( target ), Req->argv[0] );

			/* neuen Client-Nick speichern */
			Client_SetID( target, Req->argv[0] );
		}

		return CONNECTED;
	}
	else if( Client_Type( Client ) == CLIENT_SERVER )
	{
		/* Server fuehrt neuen Client ein */

		/* Falsche Anzahl Parameter? */
		if( Req->argc != 7 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		/* Nick ueberpruefen */
		c = Client_GetFromID( Req->argv[0] );
		if( c )
		{
			/* Der neue Nick ist auf diesem Server bereits registriert:
			 * sowohl der neue, als auch der alte Client muessen nun
			 * disconnectiert werden. */
			Log( LOG_ERR, "Server %s introduces already registered nick \"%s\"!", Client_ID( Client ), Req->argv[0] );
			Kill_Nick( Req->argv[0], "Nick collision" );
			return CONNECTED;
		}

		/* Server, zu dem der Client connectiert ist, suchen */
		intr_c = Client_GetFromToken( Client, atoi( Req->argv[4] ));
		if( ! intr_c )
		{
			Log( LOG_ERR, "Server %s introduces nick \"%s\" on unknown server!?", Client_ID( Client ), Req->argv[0] );
			Kill_Nick( Req->argv[0], "Unknown server" );
			return CONNECTED;
		}

		/* Neue Client-Struktur anlegen */
		c = Client_NewRemoteUser( intr_c, Req->argv[0], atoi( Req->argv[1] ), Req->argv[2], Req->argv[3], atoi( Req->argv[4] ), Req->argv[5] + 1, Req->argv[6], TRUE );
		if( ! c )
		{
			/* Eine neue Client-Struktur konnte nicht angelegt werden.
			 * Der Client muss disconnectiert werden, damit der Netz-
			 * status konsistent bleibt. */
			Log( LOG_ALERT, "Can't create client structure! (on connection %d)", Client_Conn( Client ));
			Kill_Nick( Req->argv[0], "Server error" );
			return CONNECTED;
		}

		modes = Client_Modes( c );
		if( *modes ) Log( LOG_DEBUG, "User \"%s\" (+%s) registered (via %s, on %s, %d hop%s).", Client_Mask( c ), modes, Client_ID( Client ), Client_ID( intr_c ), Client_Hops( c ), Client_Hops( c ) > 1 ? "s": "" );
		else Log( LOG_DEBUG, "User \"%s\" registered (via %s, on %s, %d hop%s).", Client_Mask( c ), Client_ID( Client ), Client_ID( intr_c ), Client_Hops( c ), Client_Hops( c ) > 1 ? "s": "" );

		/* Andere Server, ausser dem Introducer, informieren */
		IRC_WriteStrServersPrefix( Client, Client, "NICK %s %d %s %s %d %s :%s", Req->argv[0], atoi( Req->argv[1] ) + 1, Req->argv[2], Req->argv[3], Client_MyToken( intr_c ), Req->argv[5], Req->argv[6] );

		return CONNECTED;
	}
	else return IRC_WriteStrClient( Client, ERR_ALREADYREGISTRED_MSG, Client_ID( Client ));
} /* IRC_NICK */


GLOBAL BOOLEAN IRC_USER( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

#ifndef STRICT_RFC
	if( Client_Type( Client ) == CLIENT_GOTNICK || Client_Type( Client ) == CLIENT_GOTPASS || Client_Type( Client ) == CLIENT_UNKNOWN )
#else
	if( Client_Type( Client ) == CLIENT_GOTNICK || Client_Type( Client ) == CLIENT_GOTPASS )
#endif
	{
		/* Falsche Anzahl Parameter? */
		if( Req->argc != 4 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		Client_SetUser( Client, Req->argv[0], FALSE );
		Client_SetInfo( Client, Req->argv[3] );

		Log( LOG_DEBUG, "Connection %d: got valid USER command ...", Client_Conn( Client ));
		if( Client_Type( Client ) == CLIENT_GOTNICK ) return Hello_User( Client );
		else Client_SetType( Client, CLIENT_GOTUSER );
		return CONNECTED;
	}
	else if( Client_Type( Client ) == CLIENT_USER || Client_Type( Client ) == CLIENT_SERVER || Client_Type( Client ) == CLIENT_SERVICE )
	{
		return IRC_WriteStrClient( Client, ERR_ALREADYREGISTRED_MSG, Client_ID( Client ));
	}
	else return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));
} /* IRC_USER */


GLOBAL BOOLEAN IRC_QUIT( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) == CLIENT_USER ) || ( Client_Type( Client ) == CLIENT_SERVICE ))
	{
		/* User / Service */
		
		/* Falsche Anzahl Parameter? */
		if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		if( Req->argc == 0 ) Conn_Close( Client_Conn( Client ), "Got QUIT command.", NULL, TRUE );
		else Conn_Close( Client_Conn( Client ), "Got QUIT command.", Req->argv[0], TRUE );
		
		return DISCONNECTED;
	}
	else if ( Client_Type( Client ) == CLIENT_SERVER )
	{
		/* Server */

		/* Falsche Anzahl Parameter? */
		if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		target = Client_Search( Req->prefix );
		if( ! target )
		{
			Log( LOG_ERR, "Got QUIT from %s for unknown client!?", Client_ID( Client ));
			return CONNECTED;
		}

		if( Req->argc == 0 ) Client_Destroy( target, "Got QUIT command.", NULL );
		else Client_Destroy( target, "Got QUIT command.", Req->argv[0] );

		return CONNECTED;
	}
	else return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));
} /* IRC_QUIT */


GLOBAL BOOLEAN IRC_SQUIT( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target;
	CHAR msg[LINE_LEN + 64];
	
	assert( Client != NULL );
	assert( Req != NULL );

	/* SQUIT ist nur fuer Server erlaubt */
	if( Client_Type( Client ) != CLIENT_SERVER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	Log( LOG_DEBUG, "Got SQUIT from %s for \"%s\": \"%s\" ...", Client_ID( Client ), Req->argv[0], Req->argv[1] );
	
	/* SQUIT an alle Server weiterleiten */
	IRC_WriteStrServers( Client, "SQUIT %s :%s", Req->argv[0], Req->argv[1] );

	target = Client_GetFromID( Req->argv[0] );
	if( ! target )
	{
		Log( LOG_ERR, "Got SQUIT from %s for unknown server \"%s\"!?", Client_ID( Client ), Req->argv[0] );
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
		Client_Destroy( target, msg, Req->argv[1] );
		return CONNECTED;
	}
} /* IRC_SQUIT */


GLOBAL BOOLEAN IRC_PING( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *from;

	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc < 1 ) return IRC_WriteStrClient( Client, ERR_NOORIGIN_MSG, Client_ID( Client ));
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Req->argc == 2 )
	{
		/* es wurde ein Ziel-Client angegeben */
		target = Client_GetFromID( Req->argv[1] );
		if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[1] );
		if( target != Client_ThisServer( ))
		{
			/* ok, forwarden */
			if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_GetFromID( Req->prefix );
			else from = Client;
			if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->prefix );
			return IRC_WriteStrClientPrefix( target, from, "PING %s :%s", Client_ID( from ), Req->argv[1] );
		}
	}

	Log( LOG_DEBUG, "Connection %d: got PING, sending PONG ...", Client_Conn( Client ));
	return IRC_WriteStrClient( Client, "PONG %s :%s", Client_ID( Client_ThisServer( )), Client_ID( Client ));
} /* IRC_PING */


GLOBAL BOOLEAN IRC_PONG( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *from;

	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc < 1 ) return IRC_WriteStrClient( Client, ERR_NOORIGIN_MSG, Client_ID( Client ));
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* forwarden? */
	if( Req->argc == 2 )
	{
		target = Client_GetFromID( Req->argv[1] );
		if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[1] );
		if( target != Client_ThisServer( ))
		{
			/* ok, forwarden */
			if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_GetFromID( Req->prefix );
			else from = Client;
			if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->prefix );
			return IRC_WriteStrClientPrefix( target, from, "PONG %s :%s", Client_ID( from ), Req->argv[1] );
		}
	}

	/* Der Connection-Timestamp wurde schon beim Lesen aus dem Socket
		* aktualisiert, daher muss das hier nicht mehr gemacht werden. */

	if( Client_Conn( Client ) > NONE ) Log( LOG_DEBUG, "Connection %d: received PONG. Lag: %ld seconds.", Client_Conn( Client ), time( NULL ) - Conn_LastPing( Client_Conn( Client )));
	else Log( LOG_DEBUG, "Connection %d: received PONG.", Client_Conn( Client ));

	return CONNECTED;
} /* IRC_PONG */


LOCAL BOOLEAN Hello_User( CLIENT *Client )
{
	assert( Client != NULL );

	/* Passwort ueberpruefen */
	if( strcmp( Client_Password( Client ), Conf_ServerPwd ) != 0 )
	{
		/* Falsches Passwort */
		Log( LOG_ERR, "User \"%s\" rejected (connection %d): Bad password!", Client_Mask( Client ), Client_Conn( Client ));
		Conn_Close( Client_Conn( Client ), NULL, "Bad password", TRUE );
		return DISCONNECTED;
	}

	Log( LOG_NOTICE, "User \"%s\" registered (connection %d).", Client_Mask( Client ), Client_Conn( Client ));

	/* Andere Server informieren */
	IRC_WriteStrServers( NULL, "NICK %s 1 %s %s 1 +%s :%s", Client_ID( Client ), Client_User( Client ), Client_Hostname( Client ), Client_Modes( Client ), Client_Info( Client ));

	if( ! IRC_WriteStrClient( Client, RPL_WELCOME_MSG, Client_ID( Client ), Client_Mask( Client ))) return FALSE;
	if( ! IRC_WriteStrClient( Client, RPL_YOURHOST_MSG, Client_ID( Client ), Client_ID( Client_ThisServer( )))) return FALSE;
	if( ! IRC_WriteStrClient( Client, RPL_CREATED_MSG, Client_ID( Client ), NGIRCd_StartStr )) return FALSE;
	if( ! IRC_WriteStrClient( Client, RPL_MYINFO_MSG, Client_ID( Client ), Client_ID( Client_ThisServer( )))) return FALSE;

	Client_SetType( Client, CLIENT_USER );

	if( ! IRC_Send_LUSERS( Client )) return DISCONNECTED;
	if( ! IRC_Show_MOTD( Client )) return DISCONNECTED;

	return CONNECTED;
} /* Hello_User */


LOCAL VOID Kill_Nick( CHAR *Nick, CHAR *Reason )
{
	CLIENT *c;

	assert( Nick != NULL );
	assert( Reason != NULL );

	Log( LOG_ERR, "User(s) with nick \"%s\" will be disconnected: %s", Nick, Reason );

	/* andere Server benachrichtigen */
	IRC_WriteStrServers( NULL, "KILL %s :%s", Nick, Reason );

	/* Ggf. einen eigenen Client toeten */
	c = Client_GetFromID( Nick );
	if( c && ( Client_Conn( c ) != NONE )) Conn_Close( Client_Conn( c ), NULL, Reason, TRUE );
} /* Kill_Nick */


/* -eof- */
