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
 * $Id: irc.h,v 1.35 2002/11/30 17:39:56 alex Exp $
 *
 * irc.h: IRC-Befehle (Header)
 */


#ifndef __irc_h__
#define __irc_h__


GLOBAL BOOLEAN IRC_ERROR PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_KILL PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_NOTICE PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_PRIVMSG PARAMS(( CLIENT *Client, REQUEST *Req ));


#endif


/* -eof- */
