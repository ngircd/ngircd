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
 * $Id: irc-oper.c,v 1.11 2002/11/30 15:04:57 alex Exp $
 *
 * irc-oper.c: IRC-Operator-Befehle
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <string.h>

#include "ngircd.h"
#include "resolve.h"
#include "conf.h"
#include "conn.h"
#include "client.h"
#include "channel.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"
#include "parse.h"

#include <exp.h>
#include "irc-oper.h"


GLOBAL BOOLEAN
IRC_OPER( CLIENT *Client, REQUEST *Req )
{
	INT i;

	assert( Client != NULL );
	assert( Req != NULL );

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

	if( ! Client_OperByMe( Client )) Log( LOG_NOTICE|LOG_snotice, "Got valid OPER from \"%s\", user is an IRC operator now.", Client_Mask( Client ));

	Client_SetOperByMe( Client, TRUE );
	return IRC_WriteStrClient( Client, RPL_YOUREOPER_MSG, Client_ID( Client ));
} /* IRC_OPER */


GLOBAL BOOLEAN
IRC_DIE( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if(( ! Client_HasMode( Client, 'o' )) || ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));

	Log( LOG_NOTICE|LOG_snotice, "Got DIE command from \"%s\", going down!", Client_Mask( Client ));
	NGIRCd_Quit = TRUE;
	return CONNECTED;
} /* IRC_DIE */


GLOBAL BOOLEAN
IRC_REHASH( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if(( ! Client_HasMode( Client, 'o' )) || ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));

	Log( LOG_NOTICE|LOG_snotice, "Got REHASH command from \"%s\", re-reading configuration ...", Client_Mask( Client ));
	NGIRCd_Rehash( );
	
	return CONNECTED;
} /* IRC_REHASH */


GLOBAL BOOLEAN
IRC_RESTART( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if(( ! Client_HasMode( Client, 'o' )) || ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));

	Log( LOG_NOTICE|LOG_snotice, "Got RESTART command from \"%s\", going down!", Client_Mask( Client ));
	NGIRCd_Restart = TRUE;
	return CONNECTED;
} /* IRC_RESTART */


GLOBAL BOOLEAN
IRC_CONNECT(CLIENT *Client, REQUEST *Req )
{
	/* Vorlaeufige Version zu Debug-Zwecken: es wird einfach
	 * der "passive mode" aufgehoben, mehr passiert nicht ... */

	assert( Client != NULL );
	assert( Req != NULL );

	/* Falsche Anzahl Parameter? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );
	if(( ! Client_HasMode( Client, 'o' )) || ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));

	Log( LOG_NOTICE|LOG_snotice, "Got CONNECT command from \"%s\".", Client_Mask( Client ));
	NGIRCd_Passive = FALSE;
	return CONNECTED;
} /* IRC_CONNECT */


/* -eof- */
