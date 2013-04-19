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

#ifndef __class_h__
#define __class_h__

/**
 * @file
 * User class management.
 */

#define CLASS_KLINE 0
#define CLASS_GLINE 1

#define CLASS_COUNT 2

GLOBAL void Class_Init PARAMS((void));
GLOBAL void Class_Exit PARAMS((void));

GLOBAL bool Class_AddMask PARAMS((const int Class, const char *Pattern,
				  const time_t ValidUntil, const char *Reason));
GLOBAL void Class_DeleteMask PARAMS((const int Class, const char *Pattern));

GLOBAL bool Class_GetMemberReason PARAMS((const int Class, CLIENT *Client,
					  char *reason, size_t len));
GLOBAL bool Class_HandleServerBans PARAMS((CLIENT *Client));

GLOBAL struct list_head *Class_GetList PARAMS((const int Class));

GLOBAL void Class_Expire PARAMS((void));

#endif /* __class_h__ */

/* -eof- */
