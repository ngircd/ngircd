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
 * $Id: irc.h,v 1.19 2002/01/26 18:43:11 alex Exp $
 *
 * irc.h: IRC-Befehle (Header)
 *
 * $Log: irc.h,v $
 * Revision 1.19  2002/01/26 18:43:11  alex
 * - neue Funktionen IRC_WriteStrChannelPrefix() und IRC_WriteStrChannel(),
 *   die IRC_Write_xxx_Related() sind dafuer entfallen.
 * - IRC_PRIVMSG() kann nun auch mit Channels als Ziel umgehen.
 *
 * Revision 1.18  2002/01/21 00:04:13  alex
 * - neue Funktionen IRC_JOIN, IRC_PART, IRC_WriteStrRelatedPrefix und
 *   IRC_WriteStrRelatedChannelPrefix().
 *
 * Revision 1.17  2002/01/11 23:50:55  alex
 * - LINKS implementiert, LUSERS begonnen.
 *
 * Revision 1.16  2002/01/05 19:15:03  alex
 * - Fehlerpruefung bei select() in der "Hauptschleife" korrigiert.
 *
 * Revision 1.15  2002/01/04 17:58:21  alex
 * - IRC_WriteStrXXX()-Funktionen angepasst; neuer Befehl SQUIT.
 *
 * Revision 1.14  2002/01/03 02:26:07  alex
 * - neue Befehle SERVER und NJOIN begonnen.
 *
 * Revision 1.13  2002/01/02 02:51:39  alex
 * - Copyright-Texte angepasst.
 * - neuer Befehl "ERROR".
 *
 * Revision 1.11  2001/12/31 15:33:13  alex
 * - neuer Befehl NAMES, kleinere Bugfixes.
 * - Bug bei PING behoben: war zu restriktiv implementiert :-)
 *
 * Revision 1.10  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.9  2001/12/29 03:09:31  alex
 * - Neue Funktion IRC_MODE() implementiert.
 *
 * Revision 1.8  2001/12/27 19:17:26  alex
 * - neue Befehle PRIVMSG, NOTICE, PING.
 *
 * Revision 1.7  2001/12/27 16:55:41  alex
 * - neu: IRC_WriteStrRelated(), Aenderungen auch in IRC_WriteStrClient().
 *
 * Revision 1.6  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.5  2001/12/26 03:21:46  alex
 * - PING/PONG-Befehle implementiert,
 * - Meldungen ueberarbeitet: enthalten nun (fast) immer den Nick.
 *
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
#include "channel.h"


GLOBAL VOID IRC_Init( VOID );
GLOBAL VOID IRC_Exit( VOID );

GLOBAL BOOLEAN IRC_WriteStrClient( CLIENT *Client, CHAR *Format, ... );
GLOBAL BOOLEAN IRC_WriteStrClientPrefix( CLIENT *Client, CLIENT *Prefix, CHAR *Format, ... );
GLOBAL BOOLEAN IRC_WriteStrChannel( CLIENT *Client, CHANNEL *Chan, CHAR *Format, ... );
GLOBAL BOOLEAN IRC_WriteStrChannelPrefix( CLIENT *Client, CHANNEL *Chan, CLIENT *Prefix, CHAR *Format, ... );
GLOBAL VOID IRC_WriteStrServers( CLIENT *ExceptOf, CHAR *Format, ... );
GLOBAL VOID IRC_WriteStrServersPrefix( CLIENT *ExceptOf, CLIENT *Prefix, CHAR *Format, ... );

GLOBAL BOOLEAN IRC_PASS( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_NICK( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_USER( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_SERVER( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_NJOIN( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_PING( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_PONG( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_QUIT( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_SQUIT( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_MOTD( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_LUSERS( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_LINKS( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_PRIVMSG( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_NOTICE( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_MODE( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_NAMES( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_ISON( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_WHOIS( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_USERHOST( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_OPER( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_DIE( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_RESTART( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_ERROR( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_JOIN( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_PART( CLIENT *Client, REQUEST *Req );


#endif


/* -eof- */
