/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2024 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __resolve_h__
#define __resolve_h__

/**
 * @file
 * Asynchronous resolver (header)
 */

GLOBAL bool Resolve_Addr_Ident PARAMS((PROC_STAT * s, const ng_ipaddr_t * Addr,
				 int identsock, void (*cbfunc) (int, short)));
GLOBAL bool Resolve_Name PARAMS((PROC_STAT * s, const char *Host,
				 void (*cbfunc) (int, short)));

#endif

/* -eof- */
