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
 * test program for portab.h and friends ;-)
 */


#include "portab.h"

static char UNUSED id[] = "$Id: portabtest.c,v 1.10 2002/12/26 13:19:36 alex Exp $";

#include "imp.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exp.h"


LOCAL VOID Panic PARAMS (( CHAR *Reason, INT Code ));


GLOBAL int
main( VOID )
{
	/* validate datatypes */
	if( FALSE != 0 ) Panic( "FALSE", 1 );
	if( TRUE != 1 ) Panic( "TRUE", 1 );
	if( sizeof( INT8 ) != 1 ) Panic( "INT8", 1 );
	if( sizeof( UINT8 ) != 1 ) Panic( "UINT8", 1 );
	if( sizeof( INT16 ) != 2 ) Panic( "INT16", 1 );
	if( sizeof( UINT16 ) != 2 ) Panic( "UINT16", 1 );
	if( sizeof( INT32 ) != 4 ) Panic( "INT32", 1 );
	if( sizeof( UINT32 ) != 4 ) Panic( "UINT32", 1 );

	/* check functions */
	if( ! snprintf ) Panic( "snprintf", 2 );
	if( ! vsnprintf ) Panic( "vsnprintf", 2 );
	if( ! strlcpy ) Panic( "strlcpy", 2 );
	if( ! strlcat ) Panic( "strlcat", 2 );
	
	/* ok, no error */
	return 0;
} /* portab_check_types */


LOCAL VOID
Panic( CHAR *Reason, INT Code )
{
	/* Oops, something failed!? */
	fprintf( stderr, "Oops, test for %s failed!?", Reason );
	exit( Code );
} /* Panic */


/* -eof- */
