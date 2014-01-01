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

static void
Panic(char *Reason)
{
	/* Oops, something failed!? */
	fprintf(stderr, "Oops, test for %s failed!?\n", Reason);
	exit(1);
} /* Panic */

static void
Check_snprintf(void)
{
	char str[5];

	snprintf(str, sizeof(str), "%s", "1234567890");
	if (str[4] != '\0')
		Panic("snprintf NULL byte");
	if (strlen(str) != 4)
		Panic("snprintf string length");
}

static void
Check_strlcpy(void)
{
	char str[5];

	if (strlcpy(str, "1234567890", sizeof(str)) != 10)
		Panic("strlcpy return code");
	if (str[4] != '\0')
		Panic("strlcpy NULL byte");
	if (strlen(str) != 4)
		Panic("strlcpy string length");
}

static void
Check_strlcat(void)
{
	char str[5];

	if (strlcpy(str, "12", sizeof(str)) != 2)
		Panic("strlcpy for strlcat");
	if (strlcat(str, "1234567890", sizeof(str)) != 12)
		Panic("strlcat return code");
	if (str[4] != '\0')
		Panic("strlcat NULL byte");
	if (strlen(str) != 4)
		Panic("strlcat string length");
}

#ifdef PROTOTYPES
static void
Check_vsnprintf(const int Len, const char *Format, ...)
#else
static void
Check_vsnprintf(Len, Format, va_alist)
const int Len;
const char *Format;
va_dcl
#endif
{
	char str[5];
	va_list ap;

#ifdef PROTOTYPES
	va_start(ap, Format);
#else
	va_start(ap);
#endif
	if (vsnprintf(str, sizeof(str), Format, ap) != Len)
		Panic("vsnprintf return code");
	va_end(ap);

	if (str[4] != '\0')
		Panic("vsnprintf NULL byte");
	if (strlen(str) != 4)
		Panic("vsnprintf string length");
}

GLOBAL int
main(void)
{
	/* validate datatypes */
	if (false != 0)
		Panic("false");
	if (true != 1)
		Panic("true");
	if (sizeof(UINT8) != 1)
		Panic("UINT8");
	if (sizeof(UINT16) != 2)
		Panic("UINT16");
	if (sizeof(UINT32) != 4)
		Panic("UINT32");

	/* check functions */
	Check_snprintf();
	Check_strlcpy();
	Check_strlcat();
	Check_vsnprintf(2+10, "%s%s", "ab", "1234567890");
	
	return 0;
}

/* -eof- */
