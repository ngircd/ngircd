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
 * $Id: client.h,v 1.32.2.1 2002/12/22 23:42:28 alex Exp $
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

#define CLIENT_TYPE INT


#if defined(__client_c__) | defined(S_SPLINT_S)

#include "defines.h"

typedef struct _CLIENT
{
	CHAR id[CLIENT_ID_LEN];		/* nick (user) / ID (server) */
	UINT32 hash;			/* hash of lower-case ID */
	POINTER *next;			/* pointer to next client structure */
	CLIENT_TYPE type;		/* type of client, see CLIENT_xxx */
	CONN_ID conn_id;		/* ID of the connection (if local) or NONE (remote) */
	struct _CLIENT *introducer;	/* ID of the servers which the client is connected to */
	struct _CLIENT *topserver;	/* toplevel servers (only valid if client is a server) */
	CHAR pwd[CLIENT_PASS_LEN];	/* password received of the client */
	CHAR host[CLIENT_HOST_LEN];	/* hostname of the client */
	CHAR user[CLIENT_USER_LEN];	/* user name ("login") */
	CHAR info[CLIENT_INFO_LEN];	/* long user name (user) / info text (server) */
	CHAR modes[CLIENT_MODE_LEN];	/* client modes */
	INT hops, token, mytoken;	/* "hops" and "Token" (see SERVER command) */
	BOOLEAN oper_by_me;		/* client is local IRC operator on this server? */
	CHAR away[CLIENT_AWAY_LEN];	/* AWAY text (valid if mode 'a' is set) */
	CHAR flags[CLIENT_FLAGS_LEN];	/* flags of the client */
} CLIENT;

#else

typedef POINTER CLIENT;

#endif


GLOBAL VOID Client_Init PARAMS((VOID ));
GLOBAL VOID Client_Exit PARAMS((VOID ));

GLOBAL CLIENT *Client_NewLocal PARAMS((CONN_ID Idx, CHAR *Hostname, INT Type, BOOLEAN Idented ));
GLOBAL CLIENT *Client_NewRemoteServer PARAMS((CLIENT *Introducer, CHAR *Hostname, CLIENT *TopServer, INT Hops, INT Token, CHAR *Info, BOOLEAN Idented ));
GLOBAL CLIENT *Client_NewRemoteUser PARAMS((CLIENT *Introducer, CHAR *Nick, INT Hops, CHAR *User, CHAR *Hostname, INT Token, CHAR *Modes, CHAR *Info, BOOLEAN Idented ));
GLOBAL CLIENT *Client_New PARAMS((CONN_ID Idx, CLIENT *Introducer, CLIENT *TopServer, INT Type, CHAR *ID, CHAR *User, CHAR *Hostname, CHAR *Info, INT Hops, INT Token, CHAR *Modes, BOOLEAN Idented ));

GLOBAL VOID Client_Destroy PARAMS((CLIENT *Client, CHAR *LogMsg, CHAR *FwdMsg, BOOLEAN SendQuit ));

GLOBAL CLIENT *Client_ThisServer PARAMS((VOID ));

GLOBAL CLIENT *Client_GetFromConn PARAMS((CONN_ID Idx ));
GLOBAL CLIENT *Client_GetFromToken PARAMS((CLIENT *Client, INT Token ));

GLOBAL CLIENT *Client_Search PARAMS((CHAR *ID ));
GLOBAL CLIENT *Client_First PARAMS((VOID ));
GLOBAL CLIENT *Client_Next PARAMS((CLIENT *c ));

