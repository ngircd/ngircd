/*
 * ngIRCd -- The Next Generation IRC Daemon
 */

#include "portab.h"

/**
 * @file
 * strndup() implementation. Public domain.
 */

#ifndef HAVE_STRNDUP

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

GLOBAL char *
strndup(const char *s, size_t maxlen)
{
	char *dup;
	size_t len = strlen(s);

	if (len > maxlen)
		len = maxlen;
	len++;
	dup = malloc(len);
	if (dup)
		strlcpy(dup, s, len);

	return dup;
}

#endif
