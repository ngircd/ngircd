/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton (alex@barton.de)
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
#include "conf.h"
#include "hash.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"

#include <exp.h>


#define GETID_LEN (CLIENT_NICK_LEN-1) + 1 + (CLIENT_USER_LEN-1) + 1 + (CLIENT_HOST_LEN-1) + 1


static CLIENT *This_Server, *My_Clients;

static WHOWAS My_Whowas[MAX_WHOWAS];
static int Last_Whowas = -1;
static long Max_Users, My_Max_Users;


static unsigned long Count PARAMS(( CLIENT_TYPE Type ));
static unsigned long MyCount PARAMS(( CLIENT_TYPE Type ));

static CLIENT *New_Client_Struct PARAMS(( void ));
static void Generate_MyToken PARAMS(( CLIENT *Client ));
static void Adjust_Counters PARAMS(( CLIENT *Client ));

static CLIENT *Init_New_Client PARAMS((CONN_ID Idx, CLIENT *Introducer,
				       CLIENT *TopServer, int Type, const char *ID,
				       const char *User, const char *Hostname, const char *Info,
				       int Hops, int Token, const char *Modes,
				       bool Idented));

static void Destroy_UserOrService PARAMS((CLIENT *Client,const char *Txt, const char *FwdMsg,
					bool SendQuit));


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
	if (!Conf_NoDNS) {
		h = gethostbyname( This_Server->host );
		if (h) strlcpy(This_Server->host, h->h_name, sizeof(This_Server->host));
	}
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
Client_NewLocal(CONN_ID Idx, const char *Hostname, int Type, bool Idented)
{
	return Init_New_Client(Idx, This_Server, NULL, Type, NULL, NULL,
		Hostname, NULL, 0, 0, NULL, Idented);
} /* Client_NewLocal */


/**
 * Initialize new remote server; wrapper function for Init_New_Client().
 * @return New CLIENT structure.
 */
GLOBAL CLIENT *
Client_NewRemoteServer(CLIENT *Introducer, const char *Hostname, CLIENT *TopServer,
 int Hops, int Token, const char *Info, bool Idented)
{
	return Init_New_Client(NONE, Introducer, TopServer, CLIENT_SERVER,
		Hostname, NULL, Hostname, Info, Hops, Token, NULL, Idented);
} /* Client_NewRemoteServer */


/**
 * Initialize new remote client; wrapper function for Init_New_Client().
 * @return New CLIENT structure.
 */
GLOBAL CLIENT *
Client_NewRemoteUser(CLIENT *Introducer, const char *Nick, int Hops, const char *User,
 const char *Hostname, int Token, const char *Modes, const char *Info, bool Idented)
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
  int Type, const char *ID, const char *User, const char *Hostname,
  const char *Info, int Hops, int Token, const char *Modes, bool Idented)
{
	CLIENT *client;

	assert(Idx >= NONE);
	assert(Introducer != NULL);
	assert(Hostname != NULL);

	client = New_Client_Struct();
	if (!client)
		return NULL;

	client->starttime = time(NULL);
	client->conn_id = Idx;
	client->introducer = Introducer;
	client->topserver = TopServer;
	client->type = Type;
	if (ID)
		Client_SetID(client, ID);
	if (User) {
		Client_SetUser(client, User, Idented);
		Client_SetOrigUser(client, User);
	}
	if (Hostname)
		Client_SetHostname(client, Hostname);
	if (Info)
		Client_SetInfo(client, Info);
	client->hops = Hops;
	client->token = Token;
	if (Modes)
		Client_SetModes(client, Modes);
	if (Type == CLIENT_SERVER)
		Generate_MyToken(client);

	if (strchr(client->modes, 'a'))
		strlcpy(client->away, DEFAULT_AWAY_MSG, sizeof(client->away));

	client->next = (POINTER *)My_Clients;
	My_Clients = client;

	Adjust_Counters(client);

	return client;
} /* Init_New_Client */


