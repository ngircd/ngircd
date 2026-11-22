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

#include "portab.h"

/**
 * @file
 * Sending IRC commands over the network
 */

#include <assert.h>
#ifdef PROTOTYPES
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif
#include <stdio.h>
#include <string.h>

#include "conn-func.h"
#include "channel.h"
#include "log.h"

#include "irc-write.h"

#define SEND_TO_USER 1
#define SEND_TO_SERVER 2

static const char *Get_Prefix PARAMS((CLIENT *Target, CLIENT *Client));
static void cb_writeStrServersPrefixFlag PARAMS((CLIENT *Client,
					 CLIENT *Prefix, void *Buffer));
static void Send_Marked_Connections PARAMS((CLIENT *Prefix, const char *Buffer));

/**
 * Send an error message to a client and enforce a penalty time.
 *
 * @param Client The target client.
 * @param Format Format string.
 * @return CONNECTED or DISCONNECTED.
 */
#ifdef PROTOTYPES
GLOBAL bool
IRC_WriteErrClient( CLIENT *Client, const char *Format, ... )
#else
GLOBAL bool
IRC_WriteErrClient( Client, Format, va_alist )
CLIENT *Client;
const char *Format;
va_dcl
#endif
{
	char buffer[1000];
	va_list ap;

	assert(Client != NULL);
	assert(Format != NULL);

#ifdef PROTOTYPES
	va_start(ap, Format);
#else
	va_start(ap);
#endif
	vsnprintf(buffer, sizeof(buffer), Format, ap);
	va_end(ap);

	if (Client == Client_ThisServer()) {
		/* Error in a request faked by this server itself, for example
		 * when evaluating the "Modes" of pre-defined channels: there
		 * is no connection to send it to, so log it instead. */
		Log(LOG_WARNING, "Request of this server failed: \"%s\"",
		    buffer);
		return CONNECTED;
	}

	IRC_SetPenalty(Client, 2);
	return IRC_WriteStrClientPrefix(Client, Client_ThisServer(),
					"%s", buffer);
}

/**
 * Send a message to a client.
 *
 * @param Client The target client.
 * @param Format Format string.
 * @return CONNECTED or DISCONNECTED.
 */
#ifdef PROTOTYPES
GLOBAL bool
IRC_WriteStrClient( CLIENT *Client, const char *Format, ... )
#else
GLOBAL bool
IRC_WriteStrClient( Client, Format, va_alist )
CLIENT *Client;
const char *Format;
va_dcl
#endif
{
	char buffer[1000];
	va_list ap;

	assert(Client != NULL);
	assert(Format != NULL);

#ifdef PROTOTYPES
	va_start(ap, Format);
#else
	va_start(ap);
#endif
	vsnprintf(buffer, sizeof(buffer), Format, ap);
	va_end(ap);

	return IRC_WriteStrClientPrefix(Client, Client_ThisServer(),
					"%s", buffer);
}

/**
 * Send a message to a client using a specific prefix.
 *
 * @param Client The target client.
 * @param Prefix The prefix to use.
 * @param Format Format string.
 * @return CONNECTED or DISCONNECTED.
 */
#ifdef PROTOTYPES
GLOBAL bool
IRC_WriteStrClientPrefix(CLIENT *Client, CLIENT *Prefix, const char *Format, ...)
#else
GLOBAL bool
IRC_WriteStrClientPrefix(Client, Prefix, Format, va_alist)
CLIENT *Client;
CLIENT *Prefix;
const char *Format;
va_dcl
#endif
{
	/* send text to local and remote clients */

	char buffer[1000];
	va_list ap;

	assert( Client != NULL );
	assert( Format != NULL );
	assert( Prefix != NULL );

	/* Replies to requests faked by this server itself (see above) have
	 * no connection to be sent to and are silently dropped. */
	if (Client == Client_ThisServer())
		return CONNECTED;

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf(buffer, sizeof(buffer), Format, ap);
	va_end( ap );

	return Conn_WriteStr(Client_Conn(Client_NextHop(Client)), ":%s %s",
			Get_Prefix(Client_NextHop(Client), Prefix), buffer);
}

