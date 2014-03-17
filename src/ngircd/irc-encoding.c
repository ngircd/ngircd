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

#include "portab.h"

/**
 * @file
 * IRC encoding commands
 */

#ifdef ICONV

#include <assert.h>
#include <string.h>

#include "conn-func.h"
#include "channel.h"
#include "conn-encoding.h"
#include "irc-write.h"
#include "messages.h"
#include "parse.h"
#include "tool.h"

#include "irc-encoding.h"

/**
 * Handler for the IRC+ "CHARCONV" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @returns CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_CHARCONV(CLIENT *Client, REQUEST *Req)
{
	char encoding[20];

	assert (Client != NULL);
	assert (Req != NULL);

	strlcpy(encoding, Req->argv[0], sizeof(encoding));
	ngt_UpperStr(encoding);

	if (!Conn_SetEncoding(Client_Conn(Client), encoding))
		return IRC_WriteErrClient(Client, ERR_IP_CHARCONV_MSG,
					  Client_ID(Client), encoding);

	return IRC_WriteStrClient(Client, RPL_IP_CHARCONV_MSG,
				  Client_ID(Client), encoding);
} /* IRC_CHARCONV */

#endif

/* -eof- */
