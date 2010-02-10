/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Login and logout (header)
 */


#ifndef __irc_login_h__
#define __irc_login_h__

GLOBAL bool IRC_PASS PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_NICK PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_USER PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_SERVICE PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_WEBIRC PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_PING PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_PONG PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_QUIT PARAMS((CLIENT *Client, REQUEST *Req));

#endif


/* -eof- */
