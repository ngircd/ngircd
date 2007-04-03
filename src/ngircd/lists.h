/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: lists.h,v 1.12.4.1 2007/04/03 20:23:31 fw Exp $
 *
 * Management of IRC lists: ban, invite, ... (header)
 */


#ifndef __lists_h__
#define __lists_h__
#include "portab.h"
#include "client.h"

struct list_elem;

struct list_head {
	struct list_elem *first;
};


GLOBAL struct list_elem *Lists_GetFirst PARAMS((const struct list_head *));
GLOBAL struct list_elem *Lists_GetNext PARAMS((const struct list_elem *));

GLOBAL bool Lists_Check PARAMS((struct list_head *head, CLIENT *client ));
GLOBAL bool Lists_CheckDupeMask PARAMS((const struct list_head *head, const char *mask ));

GLOBAL bool Lists_Add PARAMS((struct list_head *header, const char *Mask, bool OnlyOnce ));
GLOBAL void Lists_Del PARAMS((struct list_head *head, const char *Mask ));

GLOBAL bool Lists_AlreadyRegistered PARAMS(( const struct list_head *head, const char *Mask));

GLOBAL void Lists_Free PARAMS(( struct list_head *head ));

GLOBAL char *Lists_MakeMask PARAMS(( char *Pattern ));
GLOBAL const char *Lists_GetMask PARAMS(( const struct list_elem *e ));

#endif
/* -eof- */
