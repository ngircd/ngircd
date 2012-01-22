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
#include "stdio.h"

#include "exp.h"
#include "class.h"

struct list_head My_Classes[CLASS_COUNT];

char Reject_Reason[COMMAND_LEN];

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

GLOBAL char *
Class_GetMemberReason(const int Class, CLIENT *Client)
{
	char *reason;

	assert(Class < CLASS_COUNT);
	assert(Client != NULL);

	reason = Lists_CheckReason(&My_Classes[Class], Client);
	if (!reason)
		return NULL;

	if (!*reason)
		reason = "listed";

	switch(Class) {
		case CLASS_GLINE:
			snprintf(Reject_Reason, sizeof(Reject_Reason),
				 "\"%s\" (G-Line)", reason);
			return Reject_Reason;
		case CLASS_KLINE:
			snprintf(Reject_Reason, sizeof(Reject_Reason),
				 "\"%s\" (K-Line)", reason);
			return Reject_Reason;
	}
	return reason;
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
	char *rejectptr;

	assert(Client != NULL);

	rejectptr = Class_GetMemberReason(CLASS_GLINE, Client);
	if (!rejectptr)
		rejectptr = Class_GetMemberReason(CLASS_KLINE, Client);
	if (rejectptr) {
		Client_Reject(Client, rejectptr, true);
		return DISCONNECTED;
	}

	return CONNECTED;
}


GLOBAL bool
Class_AddMask(const int Class, const char *Mask, time_t ValidUntil,
	      const char *Reason)
{
	assert(Class < CLASS_COUNT);
	assert(Mask != NULL);
	assert(Reason != NULL);

	return Lists_Add(&My_Classes[Class], Lists_MakeMask(Mask),
			 ValidUntil, Reason);
}

GLOBAL void
Class_DeleteMask(const int Class, const char *Mask)
{
	assert(Class < CLASS_COUNT);
	assert(Mask != NULL);

	Lists_Del(&My_Classes[Class], Lists_MakeMask(Mask));
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
