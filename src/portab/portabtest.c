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

static char UNUSED id[] = "$Id: portabtest.c,v 1.9 2002/12/12 11:38:46 alex Exp $";

#include "imp.h"
#include <stdio.h>

#include "exp.h"


GLOBAL int
main( VOID )
{
	/* Datentypen pruefen */
	if( FALSE != 0 ) return 1;
	if( TRUE != 1 ) return 1;
	if( sizeof( INT8 ) != 1 ) return 1;
	if( sizeof( UINT8 ) != 1 ) return 1;
	if( sizeof( INT16 ) != 2 ) return 1;
	if( sizeof( UINT16 ) != 2 ) return 1;
	if( sizeof( INT32 ) != 4 ) return 1;
	if( sizeof( UINT32 ) != 4 ) return 1;
	
	/* kein Fehler */
	return 0;
} /* portab_check_types */


/* -eof- */
