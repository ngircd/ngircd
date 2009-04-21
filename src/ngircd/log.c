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


static char Init_Txt[127];
static bool Is_Daemon;

#ifdef DEBUG
static char Error_File[FNAME_LEN];
#endif


static void Wall_ServerNotice PARAMS(( char *Msg ));

static void
Log_Message(int Level, const char *msg)
{
	if (!Is_Daemon) {
		/* log to console */
		fprintf(stdout, "[%ld:%d %4ld] %s\n", (long)getpid(), Level,
				(long)time(NULL) - NGIRCd_Start, msg);
		fflush(stdout);
	}
#ifdef SYSLOG
	else {
		syslog(Level, "%s", msg);
	}
#endif
}


GLOBAL void
Log_Init( bool Daemon_Mode )
{
	Is_Daemon = Daemon_Mode;
	
#ifdef SYSLOG
#ifndef LOG_CONS     /* Kludge: mips-dec-ultrix4.5 has no LOG_CONS/LOG_LOCAL5 */
#define LOG_CONS 0
#endif
#ifndef LOG_LOCAL5
#define LOG_LOCAL5 0
#endif
	openlog( PACKAGE_NAME, LOG_CONS|LOG_PID, LOG_LOCAL5 );
#endif

	Log( LOG_NOTICE, "%s started.", NGIRCd_Version );
	  
	/* Information about "Operation Mode" */
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
	snprintf( Error_File, sizeof Error_File, "%s/%s-%ld.err", ERROR_DIR, PACKAGE_NAME, (long)getpid( ));

	fflush( stderr );
	if( ! freopen( Error_File, "w", stderr ))
	{
		Log( LOG_ERR, "Can't reopen stderr (\"%s\"): %s", Error_File, strerror( errno ));
		return;
	}

	fputs( ctime( &NGIRCd_Start ), stderr );
	fprintf( stderr, "%s started.\n", NGIRCd_Version );
	fprintf( stderr, "Activating: %s\n\n", Init_Txt[0] ? Init_Txt : "-" );
	fflush( stderr );

	Log(LOG_DEBUG, "Redirected stderr to \"%s\".", Error_File);
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
	closelog();
#endif
} /* Log_Exit */


/**
 * Log function for debug messages.
 * This function is only functional when the program is compiled with debug
 * code enabled; otherwise it is an empty function which the compiler will
 * hopefully mangle down to "nothing" (see log.h). Therefore you should use
 * LogDebug(...) in favor to Log(LOG_DEBUG, ...).
 * @param Format Format string like printf().
 * @param ... Further arguments.
 */
#ifdef DEBUG
# ifdef PROTOTYPES
GLOBAL void
LogDebug( const char *Format, ... )
# else
GLOBAL void
LogDebug( Format, va_alist )
const char *Format;
va_dcl
# endif /* PROTOTYPES */
{
	char msg[MAX_LOG_MSG_LEN];
	va_list ap;

	if (!NGIRCd_Debug) return;
#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( msg, MAX_LOG_MSG_LEN, Format, ap );
	va_end( ap );
	Log(LOG_DEBUG, "%s", msg);
}
#endif	/* DEBUG */


/**
 * Logging function of ngIRCd.
 * This function logs messages to the console and/or syslog, whichever is
 * suitable for the mode ngIRCd is running in (daemon vs. non-daemon).
 * If LOG_snotice is set, the log messages goes to all user with the mode +s
 * set and the local &SERVER channel, too.
 * Please note: you sould use LogDebug(...) for debug messages!
 * @param Level syslog level (LOG_xxx)
 * @param Format Format string like printf().
 * @param ... Further arguments.
 */
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

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( msg, MAX_LOG_MSG_LEN, Format, ap );
	va_end( ap );

	Log_Message(Level, msg);

	if (Level <= LOG_CRIT) {
		/* log critical messages to stderr */
		fprintf(stderr, "%s\n", msg);
		fflush(stderr);
	}

	if (snotice) {
		/* Send NOTICE to all local users with mode +s and to the
		 * local &SERVER channel */
		Wall_ServerNotice(msg);
		Channel_LogServer(msg);
	}
} /* Log */


GLOBAL void
Log_Init_Resolver( void )
{
#ifdef SYSLOG
	openlog( PACKAGE_NAME, LOG_CONS|LOG_PID, LOG_LOCAL5 );
#endif
#ifdef DEBUG
	Log_Resolver(LOG_DEBUG, "Resolver sub-process starting, PID %ld.", (long)getpid());
#endif
} /* Log_Init_Resolver */


GLOBAL void
Log_Exit_Resolver( void )
{
#ifdef DEBUG
	Log_Resolver(LOG_DEBUG, "Resolver sub-process %ld done.", (long)getpid());
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

	Log_Message(Level, msg);
} /* Log_Resolver */


/**
 * Send log messages to users flagged with the "s" mode.
 * @param Msg The message to send.
 */
static void
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
