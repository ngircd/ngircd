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
 * $Id: irc-info.h,v 1.2 2002/12/12 12:23:43 alex Exp $
 *
 * IRC info commands (header)
 */


#ifndef __irc_info_h__
#define __irc_info_h__


GLOBAL BOOLEAN IRC_ADMIN PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_ISON PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_LINKS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_LUSERS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_MOTD PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_NAMES PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_STATS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_TIME PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_USERHOST PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_VERSION PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_WHO PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_WHOIS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL BOOLEAN IRC_WHOWAS PARAMS(( CLIENT *Client, REQUEST *Req ));

GLOBAL BOOLEAN IRC_Send_LUSERS PARAMS(( CLIENT *Client ));
GLOBAL BOOLEAN IRC_Send_NAMES PARAMS(( CLIENT *Client, CHANNEL *Chan ));
GLOBAL BOOLEAN IRC_Show_MOTD PARAMS(( CLIENT *Client ));
GLOBAL BOOLEAN IRC_Send_WHO PARAMS(( CLIENT *Client, CHANNEL *Chan, BOOLEAN OnlyOps ));


#endif


/* -eof- */
