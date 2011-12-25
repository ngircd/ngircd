/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2011 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __oper_h__
#define __oper_h__

/**
 * @file
 * IRC operator functions (header)
 */

GLOBAL bool Op_NoPrivileges PARAMS((CLIENT * Client, REQUEST * Req));
GLOBAL CLIENT *Op_Check PARAMS((CLIENT * Client, REQUEST * Req));

#endif

/* -eof- */
