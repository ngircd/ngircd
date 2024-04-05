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

#ifndef signals_included_
#define signals_included_

/**
 * @file
 * Signal Handlers (header).
 */

#include "portab.h"

bool Signals_Init PARAMS((void));
void Signals_Exit PARAMS((void));

GLOBAL bool Signal_NotifySvcMgr_Possible PARAMS((void));
GLOBAL void Signal_NotifySvcMgr PARAMS((const char *message));

#endif

/* -eof- */
