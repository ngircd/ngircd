/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2023 Alexander Barton (alex@barton.de) and Contributors.
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
 * IRC commands for mode changes (like MODE, AWAY, etc.)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conn.h"
#include "channel.h"
#include "irc-macros.h"
#include "irc-write.h"
#include "lists.h"
#include "log.h"
#include "parse.h"
#include "messages.h"
#include "conf.h"

#include "irc-mode.h"

static bool Client_Mode PARAMS((CLIENT *Client, REQUEST *Req, CLIENT *Origin,
				CLIENT *Target));
static bool Channel_Mode PARAMS((CLIENT *Client, REQUEST *Req, CLIENT *Origin,
				 CHANNEL *Channel));

static bool Add_To_List PARAMS((char what, CLIENT *Prefix, CLIENT *Client,
				CHANNEL *Channel, const char *Pattern));
static bool Del_From_List PARAMS((char what, CLIENT *Prefix, CLIENT *Client,
				  CHANNEL *Channel, const char *Pattern));

static bool Send_ListChange PARAMS((const bool IsAdd, const char ModeChar,
				    CLIENT *Prefix, CLIENT *Client,
				    CHANNEL *Channel, const char *Mask));

/**
 * Handler for the IRC "MODE" command.
 *
 * This function detects whether user or channel modes should be modified
 * and calls the appropriate sub-functions.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_MODE( CLIENT *Client, REQUEST *Req )
{
	CLIENT *cl, *origin;
	CHANNEL *chan;
	bool is_valid_nick, is_valid_chan;

	assert(Client != NULL);
	assert(Req != NULL);

	_IRC_GET_SENDER_OR_RETURN_(origin, Req, Client)

	/* Test for "fake" MODE commands injected by this local instance,
	 * for example when handling the "DefaultUserModes" settings.
	 * This doesn't harm real commands, because prefixes of regular
	 * clients are checked in Validate_Prefix() and can't be faked. */
	if (Req->prefix && Client_Search(Req->prefix) == Client_ThisServer())
		Client = Client_Search(Req->prefix);

	/* Channel or user mode? */
	is_valid_nick = Client_IsValidNick(Req->argv[0]);
	is_valid_chan = Channel_IsValidName(Req->argv[0]);
	cl = NULL; chan = NULL;
	if (is_valid_nick)
		cl = Client_Search(Req->argv[0]);
	if (is_valid_chan)
		chan = Channel_Search(Req->argv[0]);

	if (cl)
		return Client_Mode(Client, Req, origin, cl);
	if (chan)
		return Channel_Mode(Client, Req, origin, chan);

	/* No target found! */
	if (is_valid_nick)
		return IRC_WriteErrClient(Client, ERR_NOSUCHNICK_MSG,
				Client_ID(Client), Req->argv[0]);
	else
		return IRC_WriteErrClient(Client, ERR_NOSUCHCHANNEL_MSG,
				Client_ID(Client), Req->argv[0]);
} /* IRC_MODE */

/**
 * Check if the "mode limit" for a client has been reached.
 *
 * This limit doesn't apply for servers or services!
 *
 * @param Client The client to check.
 * @param Count The number of modes already handled.
 * @return true if the limit has been reached.
 */
static bool
Mode_Limit_Reached(CLIENT *Client, int Count)
{
	if (Client_Type(Client) == CLIENT_SERVER
	    || Client_Type(Client) == CLIENT_SERVICE)
		return false;
	if (Count < MAX_HNDL_MODES_ARG)
		return false;
	return true;
}

