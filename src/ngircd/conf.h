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
 * $Id: conf.h,v 1.11 2002/03/10 17:50:48 alex Exp $
 *
 * conf.h: Konfiguration des ngircd (Header)
 *
 * $Log: conf.h,v $
 * Revision 1.11  2002/03/10 17:50:48  alex
 * - Handling von "--version" und "--help" nochmal geaendert ...
 *
 * Revision 1.10  2002/02/27 23:23:53  alex
 * - Includes fuer einige Header bereinigt.
 *
 * Revision 1.9  2002/01/03 02:27:20  alex
 * - das Server-Passwort kann nun konfiguriert werden.
 *
 * Revision 1.8  2002/01/02 02:49:16  alex
 * - Konfigurationsdatei "Samba like" umgestellt.
 * - es koennen nun mehrere Server und Oprtatoren konfiguriert werden.
 *
 * Revision 1.6  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.5  2001/12/30 19:26:11  alex
 * - Unterstuetzung fuer die Konfigurationsdatei eingebaut.
 *
 * Revision 1.4  2001/12/26 22:48:53  alex
 * - MOTD-Datei ist nun konfigurierbar und wird gelesen.
 *
 * Revision 1.3  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.2  2001/12/26 03:19:57  alex
 * - erste Konfigurations-Variablen definiert: PING/PONG-Timeout.
 *
 * Revision 1.1  2001/12/12 17:18:20  alex
 * - Modul fuer Server-Konfiguration begonnen.
 */


#ifndef __conf_h__
#define __conf_h__

#include <time.h>

#include "conn.h"


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


/* Konfigurationsdatei */
GLOBAL CHAR Conf_File[FNAME_LEN];

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
GLOBAL VOID Conf_Exit( VOID );


#endif


/* -eof- */