/**
 * Send a message to all client in a channel.
 *
 * The message is only sent once per remote server.
 *
 * @param Client The sending client, excluded while forwarding the message.
 * @param Channel The target channel.
 * @param Remote If not set, the message is sent to local clients only.
 * @param Format Format string.
 */
#ifdef PROTOTYPES
GLOBAL void
IRC_WriteStrChannel(CLIENT *Client, CHANNEL *Chan, bool Remote,
		    const char *Format, ...)
#else
GLOBAL void
IRC_WriteStrChannel(Client, Chan, Remote, Format, va_alist)
CLIENT *Client;
CHANNEL *Chan;
bool Remote;
const char *Format;
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
	vsnprintf(buffer, sizeof(buffer), Format, ap);
	va_end( ap );

	IRC_WriteStrChannelPrefix(Client, Chan, Client_ThisServer(),
				  Remote, "%s", buffer);
}

/**
 * Send a message to all client in a channel using a specific prefix.
 *
 * The message is only sent once per remote server.
 *
 * @param Client The sending client, excluded while forwarding the message.
 * @param Channel The target channel.
 * @param Prefix The prefix to use.
 * @param Remote If not set, the message is sent to local clients only.
 * @param Format Format string.
 */
#ifdef PROTOTYPES
GLOBAL void
IRC_WriteStrChannelPrefix(CLIENT *Client, CHANNEL *Chan, CLIENT *Prefix,
			  bool Remote, const char *Format, ...)
#else
GLOBAL void
IRC_WriteStrChannelPrefix(Client, Chan, Prefix, Remote, Format, va_alist)
CLIENT *Client;
CHANNEL *Chan;
CLIENT *Prefix;
bool Remote;
const char *Format;
va_dcl
#endif
{
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
	vsnprintf(buffer, sizeof(buffer), Format, ap);
	va_end( ap );

	Conn_ClearFlags( );

	cl2chan = Channel_FirstMember( Chan );
	while(cl2chan) {
		c = Channel_GetClient( cl2chan );
		if (!Remote) {
			if (Client_Conn(c) <= NONE)
				c = NULL;
			else if(Client_Type(c) == CLIENT_SERVER)
				c = NULL;
		}
		if(c)
			c = Client_NextHop(c);

		if(c && c != Client) {
			/* Ok, another Client */
			conn = Client_Conn(c);
			if (Client_Type(c) == CLIENT_SERVER)
				Conn_SetFlag(conn, SEND_TO_SERVER);
			else
				Conn_SetFlag(conn, SEND_TO_USER);
		}
		cl2chan = Channel_NextMember(Chan, cl2chan);
	}
	Send_Marked_Connections(Prefix, buffer);
}

/**
 * Send a message to all the servers in the network.
 *
 * @param Client The sending client, excluded while forwarding the message.
 * @param Format Format string.
 */
#ifdef PROTOTYPES
GLOBAL void
IRC_WriteStrServers(CLIENT *ExceptOf, const char *Format, ...)
#else
GLOBAL void
IRC_WriteStrServers(ExceptOf, Format, va_alist)
CLIENT *ExceptOf;
const char *Format;
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
	vsnprintf(buffer, sizeof(buffer), Format, ap);
	va_end( ap );

	IRC_WriteStrServersPrefix(ExceptOf, Client_ThisServer(), "%s", buffer);
}

/**
 * Send a message to all the servers in the network using a specific prefix.
 *
 * @param Client The sending client, excluded while forwarding the message.
 * @param Prefix The prefix to use.
 * @param Format Format string.
 */
#ifdef PROTOTYPES
GLOBAL void
IRC_WriteStrServersPrefix(CLIENT *ExceptOf, CLIENT *Prefix,
			  const char *Format, ...)
#else
GLOBAL void
IRC_WriteStrServersPrefix(ExceptOf, Prefix, Format, va_alist)
CLIENT *ExceptOf;
CLIENT *Prefix;
const char *Format;
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
	vsnprintf(buffer, sizeof(buffer), Format, ap);
	va_end( ap );

	IRC_WriteStrServersPrefixFlag( ExceptOf, Prefix, '\0', "%s", buffer );
}

