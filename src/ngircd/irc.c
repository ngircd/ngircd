/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an comBase beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: irc.c,v 1.4 2001/12/25 19:19:30 alex Exp $
 *
 * irc.c: IRC-Befehle
 *
 * $Log: irc.c,v $
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "client.h"
#include "log.h"
#include "messages.h"
#include "parse.h"

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


GLOBAL BOOLEAN IRC_WriteStr_Client( CLIENT *Client, CLIENT *Prefix, CHAR *Format, ... )
{
	/* Text an Clients, lokal bzw. remote, senden. */

	CHAR buffer[1000];
	BOOLEAN ok = CONNECTED;
	va_list ap;

	assert( Client != NULL );
	assert( Format != NULL );

	va_start( ap, Format );

	if( Client->conn_id != NONE )
	{
		/* Lokaler Client */
		vsnprintf( buffer, 1000, Format, ap );
		if( Prefix ) ok = Conn_WriteStr( Client->conn_id, ":%s %s", Prefix->host, buffer );
		else ok = Conn_WriteStr( Client->conn_id, buffer );
	}
	else
	{
		/* Remote-Client */
		Log( LOG_DEBUG, "not implemented: IRC_WriteStr_Client()" );
	}

	va_end( ap );
	return ok;
} /* IRC_WriteStr_Client */


GLOBAL BOOLEAN IRC_PASS( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	return IRC_WriteStr_Client( Client, This_Server, ERR_UNKNOWNCOMMAND_MSG, Req->command );
} /* IRC_PASS */


GLOBAL BOOLEAN IRC_NICK( CLIENT *Client, REQUEST *Req )
{
	CLIENT *c;
	
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client->type != CLIENT_SERVER && Client->type != CLIENT_SERVICE )
	{
		/* Falsche Anzahl Parameter? */
		if( Req->argc != 1 ) return IRC_WriteStr_Client( Client, This_Server, ERR_NEEDMOREPARAMS_MSG );

		/* Nick zu lang? */
		if( strlen( Req->argv[0] ) > CLIENT_NICK_LEN ) return IRC_WriteStr_Client( Client, This_Server, ERR_ERRONEUSNICKNAME_MSG, Req->argv[0] );

		/* pruefen, ob Nick bereits vergeben */
		c = My_Clients;
		while( c )
		{
			if( strcasecmp( c->nick, Req->argv[0] ) == 0 )
			{
				/* den Nick gibt es bereits */
				return IRC_WriteStr_Client( Client, This_Server, ERR_NICKNAMEINUSE_MSG, Req->argv[0] );
			}
			c = c->next;
		}
		
		/* Client-Nick registrieren */
		strcpy( Client->nick, Req->argv[0] );

		if( Client->type != CLIENT_USER )
		{
			/* Neuer Client */
			if( Client->type == CLIENT_GOTUSER ) return Hello_User( Client );
			else Client->type = CLIENT_GOTNICK;
		}
		return CONNECTED;
	}
	else return IRC_WriteStr_Client( Client, This_Server, ERR_ALREADYREGISTRED_MSG );
} /* IRC_NICK */


GLOBAL BOOLEAN IRC_USER( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client->type == CLIENT_UNKNOWN || Client->type == CLIENT_GOTNICK || Client->type == CLIENT_GOTPASS )
	{
		/* Falsche Anzahl Parameter? */
		if( Req->argc != 4 ) return IRC_WriteStr_Client( Client, This_Server, ERR_NEEDMOREPARAMS_MSG );
		
		strncpy( Client->user, Req->argv[0], CLIENT_USER_LEN );
		Client->user[CLIENT_USER_LEN] = '\0';
		strncpy( Client->name, Req->argv[3], CLIENT_NAME_LEN );
		Client->name[CLIENT_NAME_LEN] = '\0';
		
		if( Client->type == CLIENT_GOTNICK ) return Hello_User( Client );
		else Client->type = CLIENT_GOTUSER;
		return CONNECTED;
	}
	else if( Client->type == CLIENT_USER || Client->type == CLIENT_SERVER || Client->type == CLIENT_SERVICE )
	{
		return IRC_WriteStr_Client( Client, This_Server, ERR_ALREADYREGISTRED_MSG );
	}
	else return IRC_WriteStr_Client( Client, This_Server, ERR_NOTREGISTERED_MSG );
} /* IRC_USER */


GLOBAL BOOLEAN IRC_MOTD( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( ! Check_Valid_User( Client )) return CONNECTED;

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStr_Client( Client, This_Server, ERR_NEEDMOREPARAMS_MSG );
	
	return Show_MOTD( Client );	
} /* IRC_MOTD */


LOCAL BOOLEAN Check_Valid_User( CLIENT *Client )
{
	assert( Client != NULL );

	if( Client->type != CLIENT_USER )
	{
		IRC_WriteStr_Client( Client, This_Server, ERR_NOTREGISTERED_MSG );
		return FALSE;
	}
	else return TRUE;
} /* Check_Valid_User */


LOCAL BOOLEAN Hello_User( CLIENT *Client )
{
	Log( LOG_INFO, "User \"%s!%s@%s\" (%s) registered.", Client->nick, Client->user, Client->host, Client->name );

	IRC_WriteStr_Client( Client, This_Server, RPL_WELCOME_MSG, Client->nick, Client->nick, Client->user, Client->host );
	IRC_WriteStr_Client( Client, This_Server, RPL_YOURHOST_MSG, Client->nick, This_Server->host );
	IRC_WriteStr_Client( Client, This_Server, RPL_CREATED_MSG, Client->nick );
	IRC_WriteStr_Client( Client, This_Server, RPL_MYINFO_MSG, Client->nick, This_Server->host );

	Client->type = CLIENT_USER;

	return Show_MOTD( Client );
} /* Hello_User */


LOCAL BOOLEAN Show_MOTD( CLIENT *Client )
{
	IRC_WriteStr_Client( Client, This_Server, RPL_MOTDSTART_MSG, Client->nick, This_Server->host );
	IRC_WriteStr_Client( Client, This_Server, RPL_MOTD_MSG, Client->nick, "Some cool IRC server welcome message ;-)" );
	return IRC_WriteStr_Client( Client, This_Server, RPL_ENDOFMOTD_MSG, Client->nick );
} /* Show_MOTD */


/* -eof- */
