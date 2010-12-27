/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __irc_server_h__
#define __irc_server_h__

/**
 * @file
 * IRC commands for server links (header)
 */

GLOBAL bool IRC_SERVER PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_NJOIN PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_SQUIT PARAMS((CLIENT *Client, REQUEST *Req ));

GLOBAL bool IRC_ENDOFMOTD_Server PARAMS((CLIENT *Client));

#endif

/* -eof- */
