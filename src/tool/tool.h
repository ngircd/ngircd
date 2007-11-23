/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: tool.h,v 1.4 2007/11/23 16:26:05 fw Exp $
 *
 * Tool functions (Header)
 */


#ifndef __tool_h__
#define __tool_h__
#include "portab.h"

GLOBAL void ngt_TrimLastChr PARAMS((char *String, const char Chr ));

GLOBAL void ngt_TrimStr PARAMS((char *String ));

GLOBAL char *ngt_LowerStr PARAMS((char *String ));

GLOBAL bool ngt_IPStrToBin PARAMS((const char *ip_str, struct in_addr *inaddr));
#endif


/* -eof- */
