/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: irc.c,v 1.17 2001/12/31 15:33:13 alex Exp $
 *
 * irc.c: IRC-Befehle
 *
 * $Log: irc.c,v $
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


LOCAL BOOLEAN Check_Valid_User( CLIENT *Client );

LOCAL BOOLEAN Hello_User( CLIENT *Client );
LOCAL BOOLEAN Show_MOTD( CLIENT *Client );


GLOBAL VOID IRC_Init( VOID )
{
} /* IRC_Init */


GLOBAL VOID IRC_Exit( VOID )
{
} /* IRC_Exit */


GLOBAL BOOLEAN IRC_WriteStrClient( CLIENT *Client, CLIENT *Prefix, CHAR *Format, ... )
{
	/* Text an Clients, lokal bzw. remote, senden. */

	CHAR buffer[1000];
	BOOLEAN ok = CONNECTED;
	CONN_ID send_to;
	va_list ap;

	assert( Client != NULL );
	assert( Format != NULL );

	va_start( ap, Format );
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	if( Client->conn_id != NONE ) send_to = Client->conn_id;
	else send_to = Client->introducer->conn_id;

	if( Prefix ) ok = Conn_WriteStr( Client->conn_id, ":%s %s", Client_GetID( Prefix ), buffer );
	else ok = Conn_WriteStr( Client->conn_id, buffer );

	return ok;
} /* IRC_WriteStrClient */


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
	ok = IRC_WriteStrClient( Client, Client, buffer );

	return ok;
} /* IRC_WriteStrRelated */


GLOBAL BOOLEAN IRC_PASS( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client->type == CLIENT_UNKNOWN )
	{
		Log( LOG_DEBUG, "Connection %d: got PASS command ...", Client->conn_id );
		return IRC_WriteStrClient( Client, This_Server, ERR_UNKNOWNCOMMAND_MSG, Client_Nick( Client ), Req->command );
	}
	else return IRC_WriteStrClient( Client, This_Server, ERR_ALREADYREGISTRED_MSG, Client_Nick( Client ));
} /* IRC_PASS */


GLOBAL BOOLEAN IRC_NICK( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	/* Zumindest BitchX sendet NICK-USER in der falschen Reihenfolge. */
#ifndef STRICT_RFC
	if( Client->type == CLIENT_UNKNOWN || Client->type == CLIENT_GOTPASS || Client->type == CLIENT_GOTNICK || Client->type == CLIENT_GOTUSER || Client->type == CLIENT_USER )
#else
	if( Client->type == CLIENT_UNKNOWN || Client->type == CLIENT_GOTPASS || Client->type == CLIENT_GOTNICK || Client->type == CLIENT_USER  )
#endif
	{
		/* Falsche Anzahl Parameter? */
		if( Req->argc != 1 ) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

		/* Ist der Client "restricted"? */
		if( strchr( Client->modes, 'r' )) return IRC_WriteStrClient( Client, This_Server, ERR_RESTRICTED_MSG, Client_Nick( Client ));

		/* Wenn der Client zu seinem eigenen Nick wechseln will, so machen
		 * wir nichts. So macht es das Original und mind. Snak hat probleme,
		 * wenn wir es nicht so machen. Ob es so okay ist? Hm ... */
#ifndef STRICT_RFC
		if( strcasecmp( Client->nick, Req->argv[0] ) == 0 ) return CONNECTED;
#endif
		
		/* pruefen, ob Nick bereits vergeben */
		if( ! Client_CheckNick( Client, Req->argv[0] )) return CONNECTED;

		if( Client->type == CLIENT_USER )
		{
			/* Nick-Aenderung: allen mitteilen! */
			Log( LOG_INFO, "User \"%s!%s@%s\" changed nick: \"%s\" -> \"%s\".", Client->nick, Client->user, Client->host, Client->nick, Req->argv[0] );
			IRC_WriteStrRelated( Client, "NICK :%s", Req->argv[0] );
		}
		
		/* Client-Nick registrieren */
		strcpy( Client->nick, Req->argv[0] );

		if( Client->type != CLIENT_USER )
		{
			/* Neuer Client */
			Log( LOG_DEBUG, "Connection %d: got NICK command ...", Client->conn_id );
			if( Client->type == CLIENT_GOTUSER ) return Hello_User( Client );
			else Client->type = CLIENT_GOTNICK;
		}
		return CONNECTED;
	}
	else return IRC_WriteStrClient( Client, This_Server, ERR_ALREADYREGISTRED_MSG, Client_Nick( Client ));
} /* IRC_NICK */


