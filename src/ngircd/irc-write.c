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
 * $Id: irc-write.c,v 1.9 2002/10/04 11:21:46 alex Exp $
 *
 * irc-write.c: IRC-Texte und Befehle ueber Netzwerk versenden
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "conn.h"
#include "client.h"
#include "channel.h"
#include "defines.h"

#include "exp.h"
#include "irc-write.h"


LOCAL CHAR *Get_Prefix PARAMS(( CLIENT *Target, CLIENT *Client ));


#ifdef PROTOTYPES
GLOBAL BOOLEAN
IRC_WriteStrClient( CLIENT *Client, CHAR *Format, ... )
#else
GLOBAL BOOLEAN
IRC_WriteStrClient( Client, Format, va_alist )
CLIENT *Client;
CHAR *Format;
va_dcl
#endif
{
	CHAR buffer[1000];
	BOOLEAN ok = CONNECTED;
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
GLOBAL BOOLEAN
IRC_WriteStrClientPrefix( CLIENT *Client, CLIENT *Prefix, CHAR *Format, ... )
#else
GLOBAL BOOLEAN
IRC_WriteStrClientPrefix( Client, Prefix, Format, va_alist )
CLIENT *Client;
CLIENT *Prefix;
CHAR *Format;
va_dcl
#endif
{
	/* Text an Clients, lokal bzw. remote, senden. */

	CHAR buffer[1000];
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
GLOBAL BOOLEAN
IRC_WriteStrChannel( CLIENT *Client, CHANNEL *Chan, BOOLEAN Remote, CHAR *Format, ... )
#else
GLOBAL BOOLEAN
IRC_WriteStrChannel( Client, Chan, Remote, Format, va_alist )
CLIENT *Client;
CHANNEL *Chan;
BOOLEAN Remote;
CHAR *Format;
va_dcl
#endif
{
	CHAR buffer[1000];
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
GLOBAL BOOLEAN
IRC_WriteStrChannelPrefix( CLIENT *Client, CHANNEL *Chan, CLIENT *Prefix, BOOLEAN Remote, CHAR *Format, ... )
#else
GLOBAL BOOLEAN
IRC_WriteStrChannelPrefix( Client, Chan, Prefix, Remote, Format, va_alist )
CLIENT *Client;
CHANNEL *Chan;
CLIENT *Prefix;
BOOLEAN Remote;
CHAR *Format;
va_dcl
#endif
{
	BOOLEAN sock[MAX_CONNECTIONS], is_server[MAX_CONNECTIONS], ok = CONNECTED;
	CHAR buffer[1000];
	CL2CHAN *cl2chan;
	CLIENT *c;
	INT s, i;
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

	for( i = 0; i < MAX_CONNECTIONS; i++ ) sock[i] = FALSE;

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
			s = Client_Conn( c );
			assert( s >= 0 );
			assert( s < MAX_CONNECTIONS );
			sock[s] = TRUE;
			if( Client_Type( c ) == CLIENT_SERVER ) is_server[s] = TRUE;
			else is_server[s] = FALSE;
		}
		cl2chan = Channel_NextMember( Chan, cl2chan );
	}

	/* Senden ... */
	for( i = 0; i < MAX_CONNECTIONS; i++ )
	{
		if( sock[i] )
		{
			if( is_server[i] ) ok = Conn_WriteStr( i, ":%s %s", Client_ID( Prefix ), buffer );
			else ok = Conn_WriteStr( i, ":%s %s", Client_Mask( Prefix ), buffer );
			if( ! ok ) break;
		}
	}
	return ok;
} /* IRC_WriteStrChannelPrefix */


#ifdef PROTOTYPES
GLOBAL VOID
IRC_WriteStrServers( CLIENT *ExceptOf, CHAR *Format, ... )
#else
GLOBAL VOID
IRC_WriteStrServers( ExceptOf, Format, va_alist )
CLIENT *ExceptOf;
CHAR *Format;
va_dcl
#endif
{
	CHAR buffer[1000];
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
GLOBAL VOID
IRC_WriteStrServersPrefix( CLIENT *ExceptOf, CLIENT *Prefix, CHAR *Format, ... )
#else
GLOBAL VOID
IRC_WriteStrServersPrefix( ExceptOf, Prefix, Format, va_alist )
CLIENT *ExceptOf;
CLIENT *Prefix;
CHAR *Format;
va_dcl
#endif
{
	CHAR buffer[1000];
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
GLOBAL VOID
IRC_WriteStrServersPrefixFlag( CLIENT *ExceptOf, CLIENT *Prefix, CHAR Flag, CHAR *Format, ... )
#else
GLOBAL VOID
IRC_WriteStrServersPrefixFlag( ExceptOf, Prefix, Flag, Format, va_alist )
CLIENT *ExceptOf;
CLIENT *Prefix;
CHAR Flag;
CHAR *Format;
va_dcl
#endif
{
	CHAR buffer[1000];
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
GLOBAL BOOLEAN
IRC_WriteStrRelatedPrefix( CLIENT *Client, CLIENT *Prefix, BOOLEAN Remote, CHAR *Format, ... )
#else
GLOBAL BOOLEAN
IRC_WriteStrRelatedPrefix( Client, Prefix, Remote, Format, va_alist )
CLIENT *Client;
CLIENT *Prefix;
BOOLEAN Remote;
CHAR *Format;
va_dcl
#endif
{
	BOOLEAN sock[MAX_CONNECTIONS], is_server[MAX_CONNECTIONS], ok = CONNECTED;
	CL2CHAN *chan_cl2chan, *cl2chan;
	CHAR buffer[1000];
	CHANNEL *chan;
	va_list ap;
	CLIENT *c;
	INT i, s;

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
	for( i = 0; i < MAX_CONNECTIONS; i++ ) sock[i] = FALSE;

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
				s = Client_Conn( c );
				assert( s >= 0 );
				assert( s < MAX_CONNECTIONS );
				sock[s] = TRUE;
				if( Client_Type( c ) == CLIENT_SERVER ) is_server[s] = TRUE;
				else is_server[s] = FALSE;
			}
			cl2chan = Channel_NextMember( chan, cl2chan );
		}
		
		/* naechsten Channel */
		chan_cl2chan = Channel_NextChannelOf( Client, chan_cl2chan );
	}

	/* Senden ... */
	for( i = 0; i < MAX_CONNECTIONS; i++ )
	{
		if( sock[i] )
		{
			if( is_server[i] ) ok = Conn_WriteStr( i, ":%s %s", Client_ID( Prefix ), buffer );
			else ok = Conn_WriteStr( i, ":%s %s", Client_Mask( Prefix ), buffer );
			if( ! ok ) break;
		}
	}
	return ok;
} /* IRC_WriteStrRelatedPrefix */


LOCAL CHAR *
Get_Prefix( CLIENT *Target, CLIENT *Client )
{
	assert( Target != NULL );
	assert( Client != NULL );

	if( Client_Type( Target ) == CLIENT_SERVER ) return Client_ID( Client );
	else return Client_Mask( Client );
} /* Get_Prefix */


/* -eof- */
