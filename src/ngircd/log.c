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
 * $Id: log.c,v 1.28 2002/03/29 22:55:42 alex Exp $
 *
 * log.c: Logging-Funktionen
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef USE_SYSLOG
#include <syslog.h>
#endif

#include "ngircd.h"
#include "client.h"
#include "defines.h"
#include "irc-write.h"

#include "exp.h"
#include "log.h"


LOCAL CHAR Error_File[FNAME_LEN];


LOCAL VOID Wall_ServerNotice( CHAR *Msg );


GLOBAL VOID Log_Init( VOID )
{
	CHAR txt[127];
	time_t t;

#ifdef USE_SYSLOG
	/* Syslog initialisieren */
	openlog( PACKAGE, LOG_CONS|LOG_PID, LOG_LOCAL5 );
#endif

	/* Hello World! */
	Log( LOG_NOTICE, "%s started.", NGIRCd_Version( ));
	  
	/* Informationen uebern den "Operation Mode" */
	strcpy( txt, "" );
#ifdef DEBUG
	if( NGIRCd_Debug )
	{
		if( txt[0] ) strcat( txt, ", " );
		strcat( txt, "debug-mode" );
	}
#endif
	if( NGIRCd_NoDaemon )
	{
		if( txt[0] ) strcat( txt, ", " );
		strcat( txt, "no-daemon-mode" );
	}
	if( NGIRCd_Passive )
	{
		if( txt[0] ) strcat( txt, ", " );
		strcat( txt, "passive-mode" );
	}
#ifdef SNIFFER
	if( NGIRCd_Sniffer )
	{
		if( txt[0] ) strcat( txt, ", " );
		strcat( txt, "network sniffer" );
	}
#endif
	if( txt[0] ) Log( LOG_INFO, "Activating: %s.", txt );

	/* "Error-Log" initialisieren: stderr in Datei umlenken. Dort
	 * landen z.B. alle Ausgaben von assert()-Aufrufen. */
	fflush( stderr );
	sprintf( Error_File, ERROR_DIR"/"PACKAGE"-%ld.err", (INT32)getpid( ));
	if( ! freopen( Error_File, "w", stderr ))
	{
		Log( LOG_ERR, "Can't reopen stderr (\"%s\"): %s", Error_File, strerror( errno ));
		return;
	}

	fputs( ctime( &t ), stderr );
	fprintf( stderr, "%s started.\n", NGIRCd_Version( ));
	fprintf( stderr, "Activating: %s\n\n", txt[0] ? txt : "-" );
	fflush( stderr );
} /* Log_Init */


GLOBAL VOID Log_Exit( VOID )
{
	/* Good Bye! */
	Log( LOG_NOTICE, PACKAGE" done.");

	/* Error-File (stderr) loeschen */
	if( unlink( Error_File ) != 0 ) Log( LOG_ERR, "Can't delete \"%s\": %s", Error_File, strerror( errno ));

#ifdef USE_SYSLOG
	/* syslog abmelden */
	closelog( );
#endif
} /* Log_Exit */


GLOBAL VOID Log( INT Level, CONST CHAR *Format, ... )
{
	/* Eintrag in Logfile(s) schreiben */

	CHAR msg[MAX_LOG_MSG_LEN];
	BOOLEAN snotice;
	va_list ap;

	assert( Format != NULL );

	if( Level & LOG_snotice )
	{
		/* Notice an User mit "s" Mode */
		snotice = TRUE;
		Level &= ~LOG_snotice;
	}
	else snotice = FALSE;

#ifdef DEBUG
	if(( Level == LOG_DEBUG ) && ( ! NGIRCd_Debug )) return;
#else
	if( Level == LOG_DEBUG ) return;
#endif

	/* String mit variablen Argumenten zusammenbauen ... */
	va_start( ap, Format );
	vsnprintf( msg, MAX_LOG_MSG_LEN, Format, ap );
	va_end( ap );

	/* Konsole */
	if( NGIRCd_NoDaemon ) printf( "[%d] %s\n", Level, msg );

#ifdef USE_SYSLOG
	/* Syslog */
	syslog( Level, msg );
#endif

	/* lokale User mit "s"-Mode */
	if( snotice ) Wall_ServerNotice( msg );
} /* Log */


GLOBAL VOID Log_Init_Resolver( VOID )
{
#ifdef USE_SYSLOG
	openlog( PACKAGE, LOG_CONS|LOG_PID, LOG_LOCAL5 );
#endif
} /* Log_Init_Resolver */


GLOBAL VOID Log_Exit_Resolver( VOID )
{
#ifdef USE_SYSLOG
	closelog( );
#endif
} /* Log_Exit_Resolver */


GLOBAL VOID Log_Resolver( CONST INT Level, CONST CHAR *Format, ... )
{
	/* Eintrag des Resolver in Logfile(s) schreiben */

#ifndef USE_SYSLOG
	return;
#else

	CHAR msg[MAX_LOG_MSG_LEN];
	va_list ap;

	assert( Format != NULL );

#ifdef DEBUG
	if(( Level == LOG_DEBUG ) && ( ! NGIRCd_Debug )) return;
#else
	if( Level == LOG_DEBUG ) return;
#endif

	/* String mit variablen Argumenten zusammenbauen ... */
	va_start( ap, Format );
	vsnprintf( msg, MAX_LOG_MSG_LEN, Format, ap );
	va_end( ap );

	/* ... und ausgeben */
	syslog( Level, msg );

#endif
} /* Log_Resolver */


LOCAL VOID Wall_ServerNotice( CHAR *Msg )
{
	/* Server-Notice an entsprechende User verschicken */

	CLIENT *c;

	assert( Msg != NULL );

	c = Client_First( );
	while( c )
	{
		if(( Client_Conn( c ) > NONE ) && ( Client_HasMode( c, 's' ))) IRC_WriteStrClient( c, "NOTICE %s :%s", Client_ThisServer( ), Msg );
		c = Client_Next( c );
	}
} /* Wall_ServerNotice */


/* -eof- */
