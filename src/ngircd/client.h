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
 * $Id: client.h,v 1.39 2005/06/12 16:18:49 alex Exp $
 *
 * Client management (header)
 */


#ifndef __client_h__
#define __client_h__


#define CLIENT_UNKNOWN 1		/* connection of unknown type */
#define CLIENT_GOTPASS 2		/* client did send PASS */
#define CLIENT_GOTNICK 4		/* client did send NICK */
#define CLIENT_GOTUSER 8		/* client did send USER */
#define CLIENT_USER 16			/* client is an IRC user */
#define CLIENT_UNKNOWNSERVER 32		/* unregistered server connection */
#define CLIENT_GOTPASSSERVER 64		/* client did send PASS in "server style" */
#define CLIENT_SERVER 128		/* client is a server */
#define CLIENT_SERVICE 256		/* client is a service */

#define CLIENT_TYPE int


#if defined(__client_c__) | defined(S_SPLINT_S)

typedef struct _CLIENT
{
	time_t starttime;		/* Start time of link */
	char id[CLIENT_ID_LEN];		/* nick (user) / ID (server) */
	UINT32 hash;			/* hash of lower-case ID */
	POINTER *next;			/* pointer to next client structure */
	CLIENT_TYPE type;		/* type of client, see CLIENT_xxx */
	CONN_ID conn_id;		/* ID of the connection (if local) or NONE (remote) */
	struct _CLIENT *introducer;	/* ID of the servers which the client is connected to */
	struct _CLIENT *topserver;	/* toplevel servers (only valid if client is a server) */
	char pwd[CLIENT_PASS_LEN];	/* password received of the client */
	char host[CLIENT_HOST_LEN];	/* hostname of the client */
	char user[CLIENT_USER_LEN];	/* user name ("login") */
	char info[CLIENT_INFO_LEN];	/* long user name (user) / info text (server) */
	char modes[CLIENT_MODE_LEN];	/* client modes */
	int hops, token, mytoken;	/* "hops" and "Token" (see SERVER command) */
	bool oper_by_me;		/* client is local IRC operator on this server? */
	char away[CLIENT_AWAY_LEN];	/* AWAY text (valid if mode 'a' is set) */
	char flags[CLIENT_FLAGS_LEN];	/* flags of the client */
} CLIENT;

#else

typedef POINTER CLIENT;

#endif


typedef struct _WHOWAS
{
	time_t time;			/* time stamp of entry or 0 if unused */
	char id[CLIENT_NICK_LEN];	/* client nick name */
	char host[CLIENT_HOST_LEN];	/* hostname of the client */
	char user[CLIENT_USER_LEN];	/* user name ("login") */
	char info[CLIENT_INFO_LEN];	/* long user name */
	char server[CLIENT_HOST_LEN];	/* server name */
} WHOWAS;


GLOBAL void Client_Init PARAMS(( void ));
GLOBAL void Client_Exit PARAMS(( void ));

GLOBAL CLIENT *Client_NewLocal PARAMS(( CONN_ID Idx, char *Hostname, int Type, bool Idented ));
GLOBAL CLIENT *Client_NewRemoteServer PARAMS(( CLIENT *Introducer, char *Hostname, CLIENT *TopServer, int Hops, int Token, char *Info, bool Idented ));
GLOBAL CLIENT *Client_NewRemoteUser PARAMS(( CLIENT *Introducer, char *Nick, int Hops, char *User, char *Hostname, int Token, char *Modes, char *Info, bool Idented ));
GLOBAL CLIENT *Client_New PARAMS(( CONN_ID Idx, CLIENT *Introducer, CLIENT *TopServer, int Type, char *ID, char *User, char *Hostname, char *Info, int Hops, int Token, char *Modes, bool Idented ));

GLOBAL void Client_Destroy PARAMS(( CLIENT *Client, char *LogMsg, char *FwdMsg, bool SendQuit ));
#ifdef CONN_MODULE
GLOBAL void Client_DestroyNow PARAMS(( CLIENT *Client ));
#endif

GLOBAL CLIENT *Client_ThisServer PARAMS(( void ));

GLOBAL CLIENT *Client_GetFromConn PARAMS(( CONN_ID Idx ));
GLOBAL CLIENT *Client_GetFromToken PARAMS(( CLIENT *Client, int Token ));

