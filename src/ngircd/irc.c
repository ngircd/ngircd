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
 * $Id: irc.c,v 1.29 2002/01/05 23:24:54 alex Exp $
 *
 * irc.c: IRC-Befehle
 *
 * $Log: irc.c,v $
 * Revision 1.29  2002/01/05 23:24:54  alex
 * - WHOIS erweitert: Anfragen koennen an andere Server weitergeleitet werden.
 * - Vorbereitungen fuer Ident-Abfragen bei neuen Client-Strukturen.
 *
 * Revision 1.28  2002/01/05 20:08:02  alex
 * - Div. Aenderungen fuer die Server-Links (u.a. WHOIS, QUIT, NICK angepasst).
 * - Neue Funktionen IRC_WriteStrServer() und IRC_WriteStrServerPrefix().
 *
 * Revision 1.27  2002/01/05 19:15:03  alex
 * - Fehlerpruefung bei select() in der "Hauptschleife" korrigiert.
 *
 * Revision 1.26  2002/01/05 16:51:18  alex
 * - das Passwort von Servern wird nun ueberprueft (PASS- und SERVER-Befehl).
 *
 * Revision 1.25  2002/01/05 00:48:33  alex
 * - bei SQUIT wurde immer die Verbindung getrennt, auch bei Remote-Servern.
 *
 * Revision 1.24  2002/01/04 17:58:44  alex
 * - IRC_WriteStrXXX()-Funktionen eingefuehrt, groessere Anpassungen daran.
 * - neuer Befehl SQUIT, QUIT an Server-Links angepasst.
 *
 * Revision 1.23  2002/01/04 01:36:40  alex
 * - Loglevel ein wenig angepasst.
 *
 * Revision 1.22  2002/01/04 01:21:47  alex
 * - Client-Strukruren werden nur noch ueber Funktionen angesprochen.
 * - Weitere Anpassungen und Erweiterungen der Server-Links.
 *
 * Revision 1.21  2002/01/03 02:26:51  alex
 * - neue Befehle SERVER und NJOIN begonnen,
 * - begonnen, diverse IRC-Befehle an Server-Links anzupassen.
 *
 * Revision 1.20  2002/01/02 12:46:41  alex
 * - die Gross- und Kleinschreibung des Nicks kann mit NICK nun geaendert werden.
 *
 * Revision 1.19  2002/01/02 02:51:39  alex
 * - Copyright-Texte angepasst.
 * - neuer Befehl "ERROR".
 *
 * Revision 1.17  2001/12/31 15:33:13  alex
 * - neuer Befehl NAMES, kleinere Bugfixes.
 * - Bug bei PING behoben: war zu restriktiv implementiert :-)
 *
 * Revision 1.16  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.15  2001/12/30 19:26:11  alex
 * - Unterstuetzung fuer die Konfigurationsdatei eingebaut.
 *
 * Revision 1.14  2001/12/30 11:42:00  alex
 * - der Server meldet nun eine ordentliche "Start-Zeit".
 *
 * Revision 1.13  2001/12/29 03:10:06  alex
 * - Neue Funktion IRC_MODE() implementiert, div. Aenderungen.
 * - neue configure-Optione "--enable-strict-rfc".
 *
 * Revision 1.12  2001/12/27 19:17:26  alex
 * - neue Befehle PRIVMSG, NOTICE, PING.
 *
 * Revision 1.11  2001/12/27 16:55:41  alex
 * - neu: IRC_WriteStrRelated(), Aenderungen auch in IRC_WriteStrClient().
 *
 * Revision 1.10  2001/12/26 22:48:53  alex
 * - MOTD-Datei ist nun konfigurierbar und wird gelesen.
 *
 * Revision 1.9  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.8  2001/12/26 03:21:46  alex
 * - PING/PONG-Befehle implementiert,
 * - Meldungen ueberarbeitet: enthalten nun (fast) immer den Nick.
 *
 * Revision 1.7  2001/12/25 23:25:18  alex
 * - und nochmal Aenderungen am Logging ;-)
 *
 * Revision 1.6  2001/12/25 23:13:33  alex
 * - Debug-Meldungen angepasst.
 *
 * Revision 1.5  2001/12/25 22:02:42  alex
 * - neuer IRC-Befehl "/QUIT". Verbessertes Logging & Debug-Ausgaben.
 *
 * Revision 1.4  2001/12/25 19:19:30  alex
 * - bessere Fehler-Abfragen, diverse Bugfixes.
 * - Nicks werden nur einmal vergeben :-)
 * - /MOTD wird unterstuetzt.
 *
 * Revision 1.3  2001/12/24 01:34:06  alex
 * - USER und NICK wird nun in beliebiger Reihenfolge akzeptiert (wg. BitchX)
 * - MOTD-Ausgabe begonnen zu implementieren.
 *
 * Revision 1.2  2001/12/23 21:57:16  alex
 * - erste IRC-Befehle zu implementieren begonnen.
 *
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ngircd.h"
#include "client.h"
#include "conf.h"
#include "conn.h"
#include "log.h"
#include "messages.h"
#include "parse.h"
#include "tool.h"

