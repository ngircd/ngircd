/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2008 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * IRC operator functions
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <string.h>

#include "conn.h"
#include "client.h"
#include "channel.h"
#include "conf.h"
#include "log.h"
#include "parse.h"
#include "messages.h"
#include "irc-write.h"

#include <exp.h>
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
		Log(LOG_NOTICE, "No privileges: client \"%s\" (%s), command \"%s\"",
		    Req->prefix, Client_Mask(Client), Req->command);
		return IRC_WriteStrClient(from, ERR_NOPRIVILEGES_MSG,
					  Client_ID(from));
	} else {
		Log(LOG_NOTICE, "No privileges: client \"%s\", command \"%s\"",
		    Client_Mask(Client), Req->command);
		return IRC_WriteStrClient(Client, ERR_NOPRIVILEGES_MSG,
					  Client_ID(Client));
	}
} /* Op_NoPrivileges */


/**
 * Check that the client is an IRC operator allowed to administer this server.
 */
GLOBAL bool
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
		return false;
	if (!Client_HasMode(c, 'o'))
		return false;
	if (!Client_OperByMe(c) && !Conf_AllowRemoteOper)
		return false;
	/* The client is an local IRC operator, or this server is configured
	 * to trust remote operators. */
	return true;
} /* Op_Check */
