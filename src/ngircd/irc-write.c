/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2005 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Sending IRC commands over the network
 */


#include "portab.h"

static char UNUSED id[] = "$Id: irc-write.c,v 1.19 2005/07/31 20:13:08 alex Exp $";

#include "imp.h"
#include <assert.h>
#ifdef PROTOTYPES
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "conn-func.h"
#include "client.h"
#include "channel.h"

#include "exp.h"
#include "irc-write.h"


#define SEND_TO_USER 1
#define SEND_TO_SERVER 2


static char *Get_Prefix PARAMS(( CLIENT *Target, CLIENT *Client ));


#ifdef PROTOTYPES
GLOBAL bool
IRC_WriteStrClient( CLIENT *Client, char *Format, ... )
#else
GLOBAL bool
IRC_WriteStrClient( Client, Format, va_alist )
CLIENT *Client;
char *Format;
va_dcl
#endif
{
	char buffer[1000];
	bool ok = CONNECTED;
	va_list ap;

	assert( Client != NULL );
	assert( Format != NULL );

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	/* an den Client selber */
	ok = IRC_WriteStrClientPrefix( Client, Client_ThisServer( ), "%s", buffer );

	return ok;
} /* IRC_WriteStrClient */


#ifdef PROTOTYPES
GLOBAL bool
IRC_WriteStrClientPrefix( CLIENT *Client, CLIENT *Prefix, char *Format, ... )
#else
GLOBAL bool
IRC_WriteStrClientPrefix( Client, Prefix, Format, va_alist )
CLIENT *Client;
CLIENT *Prefix;
char *Format;
va_dcl
#endif
{
	/* Text an Clients, lokal bzw. remote, senden. */

	char buffer[1000];
	va_list ap;

	assert( Client != NULL );
	assert( Format != NULL );
	assert( Prefix != NULL );

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	return Conn_WriteStr( Client_Conn( Client_NextHop( Client )), ":%s %s", Get_Prefix( Client_NextHop( Client ), Prefix ), buffer );
} /* IRC_WriteStrClientPrefix */


#ifdef PROTOTYPES
GLOBAL bool
IRC_WriteStrChannel( CLIENT *Client, CHANNEL *Chan, bool Remote, char *Format, ... )
#else
GLOBAL bool
IRC_WriteStrChannel( Client, Chan, Remote, Format, va_alist )
CLIENT *Client;
CHANNEL *Chan;
bool Remote;
char *Format;
va_dcl
#endif
{
	char buffer[1000];
	va_list ap;

	assert( Client != NULL );
	assert( Format != NULL );

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	return IRC_WriteStrChannelPrefix( Client, Chan, Client_ThisServer( ), Remote, "%s", buffer );
} /* IRC_WriteStrChannel */


#ifdef PROTOTYPES
GLOBAL bool
IRC_WriteStrChannelPrefix( CLIENT *Client, CHANNEL *Chan, CLIENT *Prefix, bool Remote, char *Format, ... )
#else
GLOBAL bool
IRC_WriteStrChannelPrefix( Client, Chan, Prefix, Remote, Format, va_alist )
CLIENT *Client;
CHANNEL *Chan;
CLIENT *Prefix;
bool Remote;
char *Format;
va_dcl
#endif
{
	bool ok = CONNECTED;
	char buffer[1000];
	CL2CHAN *cl2chan;
	CONN_ID conn;
	CLIENT *c;
	va_list ap;

	assert( Client != NULL );
	assert( Chan != NULL );
	assert( Prefix != NULL );
	assert( Format != NULL );

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap  );
#endif
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	Conn_ClearFlags( );

	/* An alle Clients, die in den selben Channels sind.
	 * Dabei aber nur einmal je Remote-Server */
	cl2chan = Channel_FirstMember( Chan );
	while( cl2chan )
	{
		c = Channel_GetClient( cl2chan );
		if( ! Remote )
		{
			if( Client_Conn( c ) <= NONE ) c = NULL;
			else if( Client_Type( c ) == CLIENT_SERVER ) c = NULL;
		}
		if( c ) c = Client_NextHop( c );
			
		if( c && ( c != Client ))
		{
			/* Ok, anderer Client */
			conn = Client_Conn( c );
			if( Client_Type( c ) == CLIENT_SERVER )	Conn_SetFlag( conn, SEND_TO_SERVER );
			else Conn_SetFlag( conn, SEND_TO_USER );
		}
		cl2chan = Channel_NextMember( Chan, cl2chan );
	}

	/* Senden: alle Verbindungen durchgehen ... */
	conn = Conn_First( );
	while( conn != NONE )
	{
		/* muessen Daten ueber diese Verbindung verschickt werden? */
		if( Conn_Flag( conn ) == SEND_TO_SERVER) ok = Conn_WriteStr( conn, ":%s %s", Client_ID( Prefix ), buffer );
		else if( Conn_Flag( conn ) == SEND_TO_USER ) ok = Conn_WriteStr( conn, ":%s %s", Client_Mask( Prefix ), buffer );
		if( ! ok ) break;

		/* naechste Verbindung testen */
		conn = Conn_Next( conn );
	}

	return ok;
} /* IRC_WriteStrChannelPrefix */


#ifdef PROTOTYPES
GLOBAL void
IRC_WriteStrServers( CLIENT *ExceptOf, char *Format, ... )
#else
GLOBAL void
IRC_WriteStrServers( ExceptOf, Format, va_alist )
CLIENT *ExceptOf;
char *Format;
va_dcl
#endif
{
	char buffer[1000];
	va_list ap;

	assert( Format != NULL );

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	/* an den Client selber */
	IRC_WriteStrServersPrefix( ExceptOf, Client_ThisServer( ), "%s", buffer );
} /* IRC_WriteStrServers */


