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
 * $Id: client.h,v 1.16 2002/01/05 23:26:05 alex Exp $
 *
 * client.h: Konfiguration des ngircd (Header)
 *
 * $Log: client.h,v $
 * Revision 1.16  2002/01/05 23:26:05  alex
 * - Vorbereitungen fuer Ident-Abfragen in Client-Strukturen.
 *
 * Revision 1.15  2002/01/05 20:08:17  alex
 * - neue Funktion Client_NextHop().
 *
 * Revision 1.14  2002/01/04 01:21:22  alex
 * - Client-Strukturen koennen von anderen Modulen nun nur noch ueber die
 *   enstprechenden (zum Teil neuen) Funktionen angesprochen werden.
 *
 * Revision 1.13  2002/01/03 02:28:06  alex
 * - neue Funktion Client_CheckID(), diverse Aenderungen fuer Server-Links.
 *
 * Revision 1.12  2002/01/02 02:42:58  alex
 * - Copyright-Texte aktualisiert.
 *
 * Revision 1.11  2001/12/31 15:33:13  alex
 * - neuer Befehl NAMES, kleinere Bugfixes.
 * - Bug bei PING behoben: war zu restriktiv implementiert :-)
 *
 * Revision 1.10  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.9  2001/12/29 20:18:18  alex
 * - neue Funktion Client_SetHostname().
 *
 * Revision 1.8  2001/12/29 03:10:47  alex
 * - Client-Modes implementiert; Loglevel mal wieder angepasst.
 *
 * Revision 1.7  2001/12/27 19:13:47  alex
 * - neue Funktion Client_Search(), besseres Logging.
 *
 * Revision 1.6  2001/12/27 16:54:51  alex
 * - neue Funktion Client_GetID(), liefert die "Client ID".
 *
 * Revision 1.5  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.4  2001/12/26 03:19:16  alex
 * - neue Funktion Client_Nick().
 *
 * Revision 1.3  2001/12/25 19:21:26  alex
 * - Client-Typ ("Status") besser unterteilt, My_Clients ist zudem nun global.
 *
 * Revision 1.2  2001/12/23 22:03:47  alex
 * - einige neue Funktionen,
 * - Konstanten um "CLIENT_"-Prefix erweitert.
 *
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 */


#ifndef __client_h__
#define __client_h__

#include "channel.h"
#include "conn.h"


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


#ifdef __client_c__
typedef struct _CLIENT
{
	CHAR id[CLIENT_ID_LEN];		/* Nick (User) bzw. ID (Server) */
	POINTER *next;			/* Zeiger auf naechste Client-Struktur */
	CLIENT_TYPE type;		/* Typ des Client, vgl. CLIENT_TYPE */
	CONN_ID conn_id;		/* ID der Connection (wenn lokal) bzw. NONE (remote) */
	struct _CLIENT *introducer;	/* ID des Servers, der die Verbindung hat */
	CHAR pwd[CLIENT_PASS_LEN];	/* Passwort, welches der Client angegeben hat */
	CHAR host[CLIENT_HOST_LEN];	/* Hostname des Client */
	CHAR user[CLIENT_USER_LEN];	/* Benutzername ("Login") */
	CHAR info[CLIENT_INFO_LEN];	/* Langer Benutzername (User) bzw. Infotext (Server) */
	CHANNEL *channels[MAX_CHANNELS];/* Channel, in denen der Client Mitglied ist */
	CHAR modes[CLIENT_MODE_LEN];	/* Client Modes */
	INT hops, token;		/* "Hops" und "Token" (-> SERVER-Befehl) */
	BOOLEAN oper_by_me;		/* IRC-Operator-Status durch diesen Server? */
} CLIENT;
#else
typedef POINTER CLIENT;
#endif


GLOBAL VOID Client_Init( VOID );
GLOBAL VOID Client_Exit( VOID );

