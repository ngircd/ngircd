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
 * $Id: conf.h,v 1.14 2002/03/27 16:39:22 alex Exp $
 *
 * conf.h: Konfiguration des ngircd (Header)
 */


#ifndef __conf_h__
#define __conf_h__

#include <time.h>

#include "conn.h"
#include "defines.h"


typedef struct _Conf_Oper
{
	CHAR name[CLIENT_PASS_LEN];	/* Name (ID) des IRC-OPs */
	CHAR pwd[CLIENT_PASS_LEN];	/* Passwort */
} CONF_OPER;

typedef struct _Conf_Server
{
	CHAR host[HOST_LEN];		/* Hostname */
	CHAR ip[16];			/* IP-Adresse (von Resolver) */
	CHAR name[CLIENT_ID_LEN];	/* IRC-Client-ID */
	CHAR pwd[CLIENT_PASS_LEN];	/* Passwort */
	INT port;			/* Server-Port */
	INT group;			/* Gruppe des Servers */
	time_t lasttry;			/* Letzter Connect-Versuch */
	RES_STAT *res_stat;		/* Status des Resolver */
} CONF_SERVER;


/* Name ("Nick") des Servers */
GLOBAL CHAR Conf_ServerName[CLIENT_ID_LEN];

/* Servers-Info-Text */
GLOBAL CHAR Conf_ServerInfo[CLIENT_INFO_LEN];

/* Server-Passwort */
GLOBAL CHAR Conf_ServerPwd[CLIENT_PASS_LEN];

/* Datei mit MOTD-Text */
GLOBAL CHAR Conf_MotdFile[FNAME_LEN];

/* Ports, auf denen der Server Verbindungen entgegen nimmt */
GLOBAL INT Conf_ListenPorts[MAX_LISTEN_PORTS];
GLOBAL INT Conf_ListenPorts_Count;

/* Timeouts fuer PING und PONG */
GLOBAL INT Conf_PingTimeout;
GLOBAL INT Conf_PongTimeout;

/* Sekunden zwischen Verbindungsversuchen zu anderen Servern */
GLOBAL INT Conf_ConnectRetry;

/* Operatoren */
GLOBAL CONF_OPER Conf_Oper[MAX_OPERATORS];
GLOBAL INT Conf_Oper_Count;

/* Server */
GLOBAL CONF_SERVER Conf_Server[MAX_SERVERS];
GLOBAL INT Conf_Server_Count;


GLOBAL VOID Conf_Init( VOID );
GLOBAL INT Conf_Test( VOID );
GLOBAL VOID Conf_Exit( VOID );


#endif


/* -eof- */
