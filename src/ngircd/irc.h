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
 * $Id: irc.h,v 1.4 2001/12/25 22:02:42 alex Exp $
 *
 * irc.h: IRC-Befehle (Header)
 *
 * $Log: irc.h,v $
 * Revision 1.4  2001/12/25 22:02:42  alex
 * - neuer IRC-Befehl "/QUIT". Verbessertes Logging & Debug-Ausgaben.
 *
 * Revision 1.3  2001/12/25 19:19:30  alex
 * - bessere Fehler-Abfragen, diverse Bugfixes.
 * - Nicks werden nur einmal vergeben :-)
 * - /MOTD wird unterstuetzt.
 *
 * Revision 1.2  2001/12/23 21:57:16  alex
 * - erste IRC-Befehle zu implementieren begonnen.
 *
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 */


#ifndef __irc_h__
#define __irc_h__

#include "parse.h"


GLOBAL VOID IRC_Init( VOID );
GLOBAL VOID IRC_Exit( VOID );

GLOBAL BOOLEAN IRC_WriteStr_Client( CLIENT *Client, CLIENT *Prefix, CHAR *Format, ... );

GLOBAL BOOLEAN IRC_PASS( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_NICK( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_USER( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_QUIT( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_MOTD( CLIENT *Client, REQUEST *Req );


#endif


/* -eof- */
