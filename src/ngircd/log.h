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
 * $Id: log.h,v 1.2 2001/12/12 17:19:29 alex Exp $
 *
 * log.h: Logging-Funktionen (Header)
 *
 * $Log: log.h,v $
 * Revision 1.2  2001/12/12 17:19:29  alex
 * - LOG_ERR heisst nun LOG_ERROR.
 *
 * Revision 1.1.1.1  2001/12/11 21:53:04  alex
 * - Imported sources to CVS.
 */


#ifndef __log_h__
#define __log_h__


#define LOG_DEBUG 4
#define LOG_INFO 3
#define LOG_WARN 2
#define LOG_ERROR 1
#define LOG_FATAL 0


GLOBAL VOID Log_Init( VOID );
GLOBAL VOID Log_Exit( VOID );

GLOBAL VOID Log( CONST INT Level, CONST CHAR *Format, ... );


#endif


/* -eof- */