/**
 * Handle client mode requests
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @param Origin The originator of the MODE command (prefix).
 * @param Target The target (client) of this MODE command.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
Client_Mode( CLIENT *Client, REQUEST *Req, CLIENT *Origin, CLIENT *Target )
{
	char the_modes[COMMAND_LEN], x[2], *mode_ptr;
	bool ok, set;
	bool send_RPL_HOSTHIDDEN_MSG = false;
	int mode_arg;
	size_t len;

	/* Is the client allowed to request or change the modes? */
	if (Client_Type(Client) == CLIENT_USER) {
		/* Users are only allowed to manipulate their own modes! */
		if (Target != Client)
			return IRC_WriteErrClient(Client,
						  ERR_USERSDONTMATCH_MSG,
						  Client_ID(Client));
	}

	/* Mode request: let's answer it :-) */
	if (Req->argc == 1)
		return IRC_WriteStrClient(Origin, RPL_UMODEIS_MSG,
					  Client_ID(Target),
					  Client_Modes(Target));

	mode_arg = 1;
	mode_ptr = Req->argv[mode_arg];

	/* Initial state: set or unset modes? */
	if (*mode_ptr == '+') {
		set = true;
		strcpy(the_modes, "+");
	} else if (*mode_ptr == '-') {
		set = false;
		strcpy(the_modes, "-");
	} else
		return IRC_WriteErrClient(Origin, ERR_UMODEUNKNOWNFLAG_MSG,
					  Client_ID(Origin));

	x[1] = '\0';
	ok = CONNECTED;
	while (mode_ptr) {
		mode_ptr++;
		if (!*mode_ptr) {
			/* Try next argument if there's any */
			mode_arg++;
			if (mode_arg < Req->argc)
				mode_ptr = Req->argv[mode_arg];
			else
				break;
		}

		switch(*mode_ptr) {
		  case '+':
		  case '-':
			if ((*mode_ptr == '+' && !set)
			    || (*mode_ptr == '-' && set)) {
				/* Action modifier ("+"/"-") must be changed */
				len = strlen(the_modes) - 1;
				if (the_modes[len] == '+'
				    || the_modes[len] == '-') {
					/* Last character in the "result
					 * string" was an "action", so just
					 * overwrite it with the new action */
					the_modes[len] = *mode_ptr;
				} else {
					/* Append new modifier character to
					 * the resulting mode string */
					x[0] = *mode_ptr;
					strlcat(the_modes, x,
						sizeof(the_modes));
				}
				if (*mode_ptr == '+')
					set = true;
				else
					set = false;
			}
			continue;
		}

		/* Validate modes */
		x[0] = '\0';
		switch (*mode_ptr) {
		case 'b': /* Block private msgs */
		case 'C': /* Only messages from clients sharing a channel */
		case 'i': /* Invisible */
		case 'I': /* Hide channel list from WHOIS */
		case 's': /* Server messages */
		case 'w': /* Wallops messages */
			x[0] = *mode_ptr;
			break;
		case 'a': /* Away */
			if (Client_Type(Client) == CLIENT_SERVER) {
				x[0] = 'a';
				Client_SetAway(Origin, DEFAULT_AWAY_MSG);
			} else
				ok = IRC_WriteErrClient(Origin,
							ERR_NOPRIVILEGES_MSG,
							Client_ID(Origin));
			break;
		case 'B': /* Bot */
			if (Client_HasMode(Client, 'r'))
				ok = IRC_WriteErrClient(Origin,
							ERR_RESTRICTED_MSG,
							Client_ID(Origin));
			else
				x[0] = 'B';
			break;
		case 'c': /* Receive connect notices */
		case 'q': /* KICK-protected user */
		case 'F': /* disable flood protection */
			  /* (only settable by IRC operators!) */
			if (!set || Client_Type(Client) == CLIENT_SERVER
			    || Client_HasMode(Origin, 'o'))
				x[0] = *mode_ptr;
			else
				ok = IRC_WriteErrClient(Origin,
							ERR_NOPRIVILEGES_MSG,
							Client_ID(Origin));
			break;
		case 'o': /* IRC operator (only unsettable!) */
			if (!set || Client_Type(Client) == CLIENT_SERVER) {
				x[0] = 'o';
			} else
				ok = IRC_WriteErrClient(Origin,
							ERR_NOPRIVILEGES_MSG,
							Client_ID(Origin));
			break;
		case 'r': /* Restricted (only settable) */
			if (set || Client_Type(Client) == CLIENT_SERVER)
				x[0] = 'r';
			else
				ok = IRC_WriteErrClient(Origin,
							ERR_RESTRICTED_MSG,
							Client_ID(Origin));
			break;
		case 'R': /* Registered (not [un]settable by clients) */
			if (Client_Type(Client) == CLIENT_SERVER)
				x[0] = 'R';
			else
				ok = IRC_WriteErrClient(Origin,
							ERR_NICKREGISTER_MSG,
							Client_ID(Origin));
			break;
		case 'x': /* Cloak hostname */
			if (Client_HasMode(Client, 'r'))
				ok = IRC_WriteErrClient(Origin,
							ERR_RESTRICTED_MSG,
							Client_ID(Origin));
			else if (!set || Conf_CloakHostModeX[0]
				 || Client_Type(Client) == CLIENT_SERVER
				 || Client_HasMode(Origin, 'o')) {
				x[0] = 'x';
				send_RPL_HOSTHIDDEN_MSG = true;
			} else
				ok = IRC_WriteErrClient(Origin,
							ERR_NOPRIVILEGES_MSG,
							Client_ID(Origin));
			break;
		default:
			if (Client_Type(Client) != CLIENT_SERVER) {
				LogDebug(
				    "Unknown mode \"%c%c\" from \"%s\"!?",
				    set ? '+' : '-', *mode_ptr,
				    Client_ID(Origin));
				ok = IRC_WriteErrClient(Origin,
							ERR_UMODEUNKNOWNFLAG2_MSG,
							Client_ID(Origin),
							set ? '+' : '-',
							*mode_ptr);
				x[0] = '\0';
			} else {
				LogDebug(
				    "Handling unknown mode \"%c%c\" from \"%s\" for \"%s\" ...",
				    set ? '+' : '-', *mode_ptr,
				    Client_ID(Origin), Client_ID(Target));
				x[0] = *mode_ptr;
			}
		}

		if (!ok)
			break;

		/* Is there a valid mode change? */
		if (!x[0])
			continue;

		if (set) {
			if (Client_ModeAdd(Target, x[0]))
				strlcat(the_modes, x, sizeof(the_modes));
		} else {
			if (Client_ModeDel(Target, x[0]))
				strlcat(the_modes, x, sizeof(the_modes));
		}
	}

	/* Are there changed modes? */
	if (the_modes[1]) {
		/* Remove needless action modifier characters */
		len = strlen(the_modes) - 1;
		if (the_modes[len] == '+' || the_modes[len] == '-')
			the_modes[len] = '\0';

		if (Client_Type(Client) == CLIENT_SERVER) {
			/* Forward modes to other servers */
			if (Client_Conn(Target) != NONE) {
				/* Remote server (service?) changed modes
				 * for one of our clients. Inform it! */
				IRC_WriteStrClientPrefix(Target, Origin,
							 "MODE %s :%s",
							 Client_ID(Target),
							 the_modes);
			}
			IRC_WriteStrServersPrefix(Client, Origin,
						  "MODE %s :%s",
						  Client_ID(Target),
						  the_modes);
		} else {
			/* Send reply to client and inform other servers */
			ok = IRC_WriteStrClientPrefix(Client, Origin,
						      "MODE %s :%s",
						      Client_ID(Target),
						      the_modes);
			IRC_WriteStrServersPrefix(Client, Origin,
						  "MODE %s :%s",
						  Client_ID(Target),
						  the_modes);
		}

		if (send_RPL_HOSTHIDDEN_MSG && Client_Conn(Target) > NONE) {
			/* A new (cloaked) hostname must be announced */
			IRC_WriteStrClientPrefix(Target, Origin,
						 RPL_HOSTHIDDEN_MSG,
						 Client_ID(Target),
						 Client_HostnameDisplayed(Target));

		}

		LogDebug("%s \"%s\": Mode change, now \"%s\".",
			 Client_TypeText(Target), Client_Mask(Target),
			 Client_Modes(Target));
	}

	return ok;
} /* Client_Mode */

