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

#ifndef __irc_macros_h__
#define __irc_macros_h__

/**
 * @file
 * Macros for functions that handle IRC commands.
 */

/**
 * Make sure that number of passed parameters is equal to Count.
 *
 * If there are not exactly Count parameters, send an error to the client and
 * return from the function.
 */
#define _IRC_ARGC_EQ_OR_RETURN_(Client, Req, Count) \
if (Req->argc != Count) { \
	return IRC_WriteErrClient(Client, ERR_NEEDMOREPARAMS_MSG, \
				  Client_ID(Client), Req->command); \
}

/**
 * Make sure that number of passed parameters is less or equal than Max.
 *
 * If there are more than Max parameters, send an error to the client and
 * return from the function.
 */
#define _IRC_ARGC_LE_OR_RETURN_(Client, Req, Max) \
if (Req->argc > Max) { \
	return IRC_WriteErrClient(Client, ERR_NEEDMOREPARAMS_MSG, \
				  Client_ID(Client), Req->command); \
}

/**
 * Make sure that number of passed parameters is greater or equal than Min.
 *
 * If there aren't at least Min parameters, send an error to the client and
 * return from the function.
 */
#define _IRC_ARGC_GE_OR_RETURN_(Client, Req, Min) \
if (Req->argc < Min) { \
	return IRC_WriteErrClient(Client, ERR_NEEDMOREPARAMS_MSG, \
				  Client_ID(Client), Req->command); \
}

/**
 * Make sure that number of passed parameters is in between Min and Max.
 *
 * If there aren't at least Min parameters or if there are more than Max
 * parameters, send an error to the client and return from the function.
 */
#define _IRC_ARGC_BETWEEN_OR_RETURN_(Client, Req, Min, Max) \
if (Req->argc < Min || Req->argc > Max) { \
	return IRC_WriteErrClient(Client, ERR_NEEDMOREPARAMS_MSG, \
				  Client_ID(Client), Req->command); \
}

/**
 * Make sure that the command has a prefix.
 *
 * If there is no prefix, send an error to the client and return from
 * the function.
 */
#define _IRC_REQUIRE_PREFIX_OR_RETURN_(Client, Req) \
if (!Req->prefix) { \
	return IRC_WriteErrClient(Client, ERR_NEEDMOREPARAMS_MSG, \
				  Client_ID(Client), Req->command); \
}

/**
 * Get sender of an IRC command.
 *
 * The sender is either stored in the prefix if the command has been
 * received from a server or set to the client. If the sender is invalid,
 * send an error to the client and return from the function.
 */
#define _IRC_GET_SENDER_OR_RETURN_(Sender, Req, Client) \
	if (Client_Type(Client) == CLIENT_SERVER) { \
		if (!Req->prefix) \
			return IRC_WriteErrClient(Client, ERR_NEEDMOREPARAMS_MSG, \
						  Client_ID(Client), Req->command); \
		Sender = Client_Search(Req->prefix); \
	} else \
		Sender = Client; \
	if (!Sender) \
		return IRC_WriteErrClient(Client, ERR_NOSUCHNICK_MSG, \
					  Client_ID(Client), \
					  Req->prefix ? Req->prefix : "(none)");

/**
 * Get target of an IRC command and make sure that it is a server.
 *
 * Set the target to the local server if no target parameter is given in the
 * received command, and send an error to the client and return from the
 * function if the given target isn't resolvable to a server: the target
 * parameter can be a server name, a nick name (then the target is set to
 * the server to which this nick is connected), or a mask matching at least
 * one server name in the network.
 */
#define _IRC_GET_TARGET_SERVER_OR_RETURN_(Target, Req, Argc, From) \
	if (Req->argc > Argc) { \
		Target = Client_Search(Req->argv[Argc]); \
		if (!Target) \
			Target = Client_SearchServer(Req->argv[Argc]); \
		if (!Target) \
			return IRC_WriteErrClient(From, ERR_NOSUCHSERVER_MSG, \
					  Client_ID(From), Req->argv[Argc]); \
		if (Client_Type(Target) != CLIENT_SERVER) \
			Target = Client_Introducer(Target); \
	} else \
		Target = Client_ThisServer();

#endif	/* __irc_macros_h__ */

/* -eof- */
