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
 * $Id: client.h,v 1.1 2001/12/14 08:13:43 alex Exp $
 *
 * client.h: Konfiguration des ngircd (Header)
 *
 * $Log: client.h,v $
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 *
 */


#ifndef __client_h__
#define __client_h__

#include "conn.h"


#define ID_LEN 63			/* max. ID-Laenge; vgl. RFC 2812, 1.1 und 1.2.1 */
#define NICK_LEN 9			/* max. Nick-Laenge; vgl. RFC 2812, 1.2.1 */
#define PASS_LEN 9			/* max. Laenge des Passwortes */

#define MAX_CHANNELS 32			/* max. Anzahl Channels pro Nick */


typedef enum
{
	CLIENT_UNKNOWN,			/* Verbindung mit (noch) unbekanntem Typ */
	CLIENT_USER,			/* Client ist ein Benutzer */
	CLIENT_SERVER,			/* Client ist ein Server */
	CLIENT_SERVICE			/* Client ist ein Service */
} CLIENT_TYPE;


typedef struct _CLIENT
{
	POINTER *next;			/* Zeiger auf naechste Client-Struktur */
	CLIENT_TYPE type;		/* Typ des Client, vgl. CLIENT_TYPE */
	CONN_ID conn_id;		/* ID der Connection (wenn lokal) bzw. NONE (remote) */
	POINTER *introducer;		/* ID des Servers, der die Verbindung hat */
	CHAR nick[ID_LEN + 1];		/* Nick (bzw. Server-ID, daher auch IDLEN!) */
	CHAR pass[PASS_LEN + 1];	/* Passwort, welches der Client angegeben hat */
	CHANNEL *channels[MAX_CHANNELS];/* IDs der Channel, bzw. NULL */
} CLIENT;


GLOBAL VOID Client_Init( VOID );
GLOBAL VOID Client_Exit( VOID );


#endif


/* -eof- */