/*
 * Reply to a channel mode request.
 *
 * @param Origin The originator of the MODE command (prefix).
 * @param Channel The channel of which the modes should be sent.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
Channel_Mode_Answer_Request(CLIENT *Origin, CHANNEL *Channel)
{
	char the_modes[COMMAND_LEN], the_args[COMMAND_LEN], argadd[CLIENT_PASS_LEN];
	const char *mode_ptr;

	if (!Channel_IsMemberOf(Channel, Origin)) {
		/* Not a member: "simple" mode reply */
		if (!IRC_WriteStrClient(Origin, RPL_CHANNELMODEIS_MSG,
					Client_ID(Origin), Channel_Name(Channel),
					Channel_Modes(Channel)))
			return DISCONNECTED;
	} else {
		/* The sender is a member: generate extended reply */
		strlcpy(the_modes, Channel_Modes(Channel), sizeof(the_modes));
		mode_ptr = the_modes;
		the_args[0] = '\0';

		while(*mode_ptr) {
			switch(*mode_ptr) {
			case 'l':
				snprintf(argadd, sizeof(argadd), " %lu",
					 Channel_MaxUsers(Channel));
				strlcat(the_args, argadd, sizeof(the_args));
				break;
			case 'k':
				strlcat(the_args, " ", sizeof(the_args));
				strlcat(the_args, Channel_Key(Channel),
					sizeof(the_args));
				break;
			}
			mode_ptr++;
		}
		if (the_args[0])
			strlcat(the_modes, the_args, sizeof(the_modes));

		if (!IRC_WriteStrClient(Origin, RPL_CHANNELMODEIS_MSG,
					Client_ID(Origin), Channel_Name(Channel),
					the_modes))
			return DISCONNECTED;
	}

#ifndef STRICT_RFC
	/* Channel creation time */
	if (!IRC_WriteStrClient(Origin, RPL_CREATIONTIME_MSG,
				  Client_ID(Origin), Channel_Name(Channel),
				  Channel_CreationTime(Channel)))
		return DISCONNECTED;
#endif
	return CONNECTED;
}

