/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an comBase beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: conf.h,v 1.4 2001/12/26 22:48:53 alex Exp $
 *
 * conf.h: Konfiguration des ngircd (Header)
 *
 * $Log: conf.h,v $
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


#define FNAME_LEN 256


GLOBAL CHAR Conf_File[FNAME_LEN];	/* Konfigurationsdatei */

GLOBAL INT Conf_PingTimeout;		/* Ping Timeout */
GLOBAL INT Conf_PongTimeout;		/* Pong Timeout */

GLOBAL CHAR Conf_MotdFile[FNAME_LEN];	/* Datei mit MOTD-Text */


GLOBAL VOID Conf_Init( VOID );
GLOBAL VOID Conf_Exit( VOID );


#endif


/* -eof- */
