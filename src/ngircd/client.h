/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an comBase beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: client.h,v 1.2 2001/12/23 22:03:47 alex Exp $
 *
 * client.h: Konfiguration des ngircd (Header)
 *
 * $Log: client.h,v $
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


#define CLIENT_ID_LEN 63		/* max. ID-Laenge; vgl. RFC 2812, 1.1 und 1.2.1 */
#define CLIENT_NICK_LEN 9		/* max. Nick-Laenge; vgl. RFC 2812, 1.2.1 */
#define CLIENT_PASS_LEN 9		/* max. Laenge des Passwortes */
#define CLIENT_USER_LEN 8		/* max. Laenge des Benutzernamen ("Login") */
#define CLIENT_NAME_LEN 32		/* max. Laenge des "langen Benutzernamen" */
#define CLIENT_HOST_LEN 63		/* max. Laenge des Hostname */

#define MAX_CHANNELS 32			/* max. Anzahl Channels pro Nick */


typedef enum
{
	CLIENT_UNKNOWN,			/* Verbindung mit (noch) unbekanntem Typ */
	CLIENT_PASS,			/* Client hat PASS gesendet */
	CLIENT_NICK,			/* NICK wurde gesendet */
	CLIENT_USER,			/* Client ist ein Benutzer (USER wurde gesendet) */
	CLIENT_SERVER,			/* Client ist ein Server */
	CLIENT_SERVICE			/* Client ist ein Service */
} CLIENT_TYPE;


/* Das Verfahren, mit dem neue Client-Verbindungen beim Server registriert
 * werden, ist in RFC 2812, Abschnitt 3.1 ff. definiert. */


typedef struct _CLIENT
{
	POINTER *next;			/* Zeiger auf naechste Client-Struktur */
	CLIENT_TYPE type;		/* Typ des Client, vgl. CLIENT_TYPE */
	CONN_ID conn_id;		/* ID der Connection (wenn lokal) bzw. NONE (remote) */
	POINTER *introducer;		/* ID des Servers, der die Verbindung hat */
	CHAR nick[CLIENT_ID_LEN + 1];	/* Nick (bzw. Server-ID, daher auch IDLEN!) */
	CHAR pass[CLIENT_PASS_LEN + 1];	/* Passwort, welches der Client angegeben hat */
	CHAR host[CLIENT_HOST_LEN + 1];	/* Hostname des Client */
	CHAR user[CLIENT_USER_LEN + 1];	/* Benutzername ("Login") */
	CHAR name[CLIENT_NAME_LEN + 1];	/* Langer Benutzername */
	CHANNEL *channels[MAX_CHANNELS];/* IDs der Channel, bzw. NULL */
} CLIENT;


GLOBAL CLIENT *This_Server;


GLOBAL VOID Client_Init( VOID );
GLOBAL VOID Client_Exit( VOID );

GLOBAL CLIENT *Client_New_Local( CONN_ID Idx, CHAR *Hostname );
GLOBAL VOID Client_Destroy( CLIENT *Client );
GLOBAL CLIENT *Client_GetFromConn( CONN_ID Idx );


#endif


/* -eof- */
