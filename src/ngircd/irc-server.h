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
 * $Id: irc-server.h,v 1.4 2002/12/12 12:23:43 alex Exp $
 *
 * IRC commands for server links (header)
 */


#ifndef __irc_server_h__
#define __irc_server_h__


GLOBAL BOOLEAN IRC_SERVER PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_NJOIN PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_SQUIT PARAMS((CLIENT *Client, REQUEST *Req ));


#endif


/* -eof- */
