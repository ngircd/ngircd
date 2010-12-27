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

#define CONN_MODULE

#include "portab.h"

/**
 * @file
 * Connection management: Global functions
 */

#include "imp.h"
#include <assert.h>
#include <string.h>
#include "log.h"

#include "conn.h"
#include "client.h"

#include "exp.h"
#include "conn-func.h"


GLOBAL void
Conn_UpdateIdle( CONN_ID Idx )
{
	assert( Idx > NONE );
	My_Connections[Idx].lastprivmsg = time( NULL );
}


/*
 * Get signon time of a connection.
 */
GLOBAL time_t
Conn_GetSignon(CONN_ID Idx)
{
	assert(Idx > NONE);
	return My_Connections[Idx].signon;
}

GLOBAL time_t
Conn_GetIdle( CONN_ID Idx )
{
	/* Return Idle-Timer of a connetion */
	assert( Idx > NONE );
	return time( NULL ) - My_Connections[Idx].lastprivmsg;
} /* Conn_GetIdle */


GLOBAL time_t
Conn_LastPing( CONN_ID Idx )
{
	assert( Idx > NONE );
	return My_Connections[Idx].lastping;
} /* Conn_LastPing */


GLOBAL void
Conn_SetPenalty( CONN_ID Idx, time_t Seconds )
{
	/* set Penalty-Delay for a socket.
	 * during the penalty, the socket is ignored completely, no new
	 * data is read. This function only increases the penalty, it is
	 * not possible to decrease the penalty time.
	 */
	time_t t;
	
	assert( Idx > NONE );
	assert( Seconds >= 0 );

	t = time( NULL ) + Seconds;
	if (t > My_Connections[Idx].delaytime)
		My_Connections[Idx].delaytime = t;

#ifdef DEBUG
	Log(LOG_DEBUG, "Add penalty time on connection %d: %ld second(s).",
			Idx, (long)Seconds);
#endif
} /* Conn_SetPenalty */


GLOBAL void
Conn_ResetPenalty( CONN_ID Idx )
{
	assert( Idx > NONE );
	My_Connections[Idx].delaytime = 0;
} /* Conn_ResetPenalty */


GLOBAL void
Conn_ClearFlags( void )
{
	CONN_ID i;

	for( i = 0; i < Pool_Size; i++ ) My_Connections[i].flag = 0;
} /* Conn_ClearFlags */


GLOBAL int
Conn_Flag( CONN_ID Idx )
{
	assert( Idx > NONE );
	return My_Connections[Idx].flag;
} /* Conn_Flag */


GLOBAL void
Conn_SetFlag( CONN_ID Idx, int Flag )
{
	/* Connection markieren */

	assert( Idx > NONE );
	My_Connections[Idx].flag = Flag;
} /* Conn_SetFlag */


GLOBAL CONN_ID
Conn_First( void )
{
	/* Connection-Struktur der ersten Verbindung liefern;
	 * Ist keine Verbindung vorhanden, wird NONE geliefert. */

	CONN_ID i;
	
	for( i = 0; i < Pool_Size; i++ )
	{
		if( My_Connections[i].sock != NONE ) return i;
	}
	return NONE;
} /* Conn_First */


GLOBAL CONN_ID
Conn_Next( CONN_ID Idx )
{
	/* Naechste Verbindungs-Struktur liefern; existiert keine
	 * weitere, so wird NONE geliefert. */

	CONN_ID i = NONE;

	assert( Idx > NONE );
	
	for( i = Idx + 1; i < Pool_Size; i++ )
	{
		if( My_Connections[i].sock != NONE ) return i;
	}
	return NONE;
} /* Conn_Next */


GLOBAL UINT16
Conn_Options( CONN_ID Idx )
{
	assert( Idx > NONE );
	return My_Connections[Idx].options;
} /* Conn_Options */


/**
 * Set connection option.
 */
GLOBAL void
Conn_SetOption(CONN_ID Idx, int Option)
{
	assert(Idx > NONE);
	Conn_OPTION_ADD(&My_Connections[Idx], Option);
} /* Conn_SetOption */


/**
 * Get the start time of the connection.
 * The result is the start time in seconds since 1970-01-01, as reported
 * by the C function time(NULL).
 */
GLOBAL time_t
Conn_StartTime( CONN_ID Idx )
{
	CLIENT *c;

	assert(Idx > NONE);

	/* Search client structure for this link ... */
	c = Conn_GetClient(Idx);
	if(c != NULL)
		return Client_StartTime(c);

	return 0;
} /* Conn_StartTime */

/**
 * return number of bytes queued for writing
 */
GLOBAL size_t
Conn_SendQ( CONN_ID Idx )
{
	assert( Idx > NONE );
#ifdef ZLIB
	if( My_Connections[Idx].options & CONN_ZIP )
		return array_bytes(&My_Connections[Idx].zip.wbuf);
	else
#endif
	return array_bytes(&My_Connections[Idx].wbuf);
} /* Conn_SendQ */


/**
 * return number of messages sent on this connection so far
 */
GLOBAL long
Conn_SendMsg( CONN_ID Idx )
{

	assert( Idx > NONE );
	return My_Connections[Idx].msg_out;
} /* Conn_SendMsg */


/**
 * return number of (uncompressed) bytes sent
 * on this connection so far
 */
GLOBAL long
Conn_SendBytes( CONN_ID Idx )
{
	assert( Idx > NONE );
	return My_Connections[Idx].bytes_out;
} /* Conn_SendBytes */


/**
 * return number of bytes pending in read buffer
 */
GLOBAL size_t
Conn_RecvQ( CONN_ID Idx )
{
	assert( Idx > NONE );
#ifdef ZLIB
	if( My_Connections[Idx].options & CONN_ZIP )
		return array_bytes(&My_Connections[Idx].zip.rbuf);
	else
#endif
	return array_bytes(&My_Connections[Idx].rbuf);
} /* Conn_RecvQ */


/**
 * return number of messages received on this connection so far
 */
GLOBAL long
Conn_RecvMsg( CONN_ID Idx )
{
	assert( Idx > NONE );
	return My_Connections[Idx].msg_in;
} /* Conn_RecvMsg */


/**
 * return number of (uncompressed) bytes received on this
 * connection so far
 */
GLOBAL long
Conn_RecvBytes( CONN_ID Idx )
{
	assert( Idx > NONE );
	return My_Connections[Idx].bytes_in;
} /* Conn_RecvBytes */

/**
 * Return the remote IP address of this connection as string.
 */
GLOBAL const char *
Conn_IPA(CONN_ID Idx)
{
	assert (Idx > NONE);
	return ng_ipaddr_tostr(&My_Connections[Idx].addr);
}


GLOBAL void
Conn_ResetWCounter( void )
{
	WCounter = 0;
} /* Conn_ResetWCounter */


GLOBAL long
Conn_WCounter( void )
{
	return WCounter;
} /* Conn_WCounter */


/* -eof- */
