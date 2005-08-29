/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2005 Alexander Barton (alex@barton.de)
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

static char UNUSED id[] = "$Id: log.c,v 1.57.2.1 2005/08/29 11:19:48 alex Exp $";

#include "imp.h"
#include <assert.h>
#include <errno.h>
#ifdef PROTOTYPES
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif
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


LOCAL char Init_Txt[127];
LOCAL bool Is_Daemon;

#ifdef DEBUG
LOCAL char Error_File[FNAME_LEN];
#endif


LOCAL void Wall_ServerNotice PARAMS(( char *Msg ));


GLOBAL void
Log_Init( bool Daemon_Mode )
{
	Is_Daemon = Daemon_Mode;
	
#ifdef SYSLOG
#ifndef LOG_CONS	/* Kludge: mips-dec-ultrix4.5 has no LOG_CONS/LOG_LOCAL5 */
#define LOG_CONS 0
#endif
#ifndef LOG_LOCAL5
#define LOG_LOCAL5 0
#endif
	/* Syslog initialisieren */
	openlog( PACKAGE_NAME, LOG_CONS|LOG_PID, LOG_LOCAL5 );
#endif

	/* Hello World! */
	Log( LOG_NOTICE, "%s started.", NGIRCd_Version );
	  
	/* Informationen uebern den "Operation Mode" */
	Init_Txt[0] = '\0';
#ifdef DEBUG
	if( NGIRCd_Debug )
	{
		strlcpy( Init_Txt, "debug-mode", sizeof Init_Txt );
	}
#endif
	if( ! Is_Daemon )
	{
		if( Init_Txt[0] ) strlcat( Init_Txt, ", ", sizeof Init_Txt );
		strlcat( Init_Txt, "no-daemon-mode", sizeof Init_Txt );
	}
	if( NGIRCd_Passive )
	{
		if( Init_Txt[0] ) strlcat( Init_Txt, ", ", sizeof Init_Txt );
		strlcat( Init_Txt, "passive-mode", sizeof Init_Txt );
	}
#ifdef SNIFFER
	if( NGIRCd_Sniffer )
	{
		if( Init_Txt[0] ) strlcat( Init_Txt, ", ", sizeof Init_Txt );
		strlcat( Init_Txt, "network sniffer", sizeof Init_Txt );
	}
#endif
	if( Init_Txt[0] ) Log( LOG_INFO, "Activating: %s.", Init_Txt );

#ifdef DEBUG
	Error_File[0] = '\0';
#endif
} /* Log_Init */


#ifdef DEBUG

GLOBAL void
Log_InitErrorfile( void )
{
	/* "Error-Log" initialisieren: stderr in Datei umlenken. Dort
	 * landen z.B. alle Ausgaben von assert()-Aufrufen. */

	/* Dateiname zusammen bauen */
	snprintf( Error_File, sizeof Error_File, "%s/%s-%ld.err", ERROR_DIR, PACKAGE_NAME, (long)getpid( ));

	/* stderr umlenken */
	fflush( stderr );
	if( ! freopen( Error_File, "w", stderr ))
	{
		Log( LOG_ERR, "Can't reopen stderr (\"%s\"): %s", Error_File, strerror( errno ));
		return;
	}

	/* Einige Infos in das Error-File schreiben */
	fputs( ctime( &NGIRCd_Start ), stderr );
	fprintf( stderr, "%s started.\n", NGIRCd_Version );
	fprintf( stderr, "Activating: %s\n\n", Init_Txt[0] ? Init_Txt : "-" );
	fflush( stderr );

#ifdef DEBUG
	Log( LOG_DEBUG, "Redirected stderr to \"%s\".", Error_File );
#endif
} /* Log_InitErrfile */

#endif


GLOBAL void
Log_Exit( void )
{
	/* Good Bye! */
	if( NGIRCd_SignalRestart ) Log( LOG_NOTICE, "%s done (restarting).", PACKAGE_NAME );
	else Log( LOG_NOTICE, "%s done.", PACKAGE_NAME );

#ifdef DEBUG
	if( Error_File[0] )
	{
		/* Error-File (stderr) loeschen */
		if( unlink( Error_File ) != 0 ) Log( LOG_ERR, "Can't delete \"%s\": %s", Error_File, strerror( errno ));
	}
#endif

#ifdef SYSLOG
	/* syslog abmelden */
	closelog( );
#endif
} /* Log_Exit */


#ifdef PROTOTYPES
GLOBAL void
Log( int Level, const char *Format, ... )
#else
GLOBAL void
Log( Level, Format, va_alist )
int Level;
const char *Format;
va_dcl
#endif
{
	/* Eintrag in Logfile(s) schreiben */

	char msg[MAX_LOG_MSG_LEN];
	bool snotice;
	va_list ap;

	assert( Format != NULL );

	if( Level & LOG_snotice )
	{
		/* Notice an User mit "s" Mode */
		snotice = true;
		Level &= ~LOG_snotice;
	}
	else snotice = false;

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

	if( ! Is_Daemon )
	{
		/* auf Konsole ausgeben */
		fprintf( stdout, "[%d:%d] %s\n", (int)getpid( ), Level, msg );
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
		/* log critical messages to stderr */
		fprintf( stderr, "%s\n", msg );
		fflush( stderr );
	}

	if( snotice )
	{
		/* NOTICE an lokale User mit "s"-Mode */
		Wall_ServerNotice( msg );
	}
} /* Log */


GLOBAL void
Log_Init_Resolver( void )
{
#ifdef SYSLOG
	openlog( PACKAGE_NAME, LOG_CONS|LOG_PID, LOG_LOCAL5 );
#endif
#ifdef DEBUG
	Log_Resolver( LOG_DEBUG, "Resolver sub-process starting, PID %d.", getpid( ));
#endif
} /* Log_Init_Resolver */


GLOBAL void
Log_Exit_Resolver( void )
{
#ifdef DEBUG
	Log_Resolver( LOG_DEBUG, "Resolver sub-process %d done.", getpid( ));
#endif
#ifdef SYSLOG
	closelog( );
#endif
} /* Log_Exit_Resolver */


#ifdef PROTOTYPES
GLOBAL void
Log_Resolver( const int Level, const char *Format, ... )
#else
GLOBAL void
Log_Resolver( Level, Format, va_alist )
const int Level;
const char *Format;
va_dcl
#endif
{
	/* Eintrag des Resolver in Logfile(s) schreiben */

	char msg[MAX_LOG_MSG_LEN];
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

	if( ! Is_Daemon )
	{
		/* Output to console */
		fprintf( stdout, "[%d:%d] %s\n", (int)getpid( ), Level, msg );
		fflush( stdout );
	}
#ifdef SYSLOG
	else syslog( Level, "%s", msg );
#endif
} /* Log_Resolver */


/**
 * Send log messages to users flagged with the "s" mode.
 * @param Msg The message to send.
 */
LOCAL void
Wall_ServerNotice( char *Msg )
{
	CLIENT *c;

	assert( Msg != NULL );

	c = Client_First( );
	while(c) {
		if (Client_Conn(c) > NONE && Client_HasMode(c, 's'))
			IRC_WriteStrClient(c, "NOTICE %s :%s%s", Client_ID(c),
							NOTICE_TXTPREFIX, Msg);

		c = Client_Next( c );
	}
} /* Wall_ServerNotice */


/* -eof- */
