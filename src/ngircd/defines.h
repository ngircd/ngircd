/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2003 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: defines.h,v 1.45 2004/04/25 15:46:50 alex Exp $
 *
 * Global defines of ngIRCd.
 */

#ifndef __defines_h__
#define __defines_h__


#define NONE -1

#define FNAME_LEN 256			/* max. Laenge eines Dateinamen */

#define LINE_LEN 256			/* max. Laenge einer Konfigurationszeile */

#define HOST_LEN 256			/* max. Laenge eines Hostnamen */

#define MAX_LISTEN_PORTS 16		/* max. Anzahl von Listen-Ports */

#define MAX_OPERATORS 16		/* max. Anzahl konfigurierbarer Operatoren */

#define MAX_SERVERS 16			/* max. Anzahl konfigurierbarer Server ("Peers") */

#define MAX_DEFCHANNELS 16		/* max. Anzahl vorkonfigurierbarerr Channels */

#define MAX_SERVICES 8			/* maximum number of configurable services */

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

#define READBUFFER_LEN 2048		/* Laenge des Lesepuffers je Verbindung (Bytes) */
#define WRITEBUFFER_LEN 4096		/* Laenge des Schreibpuffers je Verbindung (Bytes) */

#ifdef ZLIB
#define ZREADBUFFER_LEN 1024		/* Laenge des Lesepuffers je Verbindung (Bytes) */
#define ZWRITEBUFFER_LEN 4096		/* Laenge des Schreibpuffers fuer Kompression (Bytes) */
#endif

#define PROTOVER "0210"			/* implementierte Protokoll-Version (RFC 2813, 4.1.1) */
#define PROTOIRC "-IRC"			/* Protokoll-Suffix (RFC 2813, 4.1.1) */
#define PROTOIRCPLUS "-IRC+"		/* Protokoll-Suffix für IRC+-Protokoll */

#ifdef IRCPLUS
# define IRCPLUSFLAGS "CL"		/* IRC+-Flags, die immer zutreffen */
#endif

#define STARTUP_DELAY 1			/* Erst n Sek. nach Start zu anderen Servern verbinden */
#define RECONNECT_DELAY 3		/* Server-Links erst nach 3 Sekunden versuchen, wieder aufzubauen */

#define USERMODES "aios"		/* unterstuetzte User-Modes */
#define CHANMODES "biklImnoPtv"		/* unterstuetzte Channel-Modes */

#define CONNECTED TRUE			/* fuer die irc-xxx-Module */
#define DISCONNECTED FALSE

#define DEFAULT_AWAY_MSG "Away"		/* Away-Meldung fuer User von anderen Servern */

#define CONFIG_FILE "/ngircd.conf"
#define MOTD_FILE "/ngircd.motd"

#define ERROR_DIR "/tmp"

#define MAX_LOG_MSG_LEN 256		/* max. Laenge einer Log-Meldung */

#define TOKEN_OUTBOUND -2		/* Kennzeichnung fuer ausgehende Server-Links im Aufbau */

#define NOTICE_TXTPREFIX ""		/* Kennzeichnung fuer Server-NOTICEs an User */

#ifdef RENDEZVOUS
#define RENDEZVOUS_TYPE "_ircu._tcp."	/* Service type to register with Rendezvous */
#endif


#endif


/* -eof- */
