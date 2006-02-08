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
 * $Id: log.h,v 1.18.2.1 2006/02/08 21:23:21 fw Exp $
 *
 * Logging functions (header)
 */


#ifndef __log_h__
#define __log_h__


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


GLOBAL void Log_Init PARAMS(( bool Daemon_Mode ));
GLOBAL void Log_Exit PARAMS(( void ));

GLOBAL void Log PARAMS(( int Level, const char *Format, ... ));
GLOBAL void LogDebug PARAMS(( const char *Format, ... ));

GLOBAL void Log_Init_Resolver PARAMS(( void ));
GLOBAL void Log_Exit_Resolver PARAMS(( void ));

GLOBAL void Log_Resolver PARAMS(( const int Level, const char *Format, ... ));

#ifdef DEBUG
GLOBAL void Log_InitErrorfile PARAMS(( void ));
#endif


#endif


/* -eof- */
