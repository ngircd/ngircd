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
 * $Id: irc.h,v 1.39 2005/03/19 18:43:49 fw Exp $
 *
 * IRC commands (header)
 */


#ifndef __irc_h__
#define __irc_h__


GLOBAL bool IRC_ERROR PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_KILL PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_NOTICE PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_PRIVMSG PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_TRACE PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_HELP PARAMS(( CLIENT *Client, REQUEST *Req ));


#endif


/* -eof- */
