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
 * $Id: irc-oper.h,v 1.1 2002/03/03 17:15:11 alex Exp $
 *
 * irc-oper.h: IRC-Operator-Befehle (Header)
 *
 * $Log: irc-oper.h,v $
 * Revision 1.1  2002/03/03 17:15:11  alex
 * - Source in weitere Module fuer IRC-Befehle aufgesplitted.
 *
 */


#ifndef __irc_oper_h__
#define __irc_oper_h__

#include "parse.h"
#include "client.h"


GLOBAL BOOLEAN IRC_OPER( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_DIE( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_RESTART( CLIENT *Client, REQUEST *Req );


#endif


/* -eof- */
