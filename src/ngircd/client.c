/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2005 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Client management.
 */


#define __client_c__


#include "portab.h"

static char UNUSED id[] = "$Id: client.c,v 1.90 2006/03/24 23:25:38 fw Exp $";

#include "imp.h"
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>

#include "defines.h"
#include "conn.h"

#include "exp.h"
#include "client.h"

#include <imp.h>
#include "ngircd.h"
#include "channel.h"
#include "resolve.h"
#include "conf.h"
#include "hash.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"

#include <exp.h>


#define GETID_LEN (CLIENT_NICK_LEN-1) + 1 + (CLIENT_USER_LEN-1) + 1 + (CLIENT_HOST_LEN-1) + 1


static CLIENT *This_Server, *My_Clients;
static char GetID_Buffer[GETID_LEN];

static WHOWAS My_Whowas[MAX_WHOWAS];
static int Last_Whowas = -1;


static long Count PARAMS(( CLIENT_TYPE Type ));
static long MyCount PARAMS(( CLIENT_TYPE Type ));

static CLIENT *New_Client_Struct PARAMS(( void ));
static void Generate_MyToken PARAMS(( CLIENT *Client ));
static void Adjust_Counters PARAMS(( CLIENT *Client ));

static CLIENT *Init_New_Client PARAMS((CONN_ID Idx, CLIENT *Introducer,
 CLIENT *TopServer, int Type, char *ID, char *User, char *Hostname,
 char *Info, int Hops, int Token, char *Modes, bool Idented));

#ifndef Client_DestroyNow
GLOBAL void Client_DestroyNow PARAMS((CLIENT *Client ));
#endif


long Max_Users = 0, My_Max_Users = 0;


GLOBAL void
Client_Init( void )
{
	struct hostent *h;
	
	This_Server = New_Client_Struct( );
	if( ! This_Server )
	{
		Log( LOG_EMERG, "Can't allocate client structure for server! Going down." );
		Log( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE_NAME );
		exit( 1 );
	}

	/* Client-Struktur dieses Servers */
	This_Server->next = NULL;
	This_Server->type = CLIENT_SERVER;
	This_Server->conn_id = NONE;
	This_Server->introducer = This_Server;
	This_Server->mytoken = 1;
	This_Server->hops = 0;

	gethostname( This_Server->host, CLIENT_HOST_LEN );
	h = gethostbyname( This_Server->host );
	if( h ) strlcpy( This_Server->host, h->h_name, sizeof( This_Server->host ));

	Client_SetID( This_Server, Conf_ServerName );
	Client_SetInfo( This_Server, Conf_ServerInfo );

	My_Clients = This_Server;
	
	memset( &My_Whowas, 0, sizeof( My_Whowas ));
} /* Client_Init */


GLOBAL void
Client_Exit( void )
{
	CLIENT *c, *next;
	int cnt;

	if( NGIRCd_SignalRestart ) Client_Destroy( This_Server, "Server going down (restarting).", NULL, false );
	else Client_Destroy( This_Server, "Server going down.", NULL, false );
	
	cnt = 0;
	c = My_Clients;
	while( c )
	{
		cnt++;
		next = (CLIENT *)c->next;
		free( c );
		c = next;
	}
	if( cnt ) Log( LOG_INFO, "Freed %d client structure%s.", cnt, cnt == 1 ? "" : "s" );
} /* Client_Exit */


GLOBAL CLIENT *
Client_ThisServer( void )
{
	return This_Server;
} /* Client_ThisServer */


/**
 * Initialize new local client; wrapper function for Init_New_Client().
 * @return New CLIENT structure.
 */
GLOBAL CLIENT *
Client_NewLocal(CONN_ID Idx, char *Hostname, int Type, bool Idented)
{
	return Init_New_Client(Idx, This_Server, NULL, Type, NULL, NULL,
		Hostname, NULL, 0, 0, NULL, Idented);
} /* Client_NewLocal */


/**
 * Initialize new remote server; wrapper function for Init_New_Client().
 * @return New CLIENT structure.
 */
GLOBAL CLIENT *
Client_NewRemoteServer(CLIENT *Introducer, char *Hostname, CLIENT *TopServer,
 int Hops, int Token, char *Info, bool Idented)
{
	return Init_New_Client(NONE, Introducer, TopServer, CLIENT_SERVER,
		Hostname, NULL, Hostname, Info, Hops, Token, NULL, Idented);
} /* Client_NewRemoteServer */


