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

#ifndef __lists_h__
#define __lists_h__

/**
 * @file
 * Management of IRC lists (header)
 */

#include "portab.h"
#include "client.h"

struct list_elem;

struct list_head {
	struct list_elem *first;
};

GLOBAL struct list_elem *Lists_GetFirst PARAMS((const struct list_head *));
GLOBAL struct list_elem *Lists_GetNext PARAMS((const struct list_elem *));

GLOBAL bool Lists_Check PARAMS((struct list_head *head, CLIENT *client));
GLOBAL bool Lists_CheckReason PARAMS((struct list_head *head, CLIENT *client,
				      char *reason, size_t len));
GLOBAL struct list_elem *Lists_CheckDupeMask PARAMS((const struct list_head *head,
					const char *mask));

GLOBAL bool Lists_Add PARAMS((struct list_head *h, const char *Mask,
			      time_t ValidUntil, const char *Reason,
			      bool OnlyOnce));
GLOBAL void Lists_Del PARAMS((struct list_head *head, const char *Mask));
GLOBAL unsigned long Lists_Count PARAMS((struct list_head *h));

GLOBAL void Lists_Free PARAMS((struct list_head *head));

GLOBAL void Lists_MakeMask PARAMS((const char *Pattern, char *mask, size_t len));
GLOBAL const char *Lists_GetMask PARAMS((const struct list_elem *e));
GLOBAL const char *Lists_GetReason PARAMS((const struct list_elem *e));
GLOBAL time_t Lists_GetValidity PARAMS((const struct list_elem *e));
GLOBAL bool Lists_GetOnlyOnce PARAMS((const struct list_elem *e));

GLOBAL void Lists_Expire PARAMS((struct list_head *h, const char *ListName));

#endif

/* -eof- */
