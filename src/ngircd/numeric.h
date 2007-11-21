/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2007 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: numeric.h,v 1.1 2007/11/21 12:20:32 alex Exp $
 *
 * Handlers for IRC numerics sent to the server (header)
 */

#ifndef __numeric_h__
#define __numeric_h__

GLOBAL bool IRC_Num_ENDOFMOTD PARAMS((CLIENT *Client, UNUSED REQUEST *Req));
GLOBAL bool IRC_Num_ISUPPORT PARAMS((CLIENT *Client, REQUEST *Req));

#endif

/* -eof- */
