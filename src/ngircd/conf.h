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
 * $Id: conf.h,v 1.7 2002/01/02 02:44:36 alex Exp $
 *
 * conf.h: Konfiguration des ngircd (Header)
 *
 * $Log: conf.h,v $
 * Revision 1.7  2002/01/02 02:44:36  alex
 * - neue Defines fuer max. Anzahl Server und Operatoren.
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


typedef struct _Conf_Oper
{
	CHAR name[CLIENT_PASS_LEN];
	CHAR pwd[CLIENT_PASS_LEN];
} CONF_OPER;

typedef struct _Conf_Server
{
	CHAR host[HOST_LEN];
	CHAR ip[16];
	CHAR name[CLIENT_ID_LEN];
	CHAR pwd[CLIENT_PASS_LEN];
	INT port;
	time_t lasttry;
	RES_STAT *res_stat;
} CONF_SERVER;


/* Konfigurationsdatei */
GLOBAL CHAR Conf_File[FNAME_LEN];

/* Name ("Nick") des Servers */
GLOBAL CHAR Conf_ServerName[CLIENT_ID_LEN];

/* Servers-Info-Text */
GLOBAL CHAR Conf_ServerInfo[CLIENT_INFO_LEN];

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
