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
 * $Id: irc-login.h,v 1.1 2002/02/27 23:26:21 alex Exp $
 *
 * irc-login.h: Anmeldung und Abmeldung im IRC (Header)
 *
 * $Log: irc-login.h,v $
 * Revision 1.1  2002/02/27 23:26:21  alex
 * - Modul aus irc.c bzw. irc.h ausgegliedert.
 *
 */


#ifndef __irc_login_h__
#define __irc_login_h__

#include "parse.h"
#include "client.h"


GLOBAL BOOLEAN IRC_PASS( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_NICK( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_USER( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_SERVER( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_NJOIN( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_PING( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_PONG( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_QUIT( CLIENT *Client, REQUEST *Req );
GLOBAL BOOLEAN IRC_SQUIT( CLIENT *Client, REQUEST *Req );


#endif


/* -eof- */
