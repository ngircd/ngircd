/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: conf.h,v 1.6 2001/12/31 02:18:51 alex Exp $
 *
 * conf.h: Konfiguration des ngircd (Header)
 *
 * $Log: conf.h,v $
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


GLOBAL CHAR Conf_File[FNAME_LEN];		/* Konfigurationsdatei */

GLOBAL CHAR Conf_ServerName[CLIENT_ID_LEN];	/* Name ("Nick") des Servers */
GLOBAL CHAR Conf_ServerInfo[CLIENT_INFO_LEN];	/* Servers-Info-Text */

GLOBAL CHAR Conf_MotdFile[FNAME_LEN];		/* Datei mit MOTD-Text */

GLOBAL INT Conf_ListenPorts[LISTEN_PORTS];	/* Ports, auf denen der Server Verbindungen */
GLOBAL INT Conf_ListenPorts_Count;		/* entgegen nimmt sowie deren Anzahl */

GLOBAL CHAR Conf_Oper[CLIENT_PASS_LEN];		/* Operator Name */
GLOBAL CHAR Conf_OperPwd[CLIENT_PASS_LEN];	/* Operator Passwort */

GLOBAL INT Conf_PingTimeout;			/* Ping Timeout */
GLOBAL INT Conf_PongTimeout;			/* Pong Timeout */


GLOBAL VOID Conf_Init( VOID );
GLOBAL VOID Conf_Exit( VOID );


#endif


/* -eof- */
