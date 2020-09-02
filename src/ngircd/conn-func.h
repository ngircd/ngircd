/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2008 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __conn_func_h__
#define __conn_func_h__

/**
 * @file
 * Connection management: Global functions (header)
 */

/* Include the header conn.h if this header is _not_ included by any module
 * containing connection handling functions. So other modules must only
 * include this conn-func.h header. */
#ifndef CONN_MODULE
#include "conn.h"
#else
#define Conn_OPTION_ADD( x, opt )   ( (x)->options |= (opt) )
#define Conn_OPTION_DEL( x, opt )   ( (x)->options &= ~(opt) )
#define Conn_OPTION_ISSET( x, opt ) ( ((x)->options & (opt)) != 0)
#endif


GLOBAL void Conn_UpdateIdle PARAMS((CONN_ID Idx));
GLOBAL void Conn_UpdatePing PARAMS((CONN_ID Idx, time_t TimeStamp));

GLOBAL time_t Conn_GetSignon PARAMS((CONN_ID Idx));
GLOBAL time_t Conn_GetIdle PARAMS(( CONN_ID Idx ));
GLOBAL time_t Conn_LastPing PARAMS(( CONN_ID Idx ));
GLOBAL time_t Conn_StartTime PARAMS(( CONN_ID Idx ));
GLOBAL size_t Conn_SendQ PARAMS(( CONN_ID Idx ));
GLOBAL size_t Conn_RecvQ PARAMS(( CONN_ID Idx ));
GLOBAL long Conn_SendMsg PARAMS(( CONN_ID Idx ));
GLOBAL long Conn_RecvMsg PARAMS(( CONN_ID Idx ));
GLOBAL long Conn_SendBytes PARAMS(( CONN_ID Idx ));
GLOBAL long Conn_RecvBytes PARAMS(( CONN_ID Idx ));
GLOBAL const char *Conn_IPA PARAMS(( CONN_ID Idx ));

GLOBAL void Conn_SetPenalty PARAMS(( CONN_ID Idx, time_t Seconds ));

GLOBAL void Conn_ClearFlags PARAMS(( void ));
GLOBAL int Conn_Flag PARAMS(( CONN_ID Idx ));
GLOBAL void Conn_SetFlag PARAMS(( CONN_ID Idx, int Flag ));

GLOBAL CONN_ID Conn_First PARAMS(( void ));
GLOBAL CONN_ID Conn_Next PARAMS(( CONN_ID Idx ));

GLOBAL UINT16 Conn_Options PARAMS(( CONN_ID Idx ));
GLOBAL void Conn_SetOption PARAMS(( CONN_ID Idx, int Option ));

GLOBAL void Conn_ResetWCounter PARAMS(( void ));
GLOBAL long Conn_WCounter PARAMS(( void ));

#endif


/* -eof- */
