/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * IRC operator commands
 */


#include "portab.h"

static char UNUSED id[] = "$Id: irc-oper.c,v 1.20 2005/03/19 18:43:48 fw Exp $";

#include "imp.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ngircd.h"
#include "resolve.h"
#include "conn.h"
#include "conf.h"
#include "client.h"
#include "channel.h"
#include "irc-write.h"
#include "log.h"
#include "match.h"
#include "messages.h"
#include "parse.h"

#include <exp.h>
#include "irc-oper.h"


GLOBAL bool
IRC_OPER( CLIENT *Client, REQUEST *Req )
{
	int i;

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

	/* Authorized Mask? */
	if( Conf_Oper[i].mask && (! Match( Conf_Oper[i].mask, Client_Mask( Client ) ))) {
		Log( LOG_WARNING, "Rejected valid OPER for \"%s\": Mask mismatch (got: \"%s\", want: \"%s\")!", Conf_Oper[i].name, Client_Mask( Client ), Conf_Oper[i].mask );
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

	Client_SetOperByMe( Client, true);
	return IRC_WriteStrClient( Client, RPL_YOUREOPER_MSG, Client_ID( Client ));
} /* IRC_OPER */


GLOBAL bool
IRC_DIE( CLIENT *Client, REQUEST *Req )
{
	/* Shut down server */

	assert( Client != NULL );
	assert( Req != NULL );

	/* Not a local IRC operator? */
	if(( ! Client_HasMode( Client, 'o' )) || ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));
	
	/* Bad number of parameters? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	Log( LOG_NOTICE|LOG_snotice, "Got DIE command from \"%s\" ...", Client_Mask( Client ));
	NGIRCd_SignalQuit = true;
	return CONNECTED;
} /* IRC_DIE */


GLOBAL bool
IRC_REHASH( CLIENT *Client, REQUEST *Req )
{
	/* Reload configuration file */

	assert( Client != NULL );
	assert( Req != NULL );

	/* Not a local IRC operator? */
	if(( ! Client_HasMode( Client, 'o' )) || ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));

	/* Bad number of parameters? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	Log( LOG_NOTICE|LOG_snotice, "Got REHASH command from \"%s\" ...", Client_Mask( Client ));
	NGIRCd_SignalRehash = true;
	
	return CONNECTED;
} /* IRC_REHASH */


GLOBAL bool
IRC_RESTART( CLIENT *Client, REQUEST *Req )
{
	/* Restart IRC server (fork a new process) */

	assert( Client != NULL );
	assert( Req != NULL );

	/* Not a local IRC operator? */
	if(( ! Client_HasMode( Client, 'o' )) || ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));

	/* Bad number of parameters? */
	if( Req->argc != 0 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	Log( LOG_NOTICE|LOG_snotice, "Got RESTART command from \"%s\" ...", Client_Mask( Client ));
	NGIRCd_SignalRestart = true;
	return CONNECTED;
} /* IRC_RESTART */


GLOBAL bool
IRC_CONNECT(CLIENT *Client, REQUEST *Req )
{
	/* Connect configured or new server */

	assert( Client != NULL );
	assert( Req != NULL );

	/* Not a local IRC operator? */
	if(( ! Client_HasMode( Client, 'o' )) || ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));

	/* Bad number of parameters? */
	if(( Req->argc != 2 ) && ( Req->argc != 5 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Invalid port number? */
	if( atoi( Req->argv[1] ) < 1 )  return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	Log( LOG_NOTICE|LOG_snotice, "Got CONNECT command from \"%s\" for \"%s\".", Client_Mask( Client ), Req->argv[0]);

	if( Req->argc == 2 )
	{
		/* Connect configured server */
		if( ! Conf_EnableServer( Req->argv[0], atoi( Req->argv[1] ))) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[0] );
	}
	else
	{
		/* Add server */
		if( ! Conf_AddServer( Req->argv[0], atoi( Req->argv[1] ), Req->argv[2], Req->argv[3], Req->argv[4] )) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[0] );
	}
	return CONNECTED;
} /* IRC_CONNECT */


GLOBAL bool
IRC_DISCONNECT(CLIENT *Client, REQUEST *Req )
{
	/* Disconnect and disable configured server */

	CONN_ID my_conn;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Not a local IRC operator? */
	if(( ! Client_HasMode( Client, 'o' )) || ( ! Client_OperByMe( Client ))) return IRC_WriteStrClient( Client, ERR_NOPRIVILEGES_MSG, Client_ID( Client ));

	/* Bad number of parameters? */
	if( Req->argc != 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	Log( LOG_NOTICE|LOG_snotice, "Got DISCONNECT command from \"%s\" for0 \"%s\".", Client_Mask( Client ), Req->argv[0]);

	/* Save ID of this connection */
	my_conn = Client_Conn( Client );

	/* Connect configured server */
	if( ! Conf_DisableServer( Req->argv[0] )) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[0] );

	/* Are we still connected or were we killed, too? */
	if( Client_GetFromConn( my_conn )) return CONNECTED;
	else return DISCONNECTED;
} /* IRC_CONNECT */


/* -eof- */