/**
 * Initialize new remote client; wrapper function for Init_New_Client().
 * @return New CLIENT structure.
 */
GLOBAL CLIENT *
Client_NewRemoteUser(CLIENT *Introducer, char *Nick, int Hops, char *User,
 char *Hostname, int Token, char *Modes, char *Info, bool Idented)
{
	return Init_New_Client(NONE, Introducer, NULL, CLIENT_USER, Nick,
		User, Hostname, Info, Hops, Token, Modes, Idented);
} /* Client_NewRemoteUser */


/**
 * Initialize new client and set up the given parameters like client type,
 * user name, host name, introducing server etc. ...
 * @return New CLIENT structure.
 */
static CLIENT *
Init_New_Client(CONN_ID Idx, CLIENT *Introducer, CLIENT *TopServer,
 int Type, char *ID, char *User, char *Hostname, char *Info, int Hops,
 int Token, char *Modes, bool Idented)
{
	CLIENT *client;

	assert( Idx >= NONE );
	assert( Introducer != NULL );
	assert( Hostname != NULL );

	client = New_Client_Struct( );
	if( ! client ) return NULL;

	/* Initialisieren */
	client->starttime = time(NULL);
	client->conn_id = Idx;
	client->introducer = Introducer;
	client->topserver = TopServer;
	client->type = Type;
	if( ID ) Client_SetID( client, ID );
	if( User ) Client_SetUser( client, User, Idented );
	if( Hostname ) Client_SetHostname( client, Hostname );
	if( Info ) Client_SetInfo( client, Info );
	client->hops = Hops;
	client->token = Token;
	if( Modes ) Client_SetModes( client, Modes );
	if( Type == CLIENT_SERVER ) Generate_MyToken( client );

	/* ist der User away? */
	if( strchr( client->modes, 'a' )) strlcpy( client->away, DEFAULT_AWAY_MSG, sizeof( client->away ));

	/* Verketten */
	client->next = (POINTER *)My_Clients;
	My_Clients = client;

	/* Adjust counters */
	Adjust_Counters( client );

	return client;
} /* Init_New_Client */


GLOBAL void
Client_Destroy( CLIENT *Client, char *LogMsg, char *FwdMsg, bool SendQuit )
{
	/* Client entfernen. */
	
	CLIENT *last, *c;
	char msg[LINE_LEN], *txt;

	assert( Client != NULL );

	if( LogMsg ) txt = LogMsg;
	else txt = FwdMsg;
	if( ! txt ) txt = "Reason unknown.";

	/* Netz-Split-Nachricht vorbereiten (noch nicht optimal) */
	if( Client->type == CLIENT_SERVER ) {
		strlcpy(msg, This_Server->id, sizeof (msg));
		strlcat(msg, " ", sizeof (msg));
		strlcat(msg, Client->id, sizeof (msg));
	}

	last = NULL;
	c = My_Clients;
	while( c )
	{
		if(( Client->type == CLIENT_SERVER ) && ( c->introducer == Client ) && ( c != Client ))
		{
			/* der Client, der geloescht wird ist ein Server. Der Client, den wir gerade
			 * pruefen, ist ein Child von diesem und muss daher auch entfernt werden */
			Client_Destroy( c, NULL, msg, false );
			last = NULL;
			c = My_Clients;
			continue;
		}
		if( c == Client )
		{
			/* Wir haben den Client gefunden: entfernen */
			if( last ) last->next = c->next;
			else My_Clients = (CLIENT *)c->next;

			if( c->type == CLIENT_USER )
			{
				if( c->conn_id != NONE )
				{
					/* Ein lokaler User */
					Log( LOG_NOTICE, "User \"%s\" unregistered (connection %d): %s", Client_Mask( c ), c->conn_id, txt );

					if( SendQuit )
					{
						/* Alle andere Server informieren! */
						if( FwdMsg ) IRC_WriteStrServersPrefix( NULL, c, "QUIT :%s", FwdMsg );
						else IRC_WriteStrServersPrefix( NULL, c, "QUIT :" );
					}
				}
				else
				{
					/* Remote User */
					Log( LOG_DEBUG, "User \"%s\" unregistered: %s", Client_Mask( c ), txt );

					if( SendQuit )
					{
						/* Andere Server informieren, ausser denen, die "in
						 * Richtung dem liegen", auf dem der User registriert
						 * ist. Von denen haben wir das QUIT ja wohl bekommen. */
						if( FwdMsg ) IRC_WriteStrServersPrefix( Client_NextHop( c ), c, "QUIT :%s", FwdMsg );
						else IRC_WriteStrServersPrefix( Client_NextHop( c ), c, "QUIT :" );
					}
				}

				/* Unregister client from channels */
				Channel_Quit( c, FwdMsg ? FwdMsg : c->id );
				
				/* Register client in My_Whowas structure */
				Client_RegisterWhowas( c );
			}
			else if( c->type == CLIENT_SERVER )
			{
				if( c != This_Server )
				{
					if( c->conn_id != NONE ) Log( LOG_NOTICE|LOG_snotice, "Server \"%s\" unregistered (connection %d): %s", c->id, c->conn_id, txt );
					else Log( LOG_NOTICE|LOG_snotice, "Server \"%s\" unregistered: %s", c->id, txt );
				}

				/* andere Server informieren */
				if( ! NGIRCd_SignalQuit )
				{
					if( FwdMsg ) IRC_WriteStrServersPrefix( Client_NextHop( c ), c, "SQUIT %s :%s", c->id, FwdMsg );
					else IRC_WriteStrServersPrefix( Client_NextHop( c ), c, "SQUIT %s :", c->id );
				}
			}
			else
			{
				if( c->conn_id != NONE )
				{
					if( c->id[0] ) Log( LOG_NOTICE, "Client \"%s\" unregistered (connection %d): %s", c->id, c->conn_id, txt );
					else Log( LOG_NOTICE, "Client unregistered (connection %d): %s", c->conn_id, txt );
				} else {
					Log(LOG_WARNING, "Unregistered unknown client \"%s\": %s",
								c->id[0] ? c->id : "(No Nick)", txt );
				}
			}

			free( c );
			break;
		}
		last = c;
		c = (CLIENT *)c->next;
	}
} /* Client_Destroy */


