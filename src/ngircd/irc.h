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
 * $Id: irc.h,v 1.30 2002/03/12 14:37:52 alex Exp $
 *
 * irc.h: IRC-Befehle (Header)
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
GLOBAL BOOLEAN IRC_WHO( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_ERROR( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_KILL( CLIENT *Client, REQUEST *Req );

GLOBAL BOOLEAN IRC_Send_NAMES( CLIENT *Client, CHANNEL *Chan );
GLOBAL BOOLEAN IRC_Send_LUSERS( CLIENT *Client );
GLOBAL BOOLEAN IRC_Show_MOTD( CLIENT *Client );
GLOBAL BOOLEAN IRC_Send_WHO( CLIENT *Client, CHANNEL *Chan, BOOLEAN OnlyOps );


#endif


/* -eof- */
