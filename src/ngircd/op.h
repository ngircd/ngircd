/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2009 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Operator management (header)
 */

#ifndef __oper_h__
#define __oper_h__

GLOBAL bool Op_NoPrivileges PARAMS((CLIENT * Client, REQUEST * Req));
GLOBAL bool Op_Check PARAMS((CLIENT * Client, REQUEST * Req));

#endif

/* -eof- */
