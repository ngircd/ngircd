/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2014 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#define __client_cap_c__

#include "portab.h"

/**
 * @file
 * Functions to deal with IRC Capabilities
 */

#include <assert.h>

#include "conn.h"
#include "log.h"

#include "client-cap.h"

GLOBAL int
Client_Cap(CLIENT *Client)
{
	assert (Client != NULL);

	return Client->capabilities;
}

GLOBAL void
Client_CapSet(CLIENT *Client, int Cap)
{
	assert(Client != NULL);
	assert(Cap >= 0);

	Client->capabilities = Cap;
	LogDebug("Set new capability of \"%s\" to %d.",
		 Client_ID(Client), Client->capabilities);
}

GLOBAL void
Client_CapAdd(CLIENT *Client, int Cap)
{
	assert(Client != NULL);
	assert(Cap > 0);

	Client->capabilities |= Cap;
	LogDebug("Add capability %d, new capability of \"%s\" is %d.",
		 Cap, Client_ID(Client), Client->capabilities);
}

GLOBAL void
Client_CapDel(CLIENT *Client, int Cap)
{
	assert(Client != NULL);
	assert(Cap > 0);

	Client->capabilities &= ~Cap;
	LogDebug("Delete capability %d, new capability of \"%s\" is %d.",
		 Cap, Client_ID(Client), Client->capabilities);
}

/* -eof- */