#include <exp.h>
#include "irc.h"


#define CONNECTED TRUE
#define DISCONNECTED FALSE


LOCAL BOOLEAN Hello_User( CLIENT *Client );
LOCAL BOOLEAN Show_MOTD( CLIENT *Client );

LOCAL VOID Kill_Nick( CHAR *Nick );


GLOBAL VOID IRC_Init( VOID )
{
} /* IRC_Init */


GLOBAL VOID IRC_Exit( VOID )
{
} /* IRC_Exit */


GLOBAL BOOLEAN IRC_WriteStrClient( CLIENT *Client, CHAR *Format, ... )
{
	CHAR buffer[1000];
	BOOLEAN ok = CONNECTED;
	va_list ap;

	assert( Client != NULL );
	assert( Format != NULL );

	va_start( ap, Format );
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	/* an den Client selber */
	ok = IRC_WriteStrClientPrefix( Client, Client_ThisServer( ), buffer );

	return ok;
} /* IRC_WriteStrClient */


GLOBAL BOOLEAN IRC_WriteStrClientPrefix( CLIENT *Client, CLIENT *Prefix, CHAR *Format, ... )
{
	/* Text an Clients, lokal bzw. remote, senden. */

	CHAR buffer[1000];
	va_list ap;

	assert( Client != NULL );
	assert( Format != NULL );
	assert( Prefix != NULL );

	va_start( ap, Format );
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	return Conn_WriteStr( Client_Conn( Client_NextHop( Client )), ":%s %s", Client_ID( Prefix ), buffer );
} /* IRC_WriteStrClientPrefix */


GLOBAL BOOLEAN IRC_WriteStrRelated( CLIENT *Client, CHAR *Format, ... )
{
	CHAR buffer[1000];
	BOOLEAN ok = CONNECTED;
	va_list ap;

	assert( Client != NULL );
	assert( Format != NULL );

	va_start( ap, Format );
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	/* an den Client selber */
	ok = IRC_WriteStrClient( Client, buffer );

	/* FIXME */
	
	return ok;
} /* IRC_WriteStrRelated */


GLOBAL VOID IRC_WriteStrServers( CLIENT *ExceptOf, CHAR *Format, ... )
{
	CHAR buffer[1000];
	va_list ap;

	assert( Format != NULL );

	va_start( ap, Format );
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	/* an den Client selber */
	return IRC_WriteStrServersPrefix( ExceptOf, Client_ThisServer( ), buffer );
} /* IRC_WriteStrServers */