/**
 * Send a message to all the servers in the network using a specific prefix
 * and matching a "client flag".
 *
 * @param Client The sending client, excluded while forwarding the message.
 * @param Prefix The prefix to use.
 * @param Flag Client flag that must be set on the target.
 * @param Format Format string.
 */
#ifdef PROTOTYPES
GLOBAL void
IRC_WriteStrServersPrefixFlag(CLIENT *ExceptOf, CLIENT *Prefix, char Flag,
			      const char *Format, ...)
#else
GLOBAL void
IRC_WriteStrServersPrefixFlag(ExceptOf, Prefix, Flag, Format, va_alist)
CLIENT *ExceptOf;
CLIENT *Prefix;
char Flag;
const char *Format;
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
	vsnprintf(buffer, sizeof(buffer), Format, ap);
	va_end( ap );

	IRC_WriteStrServersPrefixFlag_CB(ExceptOf, Prefix, Flag,
					 cb_writeStrServersPrefixFlag, buffer);
}

/**
 * Send a message to all the servers in the network using a specific prefix
 * and matching a "client flag" using a callback function.
 *
 * @param ExceptOf The sending client, excluded while forwarding the message.
 * @param Prefix The prefix to use.
 * @param Flag Client flag that must be set on the target.
 * @param callback Callback function.
 */
GLOBAL void
IRC_WriteStrServersPrefixFlag_CB(CLIENT *ExceptOf, CLIENT *Prefix, char Flag,
		void (*callback)(CLIENT *, CLIENT *, void *), void *cb_data)
{
	CONN_ID conn, except_conn;
	CLIENT *c;

	/* A write error destroys the server, the clients behind it, and maybe
	 * more: so loop over the connection IDs, look up their clients each
	 * time, and stop when the prefix has been destroyed, too. */
	except_conn = ExceptOf ? Client_Conn(ExceptOf) : NONE;

	for (conn = Conn_First(); conn != NONE; conn = Conn_Next(conn)) {
		c = Conn_GetClient(conn);
		if (!c || Client_Type(c) != CLIENT_SERVER ||
		    c == Client_ThisServer() || conn == except_conn)
			continue;

		/* Found a target server, do the flags match? */
		if (Flag != '\0' && !Client_HasFlag(c, Flag))
			continue;

		callback(c, Prefix, cb_data);

		/* Only a write error on this connection can destroy clients */
		if (!Conn_GetClient(conn) && !Client_Exists(Prefix))
			break;
	}
}

/**
 * Send a message to all "related" clients.
 *
 * Related clients are the one that share one or more channels with the client
 * sending this message.
 *
 * The message is only sent once per remote server.
 *
 * @param Client The sending client, excluded while forwarding the message.
 * @param Prefix The prefix to use.
 * @param Remote If not set, the message is sent to local clients only.
 * @param Format Format string.
 */
#ifdef PROTOTYPES
GLOBAL void
IRC_WriteStrRelatedPrefix(CLIENT *Client, CLIENT *Prefix, bool Remote,
			  const char *Format, ...)
#else
GLOBAL void
IRC_WriteStrRelatedPrefix(Client, Prefix, Remote, Format, va_alist)
CLIENT *Client;
CLIENT *Prefix;
bool Remote;
const char *Format;
va_dcl
#endif
{
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
	vsnprintf(buffer, sizeof(buffer), Format, ap);
	va_end( ap );

	Conn_ClearFlags( );

	chan_cl2chan = Channel_FirstChannelOf( Client );
	while( chan_cl2chan )
	{
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
				conn = Client_Conn( c );
				if( Client_Type( c ) == CLIENT_SERVER ) Conn_SetFlag( conn, SEND_TO_SERVER );
				else Conn_SetFlag( conn, SEND_TO_USER );
			}
			cl2chan = Channel_NextMember( chan, cl2chan );
		}

		chan_cl2chan = Channel_NextChannelOf( Client, chan_cl2chan );
	}
	Send_Marked_Connections(Prefix, buffer);
} /* IRC_WriteStrRelatedPrefix */

