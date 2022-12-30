/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2014 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#define __client_c__

#include "portab.h"

/**
 * @file
 * Client management.
 */

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <netdb.h>

#include "conn.h"
#include "ngircd.h"
#include "channel.h"
#include "conf.h"
#include "conn-func.h"
#include "hash.h"
#include "irc-write.h"
#include "log.h"
#include "match.h"
#include "messages.h"

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

static void Free_Client PARAMS(( CLIENT **Client ));

static CLIENT *Init_New_Client PARAMS((CONN_ID Idx, CLIENT *Introducer,
				       CLIENT *TopServer, int Type, const char *ID,
				       const char *User, const char *Hostname, const char *Info,
				       int Hops, int Token, const char *Modes,
				       bool Idented));

static void Destroy_UserOrService PARAMS((CLIENT *Client,const char *Txt, const char *FwdMsg,
					bool SendQuit));

static void cb_introduceClient PARAMS((CLIENT *Client, CLIENT *Prefix,
				       void *i));

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

	/* Client structure for this server */
	This_Server->next = NULL;
	This_Server->type = CLIENT_SERVER;
	This_Server->conn_id = NONE;
	This_Server->introducer = This_Server;
	This_Server->mytoken = 1;
	This_Server->hops = 0;

	gethostname( This_Server->host, CLIENT_HOST_LEN );
	if (Conf_DNS) {
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
	while(c) {
		cnt++;
		next = (CLIENT *)c->next;
		Free_Client(&c);
		c = next;
	}
	if (cnt)
		Log(LOG_INFO, "Freed %d client structure%s.",
		    cnt, cnt == 1 ? "" : "s");
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

	if (Client_HasMode(client, 'a'))
		client->away = strdup(DEFAULT_AWAY_MSG);

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
	char msg[COMMAND_LEN];
	const char *txt;

	assert( Client != NULL );

	txt = LogMsg ? LogMsg : FwdMsg;
	if (!txt)
		txt = "Reason unknown";

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
				if (c != This_Server) {
					if (c->conn_id != NONE)
						Log(LOG_NOTICE|LOG_snotice,
						    "Server \"%s\" unregistered (connection %d): %s.",
						c->id, c->conn_id, txt);
					else
						Log(LOG_NOTICE|LOG_snotice,
						    "Server \"%s\" unregistered: %s.",
						    c->id, txt);
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
				if (c->conn_id != NONE) {
					if (c->id[0])
						Log(LOG_NOTICE,
						    "Client \"%s\" unregistered (connection %d): %s.",
						    c->id, c->conn_id, txt);
					else
						Log(LOG_NOTICE,
						    "Client unregistered (connection %d): %s.",
						    c->conn_id, txt);
				} else {
					Log(LOG_WARNING,
					    "Unregistered unknown client \"%s\": %s",
					    c->id[0] ? c->id : "(No Nick)", txt);
				}
			}

			Free_Client(&c);
			break;
		}
		last = c;
		c = (CLIENT *)c->next;
	}
} /* Client_Destroy */


/**
 * Set client hostname.
 *
 * If global hostname cloaking is in effect, don't set the real hostname
 * but the configured one.
 *
 * @param Client The client of which the hostname should be set.
 * @param Hostname The new hostname.
 */
GLOBAL void
Client_SetHostname( CLIENT *Client, const char *Hostname )
{
	assert(Client != NULL);
	assert(Hostname != NULL);

	/* Only cloak the hostmask if it has not yet been cloaked.
	 * The period or colon indicates it's still an IP address.
	 * An empty string means a rDNS lookup did not happen (yet). */
	if (Conf_CloakHost[0] && (!Client->host[0] || strchr(Client->host, '.')
				  || strchr(Client->host, ':'))) {
		char cloak[GETID_LEN];

		strlcpy(cloak, Hostname, GETID_LEN);
		strlcat(cloak, Conf_CloakHostSalt, GETID_LEN);
		snprintf(cloak, GETID_LEN, Conf_CloakHost, Hash(cloak));

		LogDebug("Updating hostname of \"%s\": \"%s\" -> \"%s\"",
			Client_ID(Client), Client->host, cloak);
		strlcpy(Client->host, cloak, sizeof(Client->host));
	} else {
		LogDebug("Updating hostname of \"%s\": \"%s\" -> \"%s\"",
			 Client_ID(Client), Client->host, Hostname);
		strlcpy(Client->host, Hostname, sizeof(Client->host));
	}
} /* Client_SetHostname */


