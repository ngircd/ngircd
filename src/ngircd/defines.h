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
 * $Id: defines.h,v 1.50 2005/03/02 16:35:11 alex Exp $
 *
 * Global defines of ngIRCd.
 */

#ifndef __defines_h__
#define __defines_h__


#define NONE -1

#define FNAME_LEN 256			/* max. length of file name */

#define LINE_LEN 256			/* max. length of a line in the configuration file */

#define HOST_LEN 256			/* max. lenght of fully qualified host names */

#define MAX_LISTEN_PORTS 16		/* max. count of listening ports */

#define MAX_OPERATORS 16		/* max. count of configurable operators */

#define MAX_SERVERS 16			/* max. count of configurable servers ("peers") */

#define MAX_DEFCHANNELS 16		/* max. count of preconfigurable channels */

#define MAX_SERVICES 8			/* max. number of configurable services */

#define CONNECTION_POOL 100		/* size of default connection pool */

#define CLIENT_ID_LEN 64		/* max. length of an IRC ID; see RFC 2812, 1.1 and 1.2.1 */
#define CLIENT_NICK_LEN 10		/* max. nick length; see. RFC 2812, 1.2.1 */
#define CLIENT_PASS_LEN 21		/* max. password length */
#define CLIENT_USER_LEN 9		/* max. length of user name ("login") */
#define CLIENT_NAME_LEN 32		/* max. length of "real names" */
#define CLIENT_HOST_LEN 64		/* max. host name length */
#define CLIENT_MODE_LEN 8		/* max. lenth of all client modes */
#define CLIENT_INFO_LEN 64		/* max. length of server info texts */
#define CLIENT_AWAY_LEN 128		/* max. length of away messages */
#define CLIENT_FLAGS_LEN 100		/* max. length of client flags */

#define CHANNEL_NAME_LEN 51		/* max. length of a channel name, see. RFC 2812, 1.3 */
#define CHANNEL_MODE_LEN 9		/* max. length of channel modes */
#define CHANNEL_TOPIC_LEN 128		/* max. length of a channel topic */

#define COMMAND_LEN 513			/* max. IRC command length, see. RFC 2812, 3.2 */

#define READBUFFER_LEN 2048		/* size of the read buffer of a connection (bytes) */
#define WRITEBUFFER_LEN 4096		/* size of the write buffer of a connection (bytes) */

#ifdef ZLIB
#define ZREADBUFFER_LEN 1024		/* compressed read buffer of a connection (bytes) */
#define ZWRITEBUFFER_LEN 4096		/* compressed write buffer of a connection (bytes) */
#endif

#define PROTOVER "0210"			/* implemented IRC protocol version (see RFC 2813, 4.1.1) */
#define PROTOIRC "-IRC"			/* protocol suffix (see RFC 2813, 4.1.1) */
#define PROTOIRCPLUS "-IRC+"		/* protokol suffix for IRC+ protocol (see doc/Protocol.txt) */

#ifdef IRCPLUS
# define IRCPLUSFLAGS "CL"		/* standard IRC+ flags */
#endif

#define STARTUP_DELAY 1			/* delay outgoing connections n seconds after startup */
#define RECONNECT_DELAY 3		/* time to delay re-connect attempts (seconds) */

#define USERMODES "aios"		/* supported user modes */
#define CHANMODES "biklImnoPstv"	/* supported channel modes */

#define CONNECTED TRUE			/* internal status codes */
#define DISCONNECTED FALSE

#define DEFAULT_AWAY_MSG "Away"		/* away message for users connected to linked servers */

#define CONFIG_FILE "/ngircd.conf"
#define MOTD_FILE "/ngircd.motd"
#define MOTD_PHRASE ""
#define CHROOT_DIR ""
#define PID_FILE ""

#define ERROR_DIR "/tmp"

#define MAX_LOG_MSG_LEN 256		/* max. length of a log message */

#define TOKEN_OUTBOUND -2		/* tag for outbound server links */

#define NOTICE_TXTPREFIX ""		/* prefix for NOTICEs from the server to users */

#ifdef RENDEZVOUS
#define RENDEZVOUS_TYPE "_ircu._tcp."	/* service type to register with Rendezvous */
#endif


#endif


/* -eof- */
