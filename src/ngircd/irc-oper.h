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
 * $Id: irc-oper.h,v 1.4 2002/08/26 23:39:22 alex Exp $
 *
 * irc-oper.h: IRC-Operator-Befehle (Header)
 */


#ifndef __irc_oper_h__
#define __irc_oper_h__


GLOBAL BOOLEAN IRC_OPER PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_DIE PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_RESTART PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_CONNECT PARAMS((CLIENT *Client, REQUEST *Req ));


#endif


/* -eof- */