/**
 * Set IP address to display for a client.
 *
 * @param Client The client.
 * @param IPAText Textual representation of the IP address or NULL to unset.
 */
GLOBAL void
Client_SetIPAText(CLIENT *Client, const char *IPAText)
{
	assert(Client != NULL);

	if (Client->ipa_text)
		free(Client->ipa_text);

	if (*IPAText)
		Client->ipa_text = strndup(IPAText, CLIENT_HOST_LEN - 1);
	else
		Client->ipa_text = NULL;
}


GLOBAL void
Client_SetID( CLIENT *Client, const char *ID )
{
	assert( Client != NULL );
	assert( ID != NULL );

	strlcpy( Client->id, ID, sizeof( Client->id ));

	if (Conf_CloakUserToNick) {
		strlcpy( Client->user, ID, sizeof( Client->user ));
		strlcpy( Client->info, ID, sizeof( Client->info ));
	}

	/* Hash */
	Client->hash = Hash( Client->id );
} /* Client_SetID */


GLOBAL void
Client_SetUser( CLIENT *Client, const char *User, bool Idented )
{
	/* set clients username */

	assert( Client != NULL );
	assert( User != NULL );

	if (Conf_CloakUserToNick) {
		strlcpy(Client->user, Client->id, sizeof(Client->user));
	} else if (Idented) {
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
Client_SetOrigUser(CLIENT UNUSED *Client, const char UNUSED *User)
{
	assert(Client != NULL);
	assert(User != NULL);

#if defined(PAM)
	strlcpy(Client->orig_user, User, sizeof(Client->orig_user));
#endif
} /* Client_SetOrigUser */


GLOBAL void
Client_SetInfo( CLIENT *Client, const char *Info )
{
	/* set client hostname */

	assert( Client != NULL );
	assert( Info != NULL );

	if (Conf_CloakUserToNick)
		strlcpy(Client->info, Client->id, sizeof(Client->info));
	else
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
Client_SetAccountName(CLIENT *Client, const char *AccountName)
{
	assert(Client != NULL);

	if (Client->account_name)
		free(Client->account_name);

	if (*AccountName)
		Client->account_name = strndup(AccountName,
					       CLIENT_NICK_LEN - 1);
	else
		Client->account_name = NULL;
}


GLOBAL void
Client_SetAway( CLIENT *Client, const char *Txt )
{
	/* Set AWAY reason of client */

	assert( Client != NULL );
	assert( Txt != NULL );

	if (Client->away)
		free(Client->away);

	Client->away = strndup(Txt, CLIENT_AWAY_LEN - 1);

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


GLOBAL bool
Client_ModeAdd( CLIENT *Client, char Mode )
{
	/* Set Mode.
	 * If Client already had Mode, return false.
	 * If the Mode was newly set, return true.
	 */

	char x[2];

	assert( Client != NULL );

	x[0] = Mode; x[1] = '\0';
	if (!Client_HasMode(Client, x[0])) {
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


/**
 * Search CLIENT structure of a given nick name.
 *
 * @return Pointer to CLIENT structure or NULL if not found.
 */
GLOBAL CLIENT *
Client_Search( const char *Nick )
{
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
}


/**
 * Search first CLIENT structure matching a given mask of a server.
 *
 * The order of servers is arbitrary, but this function makes sure that the
 * local server is always returned if the mask matches it.
 *
 * @return Pointer to CLIENT structure or NULL if no server could be found.
 */
GLOBAL CLIENT *
Client_SearchServer(const char *Mask)
{
	CLIENT *c;

	assert(Mask != NULL);

	/* First check if mask matches the local server */
	if (MatchCaseInsensitive(Mask, Client_ID(Client_ThisServer())))
		return Client_ThisServer();

	c = My_Clients;
	while (c) {
		if (Client_Type(c) == CLIENT_SERVER) {
			/* This is a server: check if Mask matches */
			if (MatchCaseInsensitive(Mask, c->id))
				return c;
		}
		c = (CLIENT *)c->next;
	}
	return NULL;
}


/**
 * Get client structure ("introducer") identified by a server token.
 * @return CLIENT structure or NULL if none could be found.
 */
GLOBAL CLIENT *
Client_GetFromToken( CLIENT *Client, int Token )
{
	CLIENT *c;

	assert( Client != NULL );

	if (!Token)
		return NULL;

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

	if(Client->type == CLIENT_USER)
		assert(strlen(Client->id) < Conf_MaxNickLength);

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
	return Client->orig_user;
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
}

/**
 * Return the cloaked hostname of a client, if set.
 * @param Client Pointer to the client structure.
 * @return Pointer to the cloaked hostname or NULL if not set.
 */
GLOBAL char *
Client_HostnameCloaked(CLIENT *Client)
{
	assert(Client != NULL);
	return Client->cloaked;
}

/**
 * Get (potentially cloaked) hostname of a client to display it to other users.
 *
 * If the client has not enabled cloaking, the real hostname is used.
 *
 * @param Client Pointer to client structure
 * @return Pointer to client hostname
 */
GLOBAL char *
Client_HostnameDisplayed(CLIENT *Client)
{
	assert(Client != NULL);

	/* Client isn't cloaked at all, return real hostname: */
	if (!Client_HasMode(Client, 'x'))
		return Client_Hostname(Client);

	/* Use an already saved cloaked hostname, if there is one */
	if (Client->cloaked)
		return Client->cloaked;

	Client_UpdateCloakedHostname(Client, NULL, NULL);
	return Client->cloaked;
}

GLOBAL const char *
Client_IPAText(CLIENT *Client)
{
	assert(Client != NULL);

	/* Not a local client? */
	if (Client_Conn(Client) <= NONE)
		return "0.0.0.0";

	if (!Client->ipa_text)
		return Conn_GetIPAInfo(Client_Conn(Client));
	else
		return Client->ipa_text;
}

/**
 * Update (and generate, if necessary) the cloaked hostname of a client.
 *
 * The newly set cloaked hostname is announced in the network using METADATA
 * commands to peers that support this feature.
 *
 * @param Client The client of which the cloaked hostname should be updated.
 * @param Origin The originator of the hostname change, or NULL if this server.
 * @param Hostname The new cloaked hostname, or NULL if it should be generated.
 */
GLOBAL void
Client_UpdateCloakedHostname(CLIENT *Client, CLIENT *Origin,
			     const char *Hostname)
{
	char Cloak_Buffer[CLIENT_HOST_LEN];

	assert(Client != NULL);
	if (!Origin)
		Origin = Client_ThisServer();

	if (!Client->cloaked) {
		Client->cloaked = malloc(CLIENT_HOST_LEN);
		if (!Client->cloaked)
			return;
	}

	if (!Hostname) {
		/* Generate new cloaked hostname */
		if (*Conf_CloakHostModeX) {
			strlcpy(Cloak_Buffer, Client->host,
				sizeof(Cloak_Buffer));
			strlcat(Cloak_Buffer, Conf_CloakHostSalt,
				sizeof(Cloak_Buffer));
			snprintf(Client->cloaked, CLIENT_HOST_LEN,
				 Conf_CloakHostModeX, Hash(Cloak_Buffer));
		} else
			strlcpy(Client->cloaked, Client_ID(Client->introducer),
				CLIENT_HOST_LEN);
	} else
		strlcpy(Client->cloaked, Hostname, CLIENT_HOST_LEN);
	LogDebug("Cloaked hostname of \"%s\" updated to \"%s\"",
		 Client_ID(Client), Client->cloaked);

	/* Inform other servers in the network */
	IRC_WriteStrServersPrefixFlag(Client_NextHop(Origin), Origin, 'M',
				      "METADATA %s cloakhost :%s",
				      Client_ID(Client), Client->cloaked);
}

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
 * nest invocations without overwriting earlier results!
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
 *
 * This client ID is used for IRC prefixes, for example.
 * Please note that this function uses a global static buffer, so you can't
 * nest invocations without overwriting earlier results!
 * If the client has not enabled cloaking, the real hostname is used.
 *
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

	snprintf(Mask_Buffer, GETID_LEN, "%s!%s@%s", Client->id, Client->user,
		 Client_HostnameDisplayed(Client));

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


GLOBAL bool
Client_HasFlag( CLIENT *Client, char Flag )
{
	assert( Client != NULL );
	return strchr( Client->flags, Flag ) != NULL;
} /* Client_HasFlag */


GLOBAL char *
Client_Away( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->away;
} /* Client_Away */


GLOBAL char *
Client_AccountName(CLIENT *Client)
{
	assert(Client != NULL);
	return Client->account_name;
}


/**
 * Make sure that a given nickname is valid.
 *
 * If the nickname is not valid for the given client, this function sends back
 * the appropriate error messages.
 *
 * @param	Client Client that wants to change the nickname.
 * @param	Nick New nickname.
 * @returns	true if nickname is valid, false otherwise.
 */
GLOBAL bool
Client_CheckNick(CLIENT *Client, char *Nick)
{
	assert(Client != NULL);
	assert(Nick != NULL);

	if (!Client_IsValidNick(Nick)) {
		if (strlen(Nick ) >= Conf_MaxNickLength)
			IRC_WriteErrClient(Client, ERR_NICKNAMETOOLONG_MSG,
					   Client_ID(Client), Nick,
					   Conf_MaxNickLength - 1);
		else
			IRC_WriteErrClient(Client, ERR_ERRONEUSNICKNAME_MSG,
					   Client_ID(Client), Nick);
		return false;
	}

	if (Client_Type(Client) != CLIENT_SERVER
	    && Client_Type(Client) != CLIENT_SERVICE) {
		/* Make sure that this isn't a restricted/forbidden nickname */
		if (Conf_NickIsBlocked(Nick)) {
			IRC_WriteErrClient(Client, ERR_FORBIDDENNICKNAME_MSG,
					   Client_ID(Client), Nick);
			return false;
		}
	}

	/* Nickname already registered? */
	if (Client_Search(Nick)) {
		IRC_WriteErrClient(Client, ERR_NICKNAMEINUSE_MSG,
			Client_ID(Client), Nick);
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
		IRC_WriteErrClient(Client, ERR_ERRONEUSNICKNAME_MSG,
				   Client_ID(Client), ID);
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
		if (c && c->type == CLIENT_USER && Client_HasMode(c, 'o' ))
			cnt++;
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


/**
 * Check that a given nickname is valid.
 *
 * @param	Nick the nickname to check.
 * @returns	true if nickname is valid, false otherwise.
 */
GLOBAL bool
Client_IsValidNick(const char *Nick)
{
	const char *ptr;
	static const char goodchars[] = ";0123456789-";

	assert (Nick != NULL);

	if (strchr(goodchars, Nick[0]))
		return false;
	if (strlen(Nick ) >= Conf_MaxNickLength)
		return false;

	ptr = Nick;
	while (*ptr) {
		if (*ptr < 'A' && !strchr(goodchars, *ptr ))
			return false;
		if (*ptr > '}')
			return false;
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


/**
 * Reject a client when logging in.
 *
 * This function is called when a client isn't allowed to connect to this
 * server. Possible reasons are bad server password, bad PAM password,
 * or that the client is G/K-Line'd.
 *
 * After calling this function, the client isn't connected any more.
 *
 * @param Client The client to reject.
 * @param Reason The reason why the client has been rejected.
 * @param InformClient If true, send the exact reason to the client.
 */
GLOBAL void
Client_Reject(CLIENT *Client, const char *Reason, bool InformClient)
{
	char info[COMMAND_LEN];

	assert(Client != NULL);
	assert(Reason != NULL);

	if (InformClient)
		snprintf(info, sizeof(info), "Access denied: %s", Reason);
	else
		strcpy(info, "Access denied: Bad password?");

	Log(LOG_ERR,
	    "User \"%s\" rejected (connection %d): %s!",
	    Client_Mask(Client), Client_Conn(Client), Reason);
	Conn_Close(Client_Conn(Client), Reason, info, true);
}


/**
 * Introduce a new user or service client in the network.
 *
 * @param From Remote server introducing the client or NULL (local).
 * @param Client New client.
 * @param Type Type of the client (CLIENT_USER or CLIENT_SERVICE).
 */
GLOBAL void
Client_Introduce(CLIENT *From, CLIENT *Client, int Type)
{
	int server;

	/* Set client type (user or service) */
	Client_SetType(Client, Type);

	if (From) {
		server = Conf_GetServer(Client_Conn(From));
		if (server > NONE && Conf_NickIsService(server, Client_ID(Client)))
			Client_SetType(Client, CLIENT_SERVICE);
		LogDebug("%s \"%s\" (+%s) registered (via %s, on %s, %d hop%s).",
			 Client_TypeText(Client), Client_Mask(Client),
			 Client_Modes(Client), Client_ID(From),
			 Client_ID(Client_Introducer(Client)),
			 Client_Hops(Client), Client_Hops(Client) > 1 ? "s": "");
	} else {
		Log(LOG_NOTICE, "%s \"%s\" registered (connection %d).",
		    Client_TypeText(Client), Client_Mask(Client),
		    Client_Conn(Client));
		Log_ServerNotice('c', "Client connecting: %s (%s@%s) [%s] - %s",
			         Client_ID(Client), Client_User(Client),
				 Client_Hostname(Client),
				 Conn_IPA(Client_Conn(Client)),
				 Client_TypeText(Client));
	}

	/* Inform other servers */
	IRC_WriteStrServersPrefixFlag_CB(From,
				From != NULL ? From : Client_ThisServer(),
				'\0', cb_introduceClient, (void *)Client);
} /* Client_Introduce */


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


/**
 * Allocate and initialize new CLIENT structure.
 *
 * @return Pointer to CLIENT structure or NULL on error.
 */
static CLIENT *
New_Client_Struct( void )
{
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
	c->hops = -1;
	c->token = -1;
	c->mytoken = -1;

	return c;
}

/**
 * Free a CLIENT structure and its member variables.
 */
static void
Free_Client(CLIENT **Client)
{
	assert(Client != NULL);
	assert(*Client != NULL);

	if ((*Client)->account_name)
		free((*Client)->account_name);
	if ((*Client)->away)
		free((*Client)->away);
	if ((*Client)->cloaked)
		free((*Client)->cloaked);
	if ((*Client)->ipa_text)
		free((*Client)->ipa_text);

	free(*Client);
	*Client = NULL;
}

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
			/* The token is already in use */
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

	/* Don't register WHOWAS information when "MorePrivacy" is enabled. */
	if (Conf_MorePrivacy)
		return;

	now = time(NULL);
	/* Don't register clients that were connected less than 30 seconds. */
	if( now - Client->starttime < 30 )
		return;

	slot = Last_Whowas + 1;
	if( slot >= MAX_WHOWAS || slot < 0 ) slot = 0;

	LogDebug( "Saving WHOWAS information to slot %d ...", slot );

	My_Whowas[slot].time = now;
	strlcpy( My_Whowas[slot].id, Client_ID( Client ),
		 sizeof( My_Whowas[slot].id ));
	strlcpy( My_Whowas[slot].user, Client_User( Client ),
		 sizeof( My_Whowas[slot].user ));
	strlcpy( My_Whowas[slot].host, Client_HostnameDisplayed( Client ),
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
		    "%s \"%s\" unregistered (connection %d): %s.",
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
		LogDebug("%s \"%s\" unregistered: %s.",
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


/**
 * Introduce a new user or service client to a remote server.
 *
 * @param To		The remote server to inform.
 * @param Prefix	Prefix for the generated commands.
 * @param data		CLIENT structure of the new client.
 */
static void
cb_introduceClient(CLIENT *To, CLIENT *Prefix, void *data)
{
	CLIENT *c = (CLIENT *)data;

	(void)Client_Announce(To, Prefix, c);

} /* cb_introduceClient */


/**
 * Announce an user or service to a server.
 *
 * This function differentiates between RFC1459 and RFC2813 server links and
 * generates the appropriate commands to register the user or service.
 *
 * @param Client	Server
 * @param Prefix	Prefix for the generated commands
 * @param User		User to announce
 */
GLOBAL bool
Client_Announce(CLIENT * Client, CLIENT * Prefix, CLIENT * User)
{
	CONN_ID conn;
	char *modes, *user, *host;

	modes = Client_Modes(User);
	user = Client_User(User) ? Client_User(User) : "-";
	host = Client_Hostname(User) ? Client_Hostname(User) : "-";

	conn = Client_Conn(Client);
	if (Conn_Options(conn) & CONN_RFC1459) {
		/* RFC 1459 mode: separate NICK and USER commands */
		if (! Conn_WriteStr(conn, "NICK %s :%d",
				    Client_ID(User), Client_Hops(User) + 1))
			return DISCONNECTED;
		if (! Conn_WriteStr(conn, ":%s USER %s %s %s :%s",
				     Client_ID(User), user, host,
				     Client_ID(Client_Introducer(User)),
				     Client_Info(User)))
			return DISCONNECTED;
		if (modes[0]) {
			if (! Conn_WriteStr(conn, ":%s MODE %s +%s",
				     Client_ID(User), Client_ID(User),
				     modes))
				return DISCONNECTED;
		}
	} else {
		/* RFC 2813 mode: one combined NICK or SERVICE command */
		if (Client_Type(User) == CLIENT_SERVICE
		    && Client_HasFlag(Client, 'S')) {
			if (!IRC_WriteStrClientPrefix(Client, Prefix,
					"SERVICE %s %d * +%s %d :%s",
					Client_Mask(User),
					Client_MyToken(Client_Introducer(User)),
					modes, Client_Hops(User) + 1,
					Client_Info(User)))
				return DISCONNECTED;
		} else {
			if (!IRC_WriteStrClientPrefix(Client, Prefix,
					"NICK %s %d %s %s %d +%s :%s",
					Client_ID(User), Client_Hops(User) + 1,
					user, host,
					Client_MyToken(Client_Introducer(User)),
					modes, Client_Info(User)))
				return DISCONNECTED;
		}
	}

	if (Client_HasFlag(Client, 'M')) {
		/* Synchronize metadata */
		if (Client_HostnameCloaked(User)) {
			if (!IRC_WriteStrClientPrefix(Client, Prefix,
					"METADATA %s cloakhost :%s",
					Client_ID(User),
					Client_HostnameCloaked(User)))
				return DISCONNECTED;
		}

		if (Client_AccountName(User)) {
			if (!IRC_WriteStrClientPrefix(Client, Prefix,
					"METADATA %s accountname :%s",
					Client_ID(User),
					Client_AccountName(User)))
				return DISCONNECTED;
		}

		if (Conn_GetCertFp(Client_Conn(User))) {
			if (!IRC_WriteStrClientPrefix(Client, Prefix,
					"METADATA %s certfp :%s",
					Client_ID(User),
					Conn_GetCertFp(Client_Conn(User))))
				return DISCONNECTED;
		}
	}

	return CONNECTED;
} /* Client_Announce */



GLOBAL void
Client_DebugDump(void)
{
	CLIENT *c;

	LogDebug("Client status:");
	c = My_Clients;
	while (c) {
		LogDebug(
		    " - %s: type=%d, host=%s, user=%s, conn=%d, start=%ld, flags=%s",
                   Client_ID(c), Client_Type(c), Client_Hostname(c),
                   Client_User(c), Client_Conn(c), Client_StartTime(c),
                   Client_Flags(c));
		c = (CLIENT *)c->next;
	}
} /* Client_DumpClients */



/* -eof- */