/**
 * Send WALLOPS message.
 *
 * @param Client The sending client, excluded while forwarding the message.
 * @param From The (remote) sender of the message.
 * @param Format Format string.
*/
#ifdef PROTOTYPES
GLOBAL void
IRC_SendWallops(CLIENT *Client, CLIENT *From, const char *Format, ...)
#else
GLOBAL void
IRC_SendWallops(Client, From, Format, va_alist )
CLIENT *Client;
CLIENT *From;
const char *Format;
va_dcl
#endif
{
	va_list ap;
	char msg[1000], from_id[CLIENT_ID_LEN], from_mask[MASK_LEN];
	CONN_ID conn, except_conn;
	CLIENT *to;

#ifdef PROTOTYPES
	va_start(ap, Format);
#else
	va_start(ap);
#endif
	vsnprintf(msg, sizeof(msg), Format, ap);
	va_end(ap);

	/* A write error destroys the client and can close other connections,
	 * including the one of the sender: so save the prefixes and the
	 * connection to exclude in advance, loop over the connection IDs, and
	 * look up their clients each time. */
	except_conn = Client_Conn(Client);
	strlcpy(from_id, Client_ID(From), sizeof(from_id));
	strlcpy(from_mask, Client_MaskCloaked(From), sizeof(from_mask));

	for (conn = Conn_First(); conn != NONE; conn = Conn_Next(conn)) {
		to = Conn_GetClient(conn);
		if (!to)
			continue;

		switch (Client_Type(to)) {
		case CLIENT_USER:
			if (Client_HasMode(to, 'w'))
				Conn_WriteStr(conn, ":%s WALLOPS :%s",
					      from_mask, msg);
			break;
		case CLIENT_SERVER:
			if (conn != except_conn)
				Conn_WriteStr(conn, ":%s WALLOPS :%s",
					      from_id, msg);
			break;
		}
	}
} /* IRC_SendWallops */

/**
 * Set a "penalty time" for an IRC client.
 *
 * Note: penalty times are never set for server links or remote clients!
 *
 * @param Client The client.
 * @param Seconds The additional "penalty time" to enforce.
 */
GLOBAL void
IRC_SetPenalty(CLIENT *Client, time_t Seconds)
{
	CONN_ID c;

	assert(Client != NULL);
	assert(Seconds > 0);

	if (Client_Type(Client) == CLIENT_SERVER)
		return;

	c = Client_Conn(Client);
	if (c <= NONE)
		return;

	Conn_SetPenalty(c, Seconds);
} /* IRC_SetPenalty */

static const char *
Get_Prefix(CLIENT *Target, CLIENT *Client)
{
	assert (Target != NULL);
	assert (Client != NULL);

	if (Client_Type(Target) == CLIENT_SERVER)
		return Client_ID(Client);
	else
		return Client_MaskCloaked(Client);
} /* Get_Prefix */

static void
cb_writeStrServersPrefixFlag(CLIENT *Client, CLIENT *Prefix, void *Buffer)
{
	IRC_WriteStrClientPrefix(Client, Prefix, "%s", Buffer);
} /* cb_writeStrServersPrefixFlag */

/**
 * Send a message to all marked connections using a specific prefix.
 *
 * @param Prefix The prefix to use.
 * @param Buffer The message to send.
 */
static void
Send_Marked_Connections(CLIENT *Prefix, const char *Buffer)
{
	CONN_ID conn;

	assert(Prefix != NULL);
	assert(Buffer != NULL);

	conn = Conn_First();
	while (conn != NONE) {
		if (Conn_Flag(conn) == SEND_TO_SERVER)
			Conn_WriteStr(conn, ":%s %s",
				      Client_ID(Prefix), Buffer);
		else if (Conn_Flag(conn) == SEND_TO_USER)
			Conn_WriteStr(conn, ":%s %s",
				      Client_MaskCloaked(Prefix), Buffer);
		conn = Conn_Next(conn);
	}
}

/* -eof- */