GLOBAL void
Client_DestroyNow( CLIENT *Client )
{
	/* Destroy client structure immediately. This function is only
	 * intended for the connection layer to remove client structures
	 * of connections that can't be established! */

	CLIENT *last, *c;

	assert( Client != NULL );

	last = NULL;
	c = My_Clients;
	while( c )
	{
		if( c == Client )
		{
			/* Wir haben den Client gefunden: entfernen */
			if( last ) last->next = c->next;
			else My_Clients = (CLIENT *)c->next;
			free( c );
			break;
		}
		last = c;
		c = (CLIENT *)c->next;
	}
} /* Client_DestroyNow */


GLOBAL void
Client_SetHostname( CLIENT *Client, char *Hostname )
{
	/* Hostname eines Clients setzen */
	
	assert( Client != NULL );
	assert( Hostname != NULL );
	
	strlcpy( Client->host, Hostname, sizeof( Client->host ));
} /* Client_SetHostname */


GLOBAL void
Client_SetID( CLIENT *Client, char *ID )
{
	/* Hostname eines Clients setzen, Hash-Wert berechnen */

	assert( Client != NULL );
	assert( ID != NULL );
	
	strlcpy( Client->id, ID, sizeof( Client->id ));

	/* Hash */
	Client->hash = Hash( Client->id );
} /* Client_SetID */


GLOBAL void
Client_SetUser( CLIENT *Client, char *User, bool Idented )
{
	/* Username eines Clients setzen */

	assert( Client != NULL );
	assert( User != NULL );
	
	if( Idented ) strlcpy( Client->user, User, sizeof( Client->user ));
	else
	{
		Client->user[0] = '~';
		strlcpy( Client->user + 1, User, sizeof( Client->user ) - 1 );
	}
} /* Client_SetUser */


GLOBAL void
Client_SetInfo( CLIENT *Client, char *Info )
{
	/* Hostname eines Clients setzen */

	assert( Client != NULL );
	assert( Info != NULL );
	
	strlcpy( Client->info, Info, sizeof( Client->info ));
} /* Client_SetInfo */


GLOBAL void
Client_SetModes( CLIENT *Client, char *Modes )
{
	/* Modes eines Clients setzen */

	assert( Client != NULL );
	assert( Modes != NULL );

	strlcpy( Client->modes, Modes, sizeof( Client->modes ));
} /* Client_SetModes */


