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
 * $Id: irc-op.h,v 1.1 2002/05/27 11:22:07 alex Exp $
 *
 * irc-op.h: Befehle zur Channel-Verwaltung (Header)
 */


#ifndef __irc_op_h__
#define __irc_op_h__


GLOBAL BOOLEAN IRC_KICK PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_BAN PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_INVITE PARAMS(( CLIENT *Client, REQUEST *Req ));


#endif


/* -eof- */
