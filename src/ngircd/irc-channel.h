/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2022 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __irc_channel_h__
#define __irc_channel_h__

/**
 * @file
 * IRC channel commands (header)
 */

GLOBAL bool IRC_JOIN PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_PART PARAMS((CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_TOPIC PARAMS((CLIENT *Client, REQUEST *Req ));

GLOBAL bool IRC_LIST PARAMS((CLIENT *Client, REQUEST *Req ));

GLOBAL bool IRC_CHANINFO PARAMS((CLIENT *Client, REQUEST *Req ));

GLOBAL bool IRC_Send_Channel_Info PARAMS((CLIENT *Client, CHANNEL *Chan));

#endif

/* -eof- */