GLOBAL void
Client_SetFlags( CLIENT *Client, char *Flags )
{
	/* Flags eines Clients setzen */

	assert( Client != NULL );
	assert( Flags != NULL );

	strlcpy( Client->flags, Flags, sizeof( Client->flags ));
} /* Client_SetFlags */


GLOBAL void
Client_SetPassword( CLIENT *Client, char *Pwd )
{
	/* Von einem Client geliefertes Passwort */

	assert( Client != NULL );
	assert( Pwd != NULL );
	
	strlcpy( Client->pwd, Pwd, sizeof( Client->pwd ));
} /* Client_SetPassword */


GLOBAL void
Client_SetAway( CLIENT *Client, char *Txt )
{
	/* Set AWAY reason of client */

	assert( Client != NULL );
	assert( Txt != NULL );

	strlcpy( Client->away, Txt, sizeof( Client->away ));
	Log( LOG_DEBUG, "User \"%s\" is away: %s", Client_Mask( Client ), Txt );
} /* Client_SetAway */


GLOBAL void
Client_SetType( CLIENT *Client, int Type )
{
	assert( Client != NULL );
	Client->type = Type;
	if( Type == CLIENT_SERVER ) Generate_MyToken( Client );
	Adjust_Counters( Client );
} /* Client_SetType */


GLOBAL void
Client_SetHops( CLIENT *Client, int Hops )
{
	assert( Client != NULL );
	Client->hops = Hops;
} /* Client_SetHops */


GLOBAL void
Client_SetToken( CLIENT *Client, int Token )
{
	assert( Client != NULL );
	Client->token = Token;
} /* Client_SetToken */


GLOBAL void
Client_SetIntroducer( CLIENT *Client, CLIENT *Introducer )
{
	assert( Client != NULL );
	assert( Introducer != NULL );
	Client->introducer = Introducer;
} /* Client_SetIntroducer */


GLOBAL void
Client_SetOperByMe( CLIENT *Client, bool OperByMe )
{
	assert( Client != NULL );
	Client->oper_by_me = OperByMe;
} /* Client_SetOperByMe */


GLOBAL bool
Client_ModeAdd( CLIENT *Client, char Mode )
{
	/* Set Mode.
	 * If Client already alread had Mode, return false.
	 * If the Mode was newly set, return true.
	 */

	char x[2];
	
	assert( Client != NULL );

	x[0] = Mode; x[1] = '\0';
	if( ! strchr( Client->modes, x[0] ))
	{
		/* Client hat den Mode noch nicht -> setzen */
		strlcat( Client->modes, x, sizeof( Client->modes ));
		return true;
	}
	else return false;
} /* Client_ModeAdd */


GLOBAL bool
Client_ModeDel( CLIENT *Client, char Mode )
{
	/* Delete Mode.
	 * If Mode was removed, return true.
	 * If Client did not have Mode, return false.
	 */

	char x[2], *p;

	assert( Client != NULL );

	x[0] = Mode; x[1] = '\0';

	p = strchr( Client->modes, x[0] );
	if( ! p ) return false;

	/* Client has Mode -> delete */
	while( *p )
	{
		*p = *(p + 1);
		p++;
	}
	return true;
} /* Client_ModeDel */


GLOBAL CLIENT *
Client_GetFromConn( CONN_ID Idx )
{
	/* return Client-Structure that belongs to the local Connection Idx.
	 * If none is found, return NULL.
	 */

	CLIENT *c;

	assert( Idx >= 0 );
	
	c = My_Clients;
	while( c )
	{
		if( c->conn_id == Idx ) return c;
		c = (CLIENT *)c->next;
	}
	return NULL;
} /* Client_GetFromConn */


GLOBAL CLIENT *
Client_Search( char *Nick )
{
	/* return Client-Structure that has the corresponding Nick.
	 * If none is found, return NULL.
	 */

	char search_id[CLIENT_ID_LEN], *ptr;
	CLIENT *c = NULL;
	UINT32 search_hash;

	assert( Nick != NULL );

	/* copy Nick and truncate hostmask if necessary */
	strlcpy( search_id, Nick, sizeof( search_id ));
	ptr = strchr( search_id, '!' );
	if( ptr ) *ptr = '\0';

	search_hash = Hash( search_id );

	c = My_Clients;
	while( c )
	{
		if( c->hash == search_hash )
		{
			/* lt. Hash-Wert: Treffer! */
			if( strcasecmp( c->id, search_id ) == 0 ) return c;
		}
		c = (CLIENT *)c->next;
	}
	return NULL;
} /* Client_Search */


