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
 * $Id: irc-mode.h,v 1.4 2002/05/27 13:09:27 alex Exp $
 *
 * irc-mode.h: IRC-Befehle zur Mode-Aenderung (MODE, AWAY, ...) (Header)
 */


#ifndef __irc_mode_h__
#define __irc_mode_h__


GLOBAL BOOLEAN IRC_MODE PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_AWAY PARAMS((CLIENT *Client, REQUEST *Req ));


#endif


/* -eof- */
