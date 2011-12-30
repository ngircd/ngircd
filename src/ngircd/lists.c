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
	struct list_elem *next;	/** pointer to next list element */
	char mask[MASK_LEN];	/** IRC mask */
	char *reason;		/** Optional "reason" text */
	time_t valid_until;	/** 0: unlimited; 1: once; t(>1): until t */
};

/**
 * Get IRC mask stored in list element.
 *
 * @param list_elem List element.
 * @return Pointer to IRC mask
 */
GLOBAL const char *
Lists_GetMask(const struct list_elem *e)
{
	assert(e != NULL);
	return e->mask;
}

/**
 * Get optional "reason" text stored in list element.
 *
 * @param list_elem List element.
 * @return Pointer to "reason" text or NULL.
 */
GLOBAL const char *
Lists_GetReason(const struct list_elem *e)
{
	assert(e != NULL);
	return e->reason;
}

/**
 * Get "validity" value stored in list element.
 *
 * @param list_elem List element.
 * @return Validity: 0=unlimited, 1=once, >1 until this time stamp.
 */
GLOBAL time_t
Lists_GetValidity(const struct list_elem *e)
{
	assert(e != NULL);
	return e->valid_until;
}

/**
 * Get first list element of a list.
 *
 * @param h List head.
 * @return Pointer to first list element.
 */
GLOBAL struct list_elem*
Lists_GetFirst(const struct list_head *h)
{
	assert(h != NULL);
	return h->first;
}

/**
 * Get next list element of a list.
 *
 * @param e Current list element.
 * @return Pointer to next list element.
 */
GLOBAL struct list_elem*
Lists_GetNext(const struct list_elem *e)
{
	assert(e != NULL);
	return e->next;
}

/**
 * Add a new mask to a list.
 *
 * @param h List head.
 * @param Mask The IRC mask to add to the list.
 * @param ValidUntil 0: unlimited, 1: only once, t>1: until given time_t.
 * @param Reason Reason string or NULL, if no reason should be saved.
 * @return true on success, false otherwise.
 */
