/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2013 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __irc_h__
#define __irc_h__

/**
 * @file
 * IRC commands (header)
 */

GLOBAL bool IRC_CheckListTooBig PARAMS((CLIENT *From, const int Count,
					const int Limit, const char *Name));

GLOBAL bool IRC_ERROR PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_KILL PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_NOTICE PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_PRIVMSG PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_SQUERY PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_TRACE PARAMS((CLIENT *Client, REQUEST *Req));
GLOBAL bool IRC_HELP PARAMS((CLIENT *Client, REQUEST *Req));

GLOBAL bool IRC_KillClient PARAMS((CLIENT *Client, CLIENT *From,
				   const char *Nick, const char *Reason));

#endif

/* -eof- */