GLOBAL CLIENT *Client_NewLocal( CONN_ID Idx, CHAR *Hostname, INT Type, BOOLEAN Idented );
GLOBAL CLIENT *Client_NewRemoteServer( CLIENT *Introducer, CHAR *Hostname, INT Hops, INT Token, CHAR *Info, BOOLEAN Idented );
GLOBAL CLIENT *Client_NewRemoteUser( CLIENT *Introducer, CHAR *Nick, INT Hops, CHAR *User, CHAR *Hostname, INT Token, CHAR *Modes, CHAR *Info, BOOLEAN Idented );
GLOBAL CLIENT *Client_New( CONN_ID Idx, CLIENT *Introducer, INT Type, CHAR *ID, CHAR *User, CHAR *Hostname, CHAR *Info, INT Hops, INT Token, CHAR *Modes, BOOLEAN Idented );

GLOBAL VOID Client_Destroy( CLIENT *Client );

GLOBAL CLIENT *Client_ThisServer( VOID );

GLOBAL CLIENT *Client_GetFromConn( CONN_ID Idx );
GLOBAL CLIENT *Client_GetFromID( CHAR *Nick );
GLOBAL CLIENT *Client_GetFromToken( CLIENT *Client, INT Token );

GLOBAL CLIENT *Client_Search( CHAR *ID );
GLOBAL CLIENT *Client_First( VOID );
GLOBAL CLIENT *Client_Next( CLIENT *c );

GLOBAL INT Client_Type( CLIENT *Client );
GLOBAL CONN_ID Client_Conn( CLIENT *Client );
GLOBAL CHAR *Client_ID( CLIENT *Client );
GLOBAL CHAR *Client_Mask( CLIENT *Client );
GLOBAL CHAR *Client_Info( CLIENT *Client );
GLOBAL CHAR *Client_User( CLIENT *Client );
GLOBAL CHAR *Client_Hostname( CLIENT *Client );
GLOBAL CHAR *Client_Password( CLIENT *Client );
GLOBAL CHAR *Client_Modes( CLIENT *Client );
GLOBAL CLIENT *Client_Introducer( CLIENT *Client );
GLOBAL BOOLEAN Client_OperByMe( CLIENT *Client );
GLOBAL INT Client_Hops( CLIENT *Client );
GLOBAL INT Client_Token( CLIENT *Client );
GLOBAL CLIENT *Client_NextHop( CLIENT *Client );

GLOBAL BOOLEAN Client_HasMode( CLIENT *Client, CHAR Mode );

GLOBAL VOID Client_SetHostname( CLIENT *Client, CHAR *Hostname );
GLOBAL VOID Client_SetID( CLIENT *Client, CHAR *Nick );
GLOBAL VOID Client_SetUser( CLIENT *Client, CHAR *User, BOOLEAN Idented );
GLOBAL VOID Client_SetInfo( CLIENT *Client, CHAR *Info );
GLOBAL VOID Client_SetPassword( CLIENT *Client, CHAR *Pwd );
GLOBAL VOID Client_SetType( CLIENT *Client, INT Type );
GLOBAL VOID Client_SetHops( CLIENT *Client, INT Hops );
GLOBAL VOID Client_SetToken( CLIENT *Client, INT Token );
GLOBAL VOID Client_SetOperByMe( CLIENT *Client, BOOLEAN OperByMe );
GLOBAL VOID Client_SetModes( CLIENT *Client, CHAR *Modes );
GLOBAL VOID Client_SetIntroducer( CLIENT *Client, CLIENT *Introducer );

GLOBAL BOOLEAN Client_ModeAdd( CLIENT *Client, CHAR Mode );
GLOBAL BOOLEAN Client_ModeDel( CLIENT *Client, CHAR Mode );

GLOBAL BOOLEAN Client_CheckNick( CLIENT *Client, CHAR *Nick );
GLOBAL BOOLEAN Client_CheckID( CLIENT *Client, CHAR *ID );

#endif


/* -eof- */
