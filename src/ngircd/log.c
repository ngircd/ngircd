/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2019 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#include "portab.h"

/**
 * @file
 * Logging functions
 */

#include <assert.h>
#ifdef PROTOTYPES
# include <stdarg.h>
#else
# include <varargs.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef SYSLOG
# include <syslog.h>
#endif

#include "ngircd.h"
#include "conn.h"
#include "channel.h"
#include "irc-write.h"
#include "conf.h"

#include "log.h"

static bool Use_Syslog;


static void
Log_Message(int Level, const char *msg)
{
	if (!Use_Syslog) {
		/* log to console */
		fprintf(stdout, "[%ld:%d %4ld] %s\n", (long)getpid(), Level,
				(long)(time(NULL) - NGIRCd_Start), msg);
		fflush(stdout);
	}
#ifdef SYSLOG
	else {
		syslog(Level, "%s", msg);
	}
#endif
}


/**
 * Initialize logging.
 * This function is called before the configuration file is read in.
 *
 * @param Syslog_Mode Set to true if ngIRCd is configured to log to the syslog.
 */
GLOBAL void
Log_Init(bool Syslog_Mode)
{
	Use_Syslog = Syslog_Mode;

#ifdef SYSLOG
#ifndef LOG_CONS     /* Kludge: mips-dec-ultrix4.5 has no LOG_CONS */
#define LOG_CONS 0
#endif
#ifdef LOG_DAEMON
	openlog(PACKAGE, LOG_CONS|LOG_PID, LOG_DAEMON);
#else
	openlog(PACKAGE, LOG_CONS|LOG_PID, 0);
#endif
#endif
	Log(LOG_NOTICE, "%s starting ...", NGIRCd_Version);
} /* Log_Init */


/**
 * Re-init logging after reading the configuration file.
 */
GLOBAL void
Log_ReInit(void)
{
#ifdef SYSLOG
#ifndef LOG_CONS     /* Kludge: mips-dec-ultrix4.5 has no LOG_CONS */
#define LOG_CONS 0
#endif
	closelog();
	openlog(PACKAGE, LOG_CONS|LOG_PID, Conf_SyslogFacility);
#endif
}


GLOBAL void
Log_Exit( void )
{
	Log(LOG_NOTICE, "%s done%s, served %lu connection%s.", PACKAGE_NAME,
	    NGIRCd_SignalRestart ? " (restarting)" : "", Conn_CountAccepted(),
	    Conn_CountAccepted() == 1 ? "" : "s");
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


/**
 * Logging function of ngIRCd.
 * This function logs messages to the console and/or syslog, whichever is
 * suitable for the mode ngIRCd is running in (daemon vs. non-daemon).
 * If LOG_snotice is set, the log messages goes to all user with the mode +s
 * set and the local &SERVER channel, too.
 * Please note: you should use LogDebug(...) for debug messages!
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

	if(( Level == LOG_DEBUG ) && ( ! NGIRCd_Debug )) return;

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( msg, MAX_LOG_MSG_LEN, Format, ap );
	va_end( ap );

	Log_Message(Level, msg);

	if (snotice) {
		/* Send NOTICE to all local users with mode +s and to the
		 * local &SERVER channel */
		Log_ServerNotice('s', "%s", msg);
		Channel_LogServer(msg);
	}
} /* Log */


GLOBAL void
Log_Init_Subprocess(char UNUSED *Name)
{
#ifdef SYSLOG
	openlog(PACKAGE, LOG_CONS|LOG_PID, Conf_SyslogFacility);
#endif
	Log_Subprocess(LOG_DEBUG, "%s sub-process starting, PID %ld.",
		     Name, (long)getpid());
}


GLOBAL void
Log_Exit_Subprocess(char UNUSED *Name)
{
	Log_Subprocess(LOG_DEBUG, "%s sub-process %ld done.",
		     Name, (long)getpid());
#ifdef SYSLOG
	closelog( );
#endif
}


#ifdef PROTOTYPES
GLOBAL void
Log_Subprocess(const int Level, const char *Format, ...)
#else
GLOBAL void
Log_Subprocess(Level, Format, va_alist)
const int Level;
const char *Format;
va_dcl
#endif
{
	char msg[MAX_LOG_MSG_LEN];
	va_list ap;

	assert(Format != NULL);

	if ((Level == LOG_DEBUG) && (!NGIRCd_Debug))
		return;

#ifdef PROTOTYPES
	va_start(ap, Format);
#else
	va_start(ap);
#endif
	vsnprintf(msg, MAX_LOG_MSG_LEN, Format, ap);
	va_end(ap);

	Log_Message(Level, msg);
}


/**
 * Send a log message to all local users flagged with the given user mode.
 * @param UserMode User mode which the target user must have set,
 * @param Format The format string.
 */
#ifdef PROTOTYPES
GLOBAL void
Log_ServerNotice(const char UserMode, const char *Format, ... )
#else
GLOBAL void
Log_ServerNotice(UserMode, Format, va_alist)
const char UserMode;
const char *Format;
va_dcl
#endif
{
	CLIENT *c;
	char msg[MAX_LOG_MSG_LEN];
	va_list ap;

	assert(Format != NULL);

#ifdef PROTOTYPES
	va_start(ap, Format);
#else
	va_start(ap);
#endif
	vsnprintf(msg, MAX_LOG_MSG_LEN, Format, ap);
	va_end(ap);

	for(c=Client_First(); c != NULL; c=Client_Next(c)) {
		if (Client_Conn(c) > NONE && Client_HasMode(c, UserMode))
			IRC_WriteStrClient(c, "NOTICE %s :%s%s", Client_ID(c),
							NOTICE_TXTPREFIX, msg);
	}
} /* Log_ServerNotice */


/* -eof- */
