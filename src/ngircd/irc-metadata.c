/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2015 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#define __irc_metadata_c__

#include "portab.h"

/**
 * @file
 * IRC metadata commands
 */

#include <assert.h>
#include <strings.h>
#include <stdio.h>

#include "conn-func.h"
#include "channel.h"
#include "irc-macros.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"
#include "parse.h"

#include "irc-metadata.h"

/**
 * Handler for the IRC+ "METADATA" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @returns CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_METADATA(CLIENT *Client, REQUEST *Req)
{
	CLIENT *prefix, *target;
	char new_flags[COMMAND_LEN];

	assert(Client != NULL);
	assert(Req != NULL);

	_IRC_REQUIRE_PREFIX_OR_RETURN_(Client, Req)

	prefix = Client_Search(Req->prefix);
	if (!prefix)
		return IRC_WriteErrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->prefix);

	target = Client_Search(Req->argv[0]);
	if (!target)
		return IRC_WriteErrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->argv[0]);

	LogDebug("Got \"METADATA\" command from \"%s\" for client \"%s\": \"%s=%s\".",
		 Client_ID(prefix), Client_ID(target),
		 Req->argv[1], Req->argv[2]);

	/* Mark client: it has received a METADATA command */
	if (!Client_HasFlag(target, 'M')) {
		snprintf(new_flags, sizeof new_flags, "%sM",
			 Client_Flags(target));
		Client_SetFlags(target, new_flags);
	}

	if (strcasecmp(Req->argv[1], "cloakhost") == 0) {
		/* Set or remove a "cloaked hostname". */
		Client_UpdateCloakedHostname(target, prefix,
					     *Req->argv[2] ? Req->argv[2] : NULL);
		if (Client_Conn(target) > NONE && Client_HasMode(target, 'x'))
			IRC_WriteStrClientPrefix(target, prefix,
					RPL_HOSTHIDDEN_MSG, Client_ID(target),
					Client_HostnameDisplayed(target));
		/* The Client_UpdateCloakedHostname() function already
		 * forwarded the METADATA command, don't do it twice: */
		return CONNECTED;
	}
	else if (*Req->argv[2] && strcasecmp(Req->argv[1], "host") == 0) {
		Client_SetHostname(target, Req->argv[2]);
		if (Client_Conn(target) > NONE && !Client_HasMode(target, 'x'))
			IRC_WriteStrClientPrefix(target, prefix,
						 RPL_HOSTHIDDEN_MSG, Client_ID(target),
						 Client_HostnameDisplayed(target));
	} else if (strcasecmp(Req->argv[1], "info") == 0)
		Client_SetInfo(target, Req->argv[2]);
	else if (*Req->argv[2] && strcasecmp(Req->argv[1], "user") == 0)
		Client_SetUser(target, Req->argv[2], true);
	else if (strcasecmp(Req->argv[1], "accountname") == 0)
		Client_SetAccountName(target, Req->argv[2]);
	else if (*Req->argv[2] && strcasecmp(Req->argv[1], "certfp") == 0)
		Conn_SetCertFp(Client_Conn(target), Req->argv[2]);
	else
		Log(LOG_WARNING,
		    "Ignored metadata update from \"%s\" for client \"%s\": \"%s=%s\" - unknown key!",
		    Client_ID(Client), Client_ID(target),
		    Req->argv[1], Req->argv[2]);

	/* Forward the METADATA command to peers that support it: */
	IRC_WriteStrServersPrefixFlag(Client, prefix, 'M', "METADATA %s %s :%s",
				Client_ID(target), Req->argv[1], Req->argv[2]);
	return CONNECTED;
}