GLOBAL VOID IRC_WriteStrServersPrefix( CLIENT *ExceptOf, CLIENT *Prefix, CHAR *Format, ... )
{
	CHAR buffer[1000];
	CLIENT *c;
	va_list ap;
	
	assert( Format != NULL );
	assert( Prefix != NULL );

	va_start( ap, Format );
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );
	
	c = Client_First( );
	while( c )
	{
		if(( Client_Type( c ) == CLIENT_SERVER ) && ( Client_Conn( c ) > NONE ) && ( c != Client_ThisServer( )) && ( c != ExceptOf ))
		{
			/* Ziel-Server gefunden */
			IRC_WriteStrClientPrefix( c, Prefix, buffer );
		}
		c = Client_Next( c );
	}
} /* IRC_WriteStrServersPrefix */


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
	CLIENT *c;
	INT i;
	
	assert( Client != NULL );
	assert( Req != NULL );

	/* Fehler liefern, wenn kein lokaler Client */
	if( Client_Conn( Client ) <= NONE ) return IRC_WriteStrClient( Client, ERR_UNKNOWNCOMMAND_MSG, Client_ID( Client ), Req->command );

	if( Client_Type( Client ) == CLIENT_GOTPASSSERVER )
	{
		/* Verbindung soll als Server-Server-Verbindung registriert werden */
		Log( LOG_DEBUG, "Connection %d: got SERVER command (new server link) ...", Client_Conn( Client ));

		/* Falsche Anzahl Parameter? */
		if( Req->argc != 3 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		/* Ist dieser Server bei uns konfiguriert? */
		for( i = 0; i < Conf_Server_Count; i++ ) if( strcasecmp( Req->argv[0], Conf_Server[i].name ) == 0 ) break;
		if( i >= Conf_Server_Count )
		{
			/* Server ist nicht konfiguriert! */
			Log( LOG_ALERT, "Connection %d: Server \"%s\" not configured here!", Client_Conn( Client ), Req->argv[0] );
			Conn_Close( Client_Conn( Client ), "Server not configured here!" );
			return DISCONNECTED;
		}
		if( strcmp( Client_Password( Client ), Conf_Server[i].pwd ) != 0 )
		{
			/* Falsches Passwort */
			Log( LOG_ALERT, "Connection %d: Bad password for server \"%s\"!", Client_Conn( Client ), Req->argv[0] );
			Conn_Close( Client_Conn( Client ), "Bad password!" );
			return DISCONNECTED;
		}
		
		/* Ist ein Server mit dieser ID bereits registriert? */
		if( ! Client_CheckID( Client, Req->argv[0] )) return DISCONNECTED;

		/* Server-Strukturen fuellen ;-) */
		Client_SetID( Client, Req->argv[0] );
		Client_SetToken( Client, atoi( Req->argv[1] ));
		Client_SetInfo( Client, Req->argv[2] );
		Client_SetHops( Client, 1 );

		Log( LOG_NOTICE, "Server \"%s\" registered (connection %d, 1 hop - direct link).", Client_ID( Client ), Client_Conn( Client ));

		Client_SetType( Client, CLIENT_SERVER );
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

		/* Neue Client-Struktur anlegen */
		c = Client_NewRemoteServer( Client, Req->argv[0], atoi( Req->argv[1] ), atoi( Req->argv[2] ), ptr, TRUE );
		if( ! c )
		{
			/* Neue Client-Struktur konnte nicht angelegt werden */
			Log( LOG_ALERT, "Can't allocate client structure for server! (on connection %d)", Client_Conn( Client ));
			Conn_Close( Client_Conn( Client ), "Can't allocate client structure for remote server!" );
			return DISCONNECTED;
		}

		/* Log-Meldung zusammenbauen und ausgeben */
		if(( Client_Hops( c ) > 1 ) && ( Req->prefix[0] )) sprintf( str, "connected to %s, ", Req->prefix );
		else strcpy( str, "" );
		Log( LOG_NOTICE, "Server \"%s\" registered (via %s, %s%d hop%s).", Client_ID( c ), Client_ID( Client ), str, Client_Hops( c ), Client_Hops( c ) > 1 ? "s": "" );
		
		return CONNECTED;
	}
	else return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );
} /* IRC_SERVER */


GLOBAL BOOLEAN IRC_NJOIN( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_SERVER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTEREDSERVER_MSG, Client_ID( Client ));

	return CONNECTED;
} /* IRC_NJOIN */