bool
Lists_Add(struct list_head *h, const char *Mask, time_t ValidUntil,
	  const char *Reason)
{
	struct list_elem *e, *newelem;

	assert(h != NULL);
	assert(Mask != NULL);

	e = Lists_CheckDupeMask(h, Mask);
	if (e) {
		e->valid_until = ValidUntil;
		if (e->reason)
			free(e->reason);
		e->reason = malloc(strlen(Reason) + 1);
		if (e->reason)
			strlcpy(e->reason, Reason, strlen(Reason) + 1);
		else
			Log(LOG_EMERG,
			    "Can't allocate memory for new list reason text!");
		return true;
	}

	e = Lists_GetFirst(h);

	newelem = malloc(sizeof(struct list_elem));
	if (!newelem) {
		Log(LOG_EMERG,
		    "Can't allocate memory for new list entry!");
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
	h->first = newelem;

	return true;
}

/**
 * Delete a list element from a list.
 *
 * @param h List head.
 * @param p Pointer to previous list element or NULL, if there is none.
 * @param victim List element to delete.
 */
static void
Lists_Unlink(struct list_head *h, struct list_elem *p, struct list_elem *victim)
{
	assert(victim != NULL);
	assert(h != NULL);

	if (p)
		p->next = victim->next;
	else
		h->first = victim->next;

	if (victim->reason)
		free(victim->reason);

	free(victim);
}

/**
 * Delete a given IRC mask from a list.
 *
 * @param h List head.
 * @param Mask IRC mask to delete from the list.
 */
GLOBAL void
Lists_Del(struct list_head *h, const char *Mask)
{
	struct list_elem *e, *last, *victim;

	assert(h != NULL);
	assert(Mask != NULL);

	last = NULL;
	e = Lists_GetFirst(h);
	while (e) {
		if (strcasecmp(e->mask, Mask) == 0) {
			LogDebug("Deleted \"%s\" from list", e->mask);
			victim = e;
			e = victim->next;
			Lists_Unlink(h, last, victim);
			continue;
		}
		last = e;
		e = e->next;
	}
}

/**
 * Free a complete list.
 *
 * @param head List head.
 */
GLOBAL void
Lists_Free(struct list_head *head)
{
	struct list_elem *e, *victim;

	assert(head != NULL);

	e = head->first;
	head->first = NULL;
	while (e) {
		LogDebug("Deleted \"%s\" from list" , e->mask);
		victim = e;
		e = e->next;
		if (victim->reason)
			free(victim->reason);
		free(victim);
	}
}

/**
 * Check if an IRC mask is already contained in a list.
 *
 * @param h List head.
 * @param Mask IRC mask to test.
 * @return true if mask is already stored in the list, false otherwise.
 */
GLOBAL struct list_elem *
Lists_CheckDupeMask(const struct list_head *h, const char *Mask )
{
	struct list_elem *e;
	e = h->first;
	while (e) {
		if (strcasecmp(e->mask, Mask) == 0)
			return e;
		e = e->next;
	}
	return NULL;
}

/**
 * Generate a valid IRC mask from "any" string given.
 *
 * Attention: This mask is only valid until the next call to Lists_MakeMask(),
 * because a single global buffer ist used! You have to copy the generated
 * mask to some sane location yourself!
 *
 * @param Pattern Source string to generate an IRC mask for.
 * @return Pointer to global result buffer.
 */
GLOBAL const char *
Lists_MakeMask(const char *Pattern)
{
	static char TheMask[MASK_LEN];
	char *excl, *at;

	assert(Pattern != NULL);

	excl = strchr(Pattern, '!');
	at = strchr(Pattern, '@');

	if (at && at < excl)
		excl = NULL;

	if (!at && !excl) {
		/* Neither "!" nor "@" found: use string as nick name */
		strlcpy(TheMask, Pattern, sizeof(TheMask) - 5);
		strlcat(TheMask, "!*@*", sizeof(TheMask));
		return TheMask;
	}

	if (!at && excl) {
		/* Domain part is missing */
		strlcpy(TheMask, Pattern, sizeof(TheMask) - 3);
		strlcat(TheMask, "@*", sizeof(TheMask));
		return TheMask;
	}

	if (at && !excl) {
		/* User name is missing */
		*at = '\0'; at++;
		strlcpy(TheMask, Pattern, sizeof(TheMask) - 5);
		strlcat(TheMask, "!*@", sizeof(TheMask));
		strlcat(TheMask, at, sizeof(TheMask));
		return TheMask;
	}

	/* All parts (nick, user and domain name) are given */
	strlcpy(TheMask, Pattern, sizeof(TheMask));
	return TheMask;
} /* Lists_MakeMask */

/**
 * Check if a client is listed in a list.
 *
 * @param h List head.
 * @param Client Client to check.
 * @return true if client is listed, false if not.
 */
bool
Lists_Check( struct list_head *h, CLIENT *Client)
{
	struct list_elem *e, *last, *next;

	assert(h != NULL);

	e = h->first;
	last = NULL;

	while (e) {
		next = e->next;
		if (Match(e->mask, Client_Mask(Client))) {
			if (e->valid_until == 1) {
				/* Entry is valid only once, delete it */
				LogDebug("Deleted \"%s\" from list (used).",
					 e->mask);
				Lists_Unlink(h, last, e);
			}
			return true;
		}
		last = e;
		e = next;
	}

	return false;
}

/**
 * Check list and purge expired entries.
 *
 * @param h List head.
 */
GLOBAL void
Lists_Expire(struct list_head *h, const char *ListName)
{
	struct list_elem *e, *last, *next;
	time_t now;

	assert(h != NULL);

	e = h->first;
	last = NULL;
	now = time(NULL);

	while (e) {
		next = e->next;
		if (e->valid_until > 1 && e->valid_until < now) {
			/* Entry is expired, delete it */
			if (e->reason)
				Log(LOG_INFO,
				    "Deleted \"%s\" (\"%s\") from %s list (expired).",
				    e->mask, e->reason, ListName);
			else
				Log(LOG_INFO,
				    "Deleted \"%s\" from %s list (expired).",
				    e->mask, ListName);
			Lists_Unlink(h, last, e);
			e = next;
			continue;
		}
		last = e;
		e = next;
	}
}

/* -eof- */