GLOBAL INT Client_Type PARAMS((CLIENT *Client ));
GLOBAL CONN_ID Client_Conn PARAMS((CLIENT *Client ));
GLOBAL CHAR *Client_ID PARAMS((CLIENT *Client ));
GLOBAL CHAR *Client_Mask PARAMS((CLIENT *Client ));
GLOBAL CHAR *Client_Info PARAMS((CLIENT *Client ));
GLOBAL CHAR *Client_User PARAMS((CLIENT *Client ));
GLOBAL CHAR *Client_Hostname PARAMS((CLIENT *Client ));
GLOBAL CHAR *Client_Password PARAMS((CLIENT *Client ));
GLOBAL CHAR *Client_Modes PARAMS((CLIENT *Client ));
GLOBAL CHAR *Client_Flags PARAMS((CLIENT *Client ));
GLOBAL CLIENT *Client_Introducer PARAMS((CLIENT *Client ));
GLOBAL BOOLEAN Client_OperByMe PARAMS((CLIENT *Client ));
GLOBAL INT Client_Hops PARAMS((CLIENT *Client ));
GLOBAL INT Client_Token PARAMS((CLIENT *Client ));
GLOBAL INT Client_MyToken PARAMS((CLIENT *Client ));
GLOBAL CLIENT *Client_TopServer PARAMS((CLIENT *Client ));
GLOBAL CLIENT *Client_NextHop PARAMS((CLIENT *Client ));
GLOBAL CHAR *Client_Away PARAMS((CLIENT *Client ));

GLOBAL BOOLEAN Client_HasMode PARAMS((CLIENT *Client, CHAR Mode ));

GLOBAL VOID Client_SetHostname PARAMS((CLIENT *Client, CHAR *Hostname ));
GLOBAL VOID Client_SetID PARAMS((CLIENT *Client, CHAR *Nick ));
GLOBAL VOID Client_SetUser PARAMS((CLIENT *Client, CHAR *User, BOOLEAN Idented ));
GLOBAL VOID Client_SetInfo PARAMS((CLIENT *Client, CHAR *Info ));
GLOBAL VOID Client_SetPassword PARAMS((CLIENT *Client, CHAR *Pwd ));
GLOBAL VOID Client_SetType PARAMS((CLIENT *Client, INT Type ));
GLOBAL VOID Client_SetHops PARAMS((CLIENT *Client, INT Hops ));
GLOBAL VOID Client_SetToken PARAMS((CLIENT *Client, INT Token ));
GLOBAL VOID Client_SetOperByMe PARAMS((CLIENT *Client, BOOLEAN OperByMe ));
GLOBAL VOID Client_SetModes PARAMS((CLIENT *Client, CHAR *Modes ));
GLOBAL VOID Client_SetFlags PARAMS((CLIENT *Client, CHAR *Flags ));
GLOBAL VOID Client_SetIntroducer PARAMS((CLIENT *Client, CLIENT *Introducer ));
GLOBAL VOID Client_SetAway PARAMS((CLIENT *Client, CHAR *Txt ));

GLOBAL BOOLEAN Client_ModeAdd PARAMS((CLIENT *Client, CHAR Mode ));
GLOBAL BOOLEAN Client_ModeDel PARAMS((CLIENT *Client, CHAR Mode ));

GLOBAL BOOLEAN Client_CheckNick PARAMS((CLIENT *Client, CHAR *Nick ));
GLOBAL BOOLEAN Client_CheckID PARAMS((CLIENT *Client, CHAR *ID ));

GLOBAL LONG Client_UserCount PARAMS((VOID ));
GLOBAL LONG Client_ServiceCount PARAMS((VOID ));
GLOBAL LONG Client_ServerCount PARAMS((VOID ));
GLOBAL LONG Client_OperCount PARAMS((VOID ));
GLOBAL LONG Client_UnknownCount PARAMS((VOID ));
GLOBAL LONG Client_MyUserCount PARAMS((VOID ));
GLOBAL LONG Client_MyServiceCount PARAMS((VOID ));
GLOBAL LONG Client_MyServerCount PARAMS((VOID ));
GLOBAL LONG Client_MaxUserCount PARAMS(( VOID ));
GLOBAL LONG Client_MyMaxUserCount PARAMS(( VOID ));

GLOBAL BOOLEAN Client_IsValidNick PARAMS((CHAR *Nick ));


#endif


/* -eof- */
