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
 * $Id: log.h,v 1.13 2002/12/12 12:23:43 alex Exp $
 *
 * Logging functions (header)
 */


#ifndef __log_h__
#define __log_h__


#ifdef USE_SYSLOG
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


GLOBAL VOID Log_Init PARAMS((VOID ));
GLOBAL VOID Log_Exit PARAMS((VOID ));

GLOBAL VOID Log_InitErrorfile PARAMS((VOID ));
GLOBAL VOID Log PARAMS((INT Level, CONST CHAR *Format, ... ));

GLOBAL VOID Log_Init_Resolver PARAMS((VOID ));
GLOBAL VOID Log_Exit_Resolver PARAMS((VOID ));

GLOBAL VOID Log_Resolver PARAMS((CONST INT Level, CONST CHAR *Format, ... ));


#endif


/* -eof- */