GLOBAL BOOLEAN IRC_USER( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

#ifndef STRICT_RFC
	if( Client->type == CLIENT_GOTNICK || Client->type == CLIENT_GOTPASS || Client->type == CLIENT_UNKNOWN )
#else
	if( Client->type == CLIENT_GOTNICK || Client->type == CLIENT_GOTPASS )
#endif
	{
		/* Falsche Anzahl Parameter? */
		if( Req->argc != 4 ) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

		strncpy( Client->user, Req->argv[0], CLIENT_USER_LEN );
		Client->user[CLIENT_USER_LEN - 1] = '\0';
		strncpy( Client->name, Req->argv[3], CLIENT_NAME_LEN );
		Client->name[CLIENT_NAME_LEN - 1] = '\0';

		Log( LOG_DEBUG, "Connection %d: got USER command ...", Client->conn_id );
		if( Client->type == CLIENT_GOTNICK ) return Hello_User( Client );
		else Client->type = CLIENT_GOTUSER;
		return CONNECTED;
	}
	else if( Client->type == CLIENT_USER || Client->type == CLIENT_SERVER || Client->type == CLIENT_SERVICE )
	{
		return IRC_WriteStrClient( Client, This_Server, ERR_ALREADYREGISTRED_MSG, Client_Nick( Client ));
	}
	else return IRC_WriteStrClient( Client, This_Server, ERR_NOTREGISTERED_MSG, Client_Nick( Client ));
} /* IRC_USER */


GLOBAL BOOLEAN IRC_QUIT( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client->type != CLIENT_SERVER && Client->type != CLIENT_SERVICE )
	{
		/* Falsche Anzahl Parameter? */
		if( Req->argc > 1 ) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

		Conn_Close( Client->conn_id, "Client wants to quit." );
		return DISCONNECTED;
	}
	else return IRC_WriteStrClient( Client, This_Server, ERR_NOTREGISTERED_MSG, Client_Nick( Client ));
} /* IRC_QUIT */


GLOBAL BOOLEAN IRC_PING( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if( Req->argc < 1 ) return IRC_WriteStrClient( Client, This_Server, ERR_NOORIGIN_MSG, Client_Nick( Client ));
	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

	return IRC_WriteStrClient( Client, This_Server, "PONG %s :%s", Client_Nick( This_Server), Client_Nick( Client ));
} /* IRC_PING */


GLOBAL BOOLEAN IRC_PONG( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if( Req->argc < 1 ) return IRC_WriteStrClient( Client, This_Server, ERR_NOORIGIN_MSG, Client_Nick( Client ));
	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

	/* Der Connection-Timestamp wurde schon beim Lesen aus dem Socket
	 * aktualisiert, daher muss das hier nicht mehr gemacht werden. */

	Log( LOG_DEBUG, "Connection %d: received PONG.", Client->conn_id );
	return CONNECTED;
} /* IRC_PONG */


GLOBAL BOOLEAN IRC_MOTD( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

	return Show_MOTD( Client );
} /* IRC_MOTD */


