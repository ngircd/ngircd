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
 * $Id: match.h,v 1.4 2006/10/06 21:23:47 fw Exp $
 *
 * Wildcard pattern matching (header)
 */


#ifndef __match_h__
#define __match_h__


GLOBAL bool Match PARAMS(( const char *Pattern, const char *String ));
GLOBAL bool MatchCaseInsensitive PARAMS(( const char *Pattern, const char *searchme ));


#endif


/* -eof- */
