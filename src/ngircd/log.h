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
 * $Id: log.h,v 1.10 2002/03/27 20:53:31 alex Exp $
 *
 * log.h: Logging-Funktionen (Header)
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


GLOBAL VOID Log_Init( VOID );
GLOBAL VOID Log_Exit( VOID );

GLOBAL VOID Log( INT Level, CONST CHAR *Format, ... );

GLOBAL VOID Log_Init_Resolver( VOID );
GLOBAL VOID Log_Exit_Resolver( VOID );

GLOBAL VOID Log_Resolver( CONST INT Level, CONST CHAR *Format, ... );


#endif


/* -eof- */
