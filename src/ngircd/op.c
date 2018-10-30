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
 * IRC operator functions
 */

#include <assert.h>

#include "conn.h"
#include "channel.h"
#include "conf.h"
#include "log.h"
#include "parse.h"
#include "messages.h"
#include "irc-write.h"

#include "op.h"

/**
 * Return and log a "no privileges" message.
 */
GLOBAL bool
Op_NoPrivileges(CLIENT * Client, REQUEST * Req)
{
	CLIENT *from = NULL;

	if (Req->prefix)
		from = Client_Search(Req->prefix);

	if (from) {
		Log(LOG_ERR|LOG_snotice,
		    "No privileges: client \"%s\" (%s), command \"%s\"!",
		    Req->prefix, Client_Mask(Client), Req->command);
		return IRC_WriteErrClient(from, ERR_NOPRIVILEGES_MSG,
					  Client_ID(from));
	} else {
		Log(LOG_ERR|LOG_snotice,
		    "No privileges: client \"%s\", command \"%s\"!",
		    Client_Mask(Client), Req->command);
		return IRC_WriteErrClient(Client, ERR_NOPRIVILEGES_MSG,
					  Client_ID(Client));
	}
} /* Op_NoPrivileges */

/**
 * Check that the originator of a request is an IRC operator and allowed
 * to administer this server.
 *
 * @param Client Client from which the command has been received.
 * @param Req Request structure.
 * @return CLIENT structure of the client that initiated the command or
 *	   NULL if client is not allowed to execute operator commands.
 */
GLOBAL CLIENT *
Op_Check(CLIENT * Client, REQUEST * Req)
{
	CLIENT *c;

	assert(Client != NULL);
	assert(Req != NULL);

	if (Client_Type(Client) == CLIENT_SERVER && Req->prefix)
		c = Client_Search(Req->prefix);
	else
		c = Client;

	if (!c)
		return NULL;
	if (Client_Type(Client) == CLIENT_SERVER
	    && Client_Type(c) == CLIENT_SERVER)
		return c;
	if (!Client_HasMode(c, 'o'))
		return NULL;
	if (Client_Conn(c) <= NONE && !Conf_AllowRemoteOper)
		return NULL;

	/* The client is an local IRC operator, or this server is configured
	 * to trust remote operators. */
	return c;
} /* Op_Check */

/* -eof- */