#ifdef PROTOTYPES
GLOBAL void
IRC_WriteStrServersPrefix( CLIENT *ExceptOf, CLIENT *Prefix, char *Format, ... )
#else
GLOBAL void
IRC_WriteStrServersPrefix( ExceptOf, Prefix, Format, va_alist )
CLIENT *ExceptOf;
CLIENT *Prefix;
char *Format;
va_dcl
#endif
{
	char buffer[1000];
	va_list ap;

	assert( Format != NULL );
	assert( Prefix != NULL );

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	IRC_WriteStrServersPrefixFlag( ExceptOf, Prefix, '\0', "%s", buffer );
} /* IRC_WriteStrServersPrefix */
	

#ifdef PROTOTYPES
GLOBAL void
IRC_WriteStrServersPrefixFlag( CLIENT *ExceptOf, CLIENT *Prefix, char Flag, char *Format, ... )
#else
GLOBAL void
IRC_WriteStrServersPrefixFlag( ExceptOf, Prefix, Flag, Format, va_alist )
CLIENT *ExceptOf;
CLIENT *Prefix;
char Flag;
char *Format;
va_dcl
#endif
{
	char buffer[1000];
	CLIENT *c;
	va_list ap;
	
	assert( Format != NULL );
	assert( Prefix != NULL );

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );
	
	c = Client_First( );
	while( c )
	{
		if(( Client_Type( c ) == CLIENT_SERVER ) && ( Client_Conn( c ) > NONE ) && ( c != Client_ThisServer( )) && ( c != ExceptOf ))
		{
			/* Ziel-Server gefunden. Nun noch pruefen, ob Flags stimmen */
			if(( Flag == '\0' ) || ( strchr( Client_Flags( c ), Flag ) != NULL )) IRC_WriteStrClientPrefix( c, Prefix, "%s", buffer );
		}
		c = Client_Next( c );
	}
} /* IRC_WriteStrServersPrefixFlag */


#ifdef PROTOTYPES
GLOBAL bool
IRC_WriteStrRelatedPrefix( CLIENT *Client, CLIENT *Prefix, bool Remote, char *Format, ... )
#else
GLOBAL bool
IRC_WriteStrRelatedPrefix( Client, Prefix, Remote, Format, va_alist )
CLIENT *Client;
CLIENT *Prefix;
bool Remote;
char *Format;
va_dcl
#endif
{
	bool ok = CONNECTED;
	CL2CHAN *chan_cl2chan, *cl2chan;
	char buffer[1000];
	CHANNEL *chan;
	CONN_ID conn;
	va_list ap;
	CLIENT *c;

	assert( Client != NULL );
	assert( Prefix != NULL );
	assert( Format != NULL );

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( buffer, 1000, Format, ap );
	va_end( ap );

	/* initialisieren */
	Conn_ClearFlags( );

	/* An alle Clients, die in einem Channel mit dem "Ausloeser" sind,
	 * den Text schicken. An Remote-Server aber jeweils nur einmal. */
	chan_cl2chan = Channel_FirstChannelOf( Client );
	while( chan_cl2chan )
	{
		/* Channel des Users durchsuchen */
		chan = Channel_GetChannel( chan_cl2chan );
		cl2chan = Channel_FirstMember( chan );
		while( cl2chan )
		{
			c = Channel_GetClient( cl2chan );
			if( ! Remote )
			{
				if( Client_Conn( c ) <= NONE ) c = NULL;
				else if( Client_Type( c ) == CLIENT_SERVER ) c = NULL;
			}
			if( c ) c = Client_NextHop( c );

			if( c && ( c != Client ))
			{
				/* Ok, anderer Client */
				conn = Client_Conn( c );
				if( Client_Type( c ) == CLIENT_SERVER ) Conn_SetFlag( conn, SEND_TO_SERVER );
				else Conn_SetFlag( conn, SEND_TO_USER );
			}
			cl2chan = Channel_NextMember( chan, cl2chan );
		}
		
		/* naechsten Channel */
		chan_cl2chan = Channel_NextChannelOf( Client, chan_cl2chan );
	}

	/* Senden: alle Verbindungen durchgehen ... */
	conn = Conn_First( );
	while( conn != NONE )
	{
		/* muessen ueber diese Verbindung Daten gesendet werden? */
		if( Conn_Flag( conn ) == SEND_TO_SERVER ) ok = Conn_WriteStr( conn, ":%s %s", Client_ID( Prefix ), buffer );
		else if( Conn_Flag( conn ) == SEND_TO_USER ) ok = Conn_WriteStr( conn, ":%s %s", Client_Mask( Prefix ), buffer );
		if( ! ok ) break;

		/* naechste Verbindung testen */
		conn = Conn_Next( conn );
	}
	return ok;
} /* IRC_WriteStrRelatedPrefix */


GLOBAL void
IRC_SetPenalty( CLIENT *Client, int Seconds )
{
	CONN_ID c;
	
	assert( Client != NULL );
	assert( Seconds > 0 );
	
	if( Client_Type( Client ) == CLIENT_SERVER ) return;
	
	c = Client_Conn( Client );
	if( c > NONE ) Conn_SetPenalty( c, Seconds );		
} /* IRC_SetPenalty */


static char *
Get_Prefix( CLIENT *Target, CLIENT *Client )
{
	assert( Target != NULL );
	assert( Client != NULL );

	if( Client_Type( Target ) == CLIENT_SERVER ) return Client_ID( Client );
	else return Client_Mask( Client );
} /* Get_Prefix */


/* -eof- */
