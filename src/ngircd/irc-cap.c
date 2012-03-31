/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2012 Alexander Barton (alex@barton.de) and Contributors.
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
 * Handler for IRC capability ("CAP") commands
 */

#include "imp.h"
#include <assert.h>
#include <string.h>

#include "defines.h"
#include "conn.h"
#include "channel.h"
#include "client-cap.h"
#include "irc-write.h"
#include "log.h"
#include "login.h"
#include "messages.h"
#include "parse.h"

#include "exp.h"
#include "irc-cap.h"

bool Handle_CAP_LS PARAMS((CLIENT *Client, char *Arg));
bool Handle_CAP_LIST PARAMS((CLIENT *Client, char *Arg));
bool Handle_CAP_REQ PARAMS((CLIENT *Client, char *Arg));
bool Handle_CAP_ACK PARAMS((CLIENT *Client, char *Arg));
bool Handle_CAP_CLEAR PARAMS((CLIENT *Client));
bool Handle_CAP_END PARAMS((CLIENT *Client));

/**
 * Handler for the IRCv3 "CAP" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @returns CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_CAP(CLIENT *Client, REQUEST *Req)
{
	assert(Client != NULL);
	assert(Req != NULL);

	/* Bad number of prameters? */
	if (Req->argc < 1 || Req->argc > 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	LogDebug("Got \"%s %s\" command from \"%s\" ...",
		 Req->command, Req->argv[0], Client_ID(Client));

	if (Req->argc == 1) {
		if (strcasecmp(Req->argv[0], "CLEAR") == 0)
			return Handle_CAP_CLEAR(Client);
		if (strcasecmp(Req->argv[0], "END") == 0)
			return Handle_CAP_END(Client);
	}
	if (Req->argc >= 1 && Req->argc <= 2) {
		if (strcasecmp(Req->argv[0], "LS") == 0)
			return Handle_CAP_LS(Client, Req->argv[1]);
		if (strcasecmp(Req->argv[0], "LIST") == 0)
			return Handle_CAP_LIST(Client, Req->argv[1]);
	}
	if (Req->argc == 2) {
		if (strcasecmp(Req->argv[0], "REQ") == 0)
			return Handle_CAP_REQ(Client, Req->argv[1]);
		if (strcasecmp(Req->argv[0], "ACK") == 0)
			return Handle_CAP_ACK(Client, Req->argv[1]);
	}

	return IRC_WriteStrClient(Client, ERR_INVALIDCAP_MSG,
				  Client_ID(Client), Req->argv[0]);
}

/**
 * Handler for the "CAP LS" command.
 *
 * @param Client The client from which this command has been received.
 * @param Arg Command argument or NULL.
 * @returns CONNECTED or DISCONNECTED.
 */
bool
Handle_CAP_LS(CLIENT *Client, UNUSED char *Arg)
{
	assert(Client != NULL);

	if (Client_Type(Client) != CLIENT_USER)
		Client_CapAdd(Client, CLIENT_CAP_PENDING);

	Client_CapAdd(Client, CLIENT_CAP_SUPPORTED);
	return IRC_WriteStrClient(Client, "CAP %s LS :", Client_ID(Client));
}

/**
 * Handler for the "CAP LIST" command.
 *
 * @param Client The client from which this command has been received.
 * @param Arg Command argument or NULL.
 * @returns CONNECTED or DISCONNECTED.
 */
bool
Handle_CAP_LIST(CLIENT *Client, UNUSED char *Arg)
{
	assert(Client != NULL);

	return IRC_WriteStrClient(Client, "CAP %s LIST :", Client_ID(Client));
}

/**
 * Handler for the "CAP REQ" command.
 *
 * @param Client The client from which this command has been received.
 * @param Arg Command argument.
 * @returns CONNECTED or DISCONNECTED.
 */
bool
Handle_CAP_REQ(CLIENT *Client, char *Arg)
{
	assert(Client != NULL);
	assert(Arg != NULL);

	return IRC_WriteStrClient(Client, "CAP %s NAK :%s",
				  Client_ID(Client), Arg);
}

/**
 * Handler for the "CAP ACK" command.
 *
 * @param Client The client from which this command has been received.
 * @param Arg Command argument.
 * @returns CONNECTED or DISCONNECTED.
 */
bool
Handle_CAP_ACK(CLIENT *Client, char *Arg)
{
	assert(Client != NULL);
	assert(Arg != NULL);

	return CONNECTED;
}

/**
 * Handler for the "CAP CLEAR" command.
 *
 * @param Client The client from which this command has been received.
 * @returns CONNECTED or DISCONNECTED.
 */
bool
Handle_CAP_CLEAR(CLIENT *Client)
{
	assert(Client != NULL);

	return IRC_WriteStrClient(Client, "CAP %s ACK :", Client_ID(Client));
}

/**
 * Handler for the "CAP END" command.
 *
 * @param Client The client from which this command has been received.
 * @returns CONNECTED or DISCONNECTED.
 */
bool
Handle_CAP_END(CLIENT *Client)
{
	assert(Client != NULL);

	if (Client_Type(Client) != CLIENT_USER) {
		/* User is still logging in ... */
		Client_CapDel(Client, CLIENT_CAP_PENDING);

		if (Client_Type(Client) == CLIENT_GOTUSER) {
			/* Only "CAP END" was missing: log in! */
			return Login_User(Client);
		}
	}

	return CONNECTED;
}

/* -eof- */
