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
 * $Id: defines.h,v 1.36 2002/11/23 16:08:18 alex Exp $
 *
 * defines.h: (globale) Konstanten
 */

#ifndef __defines_h__
#define __defines_h__


#define NONE -1

#define TIME_RES 2			/* Zeit-Aufloesung des Servers in Sekunden */

#define FNAME_LEN 256			/* max. Laenge eines Dateinamen */

#define LINE_LEN 256			/* max. Laenge einer Konfigurationszeile */

#define HOST_LEN 256			/* max. Laenge eines Hostnamen */

#define MAX_LISTEN_PORTS 16		/* max. Anzahl von Listen-Ports */

#define MAX_OPERATORS 16		/* max. Anzahl konfigurierbarer Operatoren */

#define MAX_SERVERS 16			/* max. Anzahl konfigurierbarer Server ("Peers") */

#define MAX_DEFCHANNELS 16		/* max. Anzahl vorkonfigurierbarerr Channels */

#define CONNECTION_POOL 100		/* Anzahl Verbindungs-Strukturen, die blockweise alloziert werden */

#define CLIENT_ID_LEN 64		/* max. ID-Laenge; vgl. RFC 2812, 1.1 und 1.2.1 */
#define CLIENT_NICK_LEN 10		/* max. Nick-Laenge; vgl. RFC 2812, 1.2.1 */
#define CLIENT_PASS_LEN 9		/* max. Laenge des Passwortes */
#define CLIENT_USER_LEN 9		/* max. Laenge des Benutzernamen ("Login") */
#define CLIENT_NAME_LEN 32		/* max. Laenge des "langen Benutzernamen" */
#define CLIENT_HOST_LEN 64		/* max. Laenge des Hostname */
#define CLIENT_MODE_LEN 8		/* max. Laenge der Client-Modes */
#define CLIENT_INFO_LEN 64		/* max. Infotext-Laenge (Server) */
#define CLIENT_AWAY_LEN 128		/* max. Laenger der AWAY-Nachricht */
#define CLIENT_FLAGS_LEN 100		/* max. Laenger der Client-Flags */

#define CHANNEL_NAME_LEN 51		/* max. Laenge eines Channel-Namens, vgl. RFC 2812, 1.3 */
#define CHANNEL_MODE_LEN 8		/* max. Laenge der Channel-Modes */
#define CHANNEL_TOPIC_LEN 128		/* max. Laenge eines Channel-Topics */

#define COMMAND_LEN 513			/* max. Laenge eines Befehls, vgl. RFC 2812, 3.2 */

#define READBUFFER_LEN 2 * COMMAND_LEN	/* Laenge des Lesepuffers je Verbindung (Bytes) */
#define WRITEBUFFER_LEN 4096		/* Laenge des Schreibpuffers je Verbindung (Bytes) */

#define PROTOVER "0210"			/* implementierte Protokoll-Version (RFC 2813, 4.1.1) */
#define PROTOIRC "-IRC"			/* Protokoll-Suffix (RFC 2813, 4.1.1) */
#define PROTOIRCPLUS "-IRC+"		/* Protokoll-Suffix für IRC+-Protokoll */

#ifdef IRCPLUS
# define IRCPLUSFLAGS "C"		/* IRC+-Flags, die immer zutreffen */
#endif

#define STARTUP_DELAY 1			/* Erst n Sek. nach Start zu anderen Servern verbinden */
#define RECONNECT_DELAY 3		/* Server-Links erst nach 3 Sekunden versuchen, wieder aufzubauen */

#define USERMODES "aios"		/* unterstuetzte User-Modes */
#define CHANMODES "biImnoPtv"		/* unterstuetzte Channel-Modes */

#define CONNECTED TRUE			/* fuer die irc-xxx-Module */
#define DISCONNECTED FALSE

#define DEFAULT_AWAY_MSG "Away"		/* Away-Meldung fuer User von anderen Servern */

#ifdef PROTOTYPES
# define CONFIG_FILE SYSCONFDIR"/ngircd.conf"
# define MOTD_FILE SYSCONFDIR"/ngircd.motd"
#else
# define CONFIG_FILE "ngircd.conf"
# define MOTD_FILE "ngircd.motd"
#endif
#define ERROR_DIR "/tmp"

#define MAX_LOG_MSG_LEN 256		/* max. Laenge einer Log-Meldung */

#define TOKEN_OUTBOUND -2		/* Kennzeichnung fuer ausgehende Server-Links im Aufbau */

#define NOTICE_TXTPREFIX "*** "		/* Kennzeichnung fuer Server-NOTICEs an User */


#endif


/* -eof- */
