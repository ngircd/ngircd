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
 * $Id: irc-info.h,v 1.1 2002/11/30 17:39:56 alex Exp $
 *
 * irc-info.h: IRC-Info-Befehle (Header)
 */


#ifndef __irc_info_h__
#define __irc_info_h__


GLOBAL BOOLEAN IRC_ADMIN PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_ISON PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_LINKS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_LUSERS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_MOTD PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_NAMES PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_STATS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_TIME PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_USERHOST PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_VERSION PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_WHO PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_WHOIS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_WHOWAS PARAMS(( CLIENT *Client, REQUEST *Req ));

GLOBAL BOOLEAN IRC_Send_LUSERS PARAMS(( CLIENT *Client ));
GLOBAL BOOLEAN IRC_Send_NAMES PARAMS(( CLIENT *Client, CHANNEL *Chan ));
GLOBAL BOOLEAN IRC_Show_MOTD PARAMS(( CLIENT *Client ));
GLOBAL BOOLEAN IRC_Send_WHO PARAMS(( CLIENT *Client, CHANNEL *Chan, BOOLEAN OnlyOps ));


#endif


/* -eof- */
