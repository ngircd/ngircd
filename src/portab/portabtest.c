/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2013 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#include "portab.h"

/**
 * @file
 * Test program for portab.h and friends ;-)
 */

#include "imp.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exp.h"

static void Panic PARAMS (( char *Reason, int Code ));

GLOBAL int
main(void)
{
	/* validate datatypes */
	if (false != 0)
		Panic("false", 1);
	if (true != 1)
		Panic("true", 1);
	if (sizeof(UINT8) != 1)
		Panic("UINT8", 1);
	if (sizeof(UINT16) != 2)
		Panic("UINT16", 1);
	if (sizeof(UINT32) != 4)
		Panic("UINT32", 1);

#ifdef PROTOTYPES
	/* check functions */
	if (!snprintf)
		Panic("snprintf", 2);
	if (!vsnprintf)
		Panic("vsnprintf", 2);
	if (!strlcpy)
		Panic("strlcpy", 2);
	if (!strlcat)
		Panic("strlcat", 2);
#endif
	
	/* ok, no error */
	return 0;
} /* portab_check_types */

static void
Panic(char *Reason, int Code)
{
	/* Oops, something failed!? */
	fprintf(stderr, "Oops, test for %s failed!?", Reason);
	exit(Code);
} /* Panic */

/* -eof- */
