/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __tool_h__
#define __tool_h__

/**
 * @file
 * Tool functions (Header)
 */

#include "portab.h"

GLOBAL void ngt_TrimLastChr PARAMS((char *String, const char Chr ));

GLOBAL void ngt_TrimStr PARAMS((char *String ));

GLOBAL char *ngt_UpperStr PARAMS((char *String ));
GLOBAL char *ngt_LowerStr PARAMS((char *String ));

GLOBAL char *ngt_RandomStr PARAMS((char *String, const size_t len));

#ifdef SYSLOG
GLOBAL const char *ngt_SyslogFacilityName PARAMS((int Facility));
GLOBAL int ngt_SyslogFacilityID PARAMS((char *Name, int DefaultFacility));
#endif

#endif

/* -eof- */
