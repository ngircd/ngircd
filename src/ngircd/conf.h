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
 * $Id: conf.h,v 1.22 2002/12/12 11:26:08 alex Exp $
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
} CONF_SERVER;

typedef struct _Conf_Channel
{
	CHAR name[CHANNEL_NAME_LEN];	/* Name of the channel */
	CHAR modes[CHANNEL_MODE_LEN];	/* Initial channel modes */
	CHAR topic[CHANNEL_TOPIC_LEN];	/* Initial topic */
} CONF_CHANNEL;


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
GLOBAL INT Conf_Server_Count;

/* Pre-defined channels */
GLOBAL CONF_CHANNEL Conf_Channel[MAX_DEFCHANNELS];
GLOBAL INT Conf_Channel_Count;

/* Are IRC operators allowed to always use MODE? */
GLOBAL BOOLEAN Conf_OperCanMode;

/* Maximum number of connections to this server */
GLOBAL LONG Conf_MaxConnections;


GLOBAL VOID Conf_Init PARAMS((VOID ));
GLOBAL INT Conf_Test PARAMS((VOID ));


#endif


/* -eof- */
