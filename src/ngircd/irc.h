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
 * $Id: irc.h,v 1.27 2002/02/27 23:26:36 alex Exp $
 *
 * irc.h: IRC-Befehle (Header)
 *
 * $Log: irc.h,v $
 * Revision 1.27  2002/02/27 23:26:36  alex
 * - einige Funktionen in irc-xxx-Module ausgegliedert.
 *
 * Revision 1.26  2002/02/27 20:33:13  alex
 * - Channel-Topics implementiert.
 *
 * Revision 1.25  2002/02/27 18:23:46  alex
 * - IRC-Befehl "AWAY" implementert.
 *
 * Revision 1.24  2002/02/23 21:39:48  alex
 * - IRC-Befehl KILL sowie Kills bei Nick Collsisions implementiert.
 *
 * Revision 1.23  2002/02/17 23:38:58  alex
 * - neuer IRC-Befehl VERSION implementiert: IRC_VERSION().
 */


#ifndef __irc_h__
#define __irc_h__

#include "parse.h"
#include "client.h"
#include "channel.h"


GLOBAL BOOLEAN IRC_MOTD( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_LUSERS( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_LINKS( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_VERSION( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_PRIVMSG( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_NOTICE( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_NAMES( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_ISON( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_WHOIS( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_USERHOST( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_OPER( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_DIE( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_RESTART( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_ERROR( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_KILL( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_JOIN( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_PART( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_TOPIC( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_Send_NAMES( CLIENT *Client, CHANNEL *Chan );
GLOBAL BOOLEAN IRC_Send_LUSERS( CLIENT *Client );
GLOBAL BOOLEAN IRC_Show_MOTD( CLIENT *Client );


#endif


/* -eof- */