GLOBAL CLIENT *
Client_GetFromToken( CLIENT *Client, int Token )
{
	/* Client-Struktur, die den entsprechenden Introducer (=Client)
	 * und das gegebene Token hat, liefern. Wird keine gefunden,
	 * so wird NULL geliefert. */

	CLIENT *c;

	assert( Client != NULL );
	assert( Token > 0 );

	c = My_Clients;
	while( c )
	{
		if(( c->type == CLIENT_SERVER ) && ( c->introducer == Client ) && ( c->token == Token )) return c;
		c = (CLIENT *)c->next;
	}
	return NULL;
} /* Client_GetFromToken */


GLOBAL int
Client_Type( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->type;
} /* Client_Type */


GLOBAL CONN_ID
Client_Conn( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->conn_id;
} /* Client_Conn */


GLOBAL char *
Client_ID( CLIENT *Client )
{
	assert( Client != NULL );

#ifdef DEBUG
	if( Client->type == CLIENT_USER ) assert( strlen( Client->id ) < CLIENT_NICK_LEN );
#endif
						   
	if( Client->id[0] ) return Client->id;
	else return "*";
} /* Client_ID */


GLOBAL char *
Client_Info( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->info;
} /* Client_Info */


GLOBAL char *
Client_User( CLIENT *Client )
{
	assert( Client != NULL );
	if( Client->user[0] ) return Client->user;
	else return "~";
} /* Client_User */


GLOBAL char *
Client_Hostname( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->host;
} /* Client_Hostname */


GLOBAL char *
Client_Password( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->pwd;
} /* Client_Password */


GLOBAL char *
Client_Modes( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->modes;
} /* Client_Modes */


GLOBAL char *
Client_Flags( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->flags;
} /* Client_Flags */


GLOBAL bool
Client_OperByMe( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->oper_by_me;
} /* Client_OperByMe */


GLOBAL int
Client_Hops( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->hops;
} /* Client_Hops */


GLOBAL int
Client_Token( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->token;
} /* Client_Token */


GLOBAL int
Client_MyToken( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->mytoken;
} /* Client_MyToken */


GLOBAL CLIENT *
Client_NextHop( CLIENT *Client )
{
	CLIENT *c;
	
	assert( Client != NULL );

	c = Client;
	while( c->introducer && ( c->introducer != c ) && ( c->introducer != This_Server )) c = c->introducer;
	return c;
} /* Client_NextHop */


GLOBAL char *
Client_Mask( CLIENT *Client )
{
	/* Client-"ID" liefern, wie sie z.B. fuer
	 * Prefixe benoetigt wird. */

	assert( Client != NULL );
	
	if( Client->type == CLIENT_SERVER ) return Client->id;

	snprintf( GetID_Buffer, GETID_LEN, "%s!%s@%s", Client->id, Client->user, Client->host );
	return GetID_Buffer;
} /* Client_Mask */


GLOBAL CLIENT *
Client_Introducer( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->introducer;
} /* Client_Introducer */


GLOBAL CLIENT *
Client_TopServer( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->topserver;
} /* Client_TopServer */


GLOBAL bool
Client_HasMode( CLIENT *Client, char Mode )
{
	assert( Client != NULL );
	return strchr( Client->modes, Mode ) != NULL;
} /* Client_HasMode */


GLOBAL char *
Client_Away( CLIENT *Client )
{
	/* AWAY-Text liefern */

	assert( Client != NULL );
	return Client->away;
} /* Client_Away */


GLOBAL bool
Client_CheckNick( CLIENT *Client, char *Nick )
{
	/* Nick ueberpruefen */

	assert( Client != NULL );
	assert( Nick != NULL );
	
	/* Nick ungueltig? */
	if( ! Client_IsValidNick( Nick ))
	{
		IRC_WriteStrClient( Client, ERR_ERRONEUSNICKNAME_MSG, Client_ID( Client ), Nick );
		return false;
	}

	/* Nick bereits vergeben? */
	if( Client_Search( Nick ))
	{
		/* den Nick gibt es bereits */
		IRC_WriteStrClient( Client, ERR_NICKNAMEINUSE_MSG, Client_ID( Client ), Nick );
		return false;
	}

	return true;
} /* Client_CheckNick */