GLOBAL BOOLEAN IRC_PRIVMSG( CLIENT *Client, REQUEST *Req )
{
	CLIENT *to;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if( Req->argc == 0 ) return IRC_WriteStrClient( Client, This_Server, ERR_NORECIPIENT_MSG, Client_Nick( Client ), Req->command );
	if( Req->argc == 1 ) return IRC_WriteStrClient( Client, This_Server, ERR_NOTEXTTOSEND_MSG, Client_Nick( Client ));
	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

	to = Client_Search( Req->argv[0] );
	if( to )
	{
		/* Okay, Ziel ist ein User */
		if( Client->conn_id >= 0 ) Conn_UpdateIdle( Client->conn_id );
		return IRC_WriteStrClient( to, Client, "PRIVMSG %s :%s", to->nick, Req->argv[1] );
	}

	return IRC_WriteStrClient( Client, This_Server, ERR_NOSUCHNICK_MSG, Client_Nick( Client ), Req->argv[0] );
} /* IRC_PRIVMSG */


GLOBAL BOOLEAN IRC_NOTICE( CLIENT *Client, REQUEST *Req )
{
	CLIENT *to;

	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return CONNECTED;

	to = Client_Search( Req->argv[0] );
	if( to )
	{
		/* Okay, Ziel ist ein User */
		return IRC_WriteStrClient( to, Client, "NOTICE %s :%s", to->nick, Req->argv[1] );
	}

	return CONNECTED;
} /* IRC_NOTICE */


GLOBAL BOOLEAN IRC_MODE( CLIENT *Client, REQUEST *Req )
{
	CHAR x[2], new_modes[CLIENT_MODE_LEN], *ptr, *p;
	BOOLEAN set, ok;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if(( Req->argc > 2 ) || ( Req->argc < 1 )) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

	/* MODE ist nur fuer sich selber zulaessig */
	if( Client_Search( Req->argv[0] ) != Client ) return IRC_WriteStrClient( Client, This_Server, ERR_USERSDONTMATCH_MSG, Client_Nick( Client ));

	/* Werden die Modes erfragt? */
	if( Req->argc == 1 ) return IRC_WriteStrClient( Client, This_Server, RPL_UMODEIS_MSG, Client_Nick( Client ), Client->modes );

	ptr = Req->argv[1];

	/* Sollen Modes gesetzt oder geloescht werden? */
	if( *ptr == '+' ) set = TRUE;
	else if( *ptr == '-' ) set = FALSE;
	else return IRC_WriteStrClient( Client, This_Server, ERR_UMODEUNKNOWNFLAG_MSG, Client_Nick( Client ));

	/* Reply-String mit Aenderungen vorbereiten */
	if( set ) strcpy( new_modes, "+" );
	else strcpy( new_modes, "-" );

	ptr++;
	ok = TRUE;
	x[1] = '\0';
	while( *ptr )
	{
		x[0] = '\0';
		switch( *ptr )
		{
			case 'i':
				/* invisible */
				x[0] = 'i';
				break;
			case 'r':
				/* restricted (kann nur gesetzt werden) */
				if( set ) x[0] = 'r';
				else ok = IRC_WriteStrClient( Client, This_Server, ERR_RESTRICTED_MSG, Client_Nick( Client ));
				break;
			case 'o':
				/* operator (kann nur geloescht werden) */
				if( ! set )
				{
					Client->oper_by_me = FALSE;
					x[0] = 'o';
				}
				else ok = IRC_WriteStrClient( Client, This_Server, ERR_UMODEUNKNOWNFLAG_MSG, Client_Nick( Client ));
				break;
			default:
				ok = IRC_WriteStrClient( Client, This_Server, ERR_UMODEUNKNOWNFLAG_MSG, Client_Nick( Client ));
				x[0] = '\0';
		}
		if( ! ok ) break;

		ptr++;
		if( ! x[0] ) continue;

		/* Okay, gueltigen Mode gefunden */
		if( set )
		{
			/* Modes sollen gesetzt werden */
			if( ! strchr( Client->modes, x[0] ))
			{
				/* Client hat den Mode noch nicht -> setzen */
				strcat( Client->modes, x );
				strcat( new_modes, x );
			}
		}
		else
		{
			/* Modes sollen geloescht werden */
			p = strchr( Client->modes, x[0] );
			if( p )
			{
				/* Client hat den Mode -> loeschen */
				while( *p )
				{
					*p = *(p + 1);
					p++;
				}
				strcat( new_modes, x );
			}
		}
	}
	
	/* Geanderte Modes? */
	if( new_modes[1] && ok )
	{
		ok = IRC_WriteStrRelated( Client, "MODE %s :%s", Client->nick, new_modes );
		Log( LOG_DEBUG, "User \"%s!%s@%s\": Mode change, now \"%s\".", Client->nick, Client->user, Client->host, Client->modes );
	}
	return ok;
} /* IRC_MODE */


