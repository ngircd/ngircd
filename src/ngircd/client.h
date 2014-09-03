/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2013 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __client_h__
#define __client_h__

/**
 * @file
 * Client management (header)
 */

#define CLIENT_UNKNOWN		0x0001	/* connection of unknown type */
#define CLIENT_GOTPASS		0x0002	/* client did send PASS */
#define CLIENT_GOTNICK		0x0004	/* client did send NICK */
#define CLIENT_GOTUSER		0x0008	/* client did send USER */
#define CLIENT_USER		0x0010	/* client is an IRC user */
#define CLIENT_SERVER		0x0020	/* client is a server */
#define CLIENT_SERVICE		0x0040	/* client is a service */
#define CLIENT_UNKNOWNSERVER	0x0080	/* unregistered server connection */
#define CLIENT_GOTPASS_2813	0x0100	/* client did send PASS, RFC 2813 style */
#ifndef STRICT_RFC
# define CLIENT_WAITAUTHPING	0x0200	/* waiting for AUTH PONG from client */
#endif
#define CLIENT_WAITCAPEND	0x0400	/* waiting for "CAP END" command */
#define CLIENT_ANY		0xFFFF

#define CLIENT_TYPE int

#include "defines.h"

#if defined(__client_c__) | defined(__client_cap_c__) | defined(S_SPLINT_S)

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
	char host[CLIENT_HOST_LEN];	/* hostname of the client */
	char *cloaked;			/* cloaked hostname of the client */
	char *ipa_text;			/* textual representaton of IP address */
	char user[CLIENT_USER_LEN];	/* user name ("login") */
#if defined(PAM)
	char orig_user[CLIENT_AUTHUSER_LEN];
					/* original user name supplied by USER command */
#endif
	char info[CLIENT_INFO_LEN];	/* long user name (user) / info text (server) */
	char modes[CLIENT_MODE_LEN];	/* client modes */
	int hops, token, mytoken;	/* "hops" and "Token" (see SERVER command) */
	char *away;			/* AWAY text (valid if mode 'a' is set) */
	char flags[CLIENT_FLAGS_LEN];	/* flags of the client */
	char *account_name;		/* login account (for services) */
	int capabilities;		/* enabled IRC capabilities */
} CLIENT;

#else

typedef POINTER CLIENT;

#endif


typedef struct _WHOWAS
{
	time_t time;			/* time stamp of entry or 0 if unused */
	char id[CLIENT_NICK_LEN];	/* client nickname */
	char host[CLIENT_HOST_LEN];	/* hostname of the client */
	char user[CLIENT_USER_LEN];	/* user name ("login") */
	char info[CLIENT_INFO_LEN];	/* long user name */
	char server[CLIENT_HOST_LEN];	/* server name */
} WHOWAS;


GLOBAL void Client_Init PARAMS(( void ));
GLOBAL void Client_Exit PARAMS(( void ));

GLOBAL CLIENT *Client_NewLocal PARAMS(( CONN_ID Idx, const char *Hostname, int Type, bool Idented ));
GLOBAL CLIENT *Client_NewRemoteServer PARAMS(( CLIENT *Introducer, const char *Hostname, CLIENT *TopServer, int Hops, int Token, const char *Info, bool Idented ));
GLOBAL CLIENT *Client_NewRemoteUser PARAMS(( CLIENT *Introducer, const char *Nick, int Hops, const char *User, const char *Hostname, int Token, const char *Modes, const char *Info, bool Idented ));

GLOBAL void Client_Destroy PARAMS(( CLIENT *Client, const char *LogMsg, const char *FwdMsg, bool SendQuit ));

GLOBAL CLIENT *Client_ThisServer PARAMS(( void ));

GLOBAL CLIENT *Client_GetFromToken PARAMS(( CLIENT *Client, int Token ));

GLOBAL bool Client_Announce PARAMS(( CLIENT *Client, CLIENT *Prefix, CLIENT *User ));

GLOBAL CLIENT *Client_Search PARAMS(( const char *ID ));
GLOBAL CLIENT *Client_SearchServer PARAMS(( const char *ID ));
GLOBAL CLIENT *Client_First PARAMS(( void ));
GLOBAL CLIENT *Client_Next PARAMS(( CLIENT *c ));

