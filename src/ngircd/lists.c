/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2018 Alexander Barton (alex@barton.de) and Contributors.
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "conn.h"
#include "log.h"
#include "match.h"

#include "lists.h"

struct list_elem {
	struct list_elem *next;	/** pointer to next list element */
	char mask[MASK_LEN];	/** IRC mask */
	char *reason;		/** Optional "reason" text */
	time_t valid_until;	/** 0: unlimited; t(>0): until t */
	bool onlyonce;
};

/**
 * Get IRC mask stored in list element.
 *
 * @param e List element.
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
 * @param e List element.
 * @return Pointer to "reason" text or empty string ("").
 */
GLOBAL const char *
Lists_GetReason(const struct list_elem *e)
{
	assert(e != NULL);
	return e->reason ? e->reason : "";
}

/**
 * Get "validity" value stored in list element.
 *
 * @param e List element.
 * @return Validity: 0=unlimited, >0 until this time stamp.
 */
GLOBAL time_t
Lists_GetValidity(const struct list_elem *e)
{
	assert(e != NULL);
	return e->valid_until;
}

/**
 * Get "onlyonce" value stored in list element.
 *
 * @param e List element.
 * @return True if the element was stored for single use, false otherwise.
 */
GLOBAL bool
Lists_GetOnlyOnce(const struct list_elem *e)
{
	assert(e != NULL);
	return e->onlyonce;
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
	  const char *Reason, bool OnlyOnce)
{
	struct list_elem *e, *newelem;

	assert(h != NULL);
	assert(Mask != NULL);

	e = Lists_CheckDupeMask(h, Mask);
	if (e) {
		e->valid_until = ValidUntil;
		if (Reason) {
			if (e->reason)
				free(e->reason);
			e->reason = strdup(Reason);
		}
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
		newelem->reason = strdup(Reason);
		if (!newelem->reason)
			Log(LOG_EMERG,
			    "Can't allocate memory for new list reason text!");
	}
	else
		newelem->reason = NULL;
	newelem->valid_until = ValidUntil;
	newelem->onlyonce = OnlyOnce;
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
 * @param Pattern Source string to generate an IRC mask for.
 * @param mask    Buffer to store the mask.
 * @param len     Size of the buffer.
 */
GLOBAL void
Lists_MakeMask(const char *Pattern, char *mask, size_t len)
{
	char *excl, *at;

	assert(Pattern != NULL);

	excl = strchr(Pattern, '!');
	at = strchr(Pattern, '@');

	if (at && at < excl)
		excl = NULL;

	if (!at && !excl) {
		/* Neither "!" nor "@" found: use string as nickname */
		strlcpy(mask, Pattern, len - 5);
		strlcat(mask, "!*@*", len);
	} else if (!at && excl) {
		/* Domain part is missing */
		strlcpy(mask, Pattern, len - 3);
		strlcat(mask, "@*", len);
	} else if (at && !excl) {
		/* User name is missing */
		*at = '\0'; at++;
		strlcpy(mask, Pattern, len - 5);
		strlcat(mask, "!*@", len);
		strlcat(mask, at, len);
		at--; *at = '@';
	} else {
		/* All parts (nick, user and domain name) are given */
		strlcpy(mask, Pattern, len);
	}
} /* Lists_MakeMask */

/**
 * Check if a client is listed in a list.
 *
 * @param h List head.
 * @param Client Client to check.
 * @return true if client is listed, false if not.
 */
bool
Lists_Check(struct list_head *h, CLIENT *Client)
{
	return Lists_CheckReason(h, Client, NULL, 0);
}

/**
 * Check if a client is listed in a list and store the reason.
 *
 * @param h      List head.
 * @param Client Client to check.
 * @param reason Buffer to store the reason.
 * @param len    Size of the buffer if reason should be saved.
 * @return true if client is listed, false if not.
 */
bool
Lists_CheckReason(struct list_head *h, CLIENT *Client, char *reason, size_t len)
{
	struct list_elem *e, *last, *next;

	assert(h != NULL);

	e = h->first;
	last = NULL;

	while (e) {
		next = e->next;
		if (MatchCaseInsensitive(e->mask, Client_MaskCloaked(Client)) || MatchCaseInsensitive(e->mask, Client_Mask(Client))) {
			if (len && e->reason)
				strlcpy(reason, e->reason, len);
			if (e->onlyonce) {
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
		if (e->valid_until > 0 && e->valid_until < now) {
			/* Entry is expired, delete it */
			if (e->reason)
				Log(LOG_NOTICE|LOG_snotice,
				    "Deleted \"%s\" (\"%s\") from %s list (expired).",
				    e->mask, e->reason, ListName);
			else
				Log(LOG_NOTICE|LOG_snotice,
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

/**
 * Return the number of entries of a list.
 *
 * @param h List head.
 * @return Number of items.
 */
GLOBAL unsigned long
Lists_Count(struct list_head *h)
{
	struct list_elem *e;
	unsigned long count = 0;

	assert(h != NULL);

	e = h->first;
	while (e) {
		count++;
		e = e->next;
	}
	return count;
}

/* -eof- */
