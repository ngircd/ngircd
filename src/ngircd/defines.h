/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2014 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __defines_h__
#define __defines_h__

/**
 * @file
 * Global constants ("#defines") used by the ngIRCd.
 */


/* Internal flags */

/** Flag: there is no connection. */
#define NONE -1

/** Flag: connection is (still) established. */
#define CONNECTED true

/** Flag: connection isn't established (any more). */
#define DISCONNECTED false

/** Tag for outbound server links. */
#define TOKEN_OUTBOUND -2


/* Generic buffer sizes */

/** Max. length of a line in the configuration file. */
#define LINE_LEN 1024

/** Max. length of a log message. */
#define MAX_LOG_MSG_LEN 256

/** Max. length of file name. */
#define FNAME_LEN 256

/** Max. length of fully qualified host names (e. g. "abc.domain.tld"). */
#define HOST_LEN 256

/** Max. length of random salt */
#define RANDOM_SALT_LEN 32

/* Size of structures */

/** Max. count of configurable servers. */
#define MAX_SERVERS 64

/** Max. number of WHOWAS list items that can be stored. */
#define MAX_WHOWAS 64

/** Size of default connection pool. */
#define CONNECTION_POOL 100

/** Size of buffer for PAM service name. */
#define MAX_PAM_SERVICE_NAME_LEN 64


/* Hard-coded (default) options */

/** Delay after startup before outgoing connections are initiated in seconds. */
#define STARTUP_DELAY 1

/** Time to delay re-connect attempts in seconds. */
#define RECONNECT_DELAY 3

/** Configuration file name. */
#define CONFIG_FILE "/ngircd.conf"

/** Directory containing optional configuration snippets. */
#define CONFIG_DIR "/ngircd.conf.d"

/** Name of the MOTD file. */
#define MOTD_FILE "/ngircd.motd"

/** Name of the help file. */
#define HELP_FILE "/Commands.txt"

/** Default chroot() directory. */
#define CHROOT_DIR ""

/** Default file for the process ID. */
#define PID_FILE ""


/* Sizes of "IRC elements": nicks, users, ... */

/** Max. length of an IRC ID (incl. NULL); see RFC 2812 section 1.1 and 1.2.1. */
#define CLIENT_ID_LEN 64

/** Default nick length (including NULL), see. RFC 2812 section 1.2.1. */
#define CLIENT_NICK_LEN_DEFAULT 10

/** Maximum nickname length (including NULL). */
#define CLIENT_NICK_LEN 32

/** Max. password length (including NULL). */
#define CLIENT_PASS_LEN 65

/** Max. length of user name ("login"; incl. NULL), RFC 2812, section 1.2.1. */
#ifndef STRICT_RFC
# define CLIENT_USER_LEN 20
#else
# define CLIENT_USER_LEN 10
#endif
/** Max. length of user names saved for authentication (used by PAM) */
#ifdef PAM
# define CLIENT_AUTHUSER_LEN 64
#endif

/** Max. length of "real names" (including NULL). */
#define CLIENT_NAME_LEN 32

/** Max. host name length (including NULL). */
#define CLIENT_HOST_LEN 64

/** Max. mask lenght (including NULL). */
#define MASK_LEN (2 * CLIENT_HOST_LEN)

/** Max. length of all client modes (including NULL). */
#define CLIENT_MODE_LEN 21

/** Max. length of server info texts (including NULL). */
#define CLIENT_INFO_LEN 128

/** Max. length of away messages (including NULL). */
#define CLIENT_AWAY_LEN 128

/** Max. length of client flags (including NULL). */
#define CLIENT_FLAGS_LEN 16

/** Max. length of a channel name (including NULL), see RFC 2812 section 1.3. */
#define CHANNEL_NAME_LEN 51

/** Max. length of channel modes (including NULL). */
#define CHANNEL_MODE_LEN 21

/** Max. IRC command length (including NULL), see. RFC 2812 section 3.2. */
#define COMMAND_LEN 513


/* Read and write buffer sizes */

/** Size of the read buffer of a connection in bytes. */
#define READBUFFER_LEN 2048

/** Size that triggers write buffer flushing if more space is needed. */
#define WRITEBUFFER_FLUSH_LEN 4096

/** Maximum size of the write buffer of a connection in bytes. */
#define WRITEBUFFER_MAX_LEN 32768

/** Maximum size of the write buffer of a server link connection in bytes. */
#define WRITEBUFFER_SLINK_LEN 65536


/* IRC/IRC+ protocol */

/** Implemented IRC protocol version, see RFC 2813 section 4.1.1. */
#define PROTOVER "0210"

/** Protocol suffix, see RFC 2813 section 4.1.1. */
#define PROTOIRC "-IRC"

/** Protocol suffix used by the IRC+ protocol, see <doc/Protocol.txt>. */
#define PROTOIRCPLUS "-IRC+"

#ifdef IRCPLUS
/** Standard IRC+ flags. */
# define IRCPLUSFLAGS "CHLMSX"
#endif

/** Supported user modes. */
#define USERMODES "abBcCFiIoqrRswx"

/** Supported channel modes. */
#define CHANMODES "abehiIklmMnoOPqQrRstvVz"

/** Supported channel types. */
#define CHANTYPES "#&+"

/** Away message for users connected to linked servers. */
#define DEFAULT_AWAY_MSG "Away"

/** Default ID for "topic owner". */
#define DEFAULT_TOPIC_ID "-Server-"

/** Prefix for NOTICEs from the server to users. Some servers use '*'. */
#define NOTICE_TXTPREFIX ""

/** Suffix for oversized messages that have been shortened and cut off. */
#define CUT_TXTSUFFIX "[CUT]"


/* Defaults and limits for IRC commands */

/** Max. number of elemets allowed in channel invite and ban lists. */
#define MAX_HNDL_CHANNEL_LISTS 50

/** Max. number of channel modes with arguments per MODE command. */
#define MAX_HNDL_MODES_ARG 5

/** Max. number of targets per PRIVMSG/NOTICE/... command. */
#define MAX_HNDL_TARGETS 25

/** Max. number of WHO replies. */
#define MAX_RPL_WHO 25

/** Max. number of WHOIS replies. */
#define MAX_RPL_WHOIS 10

/** Default count of WHOWAS command replies. */
#define DEF_RPL_WHOWAS 5

/** Max count of WHOWAS command replies. */
#define MAX_RPL_WHOWAS 25


#endif

/* -eof- */
