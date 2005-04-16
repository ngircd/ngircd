/*
 * ngIRCd -- The Next Generation IRC Daemon
 *
 * strdup() implementation.  Public domain.
 *
 * $Id: strdup.c,v 1.1 2005/04/16 09:20:53 fw Exp $
 */

#include "portab.h"

#include "imp.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "exp.h"

#ifndef HAVE_STRDUP

GLOBAL char *
strdup( const char *s )
{
 char *dup;
 size_t len = strlen( s );
 size_t alloc = len + 1;

 if (len >= alloc ) return NULL;
 dup = malloc( alloc );
 if (dup) strlcpy(dup, s, alloc );

return dup;
}

#endif

