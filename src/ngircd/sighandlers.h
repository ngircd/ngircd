/*
 * ngIRCd -- The Next Generation IRC Daemon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef signals_included_
#define signals_included_

#include "portab.h"

bool Signals_Init PARAMS((void));
void Signals_Exit PARAMS((void));

#endif
