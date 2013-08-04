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

#ifdef PAM

#ifndef __pam_h__
#define __pam_h__

/**
 * @file
 * PAM User Authentication (header)
 */

GLOBAL bool PAM_Authenticate PARAMS((CLIENT *Client));

#endif	/* __pam_h__ */

#endif	/* PAM */

/* -eof- */