/**
 * Handle channel mode and channel-user mode changes
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @param Origin The originator of the MODE command (prefix).
 * @param Channel The target channel of this MODE command.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
Channel_Mode(CLIENT *Client, REQUEST *Req, CLIENT *Origin, CHANNEL *Channel)
{
	char the_modes[COMMAND_LEN], the_args[COMMAND_LEN], x[2],
	    argadd[CLIENT_PASS_LEN], *mode_ptr;
	bool connected, set, skiponce, retval, use_servermode,
	     is_halfop, is_op, is_admin, is_owner, is_machine, is_oper;
	int mode_arg, arg_arg, mode_arg_count = 0;
	CLIENT *client;
	long l;
	size_t len;

	is_halfop = is_op = is_admin = is_owner = is_machine = is_oper = false;

	if (Channel_IsModeless(Channel))
		return IRC_WriteErrClient(Client, ERR_NOCHANMODES_MSG,
				Client_ID(Client), Channel_Name(Channel));

	/* Mode request: let's answer it :-) */
	if (Req->argc <= 1)
		return Channel_Mode_Answer_Request(Origin, Channel);

	/* Check if origin is oper and opers can use mode */
	use_servermode = Conf_OperServerMode;
	if(Client_HasMode(Client, 'o') && Conf_OperCanMode) {
		is_oper = true;
	}

	/* Check if client is a server/service */
	if(Client_Type(Client) == CLIENT_SERVER ||
	   Client_Type(Client) == CLIENT_SERVICE) {
		is_machine = true;
	}

	/* Check if client is member of channel or an oper or an server/service */
	if(!Channel_IsMemberOf(Channel, Client) && !is_oper && !is_machine)
		return IRC_WriteErrClient(Origin, ERR_NOTONCHANNEL_MSG,
					  Client_ID(Origin),
					  Channel_Name(Channel));

	mode_arg = 1;
	mode_ptr = Req->argv[mode_arg];
	if (Req->argc > mode_arg + 1)
		arg_arg = mode_arg + 1;
	else
		arg_arg = -1;

	/* Initial state: set or unset modes? */
	skiponce = false;
	switch (*mode_ptr) {
	case '-':
		set = false;
		break;
	case '+':
		set = true;
		break;
	default:
		set = true;
		skiponce = true;
	}

	/* Prepare reply string */
	strcpy(the_modes, set ? "+" : "-");
	the_args[0] = '\0';

	x[1] = '\0';
	connected = CONNECTED;
	while (mode_ptr) {
		if (!skiponce)
			mode_ptr++;
		if (!*mode_ptr) {
			/* Try next argument if there's any */
			if (arg_arg < 0)
				break;
			if (arg_arg > mode_arg)
				mode_arg = arg_arg;
			else
				mode_arg++;

			if (mode_arg >= Req->argc)
				break;
			mode_ptr = Req->argv[mode_arg];

			if (Req->argc > mode_arg + 1)
				arg_arg = mode_arg + 1;
			else
				arg_arg = -1;
		}
		skiponce = false;

		switch (*mode_ptr) {
		case '+':
		case '-':
			if (((*mode_ptr == '+') && !set)
			    || ((*mode_ptr == '-') && set)) {
				/* Action modifier ("+"/"-") must be changed ... */
				len = strlen(the_modes) - 1;
				if (the_modes[len] == '+' || the_modes[len] == '-') {
					/* Adjust last action modifier in result */
					the_modes[len] = *mode_ptr;
				} else {
					/* Append modifier character to result string */
					x[0] = *mode_ptr;
					strlcat(the_modes, x, sizeof(the_modes));
				}
				set = *mode_ptr == '+';
			}
			continue;
		}

		/* Are there arguments left? */
		if (arg_arg >= Req->argc)
			arg_arg = -1;

		if(!is_machine && !is_oper) {
			if (Channel_UserHasMode(Channel, Client, 'q'))
				is_owner = true;
			if (Channel_UserHasMode(Channel, Client, 'a'))
				is_admin = true;
			if (Channel_UserHasMode(Channel, Client, 'o'))
				is_op = true;
			if (Channel_UserHasMode(Channel, Client, 'h'))
				is_halfop = true;
		}

		/* Validate modes */
		x[0] = '\0';
		argadd[0] = '\0';
		client = NULL;
		switch (*mode_ptr) {
		/* --- Channel modes --- */
		case 'R': /* Registered users only */
		case 's': /* Secret channel */
		case 'z': /* Secure connections only */
			if(!is_oper && !is_machine && !is_owner &&
			   !is_admin && !is_op) {
				connected = IRC_WriteErrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin), Channel_Name(Channel));
				goto chan_exit;
			}
			/* fall through */
		case 'i': /* Invite only */
		case 'V': /* Invite disallow */
		case 'M': /* Only identified nicks can write */
		case 'm': /* Moderated */
		case 'n': /* Only members can write */
		case 'N': /* Can't change nick while on this channel */
		case 'Q': /* No kicks */
		case 't': /* Topic locked */
			if(is_oper || is_machine || is_owner ||
			   is_admin || is_op || is_halfop)
				x[0] = *mode_ptr;
			else
				connected = IRC_WriteErrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin), Channel_Name(Channel));
			break;
		case 'k': /* Channel key */
			if (Mode_Limit_Reached(Client, mode_arg_count++))
				goto chan_exit;
			if (!set) {
				if (is_oper || is_machine || is_owner ||
				    is_admin || is_op || is_halfop) {
					x[0] = *mode_ptr;
					if (Channel_HasMode(Channel, 'k'))
						strlcpy(argadd, "*", sizeof(argadd));
					if (arg_arg > mode_arg)
						arg_arg++;
				} else
					connected = IRC_WriteErrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				break;
			}
			if (arg_arg <= mode_arg) {
				if (is_machine)
					Log(LOG_ERR,
					    "Got MODE +k without key for \"%s\" from \"%s\"! Ignored.",
					    Channel_Name(Channel), Client_ID(Origin));
				else
					connected = IRC_WriteErrClient(Origin,
						ERR_NEEDMOREPARAMS_MSG,
						Client_ID(Origin), Req->command);
				goto chan_exit;
			}
			if (!Req->argv[arg_arg][0] || strchr(Req->argv[arg_arg], ' ')) {
				if (is_machine)
					Log(LOG_ERR,
					    "Got invalid key on MODE +k for \"%s\" from \"%s\"! Ignored.",
					    Channel_Name(Channel), Client_ID(Origin));
				else
					connected = IRC_WriteErrClient(Origin,
					       ERR_INVALIDMODEPARAM_MSG,
						Client_ID(Origin),
						Channel_Name(Channel), 'k');
				goto chan_exit;
			}
			if (is_oper || is_machine || is_owner ||
			    is_admin || is_op || is_halfop) {
				Channel_ModeDel(Channel, 'k');
				Channel_SetKey(Channel, Req->argv[arg_arg]);
				strlcpy(argadd, Channel_Key(Channel), sizeof(argadd));
				x[0] = *mode_ptr;
			} else {
				connected = IRC_WriteErrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin),
					Channel_Name(Channel));
			}
			Req->argv[arg_arg][0] = '\0';
			arg_arg++;
			break;
		case 'l': /* Member limit */
			if (Mode_Limit_Reached(Client, mode_arg_count++))
				goto chan_exit;
			if (!set) {
				if (is_oper || is_machine || is_owner ||
				    is_admin || is_op || is_halfop)
					x[0] = *mode_ptr;
				else
					connected = IRC_WriteErrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				break;
			}
			if (arg_arg <= mode_arg) {
				if (is_machine)
					Log(LOG_ERR,
					    "Got MODE +l without limit for \"%s\" from \"%s\"! Ignored.",
					    Channel_Name(Channel), Client_ID(Origin));
				else
					connected = IRC_WriteErrClient(Origin,
						ERR_NEEDMOREPARAMS_MSG,
						Client_ID(Origin), Req->command);
				goto chan_exit;
			}
			l = atol(Req->argv[arg_arg]);
			if (l <= 0 || l >= 0xFFFF) {
				if (is_machine)
					Log(LOG_ERR,
					    "Got MODE +l with invalid limit for \"%s\" from \"%s\"! Ignored.",
					    Channel_Name(Channel), Client_ID(Origin));
				else
					connected = IRC_WriteErrClient(Origin,
						ERR_INVALIDMODEPARAM_MSG,
						Client_ID(Origin),
						Channel_Name(Channel), 'l');
				goto chan_exit;
			}
			if (is_oper || is_machine || is_owner ||
			    is_admin || is_op || is_halfop) {
				Channel_ModeDel(Channel, 'l');
				Channel_SetMaxUsers(Channel, l);
				snprintf(argadd, sizeof(argadd), "%ld", l);
				x[0] = *mode_ptr;
			} else {
				connected = IRC_WriteErrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin),
					Channel_Name(Channel));
			}
			Req->argv[arg_arg][0] = '\0';
			arg_arg++;
			break;
		case 'O': /* IRC operators only */
			if (set) {
				/* Only IRC operators are allowed to
				 * set the 'O' channel mode! */
				if(is_oper || is_machine)
					x[0] = 'O';
				else
					connected = IRC_WriteErrClient(Origin,
						ERR_NOPRIVILEGES_MSG,
						Client_ID(Origin));
			} else if(is_oper || is_machine || is_owner ||
				  is_admin || is_op)
				x[0] = 'O';
			else
				connected = IRC_WriteErrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin),
					Channel_Name(Channel));
			break;
		case 'P': /* Persistent channel */
			if (set) {
				/* Only IRC operators are allowed to
				 * set the 'P' channel mode! */
				if(is_oper || is_machine)
					x[0] = 'P';
				else
					connected = IRC_WriteErrClient(Origin,
						ERR_NOPRIVILEGES_MSG,
						Client_ID(Origin));
			} else if(is_oper || is_machine || is_owner ||
				  is_admin || is_op)
				x[0] = 'P';
			else
				connected = IRC_WriteErrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin),
					Channel_Name(Channel));
			break;
		/* --- Channel user modes --- */
		case 'q': /* Owner */
			if(!is_oper && !is_machine && !is_owner) {
				connected = IRC_WriteErrClient(Origin,
					ERR_CHANOPPRIVTOOLOW_MSG,
					Client_ID(Origin),
					Channel_Name(Channel));
				goto chan_exit;
			}
			/* fall through */
		case 'a': /* Channel admin */
			if(!is_oper && !is_machine && !is_owner && !is_admin) {
				connected = IRC_WriteErrClient(Origin,
					ERR_CHANOPPRIVTOOLOW_MSG,
					Client_ID(Origin),
					Channel_Name(Channel));
				goto chan_exit;
			}
			/* fall through */
		case 'o': /* Channel operator */
			if(!is_oper && !is_machine && !is_owner &&
			   !is_admin && !is_op) {
				connected = IRC_WriteErrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin),
					Channel_Name(Channel));
				goto chan_exit;
			}
			/* fall through */
		case 'h': /* Half Op */
			if(!is_oper && !is_machine && !is_owner &&
			   !is_admin && !is_op) {
				connected = IRC_WriteErrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin),
					Channel_Name(Channel));
				goto chan_exit;
			}
			/* fall through */
		case 'v': /* Voice */
			if (arg_arg > mode_arg) {
				if (is_oper || is_machine || is_owner ||
				    is_admin || is_op || is_halfop) {
					client = Client_Search(Req->argv[arg_arg]);
					if (client)
						x[0] = *mode_ptr;
					else
						connected = IRC_WriteErrClient(Origin,
							ERR_NOSUCHNICK_MSG,
							Client_ID(Origin),
							Req->argv[arg_arg]);
				} else {
					connected = IRC_WriteErrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				}
				Req->argv[arg_arg][0] = '\0';
				arg_arg++;
			} else {
#ifdef STRICT_RFC
				/* Report an error to the client, when a user
				 * mode should be changed but no nickname is
				 * given. But don't do it when not in "strict"
				 * mode, because most other servers don't do
				 * it as well and some clients send "wired"
				 * MODE commands like "MODE #chan -ooo nick". */
				connected = IRC_WriteErrClient(Origin,
					ERR_NEEDMOREPARAMS_MSG,
					Client_ID(Origin), Req->command);
#endif
				goto chan_exit;
			}
			break;
		/* --- Channel lists --- */
		case 'I': /* Invite lists */
		case 'b': /* Ban lists */
		case 'e': /* Channel exception lists */
			if (Mode_Limit_Reached(Client, mode_arg_count++))
				goto chan_exit;
			if (arg_arg > mode_arg) {
				/* modify list */
				if (is_oper || is_machine || is_owner ||
				    is_admin || is_op || is_halfop) {
					connected = set
					   ? Add_To_List(*mode_ptr, Origin,
						Client, Channel,
						Req->argv[arg_arg])
					   : Del_From_List(*mode_ptr, Origin,
						Client, Channel,
						Req->argv[arg_arg]);
				} else {
					connected = IRC_WriteErrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				}
				Req->argv[arg_arg][0] = '\0';
				arg_arg++;
			} else {
				switch (*mode_ptr) {
				case 'I':
					Channel_ShowInvites(Origin, Channel);
					break;
				case 'b':
					Channel_ShowBans(Origin, Channel);
					break;
				case 'e':
					Channel_ShowExcepts(Origin, Channel);
					break;
				}
			}
			break;
		default:
			if (Client_Type(Client) != CLIENT_SERVER) {
				LogDebug(
				    "Unknown mode \"%c%c\" from \"%s\" on %s!?",
				    set ? '+' : '-', *mode_ptr,
				    Client_ID(Origin), Channel_Name(Channel));
				connected = IRC_WriteErrClient(Origin,
					ERR_UNKNOWNMODE_MSG,
					Client_ID(Origin), *mode_ptr,
					Channel_Name(Channel));
				x[0] = '\0';
			} else {
				LogDebug(
				    "Handling unknown mode \"%c%c\" from \"%s\" on %s ...",
				    set ? '+' : '-', *mode_ptr,
				    Client_ID(Origin), Channel_Name(Channel));
				x[0] = *mode_ptr;
			}
		}

		if (!connected)
			break;

		/* Is there a valid mode change? */
		if (!x[0])
			continue;

		/* Validate target client */
		if (client && (!Channel_IsMemberOf(Channel, client))) {
			if (!IRC_WriteErrClient(Origin, ERR_USERNOTINCHANNEL_MSG,
						Client_ID(Origin),
						Client_ID(client),
						Channel_Name(Channel)))
				break;
			continue;
		}

		if (client) {
			/* Channel-User-Mode */
			retval = set
			       ? Channel_UserModeAdd(Channel, client, x[0])
			       : Channel_UserModeDel(Channel, client, x[0]);
			if (retval) {
				strlcat(the_args, " ", sizeof(the_args));
				strlcat(the_args, Client_ID(client),
					sizeof(the_args));
				strlcat(the_modes, x, sizeof(the_modes));
				LogDebug
				    ("User \"%s\": Mode change on %s, now \"%s\"",
				     Client_Mask(client), Channel_Name(Channel),
				     Channel_UserModes(Channel, client));
			}
		} else {
			/* Channel-Mode */
			retval = set
			       ? Channel_ModeAdd(Channel, x[0])
			       : Channel_ModeDel(Channel, x[0]);
			if (retval) {
				strlcat(the_modes, x, sizeof(the_modes));
				LogDebug("Channel %s: Mode change, now \"%s\".",
					 Channel_Name(Channel),
					 Channel_Modes(Channel));
			}
		}

		/* Are there additional arguments to add? */
		if (argadd[0]) {
			strlcat(the_args, " ", sizeof(the_args));
			strlcat(the_args, argadd, sizeof(the_args));
		}
	}

      chan_exit:
	/* Are there changed modes? */
	if (the_modes[1]) {
		/* Clean up mode string */
		len = strlen(the_modes) - 1;
		if ((the_modes[len] == '+') || (the_modes[len] == '-'))
			the_modes[len] = '\0';

		if (Client_Type(Client) == CLIENT_SERVER) {
			/* MODE requests for local channels from other servers
			 * are definitely invalid! */
			if (Channel_IsLocal(Channel) && Client != Client_ThisServer()) {
				Log(LOG_ALERT, "Got remote MODE command for local channel!? Ignored.");
				return CONNECTED;
			}

			/* Forward mode changes to channel users and all the
			 * other remote servers: */
			IRC_WriteStrServersPrefix(Client, Origin,
				"MODE %s %s%s", Channel_Name(Channel),
				the_modes, the_args);
			IRC_WriteStrChannelPrefix(Client, Channel, Origin,
				false, "MODE %s %s%s", Channel_Name(Channel),
				the_modes, the_args);
		} else {
			if (use_servermode)
				Origin = Client_ThisServer();
			/* Send reply to client and inform other servers and channel users */
			connected = IRC_WriteStrClientPrefix(Client, Origin,
					"MODE %s %s%s", Channel_Name(Channel),
					the_modes, the_args);
			/* Only forward requests for non-local channels */
			if (!Channel_IsLocal(Channel))
				IRC_WriteStrServersPrefix(Client, Origin,
					"MODE %s %s%s", Channel_Name(Channel),
					the_modes, the_args);
			IRC_WriteStrChannelPrefix(Client, Channel, Origin,
				false, "MODE %s %s%s", Channel_Name(Channel),
				the_modes, the_args);
		}
	}

	return connected;
} /* Channel_Mode */

