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
 * IRC commands for mode changes (like MODE, AWAY, etc.)
 */

#include "imp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "conn.h"
#include "channel.h"
#include "irc-write.h"
#include "lists.h"
#include "log.h"
#include "parse.h"
#include "messages.h"
#include "conf.h"

#include "exp.h"
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
 * See RFC 2812 section 3.1.5 ("user mode message") and section 3.2.3
 * ("channel mode message"), and RFC 2811 section 4 ("channel modes").
 *
 * @param Client	The client from which this command has been received.
 * @param Req		Request structure with prefix and all parameters.
 * @returns		CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_MODE( CLIENT *Client, REQUEST *Req )
{
	CLIENT *cl, *origin;
	CHANNEL *chan;

	assert(Client != NULL);
	assert(Req != NULL);

	/* No parameters? */
	if (Req->argc < 1)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	/* Origin for answers */
	if (Client_Type(Client) == CLIENT_SERVER) {
		origin = Client_Search(Req->prefix);
		if (!origin)
			return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
						  Client_ID(Client),
						  Req->prefix);
	} else
		origin = Client;

	/* Channel or user mode? */
	cl = NULL; chan = NULL;
	if (Client_IsValidNick(Req->argv[0]))
		cl = Client_Search(Req->argv[0]);
	if (Channel_IsValidName(Req->argv[0]))
		chan = Channel_Search(Req->argv[0]);

	if (cl)
		return Client_Mode(Client, Req, origin, cl);
	if (chan)
		return Channel_Mode(Client, Req, origin, chan);

	/* No target found! */
	return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
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
 * @param Client	The client from which this command has been received.
 * @param Req		Request structure with prefix and all parameters.
 * @param Origin	The originator of the MODE command (prefix).
 * @param Target	The target (client) of this MODE command.
 * @returns		CONNECTED or DISCONNECTED.
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
			return IRC_WriteStrClient(Client,
						  ERR_USERSDONTMATCH_MSG,
						  Client_ID(Client));
	}

	/* Mode request: let's answer it :-) */
	if (Req->argc == 1)
		return IRC_WriteStrClient(Origin, RPL_UMODEIS_MSG,
					  Client_ID(Origin),
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
		return IRC_WriteStrClient(Origin, ERR_UMODEUNKNOWNFLAG_MSG,
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
		case 'C': /* Only messages from clients sharing a channel */
		case 'i': /* Invisible */
		case 's': /* Server messages */
		case 'w': /* Wallops messages */
			x[0] = *mode_ptr;
			break;
		case 'a': /* Away */
			if (Client_Type(Client) == CLIENT_SERVER) {
				x[0] = 'a';
				Client_SetAway(Origin, DEFAULT_AWAY_MSG);
			} else
				ok = IRC_WriteStrClient(Origin,
							ERR_NOPRIVILEGES_MSG,
							Client_ID(Origin));
			break;
		case 'B': /* Bot */
			if (Client_HasMode(Client, 'r'))
				ok = IRC_WriteStrClient(Origin,
							ERR_RESTRICTED_MSG,
							Client_ID(Origin));
			else
				x[0] = 'B';
			break;
		case 'c': /* Receive connect notices
			   * (only settable by IRC operators!) */
			if (!set || Client_Type(Client) == CLIENT_SERVER
			    || Client_OperByMe(Origin))
				x[0] = 'c';
			else
				ok = IRC_WriteStrClient(Origin,
							ERR_NOPRIVILEGES_MSG,
							Client_ID(Origin));
			break;
		case 'o': /* IRC operator (only unsettable!) */
			if (!set || Client_Type(Client) == CLIENT_SERVER) {
				Client_SetOperByMe(Target, false);
				x[0] = 'o';
			} else
				ok = IRC_WriteStrClient(Origin,
							ERR_NOPRIVILEGES_MSG,
							Client_ID(Origin));
			break;
		case 'r': /* Restricted (only settable) */
			if (set || Client_Type(Client) == CLIENT_SERVER)
				x[0] = 'r';
			else
				ok = IRC_WriteStrClient(Origin,
							ERR_RESTRICTED_MSG,
							Client_ID(Origin));
			break;
		case 'R': /* Registered (not [un]settable by clients) */
			if (Client_Type(Client) == CLIENT_SERVER)
				x[0] = 'R';
			else
				ok = IRC_WriteStrClient(Origin,
							ERR_NICKREGISTER_MSG,
							Client_ID(Origin));
			break;
		case 'x': /* Cloak hostname */
			if (Client_HasMode(Client, 'r'))
				ok = IRC_WriteStrClient(Origin,
							ERR_RESTRICTED_MSG,
							Client_ID(Origin));
			else
				x[0] = 'x';
				send_RPL_HOSTHIDDEN_MSG = true;
			break;
		default:
			if (Client_Type(Client) != CLIENT_SERVER) {
				Log(LOG_DEBUG,
				    "Unknown mode \"%c%c\" from \"%s\"!?",
				    set ? '+' : '-', *mode_ptr,
				    Client_ID(Origin));
				ok = IRC_WriteStrClient(Origin,
							ERR_UMODEUNKNOWNFLAG2_MSG,
							Client_ID(Origin),
							set ? '+' : '-',
							*mode_ptr);
				x[0] = '\0';
			} else {
				Log(LOG_DEBUG,
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
			if (send_RPL_HOSTHIDDEN_MSG)
				IRC_WriteStrClient(Client, RPL_HOSTHIDDEN_MSG,
						   Client_ID(Client),
						   Client_HostnameCloaked(Client));
		}
		LogDebug("%s \"%s\": Mode change, now \"%s\".",
			 Client_TypeText(Target), Client_Mask(Target),
			 Client_Modes(Target));
	}

	IRC_SetPenalty(Client, 1);
	return ok;
} /* Client_Mode */


static bool
Channel_Mode_Answer_Request(CLIENT *Origin, CHANNEL *Channel)
{
	char the_modes[COMMAND_LEN], the_args[COMMAND_LEN], argadd[CLIENT_PASS_LEN];
	const char *mode_ptr;

	/* Member or not? -- That's the question! */
	if (!Channel_IsMemberOf(Channel, Origin))
		return IRC_WriteStrClient(Origin, RPL_CHANNELMODEIS_MSG,
			Client_ID(Origin), Channel_Name(Channel), Channel_Modes(Channel));

	/* The sender is a member: generate extended reply */
	strlcpy(the_modes, Channel_Modes(Channel), sizeof(the_modes));
	mode_ptr = the_modes;
	the_args[0] = '\0';

	while(*mode_ptr) {
		switch(*mode_ptr) {
		case 'l':
			snprintf(argadd, sizeof(argadd), " %lu", Channel_MaxUsers(Channel));
			strlcat(the_args, argadd, sizeof(the_args));
			break;
		case 'k':
			strlcat(the_args, " ", sizeof(the_args));
			strlcat(the_args, Channel_Key(Channel), sizeof(the_args));
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
#ifndef STRICT_RFC
	if (!IRC_WriteStrClient(Origin, RPL_CREATIONTIME_MSG,
				  Client_ID(Origin), Channel_Name(Channel),
				  Channel_CreationTime(Channel)))
		return DISCONNECTED;
#endif
	return CONNECTED;
}


/**
 * Handle channel mode and channel-user mode changes
 */
static bool
Channel_Mode(CLIENT *Client, REQUEST *Req, CLIENT *Origin, CHANNEL *Channel)
{
	char the_modes[COMMAND_LEN], the_args[COMMAND_LEN], x[2],
	    argadd[CLIENT_PASS_LEN], *mode_ptr;
	bool connected, set, skiponce, retval, onchannel, modeok, use_servermode;
	int mode_arg, arg_arg, mode_arg_count = 0;
	CLIENT *client;
	long l;
	size_t len;

	if (Channel_IsModeless(Channel))
		return IRC_WriteStrClient(Client, ERR_NOCHANMODES_MSG,
				Client_ID(Client), Channel_Name(Channel));

	/* Mode request: let's answer it :-) */
	if (Req->argc <= 1)
		return Channel_Mode_Answer_Request(Origin, Channel);

	Channel_CheckAdminRights(Channel, Client, Origin,
				 &onchannel, &modeok, &use_servermode);

	if (!onchannel && !modeok)
		return IRC_WriteStrClient(Origin, ERR_NOTONCHANNEL_MSG,
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

		/* Validate modes */
		x[0] = '\0';
		argadd[0] = '\0';
		client = NULL;
		switch (*mode_ptr) {
		/* --- Channel modes --- */
		case 'i': /* Invite only */
		case 'M': /* Only identified nicks can write */
		case 'm': /* Moderated */
		case 'n': /* Only members can write */
		case 'R': /* Registered users only */
		case 's': /* Secret channel */
		case 't': /* Topic locked */
		case 'z': /* Secure connections only */
			if (modeok)
				x[0] = *mode_ptr;
			else
				connected = IRC_WriteStrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin), Channel_Name(Channel));
			break;
		case 'k': /* Channel key */
			if (Mode_Limit_Reached(Client, mode_arg_count++))
				goto chan_exit;
			if (!set) {
				if (modeok)
					x[0] = *mode_ptr;
				else
					connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				break;
			}
			if (arg_arg > mode_arg) {
				if (modeok) {
					Channel_ModeDel(Channel, 'k');
					Channel_SetKey(Channel,
						       Req->argv[arg_arg]);
					strlcpy(argadd, Channel_Key(Channel),
						sizeof(argadd));
					x[0] = *mode_ptr;
				} else {
					connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				}
				Req->argv[arg_arg][0] = '\0';
				arg_arg++;
			} else {
				connected = IRC_WriteStrClient(Origin,
					ERR_NEEDMOREPARAMS_MSG,
					Client_ID(Origin), Req->command);
				goto chan_exit;
			}
			break;
		case 'l': /* Member limit */
			if (Mode_Limit_Reached(Client, mode_arg_count++))
				goto chan_exit;
			if (!set) {
				if (modeok)
					x[0] = *mode_ptr;
				else
					connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				break;
			}
			if (arg_arg > mode_arg) {
				if (modeok) {
					l = atol(Req->argv[arg_arg]);
					if (l > 0 && l < 0xFFFF) {
						Channel_ModeDel(Channel, 'l');
						Channel_SetMaxUsers(Channel, l);
						snprintf(argadd, sizeof(argadd),
							 "%ld", l);
						x[0] = *mode_ptr;
					}
				} else {
					connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				}
				Req->argv[arg_arg][0] = '\0';
				arg_arg++;
			} else {
				connected = IRC_WriteStrClient(Origin,
					ERR_NEEDMOREPARAMS_MSG,
					Client_ID(Origin), Req->command);
				goto chan_exit;
			}
			break;
		case 'O': /* IRC operators only */
			if (modeok) {
				/* Only IRC operators are allowed to
				 * set the 'O' channel mode! */
				if (set && !(Client_OperByMe(Client)
				    || Client_Type(Client) == CLIENT_SERVER))
					connected = IRC_WriteStrClient(Origin,
						ERR_NOPRIVILEGES_MSG,
						Client_ID(Origin));
				else
					x[0] = 'O';
			} else
				connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				break;
		case 'P': /* Persistent channel */
			if (modeok) {
				/* Only IRC operators are allowed to
				 * set the 'P' channel mode! */
				if (set && !(Client_OperByMe(Client)
				    || Client_Type(Client) == CLIENT_SERVER))
					connected = IRC_WriteStrClient(Origin,
						ERR_NOPRIVILEGES_MSG,
						Client_ID(Origin));
				else
					x[0] = 'P';
			} else
				connected = IRC_WriteStrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin),
					Channel_Name(Channel));
			break;
		/* --- Channel user modes --- */
		case 'a':
		case 'h':
		case 'q':
			if (Client_Type(Client) != CLIENT_SERVER) {
				connected = IRC_WriteStrClient(Origin,
					ERR_CHANOPRIVSNEEDED_MSG,
					Client_ID(Origin),
					Channel_Name(Channel));
				goto chan_exit;
			}
		case 'o': /* Channel operator */
		case 'v': /* Voice */
			if (arg_arg > mode_arg) {
				if (modeok) {
					client = Client_Search(Req->argv[arg_arg]);
					if (client)
						x[0] = *mode_ptr;
					else
						connected = IRC_WriteStrClient(Client,
							ERR_NOSUCHNICK_MSG,
							Client_ID(Client),
							Req->argv[arg_arg]);
				} else {
					connected = IRC_WriteStrClient(Origin,
						ERR_CHANOPRIVSNEEDED_MSG,
						Client_ID(Origin),
						Channel_Name(Channel));
				}
				Req->argv[arg_arg][0] = '\0';
				arg_arg++;
			} else {
				connected = IRC_WriteStrClient(Origin,
					ERR_NEEDMOREPARAMS_MSG,
					Client_ID(Origin), Req->command);
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
				if (modeok) {
					connected = set
					   ? Add_To_List(*mode_ptr, Origin,
						Client, Channel,
						Req->argv[arg_arg])
					   : Del_From_List(*mode_ptr, Origin,
						Client, Channel,
						Req->argv[arg_arg]);
				} else {
					connected = IRC_WriteStrClient(Origin,
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
				Log(LOG_DEBUG,
				    "Unknown mode \"%c%c\" from \"%s\" on %s!?",
				    set ? '+' : '-', *mode_ptr,
				    Client_ID(Origin), Channel_Name(Channel));
				connected = IRC_WriteStrClient(Origin,
					ERR_UNKNOWNMODE_MSG,
					Client_ID(Origin), *mode_ptr,
					Channel_Name(Channel));
				x[0] = '\0';
			} else {
				Log(LOG_DEBUG,
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
			if (!IRC_WriteStrClient
			    (Origin, ERR_USERNOTINCHANNEL_MSG,
			     Client_ID(Origin), Client_ID(client),
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
			if (Channel_IsLocal(Channel)) {
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

	IRC_SetPenalty(Client, 1);
	return connected;
} /* Channel_Mode */


GLOBAL bool
IRC_AWAY( CLIENT *Client, REQUEST *Req )
{
	assert( Client != NULL );
	assert( Req != NULL );

	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if(( Req->argc == 1 ) && (Req->argv[0][0] ))
	{
		Client_SetAway( Client, Req->argv[0] );
		Client_ModeAdd( Client, 'a' );
		IRC_WriteStrServersPrefix( Client, Client, "MODE %s :+a", Client_ID( Client ));
		return IRC_WriteStrClient( Client, RPL_NOWAWAY_MSG, Client_ID( Client ));
	}
	else
	{
		Client_ModeDel( Client, 'a' );
		IRC_WriteStrServersPrefix( Client, Client, "MODE %s :-a", Client_ID( Client ));
		return IRC_WriteStrClient( Client, RPL_UNAWAY_MSG, Client_ID( Client ));
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
	const char *mask;
	struct list_head *list = NULL;
	long int current_count;

	assert(Client != NULL);
	assert(Channel != NULL);
	assert(Pattern != NULL);
	assert(what == 'I' || what == 'b' || what == 'e');

	mask = Lists_MakeMask(Pattern);
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
		return IRC_WriteStrClient(Client, ERR_LISTFULL_MSG,
					  Client_ID(Client),
					  Channel_Name(Channel), mask,
					  MAX_HNDL_CHANNEL_LISTS);

	switch (what) {
		case 'I':
			if (!Channel_AddInvite(Channel, mask, false))
				return CONNECTED;
			break;
		case 'b':
			if (!Channel_AddBan(Channel, mask))
				return CONNECTED;
			break;
		case 'e':
			if (!Channel_AddExcept(Channel, mask))
				return CONNECTED;
			break;
	}
	return Send_ListChange(true, what, Prefix, Client, Channel, mask);
}


/**
 * Delete entries from channel invite, ban and exeption lists.
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
	const char *mask;
	struct list_head *list = NULL;

	assert(Client != NULL);
	assert(Channel != NULL);
	assert(Pattern != NULL);
	assert(what == 'I' || what == 'b' || what == 'e');

	mask = Lists_MakeMask(Pattern);

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
