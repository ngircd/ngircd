/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: irc-oper.h,v 1.11 2005/03/19 18:43:48 fw Exp $
 *
 * IRC operator commands (header)
 */


#ifndef __irc_oper_h__
#define __irc_oper_h__


GLOBAL bool IRC_OPER PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_DIE PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_REHASH PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_RESTART PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_CONNECT PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_DISCONNECT PARAMS((CLIENT *Client, REQUEST *Req ));


#endif


/* -eof- */
