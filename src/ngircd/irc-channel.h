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
 * $Id: irc-channel.h,v 1.4 2002/05/27 13:09:26 alex Exp $
 *
 * irc-channel.h: IRC-Channel-Befehle (Header)
 */


#ifndef __irc_channel_h__
#define __irc_channel_h__


GLOBAL BOOLEAN IRC_JOIN PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_PART PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_TOPIC PARAMS((CLIENT *Client, REQUEST *Req ));

GLOBAL BOOLEAN IRC_LIST PARAMS((CLIENT *Client, REQUEST *Req ));


#endif


/* -eof- */