GLOBAL BOOLEAN IRC_OPER( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;
	
	/* Falsche Anzahl Parameter? */
	if( Req->argc != 2 ) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );
	
	/* Ist ueberhaupt ein Operator gesetzt? */
	if(( ! Conf_Oper[0] ) || ( ! Conf_OperPwd[0] )) return IRC_WriteStrClient( Client, This_Server, ERR_PASSWDMISMATCH_MSG, Client_Nick( Client ));

	/* Stimmt der Name und das Passwort? */
	if(( strcmp( Conf_Oper, Req->argv[0] ) != 0 ) || ( strcmp( Conf_OperPwd, Req->argv[1] ) != 0 )) return IRC_WriteStrClient( Client, This_Server, ERR_PASSWDMISMATCH_MSG, Client_Nick( Client ));
	
	if( ! strchr( Client->modes, 'o' ))
	{
		/* noch kein o-Mode gesetzt */
		strcat( Client->modes, "o" );
		if( ! IRC_WriteStrRelated( Client, "MODE %s :+o", Client->nick )) return DISCONNECTED;
	}

	if( ! Client->oper_by_me ) Log( LOG_NOTICE, "User \"%s!%s@%s\" is now an IRC Operator.", Client->nick, Client->user, Client->host );

	Client->oper_by_me = TRUE;
	return IRC_WriteStrClient( Client, This_Server, RPL_YOUREOPER_MSG, Client_Nick( Client ));
} /* IRC_OPER */


GLOBAL BOOLEAN IRC_DIE( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

	if(( ! strchr( Client->modes, 'o' )) || ( ! Client->oper_by_me )) return IRC_WriteStrClient( Client, This_Server, ERR_NOPRIVILEGES_MSG, Client_Nick( Client ));

	Log( LOG_NOTICE, "Got DIE command from \"%s!%s@%s\", going down!", Client->nick, Client->user, Client->host );
	NGIRCd_Quit = TRUE;
	return CONNECTED;
} /* IRC_DIE */


GLOBAL BOOLEAN IRC_RESTART( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

	if(( ! strchr( Client->modes, 'o' )) || ( ! Client->oper_by_me )) return IRC_WriteStrClient( Client, This_Server, ERR_NOPRIVILEGES_MSG, Client_Nick( Client ));

	Log( LOG_NOTICE, "Got RESTART command from \"%s!%s@%s\", going down!", Client->nick, Client->user, Client->host );
	NGIRCd_Restart = TRUE;
	return CONNECTED;
} /* IRC_RESTART */


