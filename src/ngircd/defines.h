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
 * $Id: defines.h,v 1.5 2002/01/18 15:51:44 alex Exp $
 *
 * defines.h: (globale) Konstanten
 *
 * $Log: defines.h,v $
 * Revision 1.5  2002/01/18 15:51:44  alex
 * - Server-Verbinungen werden beim Start erst nach kurzer Pause aufgebaut.
 *
 * Revision 1.4  2002/01/07 15:29:53  alex
 * - PASSSERVERADD definiert, wird beim PASS-Befehl an Server verwendet.
 *
 * Revision 1.3  2002/01/03 02:24:00  alex
 * - Protokollversion und Suffix definiert.
 *
 * Revision 1.2  2002/01/02 02:44:36  alex
 * - neue Defines fuer max. Anzahl Server und Operatoren.
 *
 * Revision 1.1  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 */

#ifndef __defines_h__
#define __defines_h__


#define NONE -1

#define FNAME_LEN 256			/* max. Laenge eines Dateinamen */

#define LINE_LEN 256			/* max. Laenge einer Konfigurationszeile */

#define HOST_LEN 256			/* max. Laenge eines Hostnamen */

#define MAX_LISTEN_PORTS 16		/* max. Anzahl von Listen-Ports */

#define MAX_OPERATORS 8			/* max. Anzahl konfigurierbarer Operatoren */

#define MAX_SERVERS 8			/* max. Anzahl konfigurierbarer Server ("Peers") */

#define MAX_CONNECTIONS 100		/* max. Anzahl von Verbindungen an diesem Server */

#define MAX_CHANNELS 32			/* max. Anzahl Channels pro Nick */

#define CLIENT_ID_LEN 64		/* max. ID-Laenge; vgl. RFC 2812, 1.1 und 1.2.1 */
#define CLIENT_NICK_LEN 10		/* max. Nick-Laenge; vgl. RFC 2812, 1.2.1 */
#define CLIENT_PASS_LEN 9		/* max. Laenge des Passwortes */
#define CLIENT_USER_LEN 9		/* max. Laenge des Benutzernamen ("Login") */
#define CLIENT_NAME_LEN 32		/* max. Laenge des "langen Benutzernamen" */
#define CLIENT_HOST_LEN 64		/* max. Laenge des Hostname */
#define CLIENT_MODE_LEN 8		/* max. Laenge der Client-Modes */
#define CLIENT_INFO_LEN 64		/* max. Infotext-Laenge (Server) */

#define COMMAND_LEN 513			/* max. Laenge eines Befehls, vgl. RFC 2812, 3.2 */

#define CHANNEL_NAME_LEN 51		/* max. Laenge eines Channel-Namens, vgl. RFC 2812, 1.3 */

#define READBUFFER_LEN 2 * COMMAND_LEN	/* Laenge des Lesepuffers je Verbindung (Bytes) */
#define WRITEBUFFER_LEN 4096		/* Laenge des Schreibpuffers je Verbindung (Bytes) */

#define PROTOVER "0210"			/* implementierte Protokoll-Version (RFC 2813, 4.1.1) */
#define PROTOSUFFIX "-ngIRCd"		/* Protokoll-Suffix (RFC 2813, 4.1.1) */

#define PASSSERVERADD PROTOVER""PROTOSUFFIX" IRC|"PACKAGE"-"VERSION" P"

#define STARTUP_DELAY 3			/* Erst n Sek. nach Start zu anderen Servern verbinden */

#endif


/* -eof- */
