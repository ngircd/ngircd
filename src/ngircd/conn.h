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
 * $Id: conn.h,v 1.14 2002/03/29 22:54:35 alex Exp $
 *
 * conn.h: Verwaltung aller Netz-Verbindungen ("connections") (Header)
 */


#ifndef __conn_h__
#define __conn_h__


#include <time.h>


typedef INT CONN_ID;

typedef struct _Res_Stat
{
	INT pid;			/* PID des Child-Prozess */
	INT pipe[2];			/* Pipe fuer IPC */
} RES_STAT;


GLOBAL VOID Conn_Init( VOID );
GLOBAL VOID Conn_Exit( VOID );

GLOBAL BOOLEAN Conn_NewListener( CONST UINT Port );

GLOBAL VOID Conn_Handler( INT Timeout );

GLOBAL BOOLEAN Conn_Write( CONN_ID Idx, CHAR *Data, INT Len );
GLOBAL BOOLEAN Conn_WriteStr( CONN_ID Idx, CHAR *Format, ... );

GLOBAL VOID Conn_Close( CONN_ID Idx, CHAR *LogMsg, CHAR *FwdMsg, BOOLEAN InformClient );

GLOBAL VOID Conn_UpdateIdle( CONN_ID Idx );
GLOBAL time_t Conn_GetIdle( CONN_ID Idx );
GLOBAL time_t Conn_LastPing( CONN_ID Idx );


#endif


/* -eof- */
