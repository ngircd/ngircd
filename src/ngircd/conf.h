/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2013 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __conf_h__
#define __conf_h__

/**
 * @file
 * Configuration management (header)
 */

#include <time.h>

#include "defines.h"
#include "array.h"
#include "portab.h"
#include "tool.h"
#include "ng_ipaddr.h"
#include "proc.h"
#include "conf-ssl.h"

/**
 * Configured IRC operator.
 * Please note that the name of the IRC operator and his nick have nothing to
 * do with each other! The IRC operator is only identified by the name and
 * password configured in this structure.
 */
struct Conf_Oper {
	char name[CLIENT_PASS_LEN];	/**< Name (ID) */
	char pwd[CLIENT_PASS_LEN];	/**< Password */
	char *mask;			/**< Allowed host mask */
};

/**
 * Configured server.
 * Peers to which this daemon should establish an outgoing server link must
 * have set a port number; all other servers are allowed to connect to this one.
 */
typedef struct _Conf_Server
{
	char host[HOST_LEN];		/**< Hostname */
	char name[CLIENT_ID_LEN];	/**< IRC client ID */
	char pwd_in[CLIENT_PASS_LEN];	/**< Password which must be received */
	char pwd_out[CLIENT_PASS_LEN];	/**< Password to send to the peer */
	UINT16 port;			/**< Server port to connect to */
	int group;			/**< Group ID of this server */
	time_t lasttry;			/**< Time of last connection attempt */
	PROC_STAT res_stat;		/**< Status of the resolver */
	int flags;			/**< Server flags */
	CONN_ID conn_id;		/**< ID of server connection or NONE */
	ng_ipaddr_t bind_addr;		/**< Source address to use for outgoing
					     connections */
	ng_ipaddr_t dst_addr[2];	/**< List of addresses to connect to */
#ifdef SSL_SUPPORT
	bool SSLConnect;		/**< Establish connection using SSL? */
	bool SSLVerify;			/**< Verify server certificate using CA? */
#endif
	char svs_mask[CLIENT_ID_LEN];	/**< Mask of nicknames that should be
					     treated and counted as services */
} CONF_SERVER;


#ifdef SSL_SUPPORT
/** Configuration options required for SSL support */
struct SSLOptions {
	char *KeyFile;			/**< SSL key file */
	char *CertFile;			/**< SSL certificate file */
	char *DHFile;			/**< File containing DH parameters */
	array ListenPorts;		/**< Array of listening SSL ports */
	array KeyFilePassword;		/**< Key file password */
	char *CipherList;		/**< Set SSL cipher list to use */
	char *CAFile;			/**< Trusted CA certificates file */
	char *CRLFile;			/**< Certificate revocation file */
};
#endif


/** Pre-defined channels */
struct Conf_Channel {
	char name[CHANNEL_NAME_LEN];	/**< Name of the channel */
	char *modes[512];		/**< Initial channel modes to evaluate */
	char key[CLIENT_PASS_LEN];      /**< Channel key ("password", mode "k" ) */
	char topic[COMMAND_LEN];	/**< Initial topic */
	char keyfile[512];		/**< Path and name of channel key file */
	bool autojoin;			/**< 1 to make all users autojoin this channel */
	unsigned long maxusers;		/**< User limit for this channel, mode "l" */
	unsigned int modes_num;		/**< Number of channel modes to evaluate */
};


#define CONF_SFLAG_ONCE	1		/* Delete this entry after next disconnect */
#define CONF_SFLAG_DISABLED 2		/* This server configuration entry is disabled */


/** Name (ID, "nick") of this server */
GLOBAL char Conf_ServerName[CLIENT_ID_LEN];

/** Server info text */
GLOBAL char Conf_ServerInfo[CLIENT_INFO_LEN];

/** Global server passwort */
GLOBAL char Conf_ServerPwd[CLIENT_PASS_LEN];

/** Administrative information */
GLOBAL char Conf_ServerAdmin1[CLIENT_INFO_LEN];
GLOBAL char Conf_ServerAdmin2[CLIENT_INFO_LEN];
GLOBAL char Conf_ServerAdminMail[CLIENT_INFO_LEN];

/** Network name (optional, no spaces allowed) */
GLOBAL char Conf_Network[CLIENT_INFO_LEN];

/** Message of the day (MOTD) of this server */
GLOBAL array Conf_Motd;

/** Help text of this server */
GLOBAL array Conf_Helptext;

/** Array of ports this server should listen on */
GLOBAL array Conf_ListenPorts;

/** Address to which sockets should be bound to or empty (=all) */
GLOBAL char *Conf_ListenAddress;

/** User and group ID this daemon should run with */
GLOBAL uid_t Conf_UID;
GLOBAL gid_t Conf_GID;

/** The directory to chroot() into */
GLOBAL char Conf_Chroot[FNAME_LEN];

/** Full path and name of a file to which the PID of daemon should be written */
GLOBAL char Conf_PidFile[FNAME_LEN];

/** Timeout (in seconds) for PING commands */
GLOBAL int Conf_PingTimeout;

/** Timeout (in seconds) for PONG replies */
GLOBAL int Conf_PongTimeout;

/** Seconds between connection attempts to other servers */
GLOBAL int Conf_ConnectRetry;

