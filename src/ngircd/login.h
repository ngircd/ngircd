/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2012 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __login_h__
#define __login_h__

/**
 * @file
 * Functions to deal with client logins (header)
 */

GLOBAL bool Login_User PARAMS((CLIENT * Client));
GLOBAL bool Login_User_PostAuth PARAMS((CLIENT *Client));
GLOBAL void Login_Autojoin PARAMS((CLIENT *Client));

#endif

/* -eof- */
