/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: hash.c,v 1.2 2002/03/14 15:49:36 alex Exp $
 *
 * hash.c: Hash-Werte berechnen
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>

#include "exp.h"
#include "hash.h"


LOCAL UINT32 jenkins_hash( register UINT8 *k, register UINT32 length, register UINT32 initval);


GLOBAL UINT32 Hash( CHAR *String )
{
	/* Hash-Wert ueber String berechnen */
	return jenkins_hash( String, strlen( String ), 42 );
} /* Hash */


/*
 * Die hier verwendete Hash-Funktion stammt aus lookup2.c von Bob Jenkins
 * (URL: <http://burtleburtle.net/bob/c/lookup2.c>). Aus dem Header:
 * --------------------------------------------------------------------
 * lookup2.c, by Bob Jenkins, December 1996, Public Domain.
 * hash(), hash2(), hash3, and mix() are externally useful functions.
 * Routines to test the hash are included if SELF_TEST is defined.
 * You can use this free for any purpose.  It has no warranty.
 * --------------------------------------------------------------------
 * nicht alle seiner Funktionen werden hier genutzt.
 */


#define hashsize(n) ((UINT32)1<<(n))
#define hashmask(n) (hashsize(n)-1)

#define mix(a,b,c) \
{ \
	a -= b; a -= c; a ^= (c>>13); \
	b -= c; b -= a; b ^= (a<<8);  \
	c -= a; c -= b; c ^= (b>>13); \
	a -= b; a -= c; a ^= (c>>12); \
	b -= c; b -= a; b ^= (a<<16); \
	c -= a; c -= b; c ^= (b>>5);  \
	a -= b; a -= c; a ^= (c>>3);  \
	b -= c; b -= a; b ^= (a<<10); \
	c -= a; c -= b; c ^= (b>>15); \
} /* mix */


LOCAL UINT32 jenkins_hash( register UINT8 *k, register UINT32 length, register UINT32 initval)
{
	/* k: the key
	 * length: length of the key
	 * initval: the previous hash, or an arbitrary value
	 */

	register UINT32 a,b,c,len;

	/* Set up the internal state */
	len = length;
	a = b = 0x9e3779b9;	/* the golden ratio; an arbitrary value */
	c = initval;		/* the previous hash value */

	/* handle most of the key */
	while (len >= 12)
	{
		a += (k[0] +((UINT32)k[1]<<8) +((UINT32)k[2]<<16) +((UINT32)k[3]<<24));
		b += (k[4] +((UINT32)k[5]<<8) +((UINT32)k[6]<<16) +((UINT32)k[7]<<24));
		c += (k[8] +((UINT32)k[9]<<8) +((UINT32)k[10]<<16)+((UINT32)k[11]<<24));
		mix(a,b,c);
		k += 12; len -= 12;
	}

	/* handle the last 11 bytes */
	c += length;
	switch(len)		/* all the case statements fall through */
	{
		case 11: c+=((UINT32)k[10]<<24);
		case 10: c+=((UINT32)k[9]<<16);
		case 9 : c+=((UINT32)k[8]<<8);
		/* the first byte of c is reserved for the length */
		case 8 : b+=((UINT32)k[7]<<24);
		case 7 : b+=((UINT32)k[6]<<16);
		case 6 : b+=((UINT32)k[5]<<8);
		case 5 : b+=k[4];
		case 4 : a+=((UINT32)k[3]<<24);
		case 3 : a+=((UINT32)k[2]<<16);
		case 2 : a+=((UINT32)k[1]<<8);
		case 1 : a+=k[0];
		/* case 0: nothing left to add */
	}
	mix(a,b,c);

	/* report the result */
	return c;
} /* jenkins_hash */


/* -eof- */
