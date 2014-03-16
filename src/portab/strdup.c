/*
 * ngIRCd -- The Next Generation IRC Daemon
 */

#include "portab.h"

/**
 * @file
 * strdup() implementation. Public domain.
 */

#ifndef HAVE_STRDUP

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

GLOBAL char *
strdup(const char *s)
{
	char *dup;
	size_t len = strlen(s);
	size_t alloc = len + 1;

	if (len >= alloc)
		return NULL;
	dup = malloc(alloc);
	if (dup)
		strlcpy(dup, s, alloc );

	return dup;
}

#endif
