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
 * $Id: conn.h,v 1.23 2002/11/26 23:06:51 alex Exp $
 *
 * conn.h: Verwaltung aller Netz-Verbindungen ("connections") (Header)
 */


#ifndef __conn_h__
#define __conn_h__


#include <time.h>			/* wg. time_t, s.u. */


#ifdef USE_ZLIB
#define CONN_ZIP 4			/* Kompression mit zlib */
#endif


typedef INT CONN_ID;


GLOBAL VOID Conn_Init PARAMS((VOID ));
GLOBAL VOID Conn_Exit PARAMS(( VOID ));

GLOBAL INT Conn_InitListeners PARAMS(( VOID ));
GLOBAL VOID Conn_ExitListeners PARAMS(( VOID ));

GLOBAL BOOLEAN Conn_NewListener PARAMS(( CONST UINT Port ));

GLOBAL VOID Conn_Handler PARAMS(( VOID ));

GLOBAL BOOLEAN Conn_Write PARAMS(( CONN_ID Idx, CHAR *Data, INT Len ));
GLOBAL BOOLEAN Conn_WriteStr PARAMS(( CONN_ID Idx, CHAR *Format, ... ));

GLOBAL VOID Conn_Close PARAMS(( CONN_ID Idx, CHAR *LogMsg, CHAR *FwdMsg, BOOLEAN InformClient ));

GLOBAL VOID Conn_UpdateIdle PARAMS(( CONN_ID Idx ));
GLOBAL time_t Conn_GetIdle PARAMS(( CONN_ID Idx ));
GLOBAL time_t Conn_LastPing PARAMS(( CONN_ID Idx ));

GLOBAL VOID Conn_SetPenalty PARAMS(( CONN_ID Idx, time_t Seconds ));
GLOBAL VOID Conn_ResetPenalty PARAMS(( CONN_ID Idx ));

GLOBAL VOID Conn_ClearFlags PARAMS(( VOID ));
GLOBAL INT Conn_Flag PARAMS(( CONN_ID Idx ));
GLOBAL VOID Conn_SetFlag PARAMS(( CONN_ID Idx, INT Flag ));

GLOBAL VOID Conn_SetServer PARAMS(( CONN_ID Idx, INT ConfServer ));

GLOBAL CONN_ID Conn_First PARAMS(( VOID ));
GLOBAL CONN_ID Conn_Next PARAMS(( CONN_ID Idx ));

GLOBAL VOID Conn_SetOption PARAMS(( CONN_ID Idx, INT Option ));
GLOBAL VOID Conn_UnsetOption PARAMS(( CONN_ID Idx, INT Option ));
GLOBAL INT Conn_Options PARAMS(( CONN_ID Idx ));

#ifdef USE_ZLIB
GLOBAL BOOLEAN Conn_InitZip PARAMS(( CONN_ID Idx ));
#endif


GLOBAL INT Conn_MaxFD;


#endif


/* -eof- */
