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
 * $Id: irc.c,v 1.55 2002/02/12 14:40:37 alex Exp $
 *
 * irc.c: IRC-Befehle
 *
 * $Log: irc.c,v $
 * Revision 1.55  2002/02/12 14:40:37  alex
 * - via NJOIN gemeldete Benutzer wurden nicht in Channels bekannt gemacht.
 *
 * Revision 1.54  2002/02/11 23:33:35  alex
 * - weitere Anpassungen an Channel-Modes und Channel-User-Modes.
 *
 * Revision 1.53  2002/02/11 16:06:21  alex
 * - Die Quelle von MODE-Aenderungen wird nun korrekt weitergegeben.
 *
 * Revision 1.52  2002/02/11 15:52:21  alex
 * - PONG an den Server selber wurde faelschlicherweise versucht zu forwarden.
 * - Channel-Modes wurden falsch geliefert (als User-Modes).
 *
 * Revision 1.51  2002/02/11 15:15:53  alex
 * - PING und PONG werden nun auch korrekt an andere Server geforwarded.
 * - bei MODE-Meldungen wird der letzte Parameter nicht mehr mit ":" getrennt.
 *
 * Revision 1.50  2002/02/11 01:03:20  alex
 * - Aenderungen und Anpassungen an Channel-Modes und Channel-User-Modes:
 *   Modes werden besser geforwarded, lokale User, fuer die ein Channel
 *   angelegt wird, werden Channel-Operator, etc. pp. ...
 * - NJOIN's von Servern werden nun korrekt an andere Server weitergeleitet.
 *
 * Revision 1.49  2002/02/06 16:51:22  alex
 * - neue Funktion zur MODE-Behandlung, fuer Channel-Modes vorbereitet.
 *
 * Revision 1.48  2002/01/29 00:13:45  alex
 * - WHOIS zeigt nun auch die Channels an, in denen der jeweilige User Mitglied ist.
 * - zu jedem Server wird nun der "Top-Server" gespeichert, somit funktioniert
 *   LINKS wieder korrekt.
 *
 * Revision 1.47  2002/01/28 13:05:48  alex
 * - nach einem JOIN wird die Liste der Mitglieder an den Client geschickt.
 * - MODE fuer Channels wird nun komplett ignoriert (keine Fehlermeldung mehr).
 *
 * Revision 1.46  2002/01/28 01:45:43  alex
 * - SERVER-Meldungen an neue Server sind nun in der richtigen Reihenfolge.
 *
 * Revision 1.45  2002/01/28 01:18:14  alex
 * - connectierenden Servern werden Channels nun mit NJOIN bekannt gemacht.
 *
 * Revision 1.44  2002/01/28 00:55:08  alex
 * - ein neu connectierender Server wird nun korrekt im Netz bekannt gemacht.
 *
 * Revision 1.43  2002/01/27 21:56:39  alex
 * - IRC_WriteStrServersPrefixID() und IRC_WriteStrClientPrefixID() wieder entfernt.
 * - einige kleinere Fixes bezueglich Channels ...
 *
 * Revision 1.42  2002/01/27 18:28:01  alex
 * - bei NICK wurde das falsche Prefix an andere Server weitergegeben.
 *
 * Revision 1.41  2002/01/27 17:15:49  alex
 * - anderungen an den Funktions-Prototypen von IRC_WriteStrChannel() und
 *   IRC_WriteStrChannelPrefix(),
 * - neue: IRC_WriteStrClientPrefixID() und IRC_WriteStrServersPrefixID().
 *
 * Revision 1.40  2002/01/26 18:43:11  alex
 * - neue Funktionen IRC_WriteStrChannelPrefix() und IRC_WriteStrChannel(),
 *   die IRC_Write_xxx_Related() sind dafuer entfallen.
 * - IRC_PRIVMSG() kann nun auch mit Channels als Ziel umgehen.
 *
 * Revision 1.39  2002/01/21 00:05:11  alex
 * - neue Funktionen IRC_JOIN und IRC_PART begonnen, ebenso die Funktionen
 *   IRC_WriteStrRelatedPrefix und IRC_WriteStrRelatedChannelPrefix().
 * - diverse Aenderungen im Zusammenhang mit Channels.
 *
 * Revision 1.38  2002/01/18 15:31:32  alex
 * - die User-Modes bei einem NICK von einem Server wurden falsch uebernommen.
 *
 * Revision 1.37  2002/01/16 22:10:18  alex
 * - IRC_LUSERS() implementiert.
 *
 * Revision 1.36  2002/01/11 23:50:55  alex
 * - LINKS implementiert, LUSERS begonnen.
 *
 * Revision 1.35  2002/01/09 21:30:45  alex
 * - WHOIS wurde faelschlicherweise an User geforwarded statt vom Server beantwortet.
 *
 * Revision 1.34  2002/01/09 01:09:58  alex
 * - WHOIS wird im "Strict-RFC-Mode" nicht mehr automatisch geforwarded,
 * - andere Server werden nun ueber bisherige Server und User informiert.
 *
 * Revision 1.33  2002/01/07 23:42:12  alex
 * - Es werden fuer alle Server eigene Token generiert,
 * - QUIT von einem Server fuer einen User wird an andere Server geforwarded,
 * - ebenso NICK-Befehle, die "fremde" User einfuehren.
 *
 * Revision 1.32  2002/01/07 16:02:36  alex
 * - Loglevel von Remote-Mode-Aenderungen angepasst (nun Debug).
 * - Im Debug-Mode werden nun auch PING's protokolliert.
 *
 * Revision 1.31  2002/01/07 15:39:46  alex
 * - Server nimmt nun Server-Links an: PASS und SERVER entsprechend angepasst.
 * - MODE und NICK melden nun die Aenderungen an andere Server.
 *
 * Revision 1.30  2002/01/06 15:21:29  alex
 * - Loglevel und Meldungen nochmals ueberarbeitet.
 * - QUIT und SQUIT forwarden nun den Grund der Trennung,
 * - WHOIS wird nun immer an den "Original-Server" weitergeleitet.
 *
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
#include "channel.h"
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