GLOBAL BOOLEAN IRC_NICK( CLIENT *Client, REQUEST *Req )
{
	CLIENT *intr_c, *c;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Zumindest BitchX sendet NICK-USER in der falschen Reihenfolge. */
#ifndef STRICT_RFC
	if( Client_Type( Client ) == CLIENT_UNKNOWN || Client_Type( Client ) == CLIENT_GOTPASS || Client_Type( Client ) == CLIENT_GOTNICK || Client_Type( Client ) == CLIENT_GOTUSER || Client_Type( Client ) == CLIENT_USER )
#else
	if( Client_Type( Client ) == CLIENT_UNKNOWN || Client_Type( Client ) == CLIENT_GOTPASS || Client_Type( Client ) == CLIENT_GOTNICK || Client_Type( Client ) == CLIENT_USER  )
#endif
	{
		/* User-Registrierung bzw. Nick-Aenderung */

		/* Falsche Anzahl Parameter? */
		if( Req->argc != 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		/* Ist der Client "restricted"? */
		if( Client_HasMode( Client, 'r' )) return IRC_WriteStrClient( Client, ERR_RESTRICTED_MSG, Client_ID( Client ));

		/* Wenn der Client zu seinem eigenen Nick wechseln will, so machen
		 * wir nichts. So macht es das Original und mind. Snak hat probleme,
		 * wenn wir es nicht so machen. Ob es so okay ist? Hm ... */
#ifndef STRICT_RFC
		if( strcmp( Client_ID( Client ), Req->argv[0] ) == 0 ) return CONNECTED;
#endif
		
		/* pruefen, ob Nick bereits vergeben. Speziallfall: der Client
		 * will nur die Gross- und Kleinschreibung aendern. Das darf
		 * er natuerlich machen :-) */
		if( strcasecmp( Client_ID( Client ), Req->argv[0] ) != 0 )
		{
			if( ! Client_CheckNick( Client, Req->argv[0] ))	return CONNECTED;
		}

		if( Client_Type( Client ) == CLIENT_USER )
		{
			/* Nick-Aenderung: allen mitteilen! */
			Log( LOG_INFO, "User \"%s\" changed nick: \"%s\" -> \"%s\".", Client_Mask( Client ), Client_ID( Client ), Req->argv[0] );
			IRC_WriteStrRelated( Client, "NICK :%s", Req->argv[0] );
		}
		
		/* Client-Nick registrieren */
		Client_SetID( Client, Req->argv[0] );

		if( Client_Type( Client ) != CLIENT_USER )
		{
			/* Neuer Client */
			Log( LOG_DEBUG, "Connection %d: got NICK command ...", Client_Conn( Client ));
			if( Client_Type( Client ) == CLIENT_GOTUSER ) return Hello_User( Client );
			else Client_SetType( Client, CLIENT_GOTNICK );
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
			Log( LOG_ALERT, "Server %s introduces already registered nick %s!", Client_ID( Client ), Req->argv[0] );
			Kill_Nick( Req->argv[0] );
			return CONNECTED;
		}

		/* Server, zu dem der Client connectiert ist, suchen */
		intr_c = Client_GetFromToken( Client, atoi( Req->argv[4] ));
		if( ! intr_c )
		{
			Log( LOG_ALERT, "Server %s introduces nick %s with unknown host server!?", Client_ID( Client ), Req->argv[0] );
			Kill_Nick( Req->argv[0] );
			return CONNECTED;
		}

		/* Neue Client-Struktur anlegen */
		c = Client_NewRemoteUser( intr_c, Req->argv[0], atoi( Req->argv[1] ), Req->argv[2], Req->argv[3], atoi( Req->argv[4] ), Req->argv[5], Req->argv[6], TRUE );
		if( ! c )
		{
			/* Eine neue Client-Struktur konnte nicht angelegt werden.
			 * Der Client muss disconnectiert werden, damit der Netz-
			 * status konsistent bleibt. */
			Log( LOG_ALERT, "Can't allocate client structure! (on connection %d)", Client_Conn( Client ));
			Kill_Nick( Req->argv[0] );
			return CONNECTED;
		}

		Log( LOG_DEBUG, "User \"%s\" registered (via %s, on %s, %d hop%s).", Client_ID( c ), Client_ID( Client ), Client_ID( intr_c ), Client_Hops( c ), Client_Hops( c ) > 1 ? "s": "" );

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

		Log( LOG_DEBUG, "Connection %d: got USER command ...", Client_Conn( Client ));
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

		Conn_Close( Client_Conn( Client ), "Client wants to quit." );
		return DISCONNECTED;
	}
	else if ( Client_Type( Client ) == CLIENT_SERVER )
	{
		/* Server */

		/* Falsche Anzahl Parameter? */
		if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

		target = Client_Search( Req->prefix );
		if( ! target ) Log( LOG_ALERT, "Got QUIT from %s for unknown server!?", Client_ID( Client ));
		else Client_Destroy( target );
		return CONNECTED;
	}
	else return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));
} /* IRC_QUIT */


