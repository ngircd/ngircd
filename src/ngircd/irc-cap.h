/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton (alex@barton.de).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __irc_cap_h__
#define __irc_cap_h__

/**
 * @file
 * Handler for IRC capability ("CAP") commands (header)
 */

GLOBAL bool IRC_CAP PARAMS((CLIENT *Client, REQUEST *Req));

#endif

/* -eof- */
