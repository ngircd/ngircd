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
 * $Id: irc.c,v 1.2 2001/12/23 21:57:16 alex Exp $
 *
 * irc.c: IRC-Befehle
 *
 * $Log: irc.c,v $
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


LOCAL BOOLEAN Check_Valid_User( CLIENT *Client );


GLOBAL VOID IRC_Init( VOID )
{
} /* IRC_Init */


GLOBAL VOID IRC_Exit( VOID )
{
} /* IRC_Exit */


GLOBAL VOID IRC_WriteStr_Client( CLIENT *Client, CLIENT *Prefix, CHAR *Format, ... )
{
	/* Text an Clients, lokal bzw. remote, senden. */

	CHAR buffer[1000];
	va_list ap;

	assert( Client != NULL );
	assert( Format != NULL );

	va_start( ap, Format );

	if( Client->conn_id != NONE )
	{
		/* Lokaler Client */
		vsnprintf( buffer, 1000, Format, ap );
		if( Prefix ) Conn_WriteStr( Client->conn_id, ":%s %s", Prefix->host, buffer );
		else Conn_WriteStr( Client->conn_id, buffer );
	}
	else
	{
		/* Remote-Client */
		Log( LOG_DEBUG, "not implemented: IRC_WriteStr_Client()" );
	}
	va_end( ap );
} /* IRC_WriteStr_Client */


GLOBAL BOOLEAN IRC_PASS( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	IRC_WriteStr_Client( Client, This_Server, ERR_UNKNOWNCOMMAND_MSG, Req->command );
	return TRUE;
} /* IRC_PASS */


GLOBAL BOOLEAN IRC_NICK( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client->type == CLIENT_UNKNOWN || Client->type == CLIENT_PASS )
	{
		/* Neuer Client */
		if( Req->argc == 1 )
		{
			if( strlen( Req->argv[0] ) <= CLIENT_NICK_LEN )
			{
				/* Client-Nick registrieren */
				strcpy( Client->nick, Req->argv[0] );
				Client->type = CLIENT_NICK;
			}
			else IRC_WriteStr_Client( Client, This_Server, ERR_ERRONEUSNICKNAME_MSG, Req->argv[0] );
		}
		else IRC_WriteStr_Client( Client, This_Server, ERR_NEEDMOREPARAMS_MSG );
	}
	else if( Client->type == CLIENT_USER )
	{
		/* Nick-Aenderung eines Users */
		if( ! Check_Valid_User( Client )) return TRUE;
		Log( LOG_DEBUG, "not implemented: IRC_NICK()" );
	}
	else IRC_WriteStr_Client( Client, This_Server, ERR_ALREADYREGISTRED_MSG );

	return TRUE;
} /* IRC_NICK */


GLOBAL BOOLEAN IRC_USER( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Client->type == CLIENT_NICK )
	{
		if( Req->argc == 4 )
		{
			strncpy( Client->user, Req->argv[0], CLIENT_USER_LEN );
			Client->user[CLIENT_USER_LEN] = '\0';
			strncpy( Client->name, Req->argv[3], CLIENT_NAME_LEN );
			Client->name[CLIENT_NAME_LEN] = '\0';
			Log( LOG_INFO, "User \"%s!%s@%s\" (%s) registered.", Client->nick, Client->user, Client->host, Client->name );
			IRC_WriteStr_Client( Client, This_Server, RPL_WELCOME_MSG, Client->nick, Client->user, Client->host );
			IRC_WriteStr_Client( Client, This_Server, RPL_YOURHOST_MSG, This_Server->host );
			IRC_WriteStr_Client( Client, This_Server, RPL_CREATED_MSG );
			IRC_WriteStr_Client( Client, This_Server, RPL_MYINFO_MSG );
		}
		else IRC_WriteStr_Client( Client, This_Server, ERR_NEEDMOREPARAMS_MSG );
	}
	else if( Client->type == CLIENT_USER || Client->type == CLIENT_SERVER || Client->type == CLIENT_SERVICE )
	{
		IRC_WriteStr_Client( Client, This_Server, ERR_ALREADYREGISTRED_MSG );
	}
	else IRC_WriteStr_Client( Client, This_Server, ERR_NOTREGISTERED_MSG );

	return TRUE;
} /* IRC_USER */


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


/* -eof- */