GLOBAL BOOLEAN IRC_NAMES( CLIENT *Client, REQUEST *Req )
{
	CHAR rpl[COMMAND_LEN];
	CLIENT *c;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

	/* Noch alle User ausgeben, die in keinem Channel sind */
	rpl[0] = '\0';
	c = Client_First( );
	while( c )
	{
		if( c->type == CLIENT_USER )
		{
			/* Okay, das ist ein User */
			strcat( rpl, Client_Nick( c ));
			strcat( rpl, " " );
		}

		/* Antwort zu lang? Splitten. */
		if( strlen( rpl ) > 480 )
		{
			if( rpl[strlen( rpl ) - 1] == ' ' ) rpl[strlen( rpl ) - 1] = '\0';
			if( ! IRC_WriteStrClient( Client, This_Server, RPL_NAMREPLY_MSG, Client_Nick( Client ), "*", "*", rpl )) return DISCONNECTED;
			rpl[0] = '\0';
		}
		
		c = Client_Next( c );
	}
	if( rpl[0] )
	{
		/* es wurden User gefunden */
		if( rpl[strlen( rpl ) - 1] == ' ' ) rpl[strlen( rpl ) - 1] = '\0';
		if( ! IRC_WriteStrClient( Client, This_Server, RPL_NAMREPLY_MSG, Client_Nick( Client ), "*", "*", rpl )) return DISCONNECTED;
	}
	return IRC_WriteStrClient( Client, This_Server, RPL_ENDOFNAMES_MSG, Client_Nick( Client ), "*" );
} /* IRC_NAMES */


GLOBAL BOOLEAN IRC_ISON( CLIENT *Client, REQUEST *Req )
{
	CHAR rpl[COMMAND_LEN];
	CLIENT *c;
	CHAR *ptr;
	INT i;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 1 )) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

	strcpy( rpl, RPL_ISON_MSG );
	for( i = 0; i < Req->argc; i++ )
	{
		ptr = strtok( Req->argv[i], " " );
		while( ptr )
		{
			ngt_TrimStr( ptr );
			c = Client_GetFromNick( ptr );
			if( c && ( c->type == CLIENT_USER ))
			{
				/* Dieser Nick ist "online" */
				strcat( rpl, ptr );
				strcat( rpl, " " );
			}
			ptr = strtok( NULL, " " );
		}
	}
	if( rpl[strlen( rpl ) - 1] == ' ' ) rpl[strlen( rpl ) - 1] = '\0';

	return IRC_WriteStrClient( Client, This_Server, rpl, Client->nick );
} /* IRC_ISON */


GLOBAL BOOLEAN IRC_WHOIS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *c;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 1 ) || ( Req->argc > 2 )) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

	/* Client suchen */
	c = Client_GetFromNick( Req->argv[0] );
	if(( ! c ) || ( c->type != CLIENT_USER )) return IRC_WriteStrClient( Client, This_Server, ERR_NOSUCHNICK_MSG, Client_Nick( Client ), Req->argv[0] );
	
	/* Nick, User und Name */
	if( ! IRC_WriteStrClient( Client, This_Server, RPL_WHOISUSER_MSG, Client_Nick( Client ), c->nick, c->user, c->host, c->name )) return DISCONNECTED;

	/* Server */
	if( ! IRC_WriteStrClient( Client, This_Server, RPL_WHOISSERVER_MSG, Client_Nick( Client ), c->nick, c->introducer->nick, c->introducer->info )) return DISCONNECTED;

	/* IRC-Operator? */
	if( strchr( c->modes, 'o' ))
	{
		if( ! IRC_WriteStrClient( Client, This_Server, RPL_WHOISOPERATOR_MSG, Client_Nick( Client ), c->nick )) return DISCONNECTED;
	}

	/* Idle (nur lokale Clients) */
	if( c->conn_id >= 0 )
	{
		if( ! IRC_WriteStrClient( Client, This_Server, RPL_WHOISIDLE_MSG, Client_Nick( Client ), c->nick, Conn_GetIdle( c->conn_id ))) return DISCONNECTED;
	}

	/* End of Whois */
	return IRC_WriteStrClient( Client, This_Server, RPL_ENDOFWHOIS_MSG, Client_Nick( Client ), c->nick );
} /* IRC_WHOIS */


