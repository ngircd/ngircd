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
 * $Id: conn-func.h,v 1.1 2002/12/30 17:14:28 alex Exp $
 *
 * Connection management: Global functions (header)
 */


#ifndef __conn_func_h__
#define __conn_func_h__


/* Include the header conn.h if this header is _not_ included by any module
 * containing connection handling functions. So other modules must only
 * include this conn-func.h header. */
#ifndef CONN_MODULE
#	include "conn.h"
#endif


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

GLOBAL CONN_ID Conn_First PARAMS(( VOID ));
GLOBAL CONN_ID Conn_Next PARAMS(( CONN_ID Idx ));

GLOBAL VOID Conn_SetOption PARAMS(( CONN_ID Idx, INT Option ));
GLOBAL VOID Conn_UnsetOption PARAMS(( CONN_ID Idx, INT Option ));
GLOBAL INT Conn_Options PARAMS(( CONN_ID Idx ));

GLOBAL VOID Conn_ResetWCounter PARAMS(( VOID ));
GLOBAL LONG Conn_WCounter PARAMS(( VOID ));


#endif


/* -eof- */
