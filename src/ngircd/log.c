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
 * Logging functions
 */


#include "portab.h"

static char UNUSED id[] = "$Id: log.c,v 1.46 2004/05/10 23:57:46 alex Exp $";

#include "imp.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef SYSLOG
#include <syslog.h>
#endif

#include "ngircd.h"
#include "defines.h"
#include "conn.h"
#include "client.h"
#include "channel.h"
#include "irc-write.h"

#include "exp.h"
#include "log.h"


LOCAL CHAR Error_File[FNAME_LEN];
LOCAL CHAR Init_Txt[127];


LOCAL VOID Wall_ServerNotice PARAMS(( CHAR *Msg ));


GLOBAL VOID
Log_Init( VOID )
{
#ifdef SYSLOG
	/* Syslog initialisieren */
	openlog( PACKAGE_NAME, LOG_CONS|LOG_PID, LOG_LOCAL5 );
#endif

	/* Hello World! */
	Log( LOG_NOTICE, "%s started.", NGIRCd_Version( ));
	  
	/* Informationen uebern den "Operation Mode" */
	strcpy( Init_Txt, "" );
#ifdef DEBUG
	if( NGIRCd_Debug )
	{
		if( Init_Txt[0] ) strcat( Init_Txt, ", " );
		strcat( Init_Txt, "debug-mode" );
	}
#endif
	if( NGIRCd_NoDaemon )
	{
		if( Init_Txt[0] ) strcat( Init_Txt, ", " );
		strcat( Init_Txt, "no-daemon-mode" );
	}
	if( NGIRCd_Passive )
	{
		if( Init_Txt[0] ) strcat( Init_Txt, ", " );
		strcat( Init_Txt, "passive-mode" );
	}
#ifdef SNIFFER
	if( NGIRCd_Sniffer )
	{
		if( Init_Txt[0] ) strcat( Init_Txt, ", " );
		strcat( Init_Txt, "network sniffer" );
	}
#endif
	if( Init_Txt[0] ) Log( LOG_INFO, "Activating: %s.", Init_Txt );

	Error_File[0] = '\0';
} /* Log_Init */


GLOBAL VOID
Log_InitErrorfile( VOID )
{
	/* "Error-Log" initialisieren: stderr in Datei umlenken. Dort
	 * landen z.B. alle Ausgaben von assert()-Aufrufen. */

	/* Dateiname zusammen bauen */
	sprintf( Error_File, "%s/%s-%ld.err", ERROR_DIR, PACKAGE_NAME, (LONG)getpid( ));

	/* stderr umlenken */
	fflush( stderr );
	if( ! freopen( Error_File, "w", stderr ))
	{
		Log( LOG_ERR, "Can't reopen stderr (\"%s\"): %s", Error_File, strerror( errno ));
		return;
	}

	/* Einige Infos in das Error-File schreiben */
	fputs( ctime( &NGIRCd_Start ), stderr );
	fprintf( stderr, "%s started.\n", NGIRCd_Version( ));
	fprintf( stderr, "Activating: %s\n\n", Init_Txt[0] ? Init_Txt : "-" );
	fflush( stderr );

	Log( LOG_DEBUG, "Redirected stderr to \"%s\".", Error_File );
} /* Log_InitErrfile */


GLOBAL VOID
Log_Exit( VOID )
{
	/* Good Bye! */
	if( NGIRCd_SignalRestart ) Log( LOG_NOTICE, "%s done (restarting).", PACKAGE_NAME );
	else Log( LOG_NOTICE, "%s done.", PACKAGE_NAME );

	if( Error_File[0] )
	{
		/* Error-File (stderr) loeschen */
		if( unlink( Error_File ) != 0 ) Log( LOG_ERR, "Can't delete \"%s\": %s", Error_File, strerror( errno ));
	}

#ifdef SYSLOG
	/* syslog abmelden */
	closelog( );
#endif
} /* Log_Exit */


#ifdef PROTOTYPES
GLOBAL VOID
Log( INT Level, CONST CHAR *Format, ... )
#else
GLOBAL VOID
Log( Level, Format, va_alist )
INT Level;
CONST CHAR *Format;
va_dcl
#endif
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
#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( msg, MAX_LOG_MSG_LEN, Format, ap );
	va_end( ap );

	if( NGIRCd_NoDaemon )
	{
		/* auf Konsole ausgeben */
		fprintf( stdout, "[%d:%d] %s\n", getpid( ), Level, msg );
		fflush( stdout );
	}
#ifdef SYSLOG
	else
	{
		/* Syslog */
		syslog( Level, "%s", msg );
	}
#endif

	if( Level <= LOG_CRIT )
	{
		/* Kritische Meldungen in Error-File (stderr) */
		fprintf( stderr, "%s\n", msg );
		fflush( stderr );
	}

	if( snotice )
	{
		/* NOTICE an lokale User mit "s"-Mode */
		Wall_ServerNotice( msg );
	}
} /* Log */


GLOBAL VOID
Log_Init_Resolver( VOID )
{
#ifdef SYSLOG
	openlog( PACKAGE_NAME, LOG_CONS|LOG_PID, LOG_LOCAL5 );
#endif
	Log_Resolver( LOG_DEBUG, "Resolver sub-process starting, PID %d.", getpid( ));
} /* Log_Init_Resolver */


GLOBAL VOID
Log_Exit_Resolver( VOID )
{
	Log_Resolver( LOG_DEBUG, "Resolver sub-process %d done.", getpid( ));
#ifdef SYSLOG
	closelog( );
#endif
} /* Log_Exit_Resolver */


#ifdef PROTOTYPES
GLOBAL VOID
Log_Resolver( CONST INT Level, CONST CHAR *Format, ... )
#else
GLOBAL VOID
Log_Resolver( Level, Format, va_alist )
CONST INT Level;
CONST CHAR *Format;
va_dcl
#endif
{
	/* Eintrag des Resolver in Logfile(s) schreiben */

	CHAR msg[MAX_LOG_MSG_LEN];
	va_list ap;

	assert( Format != NULL );

#ifdef DEBUG
	if(( Level == LOG_DEBUG ) && ( ! NGIRCd_Debug )) return;
#else
	if( Level == LOG_DEBUG ) return;
#endif

	/* String mit variablen Argumenten zusammenbauen ... */
#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( msg, MAX_LOG_MSG_LEN, Format, ap );
	va_end( ap );

	/* Output */
	if( NGIRCd_NoDaemon )
	{
		/* Output to console */
		fprintf( stdout, "[%d:%d] %s\n", getpid( ), Level, msg );
		fflush( stdout );
	}
#ifdef SYSLOG
	else syslog( Level, msg );
#endif
} /* Log_Resolver */


LOCAL VOID
Wall_ServerNotice( CHAR *Msg )
{
	/* Server-Notice an entsprechende User verschicken */

	CLIENT *c;

	assert( Msg != NULL );

	c = Client_First( );
	while( c )
	{
		if(( Client_Conn( c ) > NONE ) && ( Client_HasMode( c, 's' ))) IRC_WriteStrClient( c, "NOTICE %s :%s%s", Client_ThisServer( ), NOTICE_TXTPREFIX, Msg );
		c = Client_Next( c );
	}
} /* Wall_ServerNotice */


/* -eof- */
