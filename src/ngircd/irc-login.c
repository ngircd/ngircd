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
 * $Id: irc-login.c,v 1.6 2002/03/11 22:04:10 alex Exp $
 *
 * irc-login.c: Anmeldung und Abmeldung im IRC
 *
 * $Log: irc-login.c,v $
 * Revision 1.6  2002/03/11 22:04:10  alex
 * - Client_Destroy() hat neuen Paramter: QUITs fuer Clients verschicken?
 *
 * Revision 1.5  2002/03/11 17:33:40  alex
 * - Log-Level von SQUIT und QUIT bei unbekannten Clients auf DEBUG herabgesetzt.
 *
 * Revision 1.4  2002/03/10 22:40:22  alex
 * - IRC_PING() ist, wenn nicht im "strict RFC"-Mode, toleranter und akzptiert
 *   beliebig viele Parameter: z.B. BitchX sendet soetwas.
 *
 * Revision 1.3  2002/03/03 17:15:11  alex
 * - Source in weitere Module fuer IRC-Befehle aufgesplitted.
 *
 * Revision 1.2  2002/03/02 00:49:11  alex
 * - Bei der USER-Registrierung wird NICK nicht mehr sofort geforwarded,
 *   sondern erst dann, wenn auch ein gueltiges USER empfangen wurde.
 *
 * Revision 1.1  2002/02/27 23:26:21  alex
 * - Modul aus irc.c bzw. irc.h ausgegliedert.
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

			/* alle betroffenen User und Server ueber Nick-Aenderung informieren */
			if( Client_Type( Client ) == CLIENT_USER ) IRC_WriteStrClientPrefix( Client, Client, "NICK :%s", Req->argv[0] );
			IRC_WriteStrServersPrefix( Client, target, "NICK :%s", Req->argv[0] );
			IRC_WriteStrRelatedPrefix( target, target, FALSE, "NICK :%s", Req->argv[0] );
			
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
			/* Den Client kennen wir nicht (mehr), also nichts zu tun. */
			Log( LOG_DEBUG, "Got QUIT from %s for unknown client!?", Client_ID( Client ));
			return CONNECTED;
		}

		if( Req->argc == 0 ) Client_Destroy( target, "Got QUIT command.", NULL, TRUE );
		else Client_Destroy( target, "Got QUIT command.", Req->argv[0], TRUE );

		return CONNECTED;
	}
	else return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));
} /* IRC_QUIT */


GLOBAL BOOLEAN IRC_PING( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *from;

	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc < 1 ) return IRC_WriteStrClient( Client, ERR_NOORIGIN_MSG, Client_ID( Client ));
#ifdef STRICT_RFC
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );
#endif

	if( Req->argc > 1 )
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