GLOBAL bool
Client_CheckID( CLIENT *Client, char *ID )
{
	/* Nick ueberpruefen */

	char str[COMMAND_LEN];
	CLIENT *c;

	assert( Client != NULL );
	assert( Client->conn_id > NONE );
	assert( ID != NULL );

	/* Nick zu lang? */
	if( strlen( ID ) > CLIENT_ID_LEN )
	{
		IRC_WriteStrClient( Client, ERR_ERRONEUSNICKNAME_MSG, Client_ID( Client ), ID );
		return false;
	}

	/* ID bereits vergeben? */
	c = My_Clients;
	while( c )
	{
		if( strcasecmp( c->id, ID ) == 0 )
		{
			/* die Server-ID gibt es bereits */
			snprintf( str, sizeof( str ), "ID \"%s\" already registered", ID );
			if( Client->conn_id != c->conn_id ) Log( LOG_ERR, "%s (on connection %d)!", str, c->conn_id );
			else Log( LOG_ERR, "%s (via network)!", str );
			Conn_Close( Client->conn_id, str, str, true);
			return false;
		}
		c = (CLIENT *)c->next;
	}

	return true;
} /* Client_CheckID */


GLOBAL CLIENT *
Client_First( void )
{
	/* Ersten Client liefern. */

	return My_Clients;
} /* Client_First */


GLOBAL CLIENT *
Client_Next( CLIENT *c )
{
	/* Naechsten Client liefern. Existiert keiner,
	 * so wird NULL geliefert. */

	assert( c != NULL );
	return (CLIENT *)c->next;
} /* Client_Next */


GLOBAL long
Client_UserCount( void )
{
	return Count( CLIENT_USER );
} /* Client_UserCount */


GLOBAL long
Client_ServiceCount( void )
{
	return Count( CLIENT_SERVICE );;
} /* Client_ServiceCount */


GLOBAL long
Client_ServerCount( void )
{
	return Count( CLIENT_SERVER );
} /* Client_ServerCount */


GLOBAL long
Client_MyUserCount( void )
{
	return MyCount( CLIENT_USER );
} /* Client_MyUserCount */


GLOBAL long
Client_MyServiceCount( void )
{
	return MyCount( CLIENT_SERVICE );
} /* Client_MyServiceCount */


