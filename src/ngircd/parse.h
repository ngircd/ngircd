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
 * $Id: parse.h,v 1.7 2002/05/27 13:09:27 alex Exp $
 *
 * parse.h: Parsen der Client-Anfragen (Header)
 */


#ifndef __parse_h__
#define __parse_h__


typedef struct _REQUEST			/* vgl. RFC 2812, 2.3 */
{
	CHAR *prefix;			/* Prefix */
	CHAR *command;			/* IRC-Befehl */
	CHAR *argv[15];			/* Parameter (max. 15: 0..14) */
	INT argc;			/* Anzahl vorhandener Parameter */
} REQUEST;


GLOBAL BOOLEAN Parse_Request PARAMS((CONN_ID Idx, CHAR *Request ));


#endif


/* -eof- */