GLOBAL CLIENT *Client_Search PARAMS(( char *ID ));
GLOBAL CLIENT *Client_First PARAMS(( void ));
GLOBAL CLIENT *Client_Next PARAMS(( CLIENT *c ));

GLOBAL int Client_Type PARAMS(( CLIENT *Client ));
GLOBAL CONN_ID Client_Conn PARAMS(( CLIENT *Client ));
GLOBAL char *Client_ID PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Mask PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Info PARAMS(( CLIENT *Client ));
GLOBAL char *Client_User PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Hostname PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Password PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Modes PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Flags PARAMS(( CLIENT *Client ));
GLOBAL CLIENT *Client_Introducer PARAMS(( CLIENT *Client ));
GLOBAL bool Client_OperByMe PARAMS(( CLIENT *Client ));
GLOBAL int Client_Hops PARAMS(( CLIENT *Client ));
GLOBAL int Client_Token PARAMS(( CLIENT *Client ));
GLOBAL int Client_MyToken PARAMS(( CLIENT *Client ));
GLOBAL CLIENT *Client_TopServer PARAMS(( CLIENT *Client ));
GLOBAL CLIENT *Client_NextHop PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Away PARAMS(( CLIENT *Client ));
GLOBAL time_t Client_StartTime PARAMS(( CLIENT *Client ));

GLOBAL bool Client_HasMode PARAMS(( CLIENT *Client, char Mode ));

GLOBAL void Client_SetHostname PARAMS(( CLIENT *Client, char *Hostname ));
GLOBAL void Client_SetID PARAMS(( CLIENT *Client, char *Nick ));
GLOBAL void Client_SetUser PARAMS(( CLIENT *Client, char *User, bool Idented ));
GLOBAL void Client_SetInfo PARAMS(( CLIENT *Client, char *Info ));
GLOBAL void Client_SetPassword PARAMS(( CLIENT *Client, char *Pwd ));
GLOBAL void Client_SetType PARAMS(( CLIENT *Client, int Type ));
GLOBAL void Client_SetHops PARAMS(( CLIENT *Client, int Hops ));
GLOBAL void Client_SetToken PARAMS(( CLIENT *Client, int Token ));
GLOBAL void Client_SetOperByMe PARAMS(( CLIENT *Client, bool OperByMe ));
GLOBAL void Client_SetModes PARAMS(( CLIENT *Client, char *Modes ));
GLOBAL void Client_SetFlags PARAMS(( CLIENT *Client, char *Flags ));
GLOBAL void Client_SetIntroducer PARAMS(( CLIENT *Client, CLIENT *Introducer ));
GLOBAL void Client_SetAway PARAMS(( CLIENT *Client, char *Txt ));

GLOBAL bool Client_ModeAdd PARAMS(( CLIENT *Client, char Mode ));
GLOBAL bool Client_ModeDel PARAMS(( CLIENT *Client, char Mode ));

GLOBAL bool Client_CheckNick PARAMS(( CLIENT *Client, char *Nick ));
GLOBAL bool Client_CheckID PARAMS(( CLIENT *Client, char *ID ));

GLOBAL long Client_UserCount PARAMS(( void ));
GLOBAL long Client_ServiceCount PARAMS(( void ));
GLOBAL long Client_ServerCount PARAMS(( void ));
GLOBAL long Client_OperCount PARAMS(( void ));
GLOBAL long Client_UnknownCount PARAMS(( void ));
GLOBAL long Client_MyUserCount PARAMS(( void ));
GLOBAL long Client_MyServiceCount PARAMS(( void ));
GLOBAL long Client_MyServerCount PARAMS(( void ));
GLOBAL long Client_MaxUserCount PARAMS((  void ));
GLOBAL long Client_MyMaxUserCount PARAMS((  void ));

GLOBAL bool Client_IsValidNick PARAMS(( char *Nick ));

GLOBAL WHOWAS *Client_GetWhowas PARAMS(( void ));
GLOBAL int Client_GetLastWhowasIndex PARAMS(( void ));

GLOBAL void Client_RegisterWhowas PARAMS(( CLIENT *Client ));


#endif


/* -eof- */
