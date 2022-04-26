/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * libarray - dynamically allocate arrays.
 * Copyright (c) 2005 Florian Westphal (westphal@foo.fh-furtwangen.de)
 */

/**
 * @file
 * Functions to dynamically allocate arrays.
 */

/* Additionan debug messages related to array handling: 0=off / 1=on */
#define DEBUG_ARRAY 0

#include "array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if DEBUG_ARRAY
# include "log.h"
#endif

#define array_UNUSABLE(x)	( !(x)->mem )


static bool
safemult_sizet(size_t a, size_t b, size_t *res)
{
	size_t tmp = a * b;

	if (b && (tmp / b != a))
		return false;

	*res = tmp;
	return true;
}


void
array_init(array *a)
{
	assert(a != NULL);
	a->mem = NULL;
	a->allocated = 0;
	a->used = 0;
}


/* if realloc() fails, array_alloc return NULL. otherwise return pointer to elem pos in array */
void *
array_alloc(array * a, size_t size, size_t pos)
{
	size_t alloc, pos_plus1 = pos + 1;
	char *tmp;

	assert(size > 0);

	if (pos_plus1 == 0 || !safemult_sizet(size, pos_plus1, &alloc))
		return NULL;

	if (a->allocated < alloc) {
#if DEBUG_ARRAY
		LogDebug("array_alloc(): changing size from %u to %u bytes.",
		    a->allocated, alloc);
#endif
		tmp = realloc(a->mem, alloc);
		if (!tmp)
			return NULL;

		a->mem = tmp;
		a->allocated = alloc;
		memset(a->mem + a->used, 0, a->allocated - a->used);
		a->used = alloc;
	}

	assert(a->allocated >= a->used);

	return a->mem + (pos * size);
}


/*return number of initialized ELEMS in a. */
size_t
array_length(const array * const a, size_t membersize)
{
	assert(a != NULL);
	assert(membersize > 0);

	if (array_UNUSABLE(a))
		return 0;

	assert(a->allocated);
	return membersize ? a->used / membersize : 0;
}


/* copy array src to array dest */
bool
array_copy(array * dest, const array * const src)
{
	if (array_UNUSABLE(src))
		return false;

	assert(src->allocated);
	return array_copyb(dest, src->mem, src->used);
}


/* return false on failure (realloc failure, invalid src/dest array) */
bool
array_copyb(array * dest, const char *src, size_t len)
{
	assert(dest != NULL);
	assert(src != NULL );

	if (!src || !dest)
		return false;

	array_trunc(dest);
	return array_catb(dest, src, len);
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
array_catb(array * dest, const char *src, size_t len)
{
	size_t tmp;
	size_t used;
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

#if DEBUG_ARRAY
	LogDebug(
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


/* append trailing NUL byte to array, but do not count it. */
bool
array_cat0_temporary(array * a)
{
	char *endpos = array_alloc(a, 1, array_bytes(a));
	if (!endpos)
		return false;

	*endpos = '\0';
	return true;
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
array_get(array * a, size_t membersize, size_t pos)
{
	size_t totalsize;
	size_t posplus1 = pos + 1;

	assert(membersize > 0);
	assert(a != NULL);

	if (!posplus1 || array_UNUSABLE(a))
		return NULL;

	if (!safemult_sizet(posplus1, membersize, &totalsize))
		return NULL;

	if (a->allocated < totalsize)
		return NULL;

	totalsize = pos * membersize;
	return a->mem + totalsize;
}


void
array_free(array * a)
{
	assert(a != NULL);
#if DEBUG_ARRAY
	LogDebug(
	    "array_free(): %u bytes free'd (%u bytes still used at time of free()).",
	    a->allocated, a->used);
#endif
	free(a->mem);
	a->mem = NULL;
	a->allocated = 0;
	a->used = 0;
}

void
array_free_wipe(array *a)
{
	size_t bytes = a->allocated;
	if (bytes)
		memset(a->mem, 0, bytes);
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
array_truncate(array * a, size_t membersize, size_t len)
{
	size_t newlen;
	assert(a != NULL);
	if (!safemult_sizet(membersize, len, &newlen))
		return;

	if (newlen <= a->allocated)
		a->used = newlen;
}


/* move elements starting at pos to beginning of array */
void
array_moveleft(array * a, size_t membersize, size_t pos)
{
	size_t bytepos;

	assert(a != NULL);
	assert(membersize > 0);

	if (!safemult_sizet(membersize, pos, &bytepos)) {
		a->used = 0;
		return;
	}

	if (!bytepos)
		return;	/* nothing to do */

#if DEBUG_ARRAY
	LogDebug(
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
