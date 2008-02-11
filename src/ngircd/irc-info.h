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
 * $Id: irc-info.h,v 1.5 2008/02/11 11:06:31 fw Exp $
 *
 * IRC info commands (header)
 */


#ifndef __irc_info_h__
#define __irc_info_h__


GLOBAL bool IRC_ADMIN PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_ISON PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_LINKS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_LUSERS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_MOTD PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_NAMES PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_STATS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_TIME PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_USERHOST PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_VERSION PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_WHO PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_WHOIS PARAMS(( CLIENT *Client, REQUEST *Req ));
GLOBAL bool IRC_WHOWAS PARAMS(( CLIENT *Client, REQUEST *Req ));

GLOBAL bool IRC_Send_LUSERS PARAMS(( CLIENT *Client ));
GLOBAL bool IRC_Send_NAMES PARAMS(( CLIENT *Client, CHANNEL *Chan ));
GLOBAL bool IRC_Show_MOTD PARAMS(( CLIENT *Client ));
GLOBAL bool IRC_Send_ISUPPORT PARAMS(( CLIENT *Client ));


#endif


/* -eof- */