GLOBAL int Client_Type PARAMS(( CLIENT *Client ));
GLOBAL CONN_ID Client_Conn PARAMS(( CLIENT *Client ));
GLOBAL char *Client_ID PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Mask PARAMS(( CLIENT *Client ));
GLOBAL char *Client_MaskCloaked PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Info PARAMS(( CLIENT *Client ));
GLOBAL char *Client_User PARAMS(( CLIENT *Client ));
#ifdef PAM
GLOBAL char *Client_OrigUser PARAMS(( CLIENT *Client ));
#endif
GLOBAL char *Client_Hostname PARAMS(( CLIENT *Client ));
GLOBAL char *Client_HostnameCloaked PARAMS((CLIENT *Client));
GLOBAL char *Client_HostnameDisplayed PARAMS(( CLIENT *Client ));
GLOBAL const char *Client_IPAText PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Modes PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Flags PARAMS(( CLIENT *Client ));
GLOBAL CLIENT *Client_Introducer PARAMS(( CLIENT *Client ));
GLOBAL int Client_Hops PARAMS(( CLIENT *Client ));
GLOBAL int Client_Token PARAMS(( CLIENT *Client ));
GLOBAL int Client_MyToken PARAMS(( CLIENT *Client ));
GLOBAL CLIENT *Client_TopServer PARAMS(( CLIENT *Client ));
GLOBAL CLIENT *Client_NextHop PARAMS(( CLIENT *Client ));
GLOBAL char *Client_Away PARAMS(( CLIENT *Client ));
GLOBAL char *Client_AccountName PARAMS((CLIENT *Client));
GLOBAL time_t Client_StartTime PARAMS(( CLIENT *Client ));

GLOBAL bool Client_HasMode PARAMS(( CLIENT *Client, char Mode ));
GLOBAL bool Client_HasFlag PARAMS(( CLIENT *Client, char Flag ));

GLOBAL void Client_SetHostname PARAMS(( CLIENT *Client, const char *Hostname ));
GLOBAL void Client_SetIPAText PARAMS(( CLIENT *Client, const char *IPAText ));
GLOBAL void Client_SetID PARAMS(( CLIENT *Client, const char *Nick ));
GLOBAL void Client_SetUser PARAMS(( CLIENT *Client, const char *User, bool Idented ));
GLOBAL void Client_SetOrigUser PARAMS(( CLIENT *Client, const char *User ));
GLOBAL void Client_SetInfo PARAMS(( CLIENT *Client, const char *Info ));
GLOBAL void Client_SetType PARAMS(( CLIENT *Client, int Type ));
GLOBAL void Client_SetHops PARAMS(( CLIENT *Client, int Hops ));
GLOBAL void Client_SetToken PARAMS(( CLIENT *Client, int Token ));
GLOBAL void Client_SetModes PARAMS(( CLIENT *Client, const char *Modes ));
GLOBAL void Client_SetFlags PARAMS(( CLIENT *Client, const char *Flags ));
GLOBAL void Client_SetIntroducer PARAMS(( CLIENT *Client, CLIENT *Introducer ));
GLOBAL void Client_SetAway PARAMS(( CLIENT *Client, const char *Txt ));
GLOBAL void Client_SetAccountName PARAMS((CLIENT *Client, const char *AccountName));

GLOBAL bool Client_ModeAdd PARAMS(( CLIENT *Client, char Mode ));
GLOBAL bool Client_ModeDel PARAMS(( CLIENT *Client, char Mode ));

GLOBAL bool Client_CheckNick PARAMS(( CLIENT *Client, char *Nick ));
GLOBAL bool Client_CheckID PARAMS(( CLIENT *Client, char *ID ));

GLOBAL long Client_UserCount PARAMS(( void ));
GLOBAL long Client_ServiceCount PARAMS(( void ));
GLOBAL long Client_ServerCount PARAMS(( void ));
GLOBAL unsigned long Client_OperCount PARAMS(( void ));
GLOBAL unsigned long Client_UnknownCount PARAMS(( void ));
GLOBAL long Client_MyUserCount PARAMS(( void ));
GLOBAL long Client_MyServiceCount PARAMS(( void ));
GLOBAL unsigned long Client_MyServerCount PARAMS(( void ));
GLOBAL long Client_MaxUserCount PARAMS((  void ));
GLOBAL long Client_MyMaxUserCount PARAMS((  void ));

GLOBAL bool Client_IsValidNick PARAMS(( const char *Nick ));

GLOBAL WHOWAS *Client_GetWhowas PARAMS(( void ));
GLOBAL int Client_GetLastWhowasIndex PARAMS(( void ));

GLOBAL void Client_RegisterWhowas PARAMS(( CLIENT *Client ));

GLOBAL const char *Client_TypeText PARAMS((CLIENT *Client));

GLOBAL void Client_Reject PARAMS((CLIENT *Client, const char *Reason,
				  bool InformClient));
GLOBAL void Client_Introduce PARAMS((CLIENT *From, CLIENT *Client, int Type));

GLOBAL void Client_UpdateCloakedHostname PARAMS((CLIENT *Client,
						 CLIENT *Originator,
						 const char *hostname));


#ifdef DEBUG
GLOBAL void Client_DebugDump PARAMS((void));
#endif

#endif

/* -eof- */
