/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2005 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Management of IRC lists: ban, invite, ...
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>

#include "defines.h"
#include "conn.h"
#include "client.h"
#include "channel.h"
#include "log.h"
#include "match.h"
#include "messages.h"
#include "irc-write.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "exp.h"
#include "lists.h"

#define MASK_LEN	(2*CLIENT_HOST_LEN)

struct list_elem {
	struct list_elem *next;
	char mask[MASK_LEN];
	bool onlyonce;
};


GLOBAL const char *
Lists_GetMask(const struct list_elem *e)
{
	return e->mask;
}


GLOBAL struct list_elem*
Lists_GetFirst(const struct list_head *h)
{
	return h->first;
}


GLOBAL struct list_elem*
Lists_GetNext(const struct list_elem *e)
{
	return e->next;
}


bool
Lists_Add(struct list_head *header, const char *Mask, bool OnlyOnce )
{
	struct list_elem *e, *newelem;

	assert( header != NULL );
	assert( Mask != NULL );

	if (Lists_CheckDupeMask(header, Mask )) return true;

	e = Lists_GetFirst(header);

	newelem = malloc(sizeof(struct list_elem));
	if( ! newelem ) {
		Log( LOG_EMERG, "Can't allocate memory for new Ban/Invite entry!" );
		return false;
	}

	strlcpy( newelem->mask, Mask, sizeof( newelem->mask ));
	newelem->onlyonce = OnlyOnce;
	newelem->next = e;
	header->first = newelem;

	LogDebug("Added \"%s\" to invite list", Mask);
	return true;
}


static void
Lists_Unlink(struct list_head *header, struct list_elem *p, struct list_elem *victim)
{
	assert(victim != NULL);
	assert(header != NULL);

	if (p) p->next = victim->next;
	else header->first = victim->next;

	free(victim);
}


GLOBAL void
Lists_Del(struct list_head *header, const char *Mask)
{
	struct list_elem *e, *last, *victim;

	assert( header != NULL );
	assert( Mask != NULL );

	last = NULL;
	e = Lists_GetFirst(header);
	while( e ) {
		if(strcasecmp( e->mask, Mask ) == 0 ) {
			LogDebug("Deleted \"%s\" from list", e->mask);
			victim = e;
			e = victim->next;
			Lists_Unlink(header, last, victim);
			continue;
		}
		last = e;
		e = e->next;
	}
}


GLOBAL void
Lists_Free(struct list_head *head)
{
	struct list_elem *e, *victim;

	assert(head != NULL);

	e = head->first;
	head->first = NULL;
	while (e) {
		LogDebug("Deleted \"%s\" from invite list" , e->mask);
		victim = e;
		e = e->next;
		free( victim );
	}
}


GLOBAL bool
Lists_CheckDupeMask(const struct list_head *h, const char *Mask )
{
	struct list_elem *e;
	e = h->first;
	while (e) {
		if (strcasecmp( e->mask, Mask ) == 0 )
			return true;
		e = e->next;
	}
	return false;
}


GLOBAL const char *
Lists_MakeMask(const char *Pattern)
{
	/* This function generats a valid IRC mask of "any" string. This
	 * mask is only valid until the next call to Lists_MakeMask(),
	 * because a single global buffer is used. You have to copy the
	 * generated mask to some sane location yourself! */

	static char TheMask[MASK_LEN];
	char *excl, *at;

	assert( Pattern != NULL );

	excl = strchr( Pattern, '!' );
	at = strchr( Pattern, '@' );

	if(( at ) && ( at < excl )) excl = NULL;

	if(( ! at ) && ( ! excl ))
	{
		/* Neither "!" nor "@" found: use string as nick name */
		strlcpy( TheMask, Pattern, sizeof( TheMask ) - 5 );
		strlcat( TheMask, "!*@*", sizeof( TheMask ));
		return TheMask;
	}

	if(( ! at ) && ( excl ))
	{
		/* Domain part is missing */
		strlcpy( TheMask, Pattern, sizeof( TheMask ) - 3 );
		strlcat( TheMask, "@*", sizeof( TheMask ));
		return TheMask;
	}

	if(( at ) && ( ! excl ))
	{
		/* User name is missing */
		*at = '\0'; at++;
		strlcpy( TheMask, Pattern, sizeof( TheMask ) - 5 );
		strlcat( TheMask, "!*@", sizeof( TheMask ));
		strlcat( TheMask, at, sizeof( TheMask ));
		return TheMask;
	}

	/* All parts (nick, user and domain name) are given */
	strlcpy( TheMask, Pattern, sizeof( TheMask ));
	return TheMask;
} /* Lists_MakeMask */



bool
Lists_Check( struct list_head *header, CLIENT *Client)
{
	struct list_elem *e, *last;

	assert( header != NULL );

	e = header->first;
	last = NULL;

	while( e ) {
		if( Match( e->mask, Client_Mask( Client ))) {
			if( e->onlyonce ) { /* delete entry */
				LogDebug("Deleted \"%s\" from list", e->mask);
				Lists_Unlink(header, last, e);
			}
			return true;
		}
		last = e;
		e = e->next;
	}

	return false;
}

/* -eof- */