GLOBAL long
Client_MyServerCount( void )
{
	CLIENT *c;
	long cnt;

	cnt = 0;
	c = My_Clients;
	while( c )
	{
		if(( c->type == CLIENT_SERVER ) && ( c->hops == 1 )) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* Client_MyServerCount */


GLOBAL long
Client_OperCount( void )
{
	CLIENT *c;
	long cnt;

	cnt = 0;
	c = My_Clients;
	while( c )
	{
		if( c && ( c->type == CLIENT_USER ) && ( strchr( c->modes, 'o' ))) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* Client_OperCount */


GLOBAL long
Client_UnknownCount( void )
{
	CLIENT *c;
	long cnt;

	cnt = 0;
	c = My_Clients;
	while( c )
	{
		if( c && ( c->type != CLIENT_USER ) && ( c->type != CLIENT_SERVICE ) && ( c->type != CLIENT_SERVER )) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* Client_UnknownCount */


GLOBAL long
Client_MaxUserCount( void )
{
	return Max_Users;
} /* Client_MaxUserCount */


GLOBAL long
Client_MyMaxUserCount( void )
{
	return My_Max_Users;
} /* Client_MyMaxUserCount */


GLOBAL bool
Client_IsValidNick( char *Nick )
{
	/* Ist der Nick gueltig? */

	char *ptr, goodchars[20];
	
	assert( Nick != NULL );

	strcpy( goodchars, ";0123456789-" );

	if( Nick[0] == '#' ) return false;
	if( strchr( goodchars, Nick[0] )) return false;
	if( strlen( Nick ) >= CLIENT_NICK_LEN ) return false;

	ptr = Nick;
	while( *ptr )
	{
		if(( *ptr < 'A' ) && ( ! strchr( goodchars, *ptr ))) return false;
		if(( *ptr > '}' ) && ( ! strchr( goodchars, *ptr ))) return false;
		ptr++;
	}
	
	return true;
} /* Client_IsValidNick */


/**
 * Return pointer to "My_Whowas" structure.
 */
GLOBAL WHOWAS *
Client_GetWhowas( void )
{
	return My_Whowas;
} /* Client_GetWhowas */

/**
 * Return the index of the last used WHOWAS entry.
 */
GLOBAL int
Client_GetLastWhowasIndex( void )
{
	return Last_Whowas;
} /* Client_GetLastWhowasIndex */


/**
 * Get the start time of this client.
 * The result is the start time in seconds since 1970-01-01, as reported
 * by the C function time(NULL).
 */
GLOBAL time_t
Client_StartTime(CLIENT *Client)
{
	assert( Client != NULL );
	return Client->starttime;
} /* Client_Uptime */


static long
Count( CLIENT_TYPE Type )
{
	CLIENT *c;
	long cnt;

	cnt = 0;
	c = My_Clients;
	while( c )
	{
		if( c->type == Type ) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* Count */


static long
MyCount( CLIENT_TYPE Type )
{
	CLIENT *c;
	long cnt;

	cnt = 0;
	c = My_Clients;
	while( c )
	{
		if(( c->introducer == This_Server ) && ( c->type == Type )) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* MyCount */


static CLIENT *
New_Client_Struct( void )
{
	/* Neue CLIENT-Struktur pre-initialisieren */
	
	CLIENT *c;
	
	c = (CLIENT *)malloc( sizeof( CLIENT ));
	if( ! c )
	{
		Log( LOG_EMERG, "Can't allocate memory! [New_Client_Struct]" );
		return NULL;
	}

	memset( c, 0, sizeof ( CLIENT ));

	c->type = CLIENT_UNKNOWN;
	c->conn_id = NONE;
	c->oper_by_me = false;
	c->hops = -1;
	c->token = -1;
	c->mytoken = -1;

	return c;
} /* New_Client */


static void
Generate_MyToken( CLIENT *Client )
{
	CLIENT *c;
	int token;

	c = My_Clients;
	token = 2;
	while( c )
	{
		if( c->mytoken == token )
		{
			/* Das Token wurde bereits vergeben */
			token++;
			c = My_Clients;
			continue;
		}
		else c = (CLIENT *)c->next;
	}
	Client->mytoken = token;
	Log( LOG_DEBUG, "Assigned token %d to server \"%s\".", token, Client->id );
} /* Generate_MyToken */


static void
Adjust_Counters( CLIENT *Client )
{
	long count;

	assert( Client != NULL );

	if( Client->type != CLIENT_USER ) return;
	
	if( Client->conn_id != NONE )
	{
		/* Local connection */
		count = Client_MyUserCount( );
		if( count > My_Max_Users ) My_Max_Users = count;
	}
	count = Client_UserCount( );
	if( count > Max_Users ) Max_Users = count;
} /* Adjust_Counters */


/**
 * Register client in My_Whowas structure for further recall by WHOWAS.
 * Note: Only clients that have been connected at least 30 seconds will be
 * registered to prevent automated IRC bots to "destroy" a nice server
 * history database.
 */
GLOBAL void
Client_RegisterWhowas( CLIENT *Client )
{
	int slot;
	time_t now;
	
	assert( Client != NULL );

	now = time(NULL);
	/* Don't register clients that were connected less than 30 seconds. */
	if( now - Client->starttime < 30 )
		return;

	slot = Last_Whowas + 1;
	if( slot >= MAX_WHOWAS || slot < 0 ) slot = 0;

#ifdef DEBUG
	Log( LOG_DEBUG, "Saving WHOWAS information to slot %d ...", slot );
#endif
	
	My_Whowas[slot].time = now;
	strlcpy( My_Whowas[slot].id, Client_ID( Client ),
		 sizeof( My_Whowas[slot].id ));
	strlcpy( My_Whowas[slot].user, Client_User( Client ),
		 sizeof( My_Whowas[slot].user ));
	strlcpy( My_Whowas[slot].host, Client_Hostname( Client ),
		 sizeof( My_Whowas[slot].host ));
	strlcpy( My_Whowas[slot].info, Client_Info( Client ),
		 sizeof( My_Whowas[slot].info ));
	strlcpy( My_Whowas[slot].server, Client_ID( Client_Introducer( Client )),
		 sizeof( My_Whowas[slot].server ));
	
	Last_Whowas = slot;
} /* Client_RegisterWhowas */


/* -eof- */
