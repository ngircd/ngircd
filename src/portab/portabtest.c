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
 * $Id: portabtest.c,v 1.7 2002/09/09 10:00:39 alex Exp $
 *
 * portabtest.c: Testprogramm fuer portab.h
 */


#include "portab.h"

#include "imp.h"
#include <stdio.h>

#include "exp.h"


LOCAL BOOLEAN portab_check_types PARAMS(( VOID ));


GLOBAL int
main( VOID )
{
	INT ret = 0;

	printf( "      - system type: %s/%s/%s\n", TARGET_CPU, TARGET_VENDOR, TARGET_OS );

	printf( "      - datatypes: ");
	if( ! portab_check_types( ))
	{
		puts( "FAILED!" );
		ret = 1;
	}
	else puts( "ok." );

	return ret;
} /* main */


LOCAL BOOLEAN
portab_check_types( VOID )
{
	if( FALSE != 0 ) return 0;
	if( TRUE != 1 ) return 0;
	if( sizeof( INT8 ) != 1 ) return 0;
	if( sizeof( UINT8 ) != 1 ) return 0;
	if( sizeof( INT16 ) != 2 ) return 0;
	if( sizeof( UINT16 ) != 2 ) return 0;
	if( sizeof( INT32 ) != 4 ) return 0;
	if( sizeof( UINT32 ) != 4 ) return 0;
	return 1;
} /* portab_check_types */


/* -eof- */
