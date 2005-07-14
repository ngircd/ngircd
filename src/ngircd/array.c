/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * functions to dynamically allocate arrays.
 * Copyright (c) 2005 Florian Westphal (westphal@foo.fh-furtwangen.de)
 *
 */

#include "array.h"

static char UNUSED id[] = "$Id: array.c,v 1.4 2005/07/14 09:14:12 alex Exp $";

#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include "log.h"

#define array_UNUSABLE(x)	( ! x->mem  || (0 == x->allocated) )

#define ALIGN_32U(x)		((x | 0x1fU) +1)
#define ALIGN_1024U(x)		((x | 0x3ffU) +1)
#define ALIGN_4096U(x)		((x | 0xfffU) +1)


static bool
safemult_uint(unsigned int a, unsigned int b, unsigned int *res)
{
	unsigned int tmp;

	if (!a || !b) {
		*res = 0;
		return true;
	}

	tmp = a * b;

	if (tmp / b != a)
		return false;

	*res = tmp;
	return true;
}


/* if realloc() fails, array_alloc return NULL. otherwise return pointer to elem pos in array */
void *
array_alloc(array * a, unsigned int size, unsigned int pos)
{
	unsigned int alloc, pos_plus1 = pos + 1;
	unsigned int aligned = 0;
	char *tmp;

	assert(size > 0);

	if (pos_plus1 < pos)
		return NULL;

	if (!safemult_uint(size, pos_plus1, &alloc))
		return NULL;

	if (a->allocated < alloc) {
		if (alloc < 128) {
			aligned = ALIGN_32U(alloc);
		} else {
			if (alloc < 4096) {
				aligned = ALIGN_1024U(alloc);
			} else {
				aligned = ALIGN_4096U(alloc);
			}
		}
#ifdef DEBUG
		Log(LOG_DEBUG, "array_alloc(): rounded %u to %u bytes.", alloc, aligned);
#endif

		assert(aligned >= alloc);

		if (aligned < alloc)	/* rounding overflow */
			return NULL;

		alloc = aligned;
#ifdef DEBUG
		Log(LOG_DEBUG, "array_alloc(): changing size from %u to %u bytes.",
		    a->allocated, aligned);
#endif

		tmp = realloc(a->mem, alloc);
		if (!tmp)
			return NULL;

		a->mem = tmp;
		a->allocated = alloc;

		assert(a->allocated > a->used);

		memset(a->mem + a->used, 0, a->allocated - a->used);

		a->used = alloc;
	}
	return a->mem + (pos * size);
}


/*return number of initialized ELEMS in a. */
unsigned int
array_length(const array * const a, unsigned int membersize)
{
	assert(a != NULL);
	assert(membersize > 0);

	if (array_UNUSABLE(a))
		return 0;

	return membersize ? a->used / membersize : 0;
}


/* copy array src to array dest */
bool
array_copy(array * dest, const array * const src)
{
	if (array_UNUSABLE(src))
		return false;

	return array_copyb(dest, src->mem, src->used);
}


/* return false if we could not append src (realloc failure, invalid src/dest array) */
bool
array_copyb(array * dest, const char *src, unsigned int len)
{
	assert(dest != NULL);
	assert(src != NULL );

	if (!len || !src)
		return true;

	if (!array_alloc(dest, 1, len))
		return false;

	dest->used = len;
	memcpy(dest->mem, src, len);
#ifdef DEBUG
	Log(LOG_DEBUG,
	    "array_copyb(): copied %u bytes to array (%u bytes allocated).",
	    len, dest->allocated);
#endif
	return true;
}


/* copy string to dest */
bool
array_copys(array * dest, const char *src)
{
	return array_copyb(dest, src, strlen(src));
}


/* append len bytes from src to the array dest.
return false if we could not append all bytes (realloc failure, invalid src/dest array) */
bool
array_catb(array * dest, const char *src, unsigned int len)
{
	unsigned int tmp;
	unsigned int used;
	char *ptr;

	assert(dest != NULL);
	assert(src != NULL);

	if (!len)
		return true;

	if (!src || !dest)
		return false;

	used = dest->used;
	tmp = used + len;

	if (tmp < used || tmp < len)	/* integer overflow */
		return false;

	if (!array_alloc(dest, 1, tmp))
		return false;

	ptr = dest->mem;

	assert(ptr != NULL);

#ifdef DEBUG
	Log(LOG_DEBUG,
	    "array_catb(): appending %u bytes to array (now %u bytes in array).",
	    len, tmp);
#endif
	memcpy(ptr + used, src, len);
	dest->used = tmp;
	return true;
}


/* append string to dest */
bool
array_cats(array * dest, const char *src)
{
	return array_catb(dest, src, strlen(src));
}


/* append trailing NUL byte to array */
bool
array_cat0(array * a)
{
	return array_catb(a, "", 1);
}


/* add contents of array src to array dest. */
bool
array_cat(array * dest, const array * const src)
{
	if (array_UNUSABLE(src))
		return false;

	return array_catb(dest, src->mem, src->used);
}


/* return pointer to the element at pos.
   return NULL if the array is unallocated, or if pos is larger than
   the number of elements stored int the array. */
void *
array_get(array * a, unsigned int membersize, unsigned int pos)
{
	unsigned int totalsize;

	assert(membersize > 0);
	assert(a != NULL);

	if (array_UNUSABLE(a))
		return NULL;

	if (!safemult_uint(pos, membersize, &totalsize))
		return NULL;

	if (a->allocated < totalsize)
		return NULL;

	return a->mem + pos * membersize;
}


void
array_free(array * a)
{
	assert(a != NULL);
#ifdef DEBUG
	Log(LOG_DEBUG,
	    "array_free(): %u bytes free'd (%u bytes still used at time of free()).",
	    a->allocated, a->used);
#endif
	free(a->mem);
	a->mem = NULL;
	a->allocated = 0;
	a->used = 0;
}


void
array_free_wipe(array * a)
{
	if (!array_UNUSABLE(a))
		memset(a->mem, 0, a->allocated);

	array_free(a);
}


void *
array_start(const array * const a)
{
	assert(a != NULL);
	return a->mem;
}


void
array_trunc(array * a)
{
	assert(a != NULL);
	a->used = 0;
}


void
array_truncate(array * a, unsigned int membersize, unsigned int len)
{
	unsigned int newlen;
	assert(a != NULL);
	if (!safemult_uint(membersize, len, &newlen))
		return;

	if (newlen <= a->allocated)
		a->used = newlen;
}


/* move elements starting at pos to beginning of array */
void
array_moveleft(array * a, unsigned int membersize, unsigned int pos)
{
	unsigned int bytepos;

	assert(a != NULL);
	assert(membersize > 0);

	if (!pos)
		return;

	if (!safemult_uint(membersize, pos, &bytepos)) {
		a->used = 0;
		return;
	}

	if (!bytepos)
		return;	/* nothing to do */

#ifdef DEBUG
	Log(LOG_DEBUG,
	    "array_moveleft(): %u bytes used in array, starting at position %u.",
	    a->used, bytepos);
#endif
	if (a->used <= bytepos) {
		a->used = 0;
		return;
	}

	a->used -= bytepos;
	memmove(a->mem, a->mem + bytepos, a->used);
}

/* -eof- */
