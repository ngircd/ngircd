/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2014 Alexander Barton (alex@barton.de) and Contributors.
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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "conn.h"
#include "lists.h"

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
Class_GetMemberReason(const int Class, CLIENT *Client, char *reason, size_t len)
{
	char str[COMMAND_LEN];

	assert(Class < CLASS_COUNT);
	assert(Client != NULL);

	strlcpy(str, "listed", sizeof(str));

	if (!Lists_CheckReason(&My_Classes[Class], Client, str, sizeof(str)))
		return false;

	switch(Class) {
		case CLASS_GLINE:
			snprintf(reason, len, "\"%s\" (G-Line)", str);
			break;
		case CLASS_KLINE:
			snprintf(reason, len, "\"%s\" (K-Line)", str);
			break;
		default:
			snprintf(reason, len, "%s", str);
			break;
	}
	return true;
}

/**
 * Check if a client is banned from this server: GLINE, KLINE.
 *
 * If a client isn't allowed to connect, it will be disconnected again.
 *
 * @param Client The client to check.
 * @return CONNECTED if client is allowed to join, DISCONNECTED if not.
 */
GLOBAL bool
Class_HandleServerBans(CLIENT *Client)
{
	char reject[COMMAND_LEN];

	assert(Client != NULL);

	if (Class_GetMemberReason(CLASS_GLINE, Client, reject, sizeof(reject)) ||
	    Class_GetMemberReason(CLASS_KLINE, Client, reject, sizeof(reject))) {
		Client_Reject(Client, reject, true);
		return DISCONNECTED;
	}

	return CONNECTED;
}


GLOBAL bool
Class_AddMask(const int Class, const char *Pattern, time_t ValidUntil,
	      const char *Reason)
{
	char mask[MASK_LEN];

	assert(Class < CLASS_COUNT);
	assert(Pattern != NULL);
	assert(Reason != NULL);

	Lists_MakeMask(Pattern, mask, sizeof(mask));
	return Lists_Add(&My_Classes[Class], mask,
			 ValidUntil, Reason, false);
}

GLOBAL void
Class_DeleteMask(const int Class, const char *Pattern)
{
	char mask[MASK_LEN];

	assert(Class < CLASS_COUNT);
	assert(Pattern != NULL);

	Lists_MakeMask(Pattern, mask, sizeof(mask));
	Lists_Del(&My_Classes[Class], mask);
}

GLOBAL struct list_head *
Class_GetList(const int Class)
{
	assert(Class < CLASS_COUNT);

	return &My_Classes[Class];
}

GLOBAL void
Class_Expire(void)
{
	Lists_Expire(&My_Classes[CLASS_GLINE], "G-Line");
	Lists_Expire(&My_Classes[CLASS_KLINE], "K-Line");
}

/* -eof- */
