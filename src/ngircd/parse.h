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
 * $Id: parse.h,v 1.5 2002/02/27 23:24:29 alex Exp $
 *
 * parse.h: Parsen der Client-Anfragen (Header)
 *
 * $Log: parse.h,v $
 * Revision 1.5  2002/02/27 23:24:29  alex
 * - ueberfluessige Init- und Exit-Funktionen entfernt.
 *
 * Revision 1.4  2002/01/02 02:43:50  alex
 * - Copyright-Text ergaenzt bzw. aktualisiert.
 *
 * Revision 1.3  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.2  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.1  2001/12/21 23:53:16  alex
 * - Modul zum Parsen von Client-Requests begonnen.
 */


#ifndef __parse_h__
#define __parse_h__

#include "conn.h"


typedef struct _REQUEST			/* vgl. RFC 2812, 2.3 */
{
	CHAR *prefix;			/* Prefix */
	CHAR *command;			/* IRC-Befehl */
	CHAR *argv[15];			/* Parameter (max. 15: 0..14) */
	INT argc;			/* Anzahl vorhandener Parameter */
} REQUEST;


GLOBAL BOOLEAN Parse_Request( CONN_ID Idx, CHAR *Request );


#endif


/* -eof- */
