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
 * Management of IRC lists: ban, invite, etc.
 */

#include "imp.h"
#include <assert.h>

#include "defines.h"
#include "conn.h"
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
	char *reason;
	time_t valid_until;	/** 0: unlimited; 1: once; t(>1): until t */
};


GLOBAL const char *
Lists_GetMask(const struct list_elem *e)
{
	return e->mask;
}

GLOBAL const char *
Lists_GetReason(const struct list_elem *e)
{
	assert(e != NULL);
	return e->reason;
}

GLOBAL time_t
Lists_GetValidity(const struct list_elem *e)
{
	assert(e != NULL);
	return e->valid_until;
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

/**
 * Add a new mask to a list.
 *
 * @param header List head.
 * @param Mask The IRC mask to add to the list.
 * @param ValidUntil 0: unlimited, 1: only once, t>1: until given time_t.
 * @param Reason Reason string or NULL, if no reason should be saved.
 * @return true on success, false otherwise.
 */
bool
Lists_Add(struct list_head *header, const char *Mask, time_t ValidUntil,
	  const char *Reason)
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

	strlcpy(newelem->mask, Mask, sizeof(newelem->mask));
	if (Reason) {
		newelem->reason = malloc(strlen(Reason) + 1);
		if (newelem->reason)
			strlcpy(newelem->reason, Reason, strlen(Reason) + 1);
		else
			Log(LOG_EMERG,
			    "Can't allocate memory for new list reason text!");
	}
	else
		newelem->reason = NULL;
	newelem->valid_until = ValidUntil;
	newelem->next = e;
	header->first = newelem;

	return true;
}


static void
Lists_Unlink(struct list_head *header, struct list_elem *p, struct list_elem *victim)
{
	assert(victim != NULL);
	assert(header != NULL);

	if (p) p->next = victim->next;
	else header->first = victim->next;

	if (victim->reason)
		free(victim->reason);
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
		if (victim->reason)
			free(victim->reason);
		free(victim);
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
	struct list_elem *e, *last, *next;

	assert( header != NULL );

	e = header->first;
	last = NULL;

	while (e) {
		next = e->next;
		if (e->valid_until > 1 && e->valid_until < time(NULL)) {
			/* Entry is expired, delete it */
			LogDebug("Deleted \"%s\" from list (expired).",
				 e->mask);
			Lists_Unlink(header, last, e);
			e = next;
			continue;
		}
		if (Match(e->mask, Client_Mask(Client))) {
			if (e->valid_until == 1 ) {
				/* Entry is valid only once, delete it */
				LogDebug("Deleted \"%s\" from list (used).",
					 e->mask);
				Lists_Unlink(header, last, e);
			}
			return true;
		}
		last = e;
		e = next;
	}

	return false;
}

/* -eof- */
