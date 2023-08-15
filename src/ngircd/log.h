/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2012 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __log_h__
#define __log_h__

/**
 * @file
 * Logging functions (header)
 */

#ifdef SYSLOG
#	include <syslog.h>
#else
#	define LOG_EMERG 0
#	define LOG_ALERT 1
#	define LOG_CRIT 2
#	define LOG_ERR 3
#	define LOG_WARNING 4
#	define LOG_NOTICE 5
#	define LOG_INFO 6
#	define LOG_DEBUG 7
#endif

#define LOG_snotice 1024

GLOBAL void Log_Init PARAMS(( bool Syslog_Mode ));
GLOBAL void Log_Exit PARAMS(( void ));

GLOBAL void Log PARAMS(( int Level, const char *Format, ... ));
GLOBAL void Log_ReInit PARAMS((void));

GLOBAL void Log_ServerNotice PARAMS((char UserMode, const char *Format, ...));

GLOBAL void LogDebug PARAMS(( const char *Format, ... ));

GLOBAL void Log_Init_Subprocess PARAMS((char *Name));
GLOBAL void Log_Exit_Subprocess PARAMS((char *Name));

GLOBAL void Log_Subprocess PARAMS((const int Level, const char *Format, ...));

GLOBAL void Log_InitErrorfile PARAMS(( void ));

#endif

/* -eof- */
