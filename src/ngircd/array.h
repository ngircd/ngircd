/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * libarray - dynamically allocate arrays.
 * Copyright (c) 2005 Florian Westphal (westphal@foo.fh-furtwangen.de)
 *
 * $Id: array.h,v 1.1 2005/07/07 18:38:14 fw Exp $
 */

#ifndef array_h_included
#define array_h_included

#include "portab.h"

typedef struct {
	char * mem;
	unsigned int allocated;
	unsigned int used;
} array;

/* allocated: mem != NULL, used >= 0 && used <= allocated, allocated > 0
   unallocated: mem == NULL, allocated == 0, used == 0 */


#define array_unallocated(x)	(array_bytes(x)==0)
#define INIT_ARRAY		{ NULL, 0, 0 }

/* allocates space for at least nmemb+1 elements of size bytes each.
 return pointer to elem at pos, or NULL if realloc() fails */
extern void * array_alloc PARAMS((array *a, unsigned int size, unsigned int pos));

/* returns the number of initialized BYTES in a. */
#define array_bytes(array)	( (array)->used )

/* returns the number of initialized ELEMS in a. */
extern unsigned int array_length PARAMS((const array* const a, unsigned int elemsize));

/* _copy functions: copy src to dest.
return true if OK, else false. (e.g. realloc failure, invalid src/dest array, ..)
In that case dest is left unchanged. */

/* copy array src to dest */
extern bool array_copy PARAMS((array* dest, const array* const src));

/* copy len bytes from src to array dest. */
extern bool array_copyb PARAMS((array* dest, const char* src, unsigned int len));

/* copy string to dest */
extern bool array_copys PARAMS((array* dest, const char* src));

/* _cat functions: append src to dest.
return true if OK, else false. (e.g. realloc failure, invalid src/dest array, ..)
In that case dest is left unchanged. */

/* append len bytes from src to array dest. */
extern bool array_catb PARAMS((array* dest, const char* src, unsigned int len));

/* append string to dest */
extern bool array_cats PARAMS((array* dest, const char* src));

/* append NUL byte to dest */
extern bool array_cat0 PARAMS((array* dest));

/* append contents of array src to array dest. */
extern bool array_cat PARAMS((array* dest, const array* const src));

/* return pointer to element at pos.
 return NULL if the array is unallocated or if pos is larger than the number
 of elements stored int the array. */
extern void* array_get PARAMS((array* a, unsigned int membersize, unsigned int pos));

/* free the contents of this array. */
extern void array_free PARAMS((array* a));

/* overwrite array with zeroes before free */
extern void array_free_wipe PARAMS((array* a));

/* return pointer to first element in this array */
extern void* array_start PARAMS((const array* const a));

/* reset this array (the memory is not free'd */
extern void array_trunc PARAMS((array* a));

/* set number of used elements in this array to len */
extern void array_truncate PARAMS((array* a, unsigned int membersize, unsigned int len));

/* move elements starting at pos to beginning of array */
extern void array_moveleft PARAMS((array* a, unsigned int membersize, unsigned int pos));

#endif
/* -eof- */
