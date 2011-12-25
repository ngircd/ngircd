/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2011 Alexander Barton (alex@barton.de) and Contributors.
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
 * User class management.
 */

#include "imp.h"
#include <assert.h>
#include <string.h>

#include "defines.h"
#include "array.h"
#include "conn.h"
#include "client.h"
#include "lists.h"
#include "match.h"

#include "exp.h"
#include "class.h"

struct list_head My_Classes[CLASS_COUNT];

GLOBAL void
Class_Init(void)
{
	memset(My_Classes, 0, sizeof(My_Classes));
}

GLOBAL void
Class_Exit(void)
{
	int i;

	for (i = 0; i < CLASS_COUNT; Lists_Free(&My_Classes[i++]));
}

GLOBAL bool
Class_IsMember(const int Class, CLIENT *Client)
{
	assert(Class < CLASS_COUNT);
	assert(Client != NULL);

	return Lists_Check(&My_Classes[Class], Client);
}

GLOBAL bool
Class_AddMask(const int Class, const char *Mask, time_t ValidUntil,
	      const char *Reason)
{
	assert(Class < CLASS_COUNT);
	assert(Mask != NULL);
	assert(Reason != NULL);

	return Lists_Add(&My_Classes[Class], Mask, ValidUntil, Reason);
}

GLOBAL void
Class_DeleteMask(const int Class, const char *Mask)
{
	assert(Class < CLASS_COUNT);
	assert(Mask != NULL);

	Lists_Del(&My_Classes[Class], Mask);
}

/* -eof- */