GLOBAL void
Client_Destroy( CLIENT *Client, const char *LogMsg, const char *FwdMsg, bool SendQuit )
{
	/* remove a client */
	
	CLIENT *last, *c;
	char msg[LINE_LEN];
	const char *txt;

	assert( Client != NULL );

	if( LogMsg ) txt = LogMsg;
	else txt = FwdMsg;
	if( ! txt ) txt = "Reason unknown.";

	/* netsplit message */
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
			/*
			 * The client that is about to be removed is a server,
			 * the client we are checking right now is a child of that
			 * server and thus has to be removed, too.
			 *
			 * Call Client_Destroy() recursively with the server as the
			 * new "object to be removed". This starts the cycle again, until
			 * all servers that are linked via the original server have been
			 * removed.
			 */
			Client_Destroy( c, NULL, msg, false );
			last = NULL;
			c = My_Clients;
			continue;
		}
		if( c == Client )
		{
			/* found  the client: remove it */
			if( last ) last->next = c->next;
			else My_Clients = (CLIENT *)c->next;

			if(c->type == CLIENT_USER || c->type == CLIENT_SERVICE)
				Destroy_UserOrService(c, txt, FwdMsg, SendQuit);
			else if( c->type == CLIENT_SERVER )
			{
				if( c != This_Server )
				{
					if( c->conn_id != NONE ) Log( LOG_NOTICE|LOG_snotice, "Server \"%s\" unregistered (connection %d): %s", c->id, c->conn_id, txt );
					else Log( LOG_NOTICE|LOG_snotice, "Server \"%s\" unregistered: %s", c->id, txt );
				}

				/* inform other servers */
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
Client_SetHostname( CLIENT *Client, const char *Hostname )
{
	assert( Client != NULL );
	assert( Hostname != NULL );

	strlcpy( Client->host, Hostname, sizeof( Client->host ));
} /* Client_SetHostname */


GLOBAL void
Client_SetID( CLIENT *Client, const char *ID )
{
	assert( Client != NULL );
	assert( ID != NULL );
	
	strlcpy( Client->id, ID, sizeof( Client->id ));

	/* Hash */
	Client->hash = Hash( Client->id );
} /* Client_SetID */


GLOBAL void
Client_SetUser( CLIENT *Client, const char *User, bool Idented )
{
	/* set clients username */

	assert( Client != NULL );
	assert( User != NULL );

	if (Idented) {
		strlcpy(Client->user, User, sizeof(Client->user));
	} else {
		Client->user[0] = '~';
		strlcpy(Client->user + 1, User, sizeof(Client->user) - 1);
	}
} /* Client_SetUser */


/**
 * Set "original" user name of a client.
 * This function saves the "original" user name, the user name specified by
 * the peer using the USER command, into the CLIENT structure. This user
 * name may be used for authentication, for example.
 * @param Client The client.
 * @param User User name to set.
 */
GLOBAL void
Client_SetOrigUser(CLIENT UNUSED *Client, const char UNUSED *User) {
	assert(Client != NULL);
	assert(User != NULL);

#if defined(PAM) && defined(IDENTAUTH)
	strlcpy(Client->orig_user, User, sizeof(Client->orig_user));
#endif
} /* Client_SetOrigUser */


GLOBAL void
Client_SetInfo( CLIENT *Client, const char *Info )
{
	/* set client hostname */

	assert( Client != NULL );
	assert( Info != NULL );

	strlcpy(Client->info, Info, sizeof(Client->info));
} /* Client_SetInfo */


GLOBAL void
Client_SetModes( CLIENT *Client, const char *Modes )
{
	assert( Client != NULL );
	assert( Modes != NULL );

	strlcpy(Client->modes, Modes, sizeof( Client->modes ));
} /* Client_SetModes */


GLOBAL void
Client_SetFlags( CLIENT *Client, const char *Flags )
{
	assert( Client != NULL );
	assert( Flags != NULL );

	strlcpy(Client->flags, Flags, sizeof(Client->flags));
} /* Client_SetFlags */


GLOBAL void
Client_SetPassword( CLIENT *Client, const char *Pwd )
{
	/* set password sent by client */

	assert( Client != NULL );
	assert( Pwd != NULL );

	strlcpy(Client->pwd, Pwd, sizeof(Client->pwd));
} /* Client_SetPassword */


GLOBAL void
Client_SetAway( CLIENT *Client, const char *Txt )
{
	/* Set AWAY reason of client */

	assert( Client != NULL );
	assert( Txt != NULL );

	strlcpy( Client->away, Txt, sizeof( Client->away ));
	LogDebug("%s \"%s\" is away: %s", Client_TypeText(Client),
		 Client_Mask(Client), Txt);
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
	if (!strchr( Client->modes, x[0])) {
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
Client_Search( const char *Nick )
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

	search_hash = Hash(search_id);

	c = My_Clients;
	while (c) {
		if (c->hash == search_hash && strcasecmp(c->id, search_id) == 0)
			return c;
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
	while (c) {
		if ((c->type == CLIENT_SERVER) && (c->introducer == Client) &&
			(c->token == Token))
				return c;
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
	if(Client->type == CLIENT_USER)
		assert(strlen(Client->id) < Conf_MaxNickLength);
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
	return Client->user[0] ? Client->user : "~";
} /* Client_User */


#ifdef PAM

/**
 * Get the "original" user name as supplied by the USER command.
 * The user name as given by the client is used for authentication instead
 * of the one detected using IDENT requests.
 * @param Client The client.
 * @return Original user name.
 */
GLOBAL char *
Client_OrigUser(CLIENT *Client) {
#ifndef IDENTAUTH
	char *user = Client->user;

	if (user[0] == '~')
		user++;
	return user;
#else
	return Client->orig_user;
#endif
} /* Client_OrigUser */

#endif


/**
 * Return the hostname of a client.
 * @param Client Pointer to client structure
 * @return Pointer to client hostname
 */
GLOBAL char *
Client_Hostname(CLIENT *Client)
{
	assert (Client != NULL);
	return Client->host;
} /* Client_Hostname */


/**
 * Get potentially cloaked hostname of a client.
 * If the client has not enabled cloaking, the real hostname is used.
 * @param Client Pointer to client structure
 * @return Pointer to client hostname
 */
GLOBAL char *
Client_HostnameCloaked(CLIENT *Client)
{
	assert(Client != NULL);
	if (Client_HasMode(Client, 'x'))
		return Client_ID(Client->introducer);
	else
		return Client_Hostname(Client);
} /* Client_HostnameCloaked */


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
	while( c->introducer && ( c->introducer != c ) && ( c->introducer != This_Server ))
		c = c->introducer;

	return c;
} /* Client_NextHop */


/**
 * Return ID of a client: "client!user@host"
 * This client ID is used for IRC prefixes, for example.
 * Please note that this function uses a global static buffer, so you can't
 * nest invocations without overwriting erlier results!
 * @param Client Pointer to client structure
 * @return Pointer to global buffer containing the client ID
 */
GLOBAL char *
Client_Mask( CLIENT *Client )
{
	static char Mask_Buffer[GETID_LEN];

	assert (Client != NULL);

	/* Servers: return name only, there is no "mask" */
	if (Client->type == CLIENT_SERVER)
		return Client->id;

	snprintf(Mask_Buffer, GETID_LEN, "%s!%s@%s",
		 Client->id, Client->user, Client->host);
	return Mask_Buffer;
} /* Client_Mask */


/**
 * Return ID of a client with cloaked hostname: "client!user@server-name"
 * This client ID is used for IRC prefixes, for example.
 * Please note that this function uses a global static buffer, so you can't
 * nest invocations without overwriting erlier results!
 * If the client has not enabled cloaking, the real hostname is used.
 * @param Client Pointer to client structure
 * @return Pointer to global buffer containing the client ID
 */
GLOBAL char *
Client_MaskCloaked(CLIENT *Client)
{
	static char Mask_Buffer[GETID_LEN];

	assert (Client != NULL);

	/* Is the client using cloaking at all? */
	if (!Client_HasMode(Client, 'x'))
	    return Client_Mask(Client);

	snprintf(Mask_Buffer, GETID_LEN, "%s!%s@%s",
		 Client->id, Client->user, Client_ID(Client->introducer));
	return Mask_Buffer;
} /* Client_MaskCloaked */


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
	assert( Client != NULL );
	return Client->away;
} /* Client_Away */


GLOBAL bool
Client_CheckNick( CLIENT *Client, char *Nick )
{
	assert( Client != NULL );
	assert( Nick != NULL );

	if (! Client_IsValidNick( Nick ))
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
	char str[COMMAND_LEN];
	CLIENT *c;

	assert( Client != NULL );
	assert( Client->conn_id > NONE );
	assert( ID != NULL );

	/* ID too long? */
	if (strlen(ID) > CLIENT_ID_LEN) {
		IRC_WriteStrClient(Client, ERR_ERRONEUSNICKNAME_MSG, Client_ID(Client), ID);
		return false;
	}

	/* ID already in use? */
	c = My_Clients;
	while (c) {
		if (strcasecmp(c->id, ID) == 0) {
			snprintf(str, sizeof(str), "ID \"%s\" already registered", ID);
			if (c->conn_id != NONE)
				Log(LOG_ERR, "%s (on connection %d)!", str, c->conn_id);
			else
				Log(LOG_ERR, "%s (via network)!", str);
			Conn_Close(Client->conn_id, str, str, true);
			return false;
		}
		c = (CLIENT *)c->next;
	}

	return true;
} /* Client_CheckID */


GLOBAL CLIENT *
Client_First( void )
{
	return My_Clients;
} /* Client_First */


GLOBAL CLIENT *
Client_Next( CLIENT *c )
{
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


GLOBAL unsigned long
Client_MyServerCount( void )
{
	CLIENT *c;
	unsigned long cnt = 0;

	c = My_Clients;
	while( c )
	{
		if(( c->type == CLIENT_SERVER ) && ( c->hops == 1 )) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* Client_MyServerCount */


GLOBAL unsigned long
Client_OperCount( void )
{
	CLIENT *c;
	unsigned long cnt = 0;

	c = My_Clients;
	while( c )
	{
		if( c && ( c->type == CLIENT_USER ) && ( strchr( c->modes, 'o' ))) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* Client_OperCount */


GLOBAL unsigned long
Client_UnknownCount( void )
{
	CLIENT *c;
	unsigned long cnt = 0;

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
Client_IsValidNick( const char *Nick )
{
	const char *ptr;
	static const char goodchars[] = ";0123456789-";

	assert( Nick != NULL );

	if( Nick[0] == '#' ) return false;
	if( strchr( goodchars, Nick[0] )) return false;
	if( strlen( Nick ) >= Conf_MaxNickLength) return false;

	ptr = Nick;
	while( *ptr )
	{
		if (( *ptr < 'A' ) && ( ! strchr( goodchars, *ptr ))) return false;
		if ( *ptr > '}' ) return false;
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


static unsigned long
Count( CLIENT_TYPE Type )
{
	CLIENT *c;
	unsigned long cnt = 0;

	c = My_Clients;
	while( c )
	{
		if( c->type == Type ) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* Count */


static unsigned long
MyCount( CLIENT_TYPE Type )
{
	CLIENT *c;
	unsigned long cnt = 0;

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
	LogDebug("Assigned token %d to server \"%s\".", token, Client->id);
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
	strlcpy( My_Whowas[slot].host, Client_HostnameCloaked( Client ),
		 sizeof( My_Whowas[slot].host ));
	strlcpy( My_Whowas[slot].info, Client_Info( Client ),
		 sizeof( My_Whowas[slot].info ));
	strlcpy( My_Whowas[slot].server, Client_ID( Client_Introducer( Client )),
		 sizeof( My_Whowas[slot].server ));

	Last_Whowas = slot;
} /* Client_RegisterWhowas */


GLOBAL const char *
Client_TypeText(CLIENT *Client)
{
	assert(Client != NULL);
	switch (Client_Type(Client)) {
		case CLIENT_USER:
			return "User";
			break;
		case CLIENT_SERVICE:
			return "Service";
			break;
		case CLIENT_SERVER:
			return "Server";
			break;
		default:
			return "Client";
	}
} /* Client_TypeText */


/**
 * Destroy user or service client.
 */
static void
Destroy_UserOrService(CLIENT *Client, const char *Txt, const char *FwdMsg, bool SendQuit)
{
	if(Client->conn_id != NONE) {
		/* Local (directly connected) client */
		Log(LOG_NOTICE,
		    "%s \"%s\" unregistered (connection %d): %s",
		    Client_TypeText(Client), Client_Mask(Client),
		    Client->conn_id, Txt);
		Log_ServerNotice('c', "Client exiting: %s (%s@%s) [%s]",
			         Client_ID(Client), Client_User(Client),
				 Client_Hostname(Client), Txt);

		if (SendQuit) {
			/* Inforam all the other servers */
			if (FwdMsg)
				IRC_WriteStrServersPrefix(NULL,
						Client, "QUIT :%s", FwdMsg );
			else
				IRC_WriteStrServersPrefix(NULL,
						Client, "QUIT :");
		}
	} else {
		/* Remote client */
		LogDebug("%s \"%s\" unregistered: %s",
			 Client_TypeText(Client), Client_Mask(Client), Txt);

		if(SendQuit) {
			/* Inform all the other servers, but the ones in the
			 * direction we got the QUIT from */
			if(FwdMsg)
				IRC_WriteStrServersPrefix(Client_NextHop(Client),
						Client, "QUIT :%s", FwdMsg );
			else
				IRC_WriteStrServersPrefix(Client_NextHop(Client),
						Client, "QUIT :" );
		}
	}

	/* Unregister client from channels */
	Channel_Quit(Client, FwdMsg ? FwdMsg : Client->id);

	/* Register client in My_Whowas structure */
	Client_RegisterWhowas(Client);
} /* Destroy_UserOrService */


#ifdef DEBUG

GLOBAL void
Client_DebugDump(void)
{
	CLIENT *c;

	Log(LOG_DEBUG, "Client status:");
	c = My_Clients;
	while (c) {
		Log(LOG_DEBUG,
		    " - %s, type=%d, host=%s, user=%s, conn=%d, start=%ld, flags=%s",
                   Client_ID(c), Client_Type(c), Client_Hostname(c),
                   Client_User(c), Client_Conn(c), Client_StartTime(c),
                   Client_Flags(c));
		c = (CLIENT *)c->next;
	}
} /* Client_DumpClients */

#endif


/* -eof- */
