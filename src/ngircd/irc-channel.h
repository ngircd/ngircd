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
 * $Id: irc-channel.h,v 1.6 2002/12/12 12:23:43 alex Exp $
 *
 * IRC channel commands (header)
 */


#ifndef __irc_channel_h__
#define __irc_channel_h__


GLOBAL BOOLEAN IRC_JOIN PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_PART PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_TOPIC PARAMS((CLIENT *Client, REQUEST *Req ));

GLOBAL BOOLEAN IRC_LIST PARAMS((CLIENT *Client, REQUEST *Req ));

GLOBAL BOOLEAN IRC_CHANINFO PARAMS((CLIENT *Client, REQUEST *Req ));


#endif


/* -eof- */