GLOBAL BOOLEAN IRC_USERHOST( CLIENT *Client, REQUEST *Req )
{
	CHAR rpl[COMMAND_LEN];
	CLIENT *c;
	INT max, i;

	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if(( Req->argc < 1 )) return IRC_WriteStrClient( Client, This_Server, ERR_NEEDMOREPARAMS_MSG, Client_Nick( Client ), Req->command );

	if( Req->argc > 5 ) max = 5;
	else max = Req->argc;
	
	strcpy( rpl, RPL_USERHOST_MSG );
	for( i = 0; i < max; i++ )
	{
		c = Client_GetFromNick( Req->argv[i] );
		if( c && ( c->type == CLIENT_USER ))
		{
			/* Dieser Nick ist "online" */
			strcat( rpl, c->nick );
			if( strchr( c->modes, 'o' )) strcat( rpl, "*" );
			strcat( rpl, "=" );
			if( strchr( c->modes, 'a' )) strcat( rpl, "-" );
			else strcat( rpl, "+" );
			strcat( rpl, c->user );
			strcat( rpl, "@" );
			strcat( rpl, c->host );
			strcat( rpl, " " );
		}
	}
	if( rpl[strlen( rpl ) - 1] == ' ' ) rpl[strlen( rpl ) - 1] = '\0';

	return IRC_WriteStrClient( Client, This_Server, rpl, Client->nick );
} /* IRC_USERHOST */	


LOCAL BOOLEAN Check_Valid_User( CLIENT *Client )
{
	assert( Client != NULL );

	if( Client->type != CLIENT_USER )
	{
		IRC_WriteStrClient( Client, This_Server, ERR_NOTREGISTERED_MSG, Client_Nick( Client ));
		return FALSE;
	}
	else return TRUE;
} /* Check_Valid_User */


LOCAL BOOLEAN Hello_User( CLIENT *Client )
{
	assert( Client != NULL );
	assert( Client->nick[0] );
	
	Log( LOG_NOTICE, "User \"%s!%s@%s\" (%s) registered (connection %d).", Client->nick, Client->user, Client->host, Client->name, Client->conn_id );

	IRC_WriteStrClient( Client, This_Server, RPL_WELCOME_MSG, Client->nick, Client_GetID( Client ));
	IRC_WriteStrClient( Client, This_Server, RPL_YOURHOST_MSG, Client->nick, This_Server->nick );
	IRC_WriteStrClient( Client, This_Server, RPL_CREATED_MSG, Client->nick, NGIRCd_StartStr );
	IRC_WriteStrClient( Client, This_Server, RPL_MYINFO_MSG, Client->nick, This_Server->nick );

	Client->type = CLIENT_USER;

	return Show_MOTD( Client );
} /* Hello_User */


LOCAL BOOLEAN Show_MOTD( CLIENT *Client )
{
	BOOLEAN ok;
	CHAR line[127];
	FILE *fd;
	
	assert( Client != NULL );
	assert( Client->nick[0] );

	fd = fopen( Conf_MotdFile, "r" );
	if( ! fd )
	{
		Log( LOG_WARNING, "Can't read MOTD file \"%s\": %s", Conf_MotdFile, strerror( errno ));
		return IRC_WriteStrClient( Client, This_Server, ERR_NOMOTD_MSG, Client->nick );
	}
	
	IRC_WriteStrClient( Client, This_Server, RPL_MOTDSTART_MSG, Client->nick, This_Server->nick );
	while( TRUE )
	{
		if( ! fgets( line, 126, fd )) break;
		if( line[strlen( line ) - 1] == '\n' ) line[strlen( line ) - 1] = '\0';
		IRC_WriteStrClient( Client, This_Server, RPL_MOTD_MSG, Client->nick, line );
	}
	ok = IRC_WriteStrClient( Client, This_Server, RPL_ENDOFMOTD_MSG, Client->nick );

	fclose( fd );
	
	return ok;
} /* Show_MOTD */


/* -eof- */
