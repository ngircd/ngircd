/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an comBase beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: log.c,v 1.4 2001/12/13 02:04:16 alex Exp $
 *
 * log.c: Logging-Funktionen
 *
 * $Log: log.c,v $
 * Revision 1.4  2001/12/13 02:04:16  alex
 * - boesen "Speicherschiesser" in Log() gefixt.
 *
 * Revision 1.3  2001/12/12 23:31:24  alex
 * - Zum Loggen wird nun auch syslog verwendet.
 *
 * Revision 1.2  2001/12/12 17:19:12  alex
 * - in Log-Meldungen wird nun auch der Level der Meldung ausgegeben.
 *
 * Revision 1.1.1.1  2001/12/11 21:53:04  alex
 * - Imported sources to CVS.
 */


#define MAX_LOG_MSG_LEN 256


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

#include <exp.h>
#include "log.h"


GLOBAL VOID Log_Init( VOID )
{
	openlog( PACKAGE, LOG_CONS|LOG_PID, LOG_DAEMON );
	Log( LOG_INFO, PACKAGE" version "VERSION" started.");
} /* Log_Init */


GLOBAL VOID Log_Exit( VOID )
{
	Log( LOG_INFO, PACKAGE" done.");
	closelog( );
} /* Log_Exit */


GLOBAL VOID Log( CONST INT Level, CONST CHAR *Format, ... )
{
	/* Eintrag in Logfile(s) schreiben */

	CHAR msg[MAX_LOG_MSG_LEN];
	va_list ap;

	assert( Format != NULL );

	/* String mit variablen Argumenten zusammenbauen ... */
	va_start( ap, Format );
	vsnprintf( msg, MAX_LOG_MSG_LEN - 1, Format, ap );
	msg[MAX_LOG_MSG_LEN - 1] = '\0';

	/* ... und ausgeben */
	printf( "[%d] %s\n", Level, msg );
	syslog( Level, msg );

	va_end( ap );
} /* Log */


/* -eof- */