GLOBAL BOOLEAN IRC_SQUIT( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target;
	
	assert( Client != NULL );
	assert( Req != NULL );

	/* SQUIT ist nur fuer Server erlaubt */
	if( Client_Type( Client ) != CLIENT_SERVER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	target = Client_GetFromID( Req->argv[0] );
	if( ! target )
	{
		Log( LOG_ALERT, "Got SQUIT from %s for unknown server \%s\"!?", Client_ID( Client ), Req->argv[0] );
		return CONNECTED;
	}

	if( target == Client ) Log( LOG_NOTICE, "Got SQUIT from %s: %s", Client_ID( Client ), Req->argv[1] );
	else Log( LOG_NOTICE, "Got SQUIT from %s for %s: %s", Client_ID( Client ), Client_ID( target ), Req->argv[1] );

	/* FIXME: das SQUIT muss an alle Server weiter-
	 * geleitet werden ... */

	if( Client_Conn( target ) > NONE )
	{
		Conn_Close( Client_Conn( target ), Req->argv[1] );
		return DISCONNECTED;
	}
	else
	{
		Client_Destroy( target );
		return CONNECTED;
	}
} /* IRC_SQUIT */


GLOBAL BOOLEAN IRC_PING( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc < 1 ) return IRC_WriteStrClient( Client, ERR_NOORIGIN_MSG, Client_ID( Client ));
	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	return IRC_WriteStrClient( Client, "PONG %s :%s", Client_ID( Client_ThisServer( )), Client_ID( Client ));
} /* IRC_PING */


GLOBAL BOOLEAN IRC_PONG( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc < 1 ) return IRC_WriteStrClient( Client, ERR_NOORIGIN_MSG, Client_ID( Client ));
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Der Connection-Timestamp wurde schon beim Lesen aus dem Socket
	 * aktualisiert, daher muss das hier nicht mehr gemacht werden. */

	Log( LOG_DEBUG, "Connection %d: received PONG.", Client_Conn( Client ));
	return CONNECTED;
} /* IRC_PONG */


GLOBAL BOOLEAN IRC_MOTD( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_USER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	return Show_MOTD( Client );
} /* IRC_MOTD */


GLOBAL BOOLEAN IRC_PRIVMSG( CLIENT *Client, REQUEST *Req )
{
	CLIENT *to, *from;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc == 0 ) return IRC_WriteStrClient( Client, ERR_NORECIPIENT_MSG, Client_ID( Client ), Req->command );
	if( Req->argc == 1 ) return IRC_WriteStrClient( Client, ERR_NOTEXTTOSEND_MSG, Client_ID( Client ));
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_GetFromID( Req->prefix );
	else from = Client;
	
	to = Client_Search( Req->argv[0] );
	if( to )
	{
		/* Okay, Ziel ist ein User */
		if( Client_Conn( from ) >= 0 ) Conn_UpdateIdle( Client_Conn( from ));
		return IRC_WriteStrClientPrefix( to, from, "PRIVMSG %s :%s", Client_ID( to ), Req->argv[1] );
	}
	else return IRC_WriteStrClient( from, ERR_NOSUCHNICK_MSG, Client_ID( from ), Req->argv[0] );
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

	to = Client_Search( Req->argv[0] );
	if( to )
	{
		/* Okay, Ziel ist ein User */
		return IRC_WriteStrClientPrefix( to, from, "NOTICE %s :%s", Client_ID( to ), Req->argv[1] );
	}
	else return CONNECTED;
} /* IRC_NOTICE */


