/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Tool functions (Header)
 */


#ifndef __tool_h__
#define __tool_h__
#include "portab.h"

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#else
# define PF_INET AF_INET
#endif

GLOBAL void ngt_TrimLastChr PARAMS((char *String, const char Chr ));

GLOBAL void ngt_TrimStr PARAMS((char *String ));

GLOBAL char *ngt_UpperStr PARAMS((char *String ));
GLOBAL char *ngt_LowerStr PARAMS((char *String ));

#ifdef SYSLOG
GLOBAL const char *ngt_SyslogFacilityName PARAMS((int Facility));
GLOBAL int ngt_SyslogFacilityID PARAMS((char *Name, int DefaultFacility));
#endif

#endif


/* -eof- */
