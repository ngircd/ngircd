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
 * $Id: irc-login.h,v 1.5 2002/12/12 12:23:43 alex Exp $
 *
 * Login and logout (header)
 */


#ifndef __irc_login_h__
#define __irc_login_h__


GLOBAL BOOLEAN IRC_PASS PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_NICK PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_USER PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_PING PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_PONG PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_QUIT PARAMS((CLIENT *Client, REQUEST *Req ));


#endif


/* -eof- */