GLOBAL BOOLEAN IRC_MODE( CLIENT *Client, REQUEST *Req )
{
	CHAR x[2], new_modes[CLIENT_MODE_LEN], *ptr;
	BOOLEAN set, ok;
	CLIENT *target;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 2 ) || ( Req->argc < 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* "Ziel-Client" suchen */
	target = Client_Search( Req->argv[0] );

	/* Wer ist der Anfragende? */
	if( Client_Type( Client ) == CLIENT_USER )
	{
		/* User: MODE ist nur fuer sich selber zulaessig */
		if( target != Client ) return IRC_WriteStrClient( Client, ERR_USERSDONTMATCH_MSG, Client_ID( Client ));
	}
	else
	{
		/* Server: gibt es den Client ueberhaupt? */
		if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->argv[0] );
	}

	/* Werden die Modes erfragt? */
	if( Req->argc == 1 ) return IRC_WriteStrClient( Client, RPL_UMODEIS_MSG, Client_ID( Client ), Client_Modes( Client ));

	ptr = Req->argv[1];

	/* Sollen Modes gesetzt oder geloescht werden? */
	if( *ptr == '+' ) set = TRUE;
	else if( *ptr == '-' ) set = FALSE;
	else return IRC_WriteStrClient( Client, ERR_UMODEUNKNOWNFLAG_MSG, Client_ID( Client ));

	/* Reply-String mit Aenderungen vorbereiten */
	if( set ) strcpy( new_modes, "+" );
	else strcpy( new_modes, "-" );

	ptr++;
	ok = TRUE;
	x[1] = '\0';
	while( *ptr )
	{
		x[0] = '\0';
		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			x[0] = *ptr;
			ok = TRUE;
		}
		else
		{
			switch( *ptr )
			{
				case 'i':
					/* invisible */
					x[0] = 'i';
					break;
				case 'r':
					/* restricted (kann nur gesetzt werden) */
					if( set ) x[0] = 'r';
					else ok = IRC_WriteStrClient( target, ERR_RESTRICTED_MSG, Client_ID( target ));
					break;
				case 'o':
					/* operator (kann nur geloescht werden) */
					if( ! set )
					{
						Client_SetOperByMe( target, FALSE );
						x[0] = 'o';
					}
					else ok = IRC_WriteStrClient( target, ERR_UMODEUNKNOWNFLAG_MSG, Client_ID( target ));
					break;
				default:
					ok = IRC_WriteStrClient( target, ERR_UMODEUNKNOWNFLAG_MSG, Client_ID( target ));
					x[0] = '\0';
			}
		}
		if( ! ok ) break;

		ptr++;
		if( ! x[0] ) continue;

		/* Okay, gueltigen Mode gefunden */
		if( set )
		{
			/* Mode setzen. Wenn der Client ihn noch nicht hatte: merken */
			if( Client_ModeAdd( target, x[0] )) strcat( new_modes, x );
		}
		else
		{
			/* Modes geloescht. Wenn der Client ihn hatte: merken */
			if( Client_ModeDel( target, x[0] )) strcat( new_modes, x );
		}
	}
	
	/* Geanderte Modes? */
	if( new_modes[1] )
	{
		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			/* Modes an andere Server forwarden */
			/* FIXME */
		}
		else
		{
			/* Bestaetigung an Client schicken */
			ok = IRC_WriteStrRelated( Client, "MODE %s :%s", Client_ID( target ), new_modes );
		}
		Log( LOG_DEBUG, "User \"%s\": Mode change, now \"%s\".", Client_Mask( target ), Client_Modes( target ));
	}
	return ok;
} /* IRC_MODE */


GLOBAL BOOLEAN IRC_OPER( CLIENT *Client, REQUEST *Req )
{
	INT i;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_USER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));
	
	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Operator suchen */
	for( i = 0; i < Conf_Oper_Count; i++)
	{
		if( Conf_Oper[i].name[0] && Conf_Oper[i].pwd[0] && ( strcmp( Conf_Oper[i].name, Req->argv[0] ) == 0 )) break;
	}
	if( i >= Conf_Oper_Count ) return IRC_WriteStrClient( Client, ERR_PASSWDMISMATCH_MSG, Client_ID( Client ));

	/* Stimmt der Name und das Passwort? */
	if(( strcmp( Conf_Oper[i].name, Req->argv[0] ) != 0 ) || ( strcmp( Conf_Oper[i].pwd, Req->argv[1] ) != 0 )) return IRC_WriteStrClient( Client, ERR_PASSWDMISMATCH_MSG, Client_ID( Client ));
	
	if( ! Client_HasMode( Client, 'o' ))
	{
		/* noch kein o-Mode gesetzt */
		Client_ModeAdd( Client, 'o' );
		if( ! IRC_WriteStrRelated( Client, "MODE %s :+o", Client_ID( Client ))) return DISCONNECTED;
	}

	if( ! Client_OperByMe( Client )) Log( LOG_NOTICE, "User \"%s\" is now an IRC Operator.", Client_Mask( Client ));

	Client_SetOperByMe( Client, TRUE );
	return IRC_WriteStrClient( Client, RPL_YOUREOPER_MSG, Client_ID( Client ));
} /* IRC_OPER */


