/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: conf.h,v 1.40 2006/05/10 21:24:01 alex Exp $
 *
 * Configuration management (header)
 */


#ifndef __conf_h__
#define __conf_h__

#include <time.h>

#include "defines.h"
#include "array.h"
#include "portab.h"

typedef struct _Conf_Oper
{
	char name[CLIENT_PASS_LEN];	/* Name (ID) of IRC operator */
	char pwd[CLIENT_PASS_LEN];	/* Password */
	char *mask;
} CONF_OPER;

typedef struct _Conf_Server
{
	char host[HOST_LEN];		/* Hostname */
	char ip[16];			/* IP address (Resolver) */
	char name[CLIENT_ID_LEN];	/* IRC-Client-ID */
	char pwd_in[CLIENT_PASS_LEN];	/* Password which must be received */
	char pwd_out[CLIENT_PASS_LEN];	/* Password to send to peer */
	UINT16 port;			/* Server port */
	int group;			/* Group of server */
	time_t lasttry;			/* Last connect attempt */
	RES_STAT res_stat;		/* Status of the resolver */
	int flags;			/* Flags */
	CONN_ID conn_id;		/* ID of server connection or NONE */
} CONF_SERVER;

typedef struct _Conf_Channel
{
	char name[CHANNEL_NAME_LEN];	/* Name of the channel */
	char modes[CHANNEL_MODE_LEN];	/* Initial channel modes */
	array topic;			/* Initial topic */
} CONF_CHANNEL;


#define CONF_SFLAG_ONCE	1		/* Delete this entry after next disconnect */
#define CONF_SFLAG_DISABLED 2		/* This server configuration entry is disabled */


/* Name ("Nick") of the servers */
GLOBAL char Conf_ServerName[CLIENT_ID_LEN];

/* Server info text */
GLOBAL char Conf_ServerInfo[CLIENT_INFO_LEN];

/* Global server passwort */
GLOBAL char Conf_ServerPwd[CLIENT_PASS_LEN];

/* Administrative information */
GLOBAL char Conf_ServerAdmin1[CLIENT_INFO_LEN];
GLOBAL char Conf_ServerAdmin2[CLIENT_INFO_LEN];
GLOBAL char Conf_ServerAdminMail[CLIENT_INFO_LEN];

/* File with MOTD text */
GLOBAL char Conf_MotdFile[FNAME_LEN];

/* Phrase with MOTD text */
GLOBAL char Conf_MotdPhrase[LINE_LEN];

/* Ports the server should listen on */
GLOBAL array Conf_ListenPorts;

/* Address to which the socket should be bound or empty (=all) */
GLOBAL char Conf_ListenAddress[16];

/* User and group ID the server should run with */
GLOBAL uid_t Conf_UID;
GLOBAL gid_t Conf_GID;

/* A directory to chroot() in */
GLOBAL char Conf_Chroot[FNAME_LEN];

/* File with PID of daemon */
GLOBAL char Conf_PidFile[FNAME_LEN];

/* Timeouts for PING and PONG */
GLOBAL int Conf_PingTimeout;
GLOBAL int Conf_PongTimeout;

/* Seconds between connect attempts to other servers */
GLOBAL int Conf_ConnectRetry;

/* Operators */
GLOBAL CONF_OPER Conf_Oper[MAX_OPERATORS];
GLOBAL unsigned int Conf_Oper_Count;

/* Servers */
GLOBAL CONF_SERVER Conf_Server[MAX_SERVERS];

/* Pre-defined channels */
GLOBAL CONF_CHANNEL Conf_Channel[MAX_DEFCHANNELS];
GLOBAL unsigned int Conf_Channel_Count;

/* Are IRC operators allowed to always use MODE? */
GLOBAL bool Conf_OperCanMode;

/* If an IRC op gives chanop privileges without being a chanop,
 * ircd2 will ignore the command. This enables a workaround:
 * It masks the command as coming from the server */
GLOBAL bool Conf_OperServerMode;

/* Maximum number of connections to this server */
GLOBAL long Conf_MaxConnections;

/* Maximum number of channels a user can join */
GLOBAL int Conf_MaxJoins;

/* Maximum number of connections per IP address */
GLOBAL int Conf_MaxConnectionsIP;


GLOBAL void Conf_Init PARAMS((void));
GLOBAL void Conf_Rehash PARAMS((void));
GLOBAL int Conf_Test PARAMS((void));

GLOBAL void Conf_UnsetServer PARAMS(( CONN_ID Idx ));
GLOBAL void Conf_SetServer PARAMS(( int ConfServer, CONN_ID Idx ));
GLOBAL int Conf_GetServer PARAMS(( CONN_ID Idx ));

GLOBAL bool Conf_EnableServer PARAMS(( char *Name, UINT16 Port ));
GLOBAL bool Conf_DisableServer PARAMS(( char *Name ));
GLOBAL bool Conf_AddServer PARAMS(( char *Name, UINT16 Port, char *Host, char *MyPwd, char *PeerPwd ));


#endif


/* -eof- */