/** Array of configured IRC operators */
GLOBAL array Conf_Opers;

/** Array of configured IRC servers */
GLOBAL CONF_SERVER Conf_Server[MAX_SERVERS];

/** Array of pre-defined channels */
GLOBAL array Conf_Channels;

/** String containing all locally allowed channel prefixes for new channels */
GLOBAL char Conf_AllowedChannelTypes[8];

/** Flag indicating if IRC operators are allowed to always use MODE (true) */
GLOBAL bool Conf_OperCanMode;

/** Flag indicating if IRC operators get AutoOp in persistent (+P) channels */
GLOBAL bool Conf_OperChanPAutoOp;

/**
 * If true, mask channel MODE commands of IRC operators to the server.
 * Background: ircd2 will ignore channel MODE commands if an IRC operator
 * gives channel operator privileges to someone without being a channel operator
 * himself. This enables a workaround: it masks the MODE command as coming
 * from the IRC server and not the IRC operator.
 */
GLOBAL bool Conf_OperServerMode;

/** Flag indicating if remote IRC operators are allowed to manage this server */
GLOBAL bool Conf_AllowRemoteOper;

/** Cloaked hostname of the clients */
GLOBAL char Conf_CloakHost[CLIENT_ID_LEN];

/** Cloaked hostname for clients that did +x */
GLOBAL char Conf_CloakHostModeX[CLIENT_ID_LEN];

/** Salt for hostname hash for cloaked hostnames */
GLOBAL char Conf_CloakHostSalt[CLIENT_ID_LEN];

/** Use nickname as user name? */
GLOBAL bool Conf_CloakUserToNick;

/** Enable all DNS functions? */
GLOBAL bool Conf_DNS;

/** Enable IDENT lookups, even when compiled with support for it */
GLOBAL bool Conf_Ident;

/** Enable "more privacy" mode and "censor" some user-related information */
GLOBAL bool Conf_MorePrivacy;

/** Enable "NOTICE *" messages on connect */
GLOBAL bool Conf_NoticeBeforeRegistration;

/** Enable all usage of PAM, even when compiled with support for it */
GLOBAL bool Conf_PAM;

/** Don't require all clients to send a password an to be PAM authenticated */
GLOBAL bool Conf_PAMIsOptional;

/** The service name to use for PAM */
GLOBAL char Conf_PAMServiceName[MAX_PAM_SERVICE_NAME_LEN];

/** Disable all CTCP commands except for /me ? */
GLOBAL bool Conf_ScrubCTCP;

/** Default user modes for new local clients */
GLOBAL char Conf_DefaultUserModes[CLIENT_MODE_LEN];

/*
 * try to connect to remote systems using the ipv6 protocol,
 * if they have an ipv6 address? (default yes)
 */
GLOBAL bool Conf_ConnectIPv6;

/** Try to connect to remote systems using the IPv4 protocol (true) */
GLOBAL bool Conf_ConnectIPv4;

/** Idle timeout (seconds), after which the daemon should exit */
GLOBAL int Conf_IdleTimeout;

/** Maximum number of simultaneous connections to this server */
GLOBAL int Conf_MaxConnections;

/** Maximum number of channels a user can join */
GLOBAL int Conf_MaxJoins;

/** Maximum number of connections per IP address */
GLOBAL int Conf_MaxConnectionsIP;

/** Maximum length of a nickname */
GLOBAL unsigned int Conf_MaxNickLength;

/** Maximum number of channels returned to /list */
GLOBAL int Conf_MaxListSize;

/** Maximum seconds to add per "penalty". -1 = unlimited. */
GLOBAL time_t Conf_MaxPenaltyTime;

#ifndef STRICT_RFC

/** Require "AUTH PING-PONG" on login */
GLOBAL bool Conf_AuthPing;

#endif

#ifdef SYSLOG

/* Syslog "facility" */
GLOBAL int Conf_SyslogFacility;

#endif

GLOBAL void Conf_Init PARAMS((void));
GLOBAL bool Conf_Rehash PARAMS((void));
GLOBAL int Conf_Test PARAMS((void));

GLOBAL void Conf_UnsetServer PARAMS(( CONN_ID Idx ));
GLOBAL bool Conf_SetServer PARAMS(( int ConfServer, CONN_ID Idx ));
GLOBAL int Conf_GetServer PARAMS(( CONN_ID Idx ));

GLOBAL bool Conf_EnableServer PARAMS(( const char *Name, UINT16 Port ));
GLOBAL bool Conf_EnablePassiveServer PARAMS((const char *Name));
GLOBAL bool Conf_DisableServer PARAMS(( const char *Name ));
GLOBAL bool Conf_AddServer PARAMS(( const char *Name, UINT16 Port, const char *Host, const char *MyPwd, const char *PeerPwd ));

GLOBAL bool Conf_NickIsService PARAMS((int ConfServer, const char *Nick));
GLOBAL bool Conf_NickIsBlocked PARAMS((const char *Nick));

#ifdef SSL_SUPPORT
GLOBAL bool Conf_SSLInUse PARAMS((void));
#endif

/* Password required by WEBIRC command */
GLOBAL char Conf_WebircPwd[CLIENT_PASS_LEN];

GLOBAL void Conf_DebugDump PARAMS((void));


#endif


/* -eof- */