/**
 * Handler for the IRC "AWAY" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_AWAY( CLIENT *Client, REQUEST *Req )
{
	assert (Client != NULL);
	assert (Req != NULL);

	if (Req->argc == 1 && Req->argv[0][0]) {
		Client_SetAway(Client, Req->argv[0]);
		Client_ModeAdd(Client, 'a');
		IRC_WriteStrServersPrefix(Client, Client, "MODE %s :+a",
					  Client_ID( Client));
		return IRC_WriteStrClient(Client, RPL_NOWAWAY_MSG,
					  Client_ID( Client));
	} else {
		Client_ModeDel(Client, 'a');
		IRC_WriteStrServersPrefix(Client, Client, "MODE %s :-a",
					  Client_ID( Client));
		return IRC_WriteStrClient(Client, RPL_UNAWAY_MSG,
					  Client_ID( Client));
	}
} /* IRC_AWAY */

/**
 * Add entries to channel invite, ban and exception lists.
 *
 * @param what Can be 'I' for invite, 'b' for ban, and 'e' for exception list.
 * @param Prefix The originator of the command.
 * @param Client The sender of the command.
 * @param Channel The channel of which the list should be modified.
 * @param Pattern The pattern to add to the list.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
Add_To_List(char what, CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel,
	    const char *Pattern)
{
	char mask[MASK_LEN];
	struct list_head *list = NULL;
	long int current_count;

	assert(Client != NULL);
	assert(Channel != NULL);
	assert(Pattern != NULL);
	assert(what == 'I' || what == 'b' || what == 'e');

	Lists_MakeMask(Pattern, mask, sizeof(mask));
	current_count = Lists_Count(Channel_GetListInvites(Channel))
			+ Lists_Count(Channel_GetListExcepts(Channel))
			+ Lists_Count(Channel_GetListBans(Channel));

	switch(what) {
		case 'I':
			list = Channel_GetListInvites(Channel);
			break;
		case 'b':
			list = Channel_GetListBans(Channel);
			break;
		case 'e':
			list = Channel_GetListExcepts(Channel);
			break;
	}

	if (Lists_CheckDupeMask(list, mask))
		return CONNECTED;
	if (Client_Type(Client) == CLIENT_USER &&
	    current_count >= MAX_HNDL_CHANNEL_LISTS)
		return IRC_WriteErrClient(Client, ERR_LISTFULL_MSG,
					  Client_ID(Client),
					  Channel_Name(Channel), mask,
					  MAX_HNDL_CHANNEL_LISTS);

	switch (what) {
		case 'I':
			if (!Channel_AddInvite(Channel, mask, false, Client_ID(Client)))
				return CONNECTED;
			break;
		case 'b':
			if (!Channel_AddBan(Channel, mask, Client_ID(Client)))
				return CONNECTED;
			break;
		case 'e':
			if (!Channel_AddExcept(Channel, mask, Client_ID(Client)))
				return CONNECTED;
			break;
	}
	return Send_ListChange(true, what, Prefix, Client, Channel, mask);
}

/**
 * Delete entries from channel invite, ban and exception lists.
 *
 * @param what Can be 'I' for invite, 'b' for ban, and 'e' for exception list.
 * @param Prefix The originator of the command.
 * @param Client The sender of the command.
 * @param Channel The channel of which the list should be modified.
 * @param Pattern The pattern to add to the list.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
Del_From_List(char what, CLIENT *Prefix, CLIENT *Client, CHANNEL *Channel,
	      const char *Pattern)
{
	char mask[MASK_LEN];
	struct list_head *list = NULL;

	assert(Client != NULL);
	assert(Channel != NULL);
	assert(Pattern != NULL);
	assert(what == 'I' || what == 'b' || what == 'e');

	Lists_MakeMask(Pattern, mask, sizeof(mask));

	switch (what) {
		case 'I':
			list = Channel_GetListInvites(Channel);
			break;
		case 'b':
			list = Channel_GetListBans(Channel);
			break;
		case 'e':
			list = Channel_GetListExcepts(Channel);
			break;
	}

	if (!Lists_CheckDupeMask(list, mask))
		return CONNECTED;
	Lists_Del(list, mask);

	return Send_ListChange(false, what, Prefix, Client, Channel, mask);
}

/**
 * Send information about changed channel invite/ban/exception lists to clients.
 *
 * @param IsAdd true if the list item has been added, false otherwise.
 * @param ModeChar The mode to use (e. g. 'b' or 'I')
 * @param Prefix The originator of the mode list change.
 * @param Client The sender of the command.
 * @param Channel The channel of which the list has been modified.
 * @param Mask The mask which has been added or removed.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
Send_ListChange(const bool IsAdd, const char ModeChar, CLIENT *Prefix,
		CLIENT *Client, CHANNEL *Channel, const char *Mask)
{
	bool ok = true;

	/* Send confirmation to the client */
	if (Client_Type(Client) == CLIENT_USER)
		ok = IRC_WriteStrClientPrefix(Client, Prefix, "MODE %s %c%c %s",
					      Channel_Name(Channel),
					      IsAdd ? '+' : '-',
					      ModeChar, Mask);

	/* to other servers */
	IRC_WriteStrServersPrefix(Client, Prefix, "MODE %s %c%c %s",
				  Channel_Name(Channel), IsAdd ? '+' : '-',
				  ModeChar, Mask);

	/* and local users in channel */
	IRC_WriteStrChannelPrefix(Client, Channel, Prefix, false,
				  "MODE %s %c%c %s", Channel_Name(Channel),
				  IsAdd ? '+' : '-', ModeChar, Mask );

	return ok;
} /* Send_ListChange */

/* -eof- */