LOCAL BOOLEAN Send_NAMES( CLIENT *Client, CHANNEL *Chan );


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


GLOBAL BOOLEAN IRC_WriteStrChannel( CLIENT *Client, CHANNEL *Chan, BOOLEAN Remote, CHAR *Format, ... )
{
	CHAR buffer[1000];
	va_list ap;

	assert( Client != NULL );
	assert( Format != NULL );

	va_start( ap, Format );
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	return IRC_WriteStrChannelPrefix( Client, Chan, Client_ThisServer( ), Remote, buffer );
} /* IRC_WriteStrChannel */


GLOBAL BOOLEAN IRC_WriteStrChannelPrefix( CLIENT *Client, CHANNEL *Chan, CLIENT *Prefix, BOOLEAN Remote, CHAR *Format, ... )
{
	CHAR buffer[1000];
	BOOLEAN sock[MAX_CONNECTIONS], ok = CONNECTED, i;
	CL2CHAN *cl2chan;
	CLIENT *c;
	INT s;
	va_list ap;

	assert( Client != NULL );
	assert( Chan != NULL );
	assert( Prefix != NULL );
	assert( Format != NULL );

	va_start( ap, Format );
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	for( i = 0; i < MAX_CONNECTIONS; i++ ) sock[i] = FALSE;

	/* An alle Clients, die in den selben Channels sind.
	 * Dabei aber nur einmal je Remote-Server */
	cl2chan = Channel_FirstMember( Chan );
	while( cl2chan )
	{
		c = Channel_GetClient( cl2chan );
		if( ! Remote )
		{
			if( Client_Conn( c ) <= NONE ) c = NULL;
			else if( Client_Type( c ) == CLIENT_SERVER ) c = NULL;
		}
		if( c ) c = Client_NextHop( c );
			
		if( c && ( c != Client ))
		{
			/* Ok, anderer Client */
			s = Client_Conn( c );
			assert( s >= 0 );
			assert( s < MAX_CONNECTIONS );
			sock[s] = TRUE;
		}
		cl2chan = Channel_NextMember( Chan, cl2chan );
	}

	/* Senden ... */
	for( i = 0; i < MAX_CONNECTIONS; i++ )
	{
		if( sock[i] )
		{
			ok = Conn_WriteStr( i, ":%s %s", Client_ID( Prefix ), buffer );
			if( ! ok ) break;
		}
	}
	return ok;
} /* IRC_WriteStrChannelPrefix */


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

		if( Client_Type( Client ) == CLIENT_USER )
		{
			/* Nick-Aenderung: allen mitteilen! */
			Log( LOG_INFO, "User \"%s\" changed nick: \"%s\" -> \"%s\".", Client_Mask( target ), Client_ID( target ), Req->argv[0] );
			IRC_WriteStrClient( Client, "NICK :%s", Req->argv[0] );
			IRC_WriteStrServersPrefix( NULL, Client, "NICK :%s", Req->argv[0] );
		}
		else if( Client_Type( Client ) == CLIENT_SERVER )
		{
			/* Nick-Aenderung: allen mitteilen! */
			Log( LOG_DEBUG, "User \"%s\" changed nick: \"%s\" -> \"%s\".", Client_Mask( target ), Client_ID( target ), Req->argv[0] );
			IRC_WriteStrServersPrefix( Client, target, "NICK :%s", Req->argv[0] );
		}
	
		/* Client-Nick registrieren */
		Client_SetID( target, Req->argv[0] );

		if(( Client_Type( target ) != CLIENT_USER ) && ( Client_Type( target ) != CLIENT_SERVER ))
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
			Log( LOG_ERR, "Server %s introduces already registered nick \"%s\"!", Client_ID( Client ), Req->argv[0] );
			Kill_Nick( Req->argv[0] );
			return CONNECTED;
		}

		/* Server, zu dem der Client connectiert ist, suchen */
		intr_c = Client_GetFromToken( Client, atoi( Req->argv[4] ));
		if( ! intr_c )
		{
			Log( LOG_ERR, "Server %s introduces nick \"%s\" on unknown server!?", Client_ID( Client ), Req->argv[0] );
			Kill_Nick( Req->argv[0] );
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
			Kill_Nick( Req->argv[0] );
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
	
	assert( Client != NULL );
	assert( Req != NULL );

	/* SQUIT ist nur fuer Server erlaubt */
	if( Client_Type( Client ) != CLIENT_SERVER ) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	target = Client_GetFromID( Req->argv[0] );
	if( ! target )
	{
		Log( LOG_ERR, "Got SQUIT from %s for unknown server \%s\"!?", Client_ID( Client ), Req->argv[0] );
		return CONNECTED;
	}

	if( target == Client ) Log( LOG_DEBUG, "Got SQUIT from %s: %s", Client_ID( Client ), Req->argv[1] );
	else Log( LOG_DEBUG, "Got SQUIT from %s for %s: %s", Client_ID( Client ), Client_ID( target ), Req->argv[1] );

	/* SQUIT an alle Server weiterleiten */
	IRC_WriteStrServers( Client, "SQUIT %s :%s", Req->argv[0], Req->argv[1] );

	if( Client_Conn( target ) > NONE )
	{
		if( Req->argv[1][0] ) Conn_Close( Client_Conn( target ), "Got SQUIT command.", Req->argv[1], TRUE );
		else Conn_Close( Client_Conn( target ), "Got SQUIT command.", NULL, TRUE );
		return DISCONNECTED;
	}
	else
	{
		Client_Destroy( target, "Got SQUIT command.", Req->argv[1] );
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
			return IRC_WriteStrClientPrefix( Client_NextHop( target ), from, "PING %s :%s", Client_ID( from ), Req->argv[1] );
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
			return IRC_WriteStrClientPrefix( Client_NextHop( target ), from, "PONG %s :%s", Client_ID( from ), Req->argv[1] );
		}
	}

	/* Der Connection-Timestamp wurde schon beim Lesen aus dem Socket
	 * aktualisiert, daher muss das hier nicht mehr gemacht werden. */

	if( Client_Conn( Client ) > NONE ) Log( LOG_DEBUG, "Connection %d: received PONG. Lag: %d seconds.", Client_Conn( Client ), time( NULL ) - Conn_LastPing( Client_Conn( Client )));
	else Log( LOG_DEBUG, "Connection %d: received PONG.", Client_Conn( Client ));

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

	to = Client_Search( Req->argv[0] );
	if( to )
	{
		/* Okay, Ziel ist ein User */
		if( Client_Conn( from ) > NONE ) Conn_UpdateIdle( Client_Conn( from ));
		return IRC_WriteStrClientPrefix( to, from, "PRIVMSG %s :%s", Client_ID( to ), Req->argv[1] );
	}

	chan = Channel_Search( Req->argv[0] );
	if( chan )
	{
		/* Okay, Ziel ist ein Channel */
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


GLOBAL BOOLEAN IRC_MODE( CLIENT *Client, REQUEST *Req )
{
	CHAR *mode_ptr, the_modes[CLIENT_MODE_LEN], x[2];
	CLIENT *cl, *chan_cl, *prefix;
	BOOLEAN set, ok;
	CHANNEL *chan;
	
	assert( Client != NULL );
	assert( Req != NULL );

	cl = chan_cl = prefix = NULL;
	chan = NULL;

	/* Valider Client? */
	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Keine Parameter? */
	if( Req->argc < 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Ziel suchen: Client bzw. Channel */
	if( Client_IsValidNick( Req->argv[0] )) cl = Client_Search( Req->argv[0] );
	if( Channel_IsValidName( Req->argv[0] )) chan = Channel_Search( Req->argv[0] );

	/* Kein Ziel gefunden? */
	if(( ! cl ) && ( ! chan )) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->argv[0] );

	assert(( cl && chan ) != TRUE );

	/* Falsche Anzahl Parameter? */
	if(( cl ) && ( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );
	if(( chan ) && ( Req->argc > 3 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Client ermitteln, wenn bei Channel-Modes mit 3 Parametern */
	if(( chan ) && (Req->argc == 3 ))
	{
		chan_cl = Client_Search( Req->argv[2] );
		if( ! chan_cl ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->argv[0] );
	}
	
	/* Wenn Anfragender ein User ist: Zugriff erlaubt? */
	if( Client_Type( Client ) == CLIENT_USER )
	{
		if( cl )
		{
			/* MODE ist nur fuer sich selber zulaessig! */
			if( cl != Client ) return IRC_WriteStrClient( Client, ERR_USERSDONTMATCH_MSG, Client_ID( Client ));
		}
		if( chan )
		{
			/* Darf der User die Channel-Modes ermitteln? */
		}
	}

	/* Werden die Modes "nur" erfragt? */
	if(( cl ) && ( Req->argc == 1 )) return IRC_WriteStrClient( Client, RPL_UMODEIS_MSG, Client_ID( Client ), Client_Modes( cl ));
	if(( chan ) && ( Req->argc == 1 )) return IRC_WriteStrClient( Client, RPL_CHANNELMODEIS_MSG, Client_ID( Client ), Channel_Name( chan ), Channel_Modes( chan ));

	mode_ptr = Req->argv[1];

	/* Sollen Modes gesetzt oder geloescht werden? */
	if( cl )
	{
		if( *mode_ptr == '+' ) set = TRUE;
		else if( *mode_ptr == '-' ) set = FALSE;
		else return IRC_WriteStrClient( Client, ERR_UMODEUNKNOWNFLAG_MSG, Client_ID( Client ));
		mode_ptr++;
	}
	else
	{
		if( *mode_ptr == '-' ) set = FALSE;
		else set = TRUE;
		if(( *mode_ptr == '-' ) || ( *mode_ptr == '+' )) mode_ptr++;
	}
	
	/* Prefix fuer Antworten etc. ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER )
	{
		prefix = Client_GetFromID( Req->prefix );
		if( ! prefix ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );
	}
	else prefix = Client;

	/* Reply-String mit Aenderungen vorbereiten */
	if( set ) strcpy( the_modes, "+" );
	else strcpy( the_modes, "-" );

	ok = TRUE;
	x[1] = '\0';
	while( *mode_ptr )
	{
		x[0] = '\0';
		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			/* Befehl kommt von einem Server, daher
			 * trauen wir ihm "unbesehen" ... */
			x[0] = *mode_ptr;
		}
		else
		{
			/* Modes validieren */
			if( cl )
			{
				/* User-Modes */
				switch( *mode_ptr )
				{
					case 'i':
						/* invisible */
						x[0] = 'i';
						break;
					case 'r':
						/* restricted (kann nur gesetzt werden) */
						if( set ) x[0] = 'r';
						else ok = IRC_WriteStrClient( Client, ERR_RESTRICTED_MSG, Client_ID( Client ));
						break;
					case 'o':
						/* operator (kann nur geloescht werden) */
						if( ! set )
						{
							Client_SetOperByMe( Client, FALSE );
							x[0] = 'o';
						}
						else ok = IRC_WriteStrClient( Client, ERR_UMODEUNKNOWNFLAG_MSG, Client_ID( Client ));
						break;
					default:
						Log( LOG_DEBUG, "Unknown mode \"%c%c\" from \"%s\"!?", set ? '+' : '-', *mode_ptr, Client_ID( Client ));
						ok = IRC_WriteStrClient( Client, ERR_UMODEUNKNOWNFLAG_MSG, Client_ID( Client ));
						x[0] = '\0';
				}
			}
			if( chan )
			{
				/* Ist der User ein Channel Operator? */
				if( ! strchr( Channel_UserModes( chan, Client ), 'o' ))
				{
					Log( LOG_DEBUG, "Can't change modes: \"%s\" is not operator on %s!", Client_ID( Client ), Channel_Name( chan ));
					ok = IRC_WriteStrClient( Client, ERR_CHANOPRIVSNEEDED_MSG, Client_ID( Client ), Channel_Name( chan ));
					break;
				}
				
				/* Channel-Modes oder Channel-User-Modes */
				if( chan_cl )
				{
					/* Channel-User-Modes */
					switch( *mode_ptr )
					{
						default:
							Log( LOG_DEBUG, "Unknown channel-user-mode \"%c%c\" from \"%s\" on \"%s\" at %s!?", set ? '+' : '-', *mode_ptr, Client_ID( Client ), Client_ID( chan_cl ), Channel_Name( chan ));
							ok = IRC_WriteStrClient( Client, ERR_UMODEUNKNOWNFLAG_MSG, Client_ID( Client ));
							x[0] = '\0';
					}
				}
				else
				{
					/* Channel-Modes */
					switch( *mode_ptr )
					{
						default:
							Log( LOG_DEBUG, "Unknown channel-mode \"%c%c\" from \"%s\" at %s!?", set ? '+' : '-', *mode_ptr, Client_ID( Client ), Channel_Name( chan ));
							ok = IRC_WriteStrClient( Client, ERR_UMODEUNKNOWNFLAG_MSG, Client_ID( Client ));
							x[0] = '\0';
					}
				}
			}
		}
		if( ! ok ) break;
		
		mode_ptr++;
		if( ! x[0] ) continue;

		/* Okay, gueltigen Mode gefunden */
		if( cl )
		{
			/* Es geht um User-Modes */
			if( set )
			{
				/* Mode setzen. Wenn der Client ihn noch nicht hatte: merken */
				if( Client_ModeAdd( cl, x[0] )) strcat( the_modes, x );
			}
			else
			{
				/* Modes geloescht. Wenn der Client ihn hatte: merken */
				if( Client_ModeDel( cl, x[0] )) strcat( the_modes, x );
			}
		}
		if( chan )
		{
			/* Es geht um Channel-Modes oder Channel-User-Modes */
			if( chan_cl )
			{
				/* Channel-User-Modes */
				if( set )
				{
					/* Mode setzen. Wenn der Channel ihn noch nicht hatte: merken */
					if( Channel_UserModeAdd( chan, chan_cl, x[0] )) strcat( the_modes, x );
				}
				else
				{
					/* Mode setzen. Wenn der Channel ihn noch nicht hatte: merken */
					if( Channel_UserModeDel( chan, chan_cl, x[0] )) strcat( the_modes, x );
				}
			}
			else
			{
				/* Channel-Mode */
				if( set )
				{
					/* Mode setzen. Wenn der Channel ihn noch nicht hatte: merken */
					if( Channel_ModeAdd( chan, x[0] )) strcat( the_modes, x );
				}
				else
				{
					/* Mode setzen. Wenn der Channel ihn noch nicht hatte: merken */
					if( Channel_ModeDel( chan, x[0] )) strcat( the_modes, x );
				}
			}
		}
	}

	/* Wurden Modes geaendert? */
	if( the_modes[1] )
	{
		if( cl )
		{
			/* Client-Mode */
			if( Client_Type( Client ) == CLIENT_SERVER )
			{
				/* Modes an andere Server forwarden */
				IRC_WriteStrServersPrefix( Client, prefix, "MODE %s :%s", Client_ID( cl ), the_modes );
			}
			else
			{
				/* Bestaetigung an Client schicken & andere Server informieren */
				ok = IRC_WriteStrClientPrefix( Client, prefix, "MODE %s %s", Client_ID( cl ), the_modes );
				IRC_WriteStrServersPrefix( Client, prefix, "MODE %s :%s", Client_ID( cl ), the_modes );
			}
			Log( LOG_DEBUG, "User \"%s\": Mode change, now \"%s\".", Client_Mask( cl ), Client_Modes( cl ));
		}
		if( chan )
		{
			/* Channel-Modes oder Channel-User-Mode */
			if( chan_cl )
			{
				/* Channel-User-Mode */
				if( Client_Type( Client ) == CLIENT_SERVER )
				{
					/* Modes an andere Server und Channel-User forwarden */
					IRC_WriteStrServersPrefix( Client, prefix, "MODE %s %s :%s", Channel_Name( chan ), the_modes, Client_ID( chan_cl));
					IRC_WriteStrChannelPrefix( Client, chan, prefix, FALSE, "MODE %s %s %s", Channel_Name( chan ), the_modes, Client_ID( chan_cl));
				}
				else
				{
					/* Bestaetigung an Client schicken & andere Server sowie Channel-User informieren */
					ok = IRC_WriteStrClientPrefix( Client, prefix, "MODE %s %s %s", Channel_Name( chan ), the_modes, Client_ID( chan_cl));
					IRC_WriteStrServersPrefix( Client, prefix, "MODE %s %s :%s", Channel_Name( chan ), the_modes, Client_ID( chan_cl));
					IRC_WriteStrChannelPrefix( Client, chan, prefix, FALSE, "MODE %s %s %s", Channel_Name( chan ), the_modes, Client_ID( chan_cl));
				}
				Log( LOG_DEBUG, "User \"%s\" on %s: Mode change, now \"%s\".", Client_Mask( chan_cl), Channel_Name( chan ), Channel_UserModes( chan, chan_cl ));
			}
			else
			{
				/* Channel-Mode */
				if( Client_Type( Client ) == CLIENT_SERVER )
				{
					/* Modes an andere Server und Channel-User forwarden */
					IRC_WriteStrServersPrefix( Client, prefix, "MODE %s :%s", Channel_Name( chan ), the_modes );
					IRC_WriteStrChannelPrefix( Client, chan, prefix, FALSE, "MODE %s %s", Channel_Name( chan ), the_modes );
				}
				else
				{
					/* Bestaetigung an Client schicken & andere Server sowie Channel-User informieren */
					ok = IRC_WriteStrClientPrefix( Client, prefix, "MODE %s %s", Channel_Name( chan ), the_modes );
					IRC_WriteStrServersPrefix( Client, prefix, "MODE %s :%s", Channel_Name( chan ), the_modes );
					IRC_WriteStrChannelPrefix( Client, chan, prefix, FALSE, "MODE %s %s", Channel_Name( chan ), the_modes );
				}
				Log( LOG_DEBUG, "Channel \"%s\": Mode change, now \"%s\".", Channel_Name( chan ), Channel_Modes( chan ));
			}
		}
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
	if( i >= Conf_Oper_Count )
	{
		Log( LOG_WARNING, "Got invalid OPER from \"%s\": Name \"%s\" not configured!", Client_Mask( Client ), Req->argv[0] );
		return IRC_WriteStrClient( Client, ERR_PASSWDMISMATCH_MSG, Client_ID( Client ));
	}

	/* Stimmt das Passwort? */
	if( strcmp( Conf_Oper[i].pwd, Req->argv[1] ) != 0 )
	{
		Log( LOG_WARNING, "Got invalid OPER from \"%s\": Bad password for \"%s\"!", Client_Mask( Client ), Conf_Oper[i].name );
		return IRC_WriteStrClient( Client, ERR_PASSWDMISMATCH_MSG, Client_ID( Client ));
	}
	
	if( ! Client_HasMode( Client, 'o' ))
	{
		/* noch kein o-Mode gesetzt */
		Client_ModeAdd( Client, 'o' );
		if( ! IRC_WriteStrClient( Client, "MODE %s :+o", Client_ID( Client ))) return DISCONNECTED;
		IRC_WriteStrServersPrefix( NULL, Client, "MODE %s :+o", Client_ID( Client ));
	}

	if( ! Client_OperByMe( Client )) Log( LOG_NOTICE, "Got valid OPER from \"%s\", user is an IRC operator now.", Client_Mask( Client ));

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
#ifndef STRICT_RFC
	else if( Client_Conn( c ) == NONE )
	{
		/* Client ist nicht von uns. Ziel-Server suchen */
		target = c;
		ptr = Req->argv[0];
	}
#endif
	else target = NULL;
	
	if( target && ( Client_NextHop( target ) != Client_ThisServer( )) && ( Client_Type( Client_NextHop( target )) == CLIENT_SERVER )) return IRC_WriteStrClientPrefix( target, from, "WHOIS %s :%s", Req->argv[0], ptr );
	
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


GLOBAL BOOLEAN IRC_LUSERS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *from;
	INT cnt;
	
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
	
	/* Users, Services und Serevr im Netz */
	if( ! IRC_WriteStrClient( target, RPL_LUSERCLIENT_MSG, Client_ID( target ), Client_UserCount( ), Client_ServiceCount( ), Client_ServerCount( ))) return DISCONNECTED;

	/* IRC-Operatoren im Netz */
	cnt = Client_OperCount( );
	if( cnt > 0 )
	{
		if( ! IRC_WriteStrClient( target, RPL_LUSEROP_MSG, Client_ID( target ), cnt )) return DISCONNECTED;
	}

	/* Unbekannt Verbindungen */
	cnt = Client_UnknownCount( );
	if( cnt > 0 )
	{
		if( ! IRC_WriteStrClient( target, RPL_LUSERUNKNOWN_MSG, Client_ID( target ), cnt )) return DISCONNECTED;
	}

	/* Channels im Netz */
	if( ! IRC_WriteStrClient( target, RPL_LUSERCHANNELS_MSG, Client_ID( target ), Channel_Count( ))) return DISCONNECTED;

	/* Channels im Netz */
	if( ! IRC_WriteStrClient( target, RPL_LUSERME_MSG, Client_ID( target ), Client_MyUserCount( ), Client_MyServiceCount( ), Client_MyServerCount( ))) return DISCONNECTED;

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


GLOBAL BOOLEAN IRC_JOIN( CLIENT *Client, REQUEST *Req )
{
	CHAR *channame, *flags, modes[8];
	BOOLEAN is_new_chan;
	CLIENT *target;
	CHANNEL *chan;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Wer ist der Absender? */
	if( Client_Type( Client ) == CLIENT_SERVER ) target = Client_GetFromID( Req->prefix );
	else target = Client;
	if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );
	
	/* Channel-Namen durchgehen */
	channame = strtok( Req->argv[0], "," );
	while( channame )
	{
		/* wird der Channel neu angelegt? */
		flags = NULL;

		if( Channel_Search( channame )) is_new_chan = FALSE;
		else is_new_chan = TRUE;

		/* Hat ein Server Channel-User-Modes uebergeben? */
		if( Client_Type( Client ) == CLIENT_SERVER )
		{
			/* Channel-Flags extrahieren */
			flags = strchr( channame, 0x7 );
			if( flags ) *flags++ = '\0';
		}

		/* neuer Channel udn lokaler Client? */
		if( is_new_chan && ( Client_Type( Client ) == CLIENT_USER ))
		{
			/* Dann soll der Client Channel-Operator werden! */
			flags = "o";
		}

		/* Channel joinen (und ggf. anlegen) */
		if( ! Channel_Join( target, channame ))
		{
			/* naechsten Namen ermitteln */
			channame = strtok( NULL, "," );
			continue;
		}
		chan = Channel_Search( channame );
		assert( chan != NULL );

		/* Modes setzen (wenn vorhanden) */
		while( flags && *flags )
		{
			Channel_UserModeAdd( chan, target, *flags );
			flags++;
		}

		/* Muessen Modes an andere Server gemeldet werden? */
		strcpy( &modes[1], Channel_UserModes( chan, target ));
		if( modes[1] ) modes[0] = 0x7;
		else modes[0] = '\0';

		/* An andere Server weiterleiten */
		IRC_WriteStrServersPrefix( Client, target, "JOIN :%s%s", channame, modes );

		/* im Channel bekannt machen */
		IRC_WriteStrChannelPrefix( Client, chan, target, FALSE, "JOIN :%s", channame );
		if( modes[1] )
		{
			/* Modes im Channel bekannt machen */
			IRC_WriteStrChannelPrefix( Client, chan, target, FALSE, "MODE %s %s :%s", channame, modes, Client_ID( target ));
		}

		if( Client_Type( Client ) == CLIENT_USER )
		{
			/* an Client bestaetigen */
			IRC_WriteStrClientPrefix( Client, target, "JOIN :%s", channame );
			/* Topic an Client schicken */
			IRC_WriteStrClient( Client, RPL_TOPIC_MSG, Client_ID( Client ), channame, "What a wonderful channel!" );
			/* Mitglieder an Client Melden */
			Send_NAMES( Client, chan );
		}
		
		/* naechsten Namen ermitteln */
		channame = strtok( NULL, "," );
	}
	return CONNECTED;
} /* IRC_JOIN */


GLOBAL BOOLEAN IRC_PART( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target;
	CHAR *chan;

	assert( Client != NULL );
	assert( Req != NULL );

	if(( Client_Type( Client ) != CLIENT_USER ) && ( Client_Type( Client ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOTREGISTERED_MSG, Client_ID( Client ));

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Wer ist der Absender? */
	if( Client_Type( Client ) == CLIENT_SERVER ) target = Client_GetFromID( Req->prefix );
	else target = Client;
	if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* Channel-Namen durchgehen */
	chan = strtok( Req->argv[0], "," );
	while( chan )
	{
		if( ! Channel_Part( target, Client, chan, Req->argc > 1 ? Req->argv[1] : Client_ID( target )))
		{
			/* naechsten Namen ermitteln */
			chan = strtok( NULL, "," );
			continue;
		}

		/* naechsten Namen ermitteln */
		chan = strtok( NULL, "," );
	}
	return CONNECTED;
} /* IRC_PART */


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
	Log( LOG_ERR, "User(s) with nick \"%s\" will be disconnected!", Nick );
	/* FIXME */
	Log( LOG_ALERT, "[Kill_Nick() not implemented - OOOPS!]" );
} /* Kill_Nick */


LOCAL BOOLEAN Send_NAMES( CLIENT *Client, CHANNEL *Chan )
{
	CHAR str[LINE_LEN + 1];
	CL2CHAN *cl2chan;
	CLIENT *cl;
	
	assert( Client != NULL );
	assert( Chan != NULL );

	/* Alle Mitglieder suchen */
	sprintf( str, RPL_NAMREPLY_MSG, Client_ID( Client ), "=", Channel_Name( Chan ));
	cl2chan = Channel_FirstMember( Chan );
	while( cl2chan )
	{
		cl = Channel_GetClient( cl2chan );

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

		/* naechstes Mitglied suchen */
		cl2chan = Channel_NextMember( Chan, cl2chan );
	}
	if( str[strlen( str ) - 1] != ':')
	{
		/* Es sind noch Daten da, die gesendet werden muessen */
		if( ! IRC_WriteStrClient( Client, str )) return DISCONNECTED;
	}

	/* Ende anzeigen */
	IRC_WriteStrClient( Client, RPL_ENDOFNAMES_MSG, Client_ID( Client ), Channel_Name( Chan ));
	
	return CONNECTED;
} /* Send_NAMES */


/* -eof- */
