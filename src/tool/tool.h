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
 * $Id: tool.h,v 1.2 2005/02/04 13:09:03 alex Exp $
 *
 * Tool functions (Header)
 */


#ifndef __tool_h__
#define __tool_h__


GLOBAL VOID ngt_TrimLastChr PARAMS((CHAR *String, CONST CHAR Chr ));

GLOBAL VOID ngt_TrimStr PARAMS((CHAR *String ));

GLOBAL CHAR *ngt_LowerStr PARAMS((CHAR *String ));


#endif


/* -eof- */
