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
 * $Id: portabtest.c,v 1.8 2002/09/09 10:05:10 alex Exp $
 *
 * portabtest.c: Testprogramm fuer portab.h
 */


#include "portab.h"

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
