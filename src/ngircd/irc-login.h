/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2012 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __irc_login_h__
#define __irc_login_h__

/**
 * @file
 * Login and logout (header)
 */

GLOBAL bool IRC_PASS PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_NICK PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_USER PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_SERVICE PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_WEBIRC PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_PING PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_PONG PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_QUIT PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_QUIT_HTTP PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_SVSNICK PARAMS(( CLIENT *Client, REQUEST *Req ));

#endif

/* -eof- */