GLOBAL BOOLEAN IRC_DIE( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_USER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if(( ! Client_HasMode( Client, 'o' )) || ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));

	Log( LOG_NOTICE, "Got DIE command from \"%s\", going down!", Client_Mask( Client ));
	NGIRCd_Quit = TRUE;
	return CONNECTED;
} /* IRC_DIE */


GLOBAL BOOLEAN IRC_RESTART( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_USER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if(( ! Client_HasMode( Client, 'o' )) || ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));

	Log( LOG_NOTICE, "Got RESTART command from \"%s\", going down!", Client_Mask( Client ));
	NGIRCd_Restart = TRUE;
	return CONNECTED;
} /* IRC_RESTART */


GLOBAL BOOLEAN IRC_NAMES( CLIENT *Client, REQUEST *Req )
{
	CHAR rpl[COMMAND_LEN];
	CLIENT *c;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client_Type( Client ) != CLIENT_USER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Noch alle User ausgeben, die in keinem Channel sind */
	rpl[0] = '\0';
	c = Client_First( );
	while( c )
	{
		if( Client_Type( c ) == CLIENT_USER )
		{
			/* Okay, das ist ein User */
			strcat( rpl, Client_ID( c ));
			strcat( rpl, " " );
		}

		/* Antwort zu lang? Splitten. */
		if( strlen( rpl ) > 480 )
		{
			if( rpl[strlen( rpl ) - 1] == ' ' ) rpl[strlen( rpl ) - 1] = '\0';
			if( ! IRC_WriteStrClient( Client, RPL_NAMREPLY_MSG, Client_ID( Client ), "*", "*", rpl )) return DISCONNECTED;
			rpl[0] = '\0';
		}
		
		c = Client_Next( c );
	}
	if( rpl[0] )
	{
		/* es wurden User gefunden */
		if( rpl[strlen( rpl ) - 1] == ' ' ) rpl[strlen( rpl ) - 1] = '\0';
		if( ! IRC_WriteStrClient( Client, RPL_NAMREPLY_MSG, Client_ID( Client ), "*", "*", rpl )) return DISCONNECTED;
	}
	return IRC_WriteStrClient( Client, RPL_ENDOFNAMES_MSG, Client_ID( Client ), "*" );
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
		/* Ziel-Server suchen */
		target = Client_GetFromID( Req->argv[1] );
		if( ! target ) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[1] );
		else return IRC_WriteStrClientPrefix( target, from, "WHOIS %s :%s", Req->argv[0], Req->argv[1] );
	}
	
	/* Nick, User und Name */
	if( ! IRC_WriteStrClient( from, RPL_WHOISUSER_MSG, Client_ID( from ), Client_ID( c ), Client_User( c ), Client_Hostname( c ), Client_Info( c ))) return DISCONNECTED;

	/* Server */
	if( ! IRC_WriteStrClient( from, RPL_WHOISSERVER_MSG, Client_ID( from ), Client_ID( c ), Client_ID( Client_Introducer( c )), Client_Info( Client_Introducer( c )))) return DISCONNECTED;

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

	/* End of Whois */
	return IRC_WriteStrClient( from, RPL_ENDOFWHOIS_MSG, Client_ID( from ), Client_ID( c ));
} /* IRC_WHOIS */


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


LOCAL BOOLEAN Hello_User( CLIENT *Client )
{
	assert( Client != NULL );

	/* Passwort ueberpruefen */
	if( strcmp( Client_Password( Client ), Conf_ServerPwd ) != 0 )
	{
		/* Falsches Passwort */
		Log( LOG_ALERT, "User \"%s\" rejected (connection %d): Bad password!", Client_Mask( Client ), Client_Conn( Client ));
		Conn_Close( Client_Conn( Client ), "Bad password!" );
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

	return Show_MOTD( Client );
} /* Hello_User */


LOCAL BOOLEAN Show_MOTD( CLIENT *Client )
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
} /* Show_MOTD */


LOCAL VOID Kill_Nick( CHAR *Nick )
{
	Log( LOG_ALERT, "User(s) with nick \"%s\" will be disconnected!", Nick );
	/* FIXME */
	Log( LOG_ALERT, "[Kill_Nick() not implemented - OOOPS!]" );
} /* Kill_Nick */


/* -eof- */
