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
 * $Id: conf.h,v 1.27 2003/09/11 12:05:28 alex Exp $
 *
 * Configuration management (header)
 */


#ifndef __conf_h__
#define __conf_h__

#include <time.h>

#include "defines.h"


typedef struct _Conf_Oper
{
	CHAR name[CLIENT_PASS_LEN];	/* Name (ID) of IRC operator */
	CHAR pwd[CLIENT_PASS_LEN];	/* Password */
} CONF_OPER;

typedef struct _Conf_Server
{
	CHAR host[HOST_LEN];		/* Hostname */
	CHAR ip[16];			/* IP address (Resolver) */
	CHAR name[CLIENT_ID_LEN];	/* IRC-Client-ID */
	CHAR pwd_in[CLIENT_PASS_LEN];	/* Password which must be received */
	CHAR pwd_out[CLIENT_PASS_LEN];	/* Password to send to peer */
	INT port;			/* Server port */
	INT group;			/* Group of server */
	time_t lasttry;			/* Last connect attempt */
	RES_STAT *res_stat;		/* Status of the resolver */
	INT flags;			/* Flags */
	CONN_ID conn_id;		/* ID of server connection or NONE */
} CONF_SERVER;

typedef struct _Conf_Channel
{
	CHAR name[CHANNEL_NAME_LEN];	/* Name of the channel */
	CHAR modes[CHANNEL_MODE_LEN];	/* Initial channel modes */
	CHAR topic[CHANNEL_TOPIC_LEN];	/* Initial topic */
} CONF_CHANNEL;


#define CONF_SFLAG_ONCE	1		/* Delete this entry after next disconnect */
#define CONF_SFLAG_DISABLED 2		/* This server configuration entry is disabled */


/* Name ("Nick") of the servers */
GLOBAL CHAR Conf_ServerName[CLIENT_ID_LEN];

/* Server info text */
GLOBAL CHAR Conf_ServerInfo[CLIENT_INFO_LEN];

/* Global server passwort */
GLOBAL CHAR Conf_ServerPwd[CLIENT_PASS_LEN];

/* Administrative information */
GLOBAL CHAR Conf_ServerAdmin1[CLIENT_INFO_LEN];
GLOBAL CHAR Conf_ServerAdmin2[CLIENT_INFO_LEN];
GLOBAL CHAR Conf_ServerAdminMail[CLIENT_INFO_LEN];

/* File with MOTD text */
GLOBAL CHAR Conf_MotdFile[FNAME_LEN];

/* Ports the server should listen on */
GLOBAL UINT Conf_ListenPorts[MAX_LISTEN_PORTS];
GLOBAL INT Conf_ListenPorts_Count;

/* Address to which the socket should be bound or empty (=all) */
GLOBAL CHAR Conf_ListenAddress[16];

/* User and group ID the server should run with */
GLOBAL UINT Conf_UID;
GLOBAL UINT Conf_GID;

/* Timeouts for PING and PONG */
GLOBAL INT Conf_PingTimeout;
GLOBAL INT Conf_PongTimeout;

/* Seconds between connect attempts to other servers */
GLOBAL INT Conf_ConnectRetry;

/* Operators */
GLOBAL CONF_OPER Conf_Oper[MAX_OPERATORS];
GLOBAL INT Conf_Oper_Count;

/* Servers */
GLOBAL CONF_SERVER Conf_Server[MAX_SERVERS];

/* Pre-defined channels */
GLOBAL CONF_CHANNEL Conf_Channel[MAX_DEFCHANNELS];
GLOBAL INT Conf_Channel_Count;

/* Are IRC operators allowed to always use MODE? */
GLOBAL BOOLEAN Conf_OperCanMode;

/* Maximum number of connections to this server */
GLOBAL LONG Conf_MaxConnections;

/* Maximum number of channels a user can join */
GLOBAL INT Conf_MaxJoins;


GLOBAL VOID Conf_Init PARAMS((VOID ));
GLOBAL VOID Conf_Rehash PARAMS((VOID ));
GLOBAL INT Conf_Test PARAMS((VOID ));

GLOBAL VOID Conf_UnsetServer PARAMS(( CONN_ID Idx ));
GLOBAL VOID Conf_SetServer PARAMS(( INT ConfServer, CONN_ID Idx ));
GLOBAL INT Conf_GetServer PARAMS(( CONN_ID Idx ));

GLOBAL BOOLEAN Conf_EnableServer PARAMS(( CHAR *Name, INT Port ));
GLOBAL BOOLEAN Conf_DisableServer PARAMS(( CHAR *Name ));
GLOBAL BOOLEAN Conf_AddServer PARAMS(( CHAR *Name, INT Port, CHAR *Host, CHAR *MyPwd, CHAR *PeerPwd ));


#endif


/* -eof- */
