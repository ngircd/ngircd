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
 * strlcpy() and strlcat() replacement functions.
 * See <http://www.openbsd.org/papers/strlcpy-paper.ps> for details.
 *
 * Code partially borrowed from compat.c of rsync, written by Andrew
 * Tridgell (1998) and Martin Pool (2002):
 * <http://samba.anu.edu.au/rsync/doxygen/head/lib_2compat_8c.html>
 */


#include "portab.h"

static char UNUSED id[] = "$Id: strlcpy.c,v 1.2 2002/12/26 14:34:11 alex Exp $";

#include "imp.h"
#include <string.h>
#include <sys/types.h>

#include "exp.h"


#ifndef HAVE_STRLCAT

GLOBAL size_t
strlcat( CHAR *dst, CONST CHAR *src, size_t size )
{
	/* Like strncat() but does not 0 fill the buffer and
	 * always null terminates. */

	size_t len1 = strlen( dst );
	size_t len2 = strlen( src );
	size_t ret = len1 + len2;
	
	if( len1 + len2 >= size ) len2 = size - ( len1 + 1 );
	if( len2 > 0 )
	{
		memcpy( dst + len1, src, len2 );
		dst[len1 + len2] = 0;
	}
	return ret;
} /* strlcat */

#endif


#ifndef HAVE_STRLCPY

GLOBAL size_t
strlcpy( CHAR *dst, CONST CHAR *src, size_t size )
{
	/* Like strncpy but does not 0 fill the buffer and
	 * always null terminates. */

	size_t len = strlen( src );

	if( size <= 0 ) return len;
	if( len >= size ) len = size - 1;
	memcpy( dst, src, len );
	dst[len] = 0;
	return len;
} /* strlcpy */

#endif


/* -eof- */
