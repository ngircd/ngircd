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
 * $Id: conn.h,v 1.25 2002/12/12 12:23:43 alex Exp $
 *
 * Connection management (header)
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
GLOBAL time_t Conn_StartTime PARAMS(( CONN_ID Idx ));
GLOBAL INT Conn_SendQ PARAMS(( CONN_ID Idx ));
GLOBAL INT Conn_RecvQ PARAMS(( CONN_ID Idx ));
GLOBAL LONG Conn_SendMsg PARAMS(( CONN_ID Idx ));
GLOBAL LONG Conn_RecvMsg PARAMS(( CONN_ID Idx ));
GLOBAL LONG Conn_SendBytes PARAMS(( CONN_ID Idx ));
GLOBAL LONG Conn_RecvBytes PARAMS(( CONN_ID Idx ));

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
GLOBAL LONG Conn_SendBytesZip PARAMS(( CONN_ID Idx ));
GLOBAL LONG Conn_RecvBytesZip PARAMS(( CONN_ID Idx ));
#endif


GLOBAL INT Conn_MaxFD;


#endif


/* -eof- */
