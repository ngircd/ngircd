/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: client.h,v 1.10 2001/12/31 02:18:51 alex Exp $
 *
 * client.h: Konfiguration des ngircd (Header)
 *
 * $Log: client.h,v $
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
	CLIENT_SERVER,			/* Client ist ein Server */
	CLIENT_SERVICE			/* Client ist ein Service */
} CLIENT_TYPE;


typedef struct _CLIENT
{
	POINTER *next;			/* Zeiger auf naechste Client-Struktur */
	CLIENT_TYPE type;		/* Typ des Client, vgl. CLIENT_TYPE */
	CONN_ID conn_id;		/* ID der Connection (wenn lokal) bzw. NONE (remote) */
	struct _CLIENT *introducer;	/* ID des Servers, der die Verbindung hat */
	CHAR nick[CLIENT_ID_LEN];	/* Nick (bzw. Server-ID, daher auch IDLEN!) */
	CHAR pass[CLIENT_PASS_LEN];	/* Passwort, welches der Client angegeben hat */
	CHAR host[CLIENT_HOST_LEN];	/* Hostname des Client */
	CHAR user[CLIENT_USER_LEN];	/* Benutzername ("Login") */
	CHAR name[CLIENT_NAME_LEN];	/* Langer Benutzername */
	CHAR info[CLIENT_INFO_LEN];	/* Infotext (Server) */
	CHANNEL *channels[MAX_CHANNELS];/* IDs der Channel, bzw. NULL */
	CHAR modes[CLIENT_MODE_LEN];	/* Client Modes */
	BOOLEAN oper_by_me;		/* Operator-Status durch diesen Server? */
} CLIENT;


GLOBAL CLIENT *This_Server;


GLOBAL VOID Client_Init( VOID );
GLOBAL VOID Client_Exit( VOID );

GLOBAL CLIENT *Client_NewLocal( CONN_ID Idx, CHAR *Hostname );
GLOBAL VOID Client_Destroy( CLIENT *Client );
GLOBAL VOID Client_SetHostname( CLIENT *Client, CHAR *Hostname );
GLOBAL CLIENT *Client_GetFromConn( CONN_ID Idx );
GLOBAL CLIENT *Client_GetFromNick( CHAR *Nick );
GLOBAL CHAR *Client_Nick( CLIENT *Client );
GLOBAL BOOLEAN Client_CheckNick( CLIENT *Client, CHAR *Nick );
GLOBAL CHAR *Client_GetID( CLIENT *Client );
GLOBAL CLIENT *Client_Search( CHAR *ID );


#endif


/* -eof- */
