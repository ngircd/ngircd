/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2018 Alexander Barton (alex@barton.de) and Contributors.
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

#include <assert.h>
#include <time.h>

#include "log.h"
#include "conn.h"
#include "conf.h"
#include "conn-func.h"

/**
 * Update "idle timestamp", the time of the last visible user action
 * (e. g. like sending messages, joining or leaving channels).
 *
 * @param Idx Connection index.
 */
GLOBAL void
Conn_UpdateIdle(CONN_ID Idx)
{
	assert(Idx > NONE);
	My_Connections[Idx].lastprivmsg = time(NULL);
}

/**
 * Update "ping timestamp", the time of the last outgoing PING request.
 *
 * the value 0 signals a newly connected client including servers during the
 * initial "server burst"; and 1 means that no PONG is pending for a PING.
 *
 * @param Idx Connection index.
 * @param TimeStamp 0, 1, or time stamp.
 */
GLOBAL void
Conn_UpdatePing(CONN_ID Idx, time_t TimeStamp)
{
	assert(Idx > NONE);
	My_Connections[Idx].lastping = TimeStamp;
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
	/* Return Idle-Timer of a connection */
	assert( Idx > NONE );
	return time( NULL ) - My_Connections[Idx].lastprivmsg;
} /* Conn_GetIdle */

GLOBAL time_t
Conn_LastPing( CONN_ID Idx )
{
	assert( Idx > NONE );
	return My_Connections[Idx].lastping;
} /* Conn_LastPing */

/**
 * Add "penalty time" for a connection.
 *
 * During the "penalty time" the socket is ignored completely, no new data
 * is read. This function only increases the penalty, it is not possible to
 * decrease the penalty time.
 *
 * @param Idx Connection index.
 * @param Seconds Seconds to add.
 * @see Conn_ResetPenalty
 */
GLOBAL void
Conn_SetPenalty(CONN_ID Idx, time_t Seconds)
{
	time_t t;

	assert(Idx > NONE);
	assert(Seconds >= 0);

	/* Limit new penalty to maximum configured, when less than 10 seconds. *
	   The latter is used to limit brute force attacks, therefore we don't *
	   want to limit that! */
	if (Conf_MaxPenaltyTime >= 0
	    && Seconds > Conf_MaxPenaltyTime
	    && Seconds < 10)
		Seconds = Conf_MaxPenaltyTime;

	t = time(NULL);
	if (My_Connections[Idx].delaytime < t)
		My_Connections[Idx].delaytime = t;

	My_Connections[Idx].delaytime += Seconds;

	LogDebug("Add penalty time on connection %d: %ld second%s, total %ld second%s.",
	    Idx, (long)Seconds, Seconds != 1 ? "s" : "",
	    My_Connections[Idx].delaytime - t,
	    My_Connections[Idx].delaytime - t != 1 ? "s" : "");
} /* Conn_SetPenalty */

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
	assert( Idx > NONE );
	My_Connections[Idx].flag = Flag;
} /* Conn_SetFlag */

GLOBAL CONN_ID
Conn_First( void )
{
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
