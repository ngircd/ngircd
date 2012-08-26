/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2012 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __match_h__
#define __match_h__

/**
 * @file
 * Wildcard pattern matching (header)
 */

GLOBAL bool Match PARAMS((const char *Pattern, const char *String));

GLOBAL bool MatchCaseInsensitive PARAMS((const char *Pattern,
					 const char *String));

GLOBAL bool MatchCaseInsensitiveList PARAMS((const char *Pattern,
					     const char *String,
					     const char *Separator));

#endif

/* -eof- */
