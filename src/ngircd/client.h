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
 * $Id: client.h,v 1.28 2002/05/27 13:09:26 alex Exp $
 *
 * client.h: Konfiguration des ngircd (Header)
 */


#ifndef __client_h__
#define __client_h__


typedef enum
{
	CLIENT_UNKNOWN,			/* Verbindung mit (noch) unbekanntem Typ */
	CLIENT_GOTPASS,			/* Client hat PASS gesendet */
	CLIENT_GOTNICK,			/* Client hat NICK gesendet */
	CLIENT_GOTUSER,			/* Client hat USER gesendet */
	CLIENT_USER,			/* Client ist ein Benutzer (USER wurde gesendet) */
	CLIENT_UNKNOWNSERVER,		/* unregistrierte Server-Verbindung */
	CLIENT_GOTPASSSERVER,		/* Client hat PASS nach "Server-Art" gesendet */
	CLIENT_SERVER,			/* Client ist ein Server */
	CLIENT_SERVICE			/* Client ist ein Service */
} CLIENT_TYPE;


#if defined(__client_c__) | defined(S_SPLINT_S)

#include "defines.h"

typedef struct _CLIENT
{
	CHAR id[CLIENT_ID_LEN];		/* Nick (User) bzw. ID (Server) */
	UINT32 hash;			/* Hash ueber die (kleingeschriebene) ID */
	POINTER *next;			/* Zeiger auf naechste Client-Struktur */
	CLIENT_TYPE type;		/* Typ des Client, vgl. CLIENT_TYPE */
	CONN_ID conn_id;		/* ID der Connection (wenn lokal) bzw. NONE (remote) */
	struct _CLIENT *introducer;	/* ID des Servers, der die Verbindung hat */
	struct _CLIENT *topserver;	/* Toplevel-Servers (nur gueltig, wenn Client ein Server ist) */
	CHAR pwd[CLIENT_PASS_LEN];	/* Passwort, welches der Client angegeben hat */
	CHAR host[CLIENT_HOST_LEN];	/* Hostname des Client */
	CHAR user[CLIENT_USER_LEN];	/* Benutzername ("Login") */
	CHAR info[CLIENT_INFO_LEN];	/* Langer Benutzername (User) bzw. Infotext (Server) */
	CHAR modes[CLIENT_MODE_LEN];	/* Client Modes */
	INT hops, token, mytoken;	/* "Hops" und "Token" (-> SERVER-Befehl) */
	BOOLEAN oper_by_me;		/* IRC-Operator-Status durch diesen Server? */
	CHAR away[CLIENT_AWAY_LEN];	/* AWAY-Text, wenn Mode 'a' gesetzt */
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
GLOBAL VOID Client_SetIntroducer PARAMS((CLIENT *Client, CLIENT *Introducer ));
GLOBAL VOID Client_SetAway PARAMS((CLIENT *Client, CHAR *Txt ));

GLOBAL BOOLEAN Client_ModeAdd PARAMS((CLIENT *Client, CHAR Mode ));
GLOBAL BOOLEAN Client_ModeDel PARAMS((CLIENT *Client, CHAR Mode ));

GLOBAL BOOLEAN Client_CheckNick PARAMS((CLIENT *Client, CHAR *Nick ));
GLOBAL BOOLEAN Client_CheckID PARAMS((CLIENT *Client, CHAR *ID ));

GLOBAL INT Client_UserCount PARAMS((VOID ));
GLOBAL INT Client_ServiceCount PARAMS((VOID ));
GLOBAL INT Client_ServerCount PARAMS((VOID ));
GLOBAL INT Client_OperCount PARAMS((VOID ));
GLOBAL INT Client_UnknownCount PARAMS((VOID ));
GLOBAL INT Client_MyUserCount PARAMS((VOID ));
GLOBAL INT Client_MyServiceCount PARAMS((VOID ));
GLOBAL INT Client_MyServerCount PARAMS((VOID ));

GLOBAL BOOLEAN Client_IsValidNick PARAMS((CHAR *Nick ));


#endif


/* -eof- */
