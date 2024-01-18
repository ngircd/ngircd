/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2014 Alexander Barton (alex@barton.de) and Contributors.
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
 * Hash calculation
 */

#include <assert.h>
#include <string.h>

#include "defines.h"
#include "tool.h"

#include "hash.h"

static UINT32 jenkins_hash PARAMS((UINT8 *k, UINT32 length, UINT32 initval));

/**
 * Calculate hash value for a given string.
 *
 * @param String Input string
 * @return 32 bit hash value
 */
GLOBAL UINT32
Hash( const char *String )
{
	char buffer[COMMAND_LEN];

	strlcpy(buffer, String, sizeof(buffer));
	return jenkins_hash((UINT8 *)ngt_LowerStr(buffer),
			    (UINT32)strlen(buffer), 42);
} /* Hash */

/*
 * This hash function originates from lookup3.c of Bob Jenkins
 * (URL: <http://burtleburtle.net/bob/c/lookup3.c>):
 * --------------------------------------------------------------------
 * lookup3.c, by Bob Jenkins, May 2006, Public Domain.
 * These are functions for producing 32-bit hashes for hash table lookup.
 * hashword(), hashlittle(), hashlittle2(), hashbig(), mix(), and final()
 * are externally useful functions.  Routines to test the hash are included
 * if SELF_TEST is defined.  You can use this free for any purpose.  It's in
 * the public domain.  It has no warranty.
 * --------------------------------------------------------------------
 * Not all of his functions are used here.
 */

#define hashsize(n) ((UINT32)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
	a -= c;  a ^= rot(c, 4);  c += b; \
	b -= a;  b ^= rot(a, 6);  a += c; \
	c -= b;  c ^= rot(b, 8);  b += a; \
	a -= c;  a ^= rot(c,16);  c += b; \
	b -= a;  b ^= rot(a,19);  a += c; \
	c -= b;  c ^= rot(b, 4);  b += a; \
} /* mix */

#define final(a,b,c) \
{ \
	c ^= b; c -= rot(b,14); \
	a ^= c; a -= rot(c,11); \
	b ^= a; b -= rot(a,25); \
	c ^= b; c -= rot(b,16); \
	a ^= c; a -= rot(c,4);  \
	b ^= a; b -= rot(a,14); \
	c ^= b; c -= rot(b,24); \
}

static UINT32
jenkins_hash(UINT8 *k, UINT32 length, UINT32 initval)
{
	/* k: the key
	 * length: length of the key
	 * initval: the previous hash, or an arbitrary value
	 */
	UINT32 a,b,c;

	/* Set up the internal state */
	a = b = c = 0xdeadbeef + length + initval;

	/* handle most of the key */
	while (length > 12) {
		a += (k[0] +((UINT32)k[1]<<8) +((UINT32)k[2]<<16) +((UINT32)k[3]<<24));
		b += (k[4] +((UINT32)k[5]<<8) +((UINT32)k[6]<<16) +((UINT32)k[7]<<24));
		c += (k[8] +((UINT32)k[9]<<8) +((UINT32)k[10]<<16)+((UINT32)k[11]<<24));
		mix(a,b,c);
		length -= 12;
		k += 12;
	}

	/*-------------------------------- last block: affect all 32 bits of (c) */
	switch(length)                   /* all the case statements fall through */

	{
		case 12: c+=((UINT32)k[11])<<24;
		/* fall through */
		case 11: c+=((UINT32)k[10]<<16);
		/* fall through */
		case 10: c+=((UINT32)k[9]<<8);
		/* fall through */
		case 9 : c+=k[8];
		/* fall through */
		case 8 : b+=((UINT32)k[7]<<24);
		/* fall through */
		case 7 : b+=((UINT32)k[6]<<16);
		/* fall through */
		case 6 : b+=((UINT32)k[5]<<8);
		/* fall through */
		case 5 : b+=k[4];
		/* fall through */
		case 4 : a+=((UINT32)k[3]<<24);
		/* fall through */
		case 3 : a+=((UINT32)k[2]<<16);
		/* fall through */
		case 2 : a+=((UINT32)k[1]<<8);
		/* fall through */
		case 1 : a+=k[0];
			 break;
		case 0 : return c;
	}
	final(a,b,c);
	return c;
} /* jenkins_hash */

/* -eof- */
