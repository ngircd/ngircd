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
 * IRC info commands
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "ngircd.h"
#include "conn-func.h"
#include "conn-zip.h"
#include "channel.h"
#include "class.h"
#include "conf.h"
#include "lists.h"
#include "messages.h"
#include "match.h"
#include "parse.h"
#include "irc.h"
#include "irc-macros.h"
#include "irc-write.h"
#include "client-cap.h"
#include "op.h"

#include "irc-info.h"

/* Local functions */

static unsigned int
t_diff(time_t *t, const time_t d)
{
	time_t diff, remain;

	diff = *t / d;
	remain = diff * d;
	*t -= remain;

	return (unsigned int)diff;
}

static unsigned int
uptime_days(time_t *now)
{
	return t_diff(now, 60 * 60 * 24);
}

static unsigned int
uptime_hrs(time_t *now)
{
	return t_diff(now, 60 * 60);
}

static unsigned int
uptime_mins(time_t *now)
{
	return t_diff(now, 60);
}

static bool
write_whoreply(CLIENT *Client, CLIENT *c, const char *channelname, const char *flags)
{
	return IRC_WriteStrClient(Client, RPL_WHOREPLY_MSG, Client_ID(Client),
				  channelname, Client_User(c),
				  Client_HostnameDisplayed(c),
				  Client_ID(Client_Introducer(c)), Client_ID(c),
				  flags, Client_Hops(c), Client_Info(c));
}

/**
 * Return channel user mode prefix(es).
 *
 * @param Client The client requesting the mode prefixes.
 * @param chan_user_modes String with channel user modes.
 * @param str String buffer to which the prefix(es) will be appended.
 * @param len Size of "str" buffer.
 * @return Pointer to "str".
 */
static char *
who_flags_qualifier(CLIENT *Client, const char *chan_user_modes,
		    char *str, size_t len)
{
	assert(Client != NULL);

	if (Client_Cap(Client) & CLIENT_CAP_MULTI_PREFIX) {
		if (strchr(chan_user_modes, 'q'))
			strlcat(str, "~", len);
		if (strchr(chan_user_modes, 'a'))
			strlcat(str, "&", len);
		if (strchr(chan_user_modes, 'o'))
			strlcat(str, "@", len);
		if (strchr(chan_user_modes, 'h'))
			strlcat(str, "%", len);
		if (strchr(chan_user_modes, 'v'))
			strlcat(str, "+", len);

		return str;
	}

	if (strchr(chan_user_modes, 'q'))
		strlcat(str, "~", len);
	else if (strchr(chan_user_modes, 'a'))
		strlcat(str, "&", len);
	else if (strchr(chan_user_modes, 'o'))
		strlcat(str, "@", len);
	else if (strchr(chan_user_modes, 'h'))
		strlcat(str, "%", len);
	else if (strchr(chan_user_modes, 'v'))
		strlcat(str, "+", len);

	return str;
}

/**
 * Send WHO reply for a "channel target" ("WHO #channel").
 *
 * @param Client Client requesting the information.
 * @param Chan Channel being requested.
 * @param OnlyOps Only display IRC operators.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
IRC_WHO_Channel(CLIENT *Client, CHANNEL *Chan, bool OnlyOps)
{
	bool is_visible, is_member, is_ircop, is_oper;
	CL2CHAN *cl2chan;
	char flags[10];
	CLIENT *c;
	int count = 0;

	assert( Client != NULL );
	assert( Chan != NULL );

	is_member = Channel_IsMemberOf(Chan, Client);
	is_oper = Client_HasMode(Client, 'o');

	/* Secret channel? */
	if (!is_member && !is_oper && Channel_HasMode(Chan, 's'))
		return IRC_WriteStrClient(Client, RPL_ENDOFWHO_MSG,
					  Client_ID(Client), Channel_Name(Chan));

	cl2chan = Channel_FirstMember(Chan);
	for (; cl2chan ; cl2chan = Channel_NextMember(Chan, cl2chan)) {
		c = Channel_GetClient(cl2chan);

		is_ircop = Client_HasMode(c, 'o');
		if (OnlyOps && !is_ircop)
			continue;

		is_visible = !Client_HasMode(c, 'i');
		if (is_member || is_visible || is_oper) {
			memset(flags, 0, sizeof(flags));

			if (Client_HasMode(c, 'a'))
				flags[0] = 'G'; /* away */
			else
				flags[0] = 'H';

			if (is_ircop)
				flags[1] = '*';

			who_flags_qualifier(Client, Channel_UserModes(Chan, c),
					    flags, sizeof(flags));

			if (!write_whoreply(Client, c, Channel_Name(Chan),
					    flags))
				return DISCONNECTED;
			count++;
		}
	}

	/* If there are a lot of clients, increase the penalty a bit */
	if (count > MAX_RPL_WHO)
		IRC_SetPenalty(Client, 1);

	return IRC_WriteStrClient(Client, RPL_ENDOFWHO_MSG, Client_ID(Client),
				  Channel_Name(Chan));
}

/**
 * Send WHO reply for a "mask target" ("WHO m*sk").
 *
 * @param Client Client requesting the information.
 * @param Mask Mask being requested or NULL for "all" clients.
 * @param OnlyOps Only display IRC operators.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
IRC_WHO_Mask(CLIENT *Client, char *Mask, bool OnlyOps)
{
	CLIENT *c;
	CL2CHAN *cl2chan;
	CHANNEL *chan;
	bool client_match, is_visible;
	char flags[3];
	int count = 0;

	assert (Client != NULL);

	if (Mask)
		ngt_LowerStr(Mask);

	IRC_SetPenalty(Client, 3);
	for (c = Client_First(); c != NULL; c = Client_Next(c)) {
		if (Client_Type(c) != CLIENT_USER)
			continue;

		if (OnlyOps && !Client_HasMode(c, 'o'))
			continue;

		if (Mask) {
			/* Match pattern against user host/server/name/nick */
			client_match = MatchCaseInsensitive(Mask,
							    Client_Hostname(c));
			if (!client_match)
				client_match = MatchCaseInsensitive(Mask,
								    Client_ID(Client_Introducer(c)));
			if (!client_match)
				client_match = MatchCaseInsensitive(Mask,
								    Client_Info(c));
			if (!client_match)
				client_match = MatchCaseInsensitive(Mask,
								    Client_ID(c));
			if (!client_match)
				continue;	/* no match: skip this client */
		}

		is_visible = !Client_HasMode(c, 'i');

		/* Target client is invisible, but mask matches exactly? */
		if (!is_visible && Mask && strcasecmp(Client_ID(c), Mask) == 0)
			is_visible = true;

		/* Target still invisible, but are both on the same channel? */
		if (!is_visible) {
			cl2chan = Channel_FirstChannelOf(Client);
			while (cl2chan && !is_visible) {
				chan = Channel_GetChannel(cl2chan);
				if (Channel_IsMemberOf(chan, c))
					is_visible = true;
				cl2chan = Channel_NextChannelOf(Client, cl2chan);
			}
		}

		if (!is_visible)	/* target user is not visible */
			continue;

		if (IRC_CheckListTooBig(Client, count, MAX_RPL_WHO, "WHO"))
			break;

		memset(flags, 0, sizeof(flags));

		if (Client_HasMode(c, 'a'))
			flags[0] = 'G'; /* away */
		else
			flags[0] = 'H';

		if (Client_HasMode(c, 'o'))
			flags[1] = '*';

		if (!write_whoreply(Client, c, "*", flags))
			return DISCONNECTED;
		count++;
	}

	return IRC_WriteStrClient(Client, RPL_ENDOFWHO_MSG, Client_ID(Client),
				  Mask ? Mask : "*");
}

/**
 * Generate WHOIS reply of one actual client.
 *
 * @param Client The client from which this command has been received.
 * @param from The client requesting the information ("originator").
 * @param c The client of which information should be returned.
 * @return CONNECTED or DISCONNECTED.
 */
static bool
IRC_WHOIS_SendReply(CLIENT *Client, CLIENT *from, CLIENT *c)
{
	char str[COMMAND_LEN];
	CL2CHAN *cl2chan;
	CHANNEL *chan;

	assert(Client != NULL);
	assert(from != NULL);
	assert(c != NULL);

	/* Nick, user, hostname and client info */
	if (!IRC_WriteStrClient(from, RPL_WHOISUSER_MSG, Client_ID(from),
				Client_ID(c), Client_User(c),
				Client_HostnameDisplayed(c), Client_Info(c)))
		return DISCONNECTED;

	/* Server */
	if (!IRC_WriteStrClient(from, RPL_WHOISSERVER_MSG, Client_ID(from),
				Client_ID(c), Client_ID(Client_Introducer(c)),
				Client_Info(Client_Introducer(c))))
		return DISCONNECTED;

	/* Channels, show only if client has no +I or if from is oper */
	if(!(Client_HasMode(c, 'I')) || Client_HasMode(from, 'o'))  {
		snprintf(str, sizeof(str), RPL_WHOISCHANNELS_MSG,
			 Client_ID(from), Client_ID(c));
		cl2chan = Channel_FirstChannelOf(c);
		while (cl2chan) {
			chan = Channel_GetChannel(cl2chan);
			assert(chan != NULL);

			/* next */
			cl2chan = Channel_NextChannelOf(c, cl2chan);

			/* Secret channel? */
			if (Channel_HasMode(chan, 's')
			    && !Channel_IsMemberOf(chan, Client))
				continue;

			/* Local channel and request is not from a user? */
			if (Client_Type(Client) == CLIENT_SERVER
			    && Channel_IsLocal(chan))
				continue;

			/* Concatenate channel names */
			if (str[strlen(str) - 1] != ':')
				strlcat(str, " ", sizeof(str));

			who_flags_qualifier(Client, Channel_UserModes(chan, c),
			                    str, sizeof(str));
			strlcat(str, Channel_Name(chan), sizeof(str));

			if (strlen(str) > (COMMAND_LEN - CHANNEL_NAME_LEN - 4)) {
				/* Line becomes too long: send it! */
				if (!IRC_WriteStrClient(Client, "%s", str))
					return DISCONNECTED;
				snprintf(str, sizeof(str), RPL_WHOISCHANNELS_MSG,
					 Client_ID(from), Client_ID(c));
			}
		}
		if(str[strlen(str) - 1] != ':') {
			/* There is data left to send: */
			if (!IRC_WriteStrClient(Client, "%s", str))
				return DISCONNECTED;
		}
	}

	/* IRC-Services? */
	if (Client_Type(c) == CLIENT_SERVICE &&
	    !IRC_WriteStrClient(from, RPL_WHOISSERVICE_MSG,
				Client_ID(from), Client_ID(c)))
		return DISCONNECTED;

	/* IRC-Operator? */
	if (Client_Type(c) != CLIENT_SERVICE &&
	    Client_HasMode(c, 'o') &&
	    !IRC_WriteStrClient(from, RPL_WHOISOPERATOR_MSG,
				Client_ID(from), Client_ID(c)))
		return DISCONNECTED;

	/* IRC-Bot? */
	if (Client_HasMode(c, 'B') &&
	    !IRC_WriteStrClient(from, RPL_WHOISBOT_MSG,
				Client_ID(from), Client_ID(c)))
		return DISCONNECTED;

	/* Connected using SSL? */
	if (Conn_UsesSSL(Client_Conn(c))) {
		if (!IRC_WriteStrClient(from, RPL_WHOISSSL_MSG, Client_ID(from),
					Client_ID(c)))
			return DISCONNECTED;

		/* Certificate fingerprint? */
		if (Conn_GetCertFp(Client_Conn(c)) &&
		    from == c &&
		    !IRC_WriteStrClient(from, RPL_WHOISCERTFP_MSG,
					Client_ID(from), Client_ID(c),
					Conn_GetCertFp(Client_Conn(c))))
			return DISCONNECTED;
	}

	/* Registered nickname? */
	if (Client_HasMode(c, 'R') &&
	    !IRC_WriteStrClient(from, RPL_WHOISREGNICK_MSG,
				Client_ID(from), Client_ID(c)))
		return DISCONNECTED;

	/* Account name metadata? */
	if (Client_AccountName(c) &&
	    !IRC_WriteStrClient(from, RPL_WHOISLOGGEDIN_MSG,
				Client_ID(from), Client_ID(c),
				Client_AccountName(c)))
		return DISCONNECTED;

	/* Local client and requester is the user itself or an IRC Op? */
	if (Client_Conn(c) > NONE &&
	    (from == c || Client_HasMode(from, 'o'))) {
		/* Client hostname */
		if (!IRC_WriteStrClient(from, RPL_WHOISHOST_MSG,
					Client_ID(from), Client_ID(c),
					Client_Hostname(c), Client_IPAText(c)))
			return DISCONNECTED;
		/* Client modes */
		if (!IRC_WriteStrClient(from, RPL_WHOISMODES_MSG,
					Client_ID(from), Client_ID(c), Client_Modes(c)))
			return DISCONNECTED;
	}

	/* Idle and signon time (local clients only!) */
	if (!Conf_MorePrivacy && Client_Conn(c) > NONE &&
	    !IRC_WriteStrClient(from, RPL_WHOISIDLE_MSG,
				Client_ID(from), Client_ID(c),
				(unsigned long)Conn_GetIdle(Client_Conn(c)),
				(unsigned long)Conn_GetSignon(Client_Conn(c))))
		return DISCONNECTED;

	/* Away? */
	if (Client_HasMode(c, 'a') &&
	    !IRC_WriteStrClient(from, RPL_AWAY_MSG,
				Client_ID(from), Client_ID(c), Client_Away(c)))
		return DISCONNECTED;

	return CONNECTED;
}

static bool
WHOWAS_EntryWrite(CLIENT *prefix, WHOWAS *entry)
{
	char t_str[60];

	(void)strftime(t_str, sizeof(t_str), "%a %b %d %H:%M:%S %Y",
		       localtime(&entry->time));

	if (!IRC_WriteStrClient(prefix, RPL_WHOWASUSER_MSG, Client_ID(prefix),
				entry->id, entry->user, entry->host, entry->info))
		return DISCONNECTED;

	return IRC_WriteStrClient(prefix, RPL_WHOISSERVER_MSG, Client_ID(prefix),
				  entry->id, entry->server, t_str);
}

#ifdef SSL_SUPPORT
static bool
Show_MOTD_SSLInfo(CLIENT *Client)
{
	char buf[COMMAND_LEN];
	char c_str[128];

	if (Conn_GetCipherInfo(Client_Conn(Client), c_str, sizeof(c_str))) {
		snprintf(buf, sizeof(buf), "Connected using Cipher %s", c_str);
		if (!IRC_WriteStrClient(Client, RPL_MOTD_MSG,
					Client_ID(Client), buf))
			return false;
	}

	if (Conn_GetCertFp(Client_Conn(Client))) {
		snprintf(buf, sizeof(buf),
			 "Your client certificate fingerprint is: %s",
			 Conn_GetCertFp(Client_Conn(Client)));
		if (!IRC_WriteStrClient(Client, RPL_MOTD_MSG,
					Client_ID(Client), buf))
			return false;
	}

	return true;
}
#else
static bool
Show_MOTD_SSLInfo(UNUSED CLIENT *c)
{
	return true;
}
#endif

/* Global functions */

/**
 * Handler for the IRC command "ADMIN".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_ADMIN(CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *prefix;

	assert( Client != NULL );
	assert( Req != NULL );

	_IRC_GET_SENDER_OR_RETURN_(prefix, Req, Client)
	_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 0, prefix)

	/* Forward? */
	if(target != Client_ThisServer()) {
		IRC_WriteStrClientPrefix(target, prefix,
					 "ADMIN %s", Client_ID(target));
		return CONNECTED;
	}

	if (!IRC_WriteStrClient(Client, RPL_ADMINME_MSG, Client_ID(prefix),
				Conf_ServerName))
		return DISCONNECTED;
	if (!IRC_WriteStrClient(Client, RPL_ADMINLOC1_MSG, Client_ID(prefix),
				Conf_ServerAdmin1))
		return DISCONNECTED;
	if (!IRC_WriteStrClient(Client, RPL_ADMINLOC2_MSG, Client_ID(prefix),
				Conf_ServerAdmin2))
		return DISCONNECTED;
	if (!IRC_WriteStrClient(Client, RPL_ADMINEMAIL_MSG, Client_ID(prefix),
				Conf_ServerAdminMail))
		return DISCONNECTED;

	return CONNECTED;
} /* IRC_ADMIN */

/**
 * Handler for the IRC command "INFO".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_INFO(CLIENT * Client, REQUEST * Req)
{
	CLIENT *target, *prefix;
	char msg[510];

	assert(Client != NULL);
	assert(Req != NULL);

	_IRC_GET_SENDER_OR_RETURN_(prefix, Req, Client)
	_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 0, prefix)

	/* Forward? */
	if (target != Client_ThisServer()) {
		IRC_WriteStrClientPrefix(target, prefix, "INFO %s",
					 Client_ID(target));
		return CONNECTED;
	}

	if (!IRC_WriteStrClient(Client, RPL_INFO_MSG, Client_ID(prefix),
				NGIRCd_Version))
		return DISCONNECTED;

#if defined(BIRTHDATE)
	char t_str[60];
	time_t t = BIRTHDATE;
	(void)strftime(t_str, sizeof(t_str), "%a %b %d %Y at %H:%M:%S (%Z)",
			localtime(&t));
	snprintf(msg, sizeof(msg), "Birth Date: %s", t_str);
	if (!IRC_WriteStrClient(Client, RPL_INFO_MSG, Client_ID(prefix), msg))
		return DISCONNECTED;
#elif defined(__DATE__) && defined(__TIME__)
	snprintf(msg, sizeof(msg), "Birth Date: %s at %s", __DATE__, __TIME__);
	if (!IRC_WriteStrClient(Client, RPL_INFO_MSG, Client_ID(prefix), msg))
		return DISCONNECTED;
#endif

	strlcpy(msg, "On-line since ", sizeof(msg));
	strlcat(msg, NGIRCd_StartStr, sizeof(msg));
	if (!IRC_WriteStrClient(Client, RPL_INFO_MSG, Client_ID(prefix), msg))
		return DISCONNECTED;

	if (!IRC_WriteStrClient(Client, RPL_ENDOFINFO_MSG, Client_ID(prefix)))
		return DISCONNECTED;

	return CONNECTED;
} /* IRC_INFO */

/**
 * Handler for the IRC "ISON" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_ISON( CLIENT *Client, REQUEST *Req )
{
	char rpl[COMMAND_LEN];
	CLIENT *c;
	char *ptr;
	int i;

	assert(Client != NULL);
	assert(Req != NULL);

	strlcpy(rpl, RPL_ISON_MSG, sizeof rpl);
	for (i = 0; i < Req->argc; i++) {
		/* "All" ircd even parse ":<x> <y> ..." arguments and split
		 * them up; so we do the same ... */
		ptr = strtok(Req->argv[i], " ");
		while (ptr) {
			ngt_TrimStr(ptr);
			c = Client_Search(ptr);
			if (c && Client_Type(c) == CLIENT_USER) {
				strlcat(rpl, Client_ID(c), sizeof(rpl));
				strlcat(rpl, " ", sizeof(rpl));
			}
			ptr = strtok(NULL, " ");
		}
	}
	ngt_TrimLastChr(rpl, ' ');

	return IRC_WriteStrClient(Client, rpl, Client_ID(Client));
} /* IRC_ISON */

/**
 * Handler for the IRC "LINKS" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_LINKS(CLIENT *Client, REQUEST *Req)
{
	CLIENT *target, *from, *c;
	char *mask;

	assert(Client != NULL);
	assert(Req != NULL);

	_IRC_GET_SENDER_OR_RETURN_(from, Req, Client)

	/* Get pointer to server mask or "*", if none given */
	if (Req->argc > 0)
		mask = Req->argv[Req->argc - 1];
	else
		mask = "*";

	/* Forward? */
	if (Req->argc == 2) {
		_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 0, from)
		if (target != Client_ThisServer()) {
			IRC_WriteStrClientPrefix(target, from,
					"LINKS %s %s", Client_ID(target),
					Req->argv[1]);
			return CONNECTED;
		}
	}

	c = Client_First();
	while (c) {
		if (Client_Type(c) == CLIENT_SERVER
		    && MatchCaseInsensitive(mask, Client_ID(c))) {
			if (!IRC_WriteStrClient(from, RPL_LINKS_MSG,
					Client_ID(from), Client_ID(c),
					Client_ID(Client_TopServer(c)
						  ? Client_TopServer(c)
						  : Client_ThisServer()),
					Client_Hops(c), Client_Info(c)))
				return DISCONNECTED;
		}
		c = Client_Next(c);
	}
	return IRC_WriteStrClient(from, RPL_ENDOFLINKS_MSG,
				  Client_ID(from), mask);
} /* IRC_LINKS */

/**
 * Handler for the IRC "LUSERS" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_LUSERS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *from;

	assert( Client != NULL );
	assert( Req != NULL );

	_IRC_GET_SENDER_OR_RETURN_(from, Req, Client)
	_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 1, from)

	/* Forward? */
	if (target != Client_ThisServer()) {
		IRC_WriteStrClientPrefix(target, from,
					 "LUSERS %s %s", Req->argv[0],
					 Client_ID(target));
		return CONNECTED;
	}

	return IRC_Send_LUSERS(from);
} /* IRC_LUSERS */

/**
 * Handler for the IRC command "SERVLIST".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_SERVLIST(CLIENT *Client, REQUEST *Req)
{
	CLIENT *c;

	assert(Client != NULL);
	assert(Req != NULL);

	if (Req->argc < 2 || strcmp(Req->argv[1], "0") == 0) {
		for (c = Client_First(); c!= NULL; c = Client_Next(c)) {
			if (Client_Type(c) != CLIENT_SERVICE)
				continue;
			if (Req->argc > 0 && !MatchCaseInsensitive(Req->argv[0],
								  Client_ID(c)))
				continue;
			if (!IRC_WriteStrClient(Client, RPL_SERVLIST_MSG,
					Client_ID(Client), Client_Mask(c),
					Client_Mask(Client_Introducer(c)), "*",
					0, Client_Hops(c), Client_Info(c)))
				return DISCONNECTED;
		}
	}

	return IRC_WriteStrClient(Client, RPL_SERVLISTEND_MSG, Client_ID(Client),
				  Req->argc > 0 ? Req->argv[0] : "*",
				  Req->argc > 1 ? Req->argv[1] : "0");
} /* IRC_SERVLIST */

/**
 * Handler for the IRC command "MOTD".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_MOTD( CLIENT *Client, REQUEST *Req )
{
	CLIENT *from, *target;

	assert( Client != NULL );
	assert( Req != NULL );

	_IRC_GET_SENDER_OR_RETURN_(from, Req, Client)
	_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 0, from)

	/* Forward? */
	if (target != Client_ThisServer()) {
		IRC_WriteStrClientPrefix(target, from, "MOTD %s",
					 Client_ID(target));
		return CONNECTED;
	}

	return IRC_Show_MOTD(from);
} /* IRC_MOTD */

/**
 * Handler for the IRC command "NAMES".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_NAMES( CLIENT *Client, REQUEST *Req )
{
	char rpl[COMMAND_LEN], *ptr;
	CLIENT *target, *from, *c;
	CHANNEL *chan;

	assert( Client != NULL );
	assert( Req != NULL );

	_IRC_GET_SENDER_OR_RETURN_(from, Req, Client)
	_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 1, from)

	/* Forward? */
	if (target != Client_ThisServer()) {
		IRC_WriteStrClientPrefix(target, from, "NAMES %s :%s",
					 Req->argv[0], Client_ID(target));
		return CONNECTED;
	}

	if (Req->argc > 0) {
		/* Return NAMES list for specific channels */
		ptr = strtok(Req->argv[0], ",");
		while(ptr) {
			chan = Channel_Search(ptr);
			if (chan && !IRC_Send_NAMES(from, chan))
				return DISCONNECTED;
			if (!IRC_WriteStrClient(from, RPL_ENDOFNAMES_MSG,
						Client_ID(from), ptr))
				return DISCONNECTED;
			ptr = strtok( NULL, "," );
		}
		return CONNECTED;
	}

	chan = Channel_First();
	while (chan) {
		if (!IRC_Send_NAMES(from, chan))
			return DISCONNECTED;
		chan = Channel_Next(chan);
	}

	/* Now print all clients which are not in any channel */
	c = Client_First();
	snprintf(rpl, sizeof(rpl), RPL_NAMREPLY_MSG, Client_ID(from), '*', "*");
	while (c) {
		if (Client_Type(c) == CLIENT_USER
		    && Channel_FirstChannelOf(c) == NULL
		    && !Client_HasMode(c, 'i'))
		{
			/* its a user, concatenate ... */
			if (rpl[strlen(rpl) - 1] != ':')
				strlcat(rpl, " ", sizeof(rpl));
			strlcat(rpl, Client_ID(c), sizeof(rpl));

			if (strlen(rpl) > COMMAND_LEN - CLIENT_NICK_LEN - 4) {
				/* Line is going too long, send now */
				if (!IRC_WriteStrClient(from, "%s", rpl))
					return DISCONNECTED;
				snprintf(rpl, sizeof(rpl), RPL_NAMREPLY_MSG,
					 Client_ID(from), '*', "*");
			}
		}
		c = Client_Next(c);
	}
	if (rpl[strlen(rpl) - 1] != ':' && !IRC_WriteStrClient(from, "%s", rpl))
		return DISCONNECTED;

	return IRC_WriteStrClient(from, RPL_ENDOFNAMES_MSG, Client_ID(from), "*");
} /* IRC_NAMES */

/**
 * Handler for the IRC command "STATS".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_STATS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *from, *target, *cl;
	CONN_ID con;
	char query;
	COMMAND *cmd;
	time_t time_now;
	unsigned int days, hrs, mins;
	struct list_head *list;
	struct list_elem *list_item;
	bool more_links = false;

	assert(Client != NULL);
	assert(Req != NULL);

	_IRC_GET_SENDER_OR_RETURN_(from, Req, Client)
	_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 1, from)

	/* Forward? */
	if (target != Client_ThisServer()) {
		IRC_WriteStrClientPrefix(target, from, "STATS %s %s",
					 Req->argv[0], Client_ID(target));
		return CONNECTED;
	}

	if (Req->argc > 0)
		query = Req->argv[0][0] ? Req->argv[0][0] : '*';
	else
		query = '*';

	switch (query) {
	case 'g':	/* Network-wide bans ("G-Lines") */
	case 'G':
	case 'k':	/* Server-local bans ("K-Lines") */
	case 'K':
		if (!Client_HasMode(from, 'o'))
		    return IRC_WriteErrClient(from, ERR_NOPRIVILEGES_MSG,
					      Client_ID(from));
		if (query == 'g' || query == 'G')
			list = Class_GetList(CLASS_GLINE);
		else
			list = Class_GetList(CLASS_KLINE);
		list_item = Lists_GetFirst(list);
		while (list_item) {
			if (!IRC_WriteStrClient(from, RPL_STATSXLINE_MSG,
						Client_ID(from), query,
						Lists_GetMask(list_item),
						Lists_GetValidity(list_item),
						Lists_GetReason(list_item)))
				return DISCONNECTED;
			list_item = Lists_GetNext(list_item);
		}
		break;
	case 'L':	/* Link status (servers and user links) */
		if (!Op_Check(from, Req))
			return Op_NoPrivileges(from, Req);
		more_links = true;
		/* fall through */
	case 'l':	/* Link status (servers and own link) */
		time_now = time(NULL);
		for (con = Conn_First(); con != NONE; con = Conn_Next(con)) {
			cl = Conn_GetClient(con);
			if (!cl)
				continue;
			if (Client_Type(cl) == CLIENT_SERVER ||
			    cl == Client ||
			    (more_links && Client_Type(cl) == CLIENT_USER)) {
#ifdef ZLIB
				if (Conn_Options(con) & CONN_ZIP) {
					if (!IRC_WriteStrClient
					    (from, RPL_STATSLINKINFOZIP_MSG,
					     Client_ID(from), Client_Mask(cl),
					     Conn_SendQ(con), Conn_SendMsg(con),
					     Zip_SendBytes(con),
					     Conn_SendBytes(con),
					     Conn_RecvMsg(con),
					     Zip_RecvBytes(con),
					     Conn_RecvBytes(con),
					     (long)(time_now - Conn_StartTime(con))))
						return DISCONNECTED;
					continue;
				}
#endif
				if (!IRC_WriteStrClient
				    (from, RPL_STATSLINKINFO_MSG,
				     Client_ID(from), Client_Mask(cl),
				     Conn_SendQ(con), Conn_SendMsg(con),
				     Conn_SendBytes(con), Conn_RecvMsg(con),
				     Conn_RecvBytes(con),
				     (long)(time_now - Conn_StartTime(con))))
					return DISCONNECTED;
			}
		}
		break;
	case 'm':	/* IRC command status (usage count) */
	case 'M':
		cmd = Parse_GetCommandStruct();
		for (; cmd->name; cmd++) {
			if (cmd->lcount == 0 && cmd->rcount == 0)
				continue;
			if (!IRC_WriteStrClient
			    (from, RPL_STATSCOMMANDS_MSG, Client_ID(from),
			     cmd->name, cmd->lcount, cmd->bytes, cmd->rcount))
				return DISCONNECTED;
		}
		break;
	case 'u':	/* Server uptime */
	case 'U':
		time_now = time(NULL) - NGIRCd_Start;
		days = uptime_days(&time_now);
		hrs = uptime_hrs(&time_now);
		mins = uptime_mins(&time_now);
		if (!IRC_WriteStrClient(from, RPL_STATSUPTIME, Client_ID(from),
				       days, hrs, mins, (unsigned int)time_now))
			return DISCONNECTED;
		break;
	}

	return IRC_WriteStrClient(from, RPL_ENDOFSTATS_MSG,
				  Client_ID(from), query);
} /* IRC_STATS */

/**
 * Handler for the IRC command "SUMMON".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_SUMMON(CLIENT * Client, UNUSED REQUEST * Req)
{
	assert(Client != NULL);

	return IRC_WriteErrClient(Client, ERR_SUMMONDISABLED_MSG,
				  Client_ID(Client));
} /* IRC_SUMMON */

/**
 * Handler for the IRC command "TIME".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_TIME( CLIENT *Client, REQUEST *Req )
{
	CLIENT *from, *target;
	char t_str[64];
	time_t t;

	assert(Client != NULL);
	assert(Req != NULL);

	_IRC_GET_SENDER_OR_RETURN_(from, Req, Client)
	_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 0, from)

	/* Forward? */
	if (target != Client_ThisServer()) {
		IRC_WriteStrClientPrefix(target, from, "TIME %s",
					 Client_ID(target));
		return CONNECTED;
	}

	t = time( NULL );
	(void)strftime(t_str, 60, "%A %B %d %Y -- %H:%M %Z", localtime(&t));
	return IRC_WriteStrClient(from, RPL_TIME_MSG, Client_ID(from),
				  Client_ID(Client_ThisServer()), t_str);
} /* IRC_TIME */

/**
 * Handler for the IRC command "USERHOST".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_USERHOST(CLIENT *Client, REQUEST *Req)
{
	char rpl[COMMAND_LEN];
	CLIENT *c;
	int max, i;

	assert(Client != NULL);
	assert(Req != NULL);

	if (Req->argc > 5)
		max = 5;
	else
		max = Req->argc;

	strlcpy(rpl, RPL_USERHOST_MSG, sizeof rpl);
	for (i = 0; i < max; i++) {
		c = Client_Search(Req->argv[i]);
		if (c && (Client_Type(c) == CLIENT_USER)) {
			/* This Nick is "online" */
			strlcat(rpl, Client_ID(c), sizeof(rpl));
			if (Client_HasMode(c, 'o'))
				strlcat(rpl, "*", sizeof(rpl));
			strlcat(rpl, "=", sizeof(rpl));
			if (Client_HasMode(c, 'a'))
				strlcat(rpl, "-", sizeof(rpl));
			else
				strlcat(rpl, "+", sizeof(rpl));
			strlcat(rpl, Client_User(c), sizeof(rpl));
			strlcat(rpl, "@", sizeof(rpl));
			strlcat(rpl, Client_HostnameDisplayed(c), sizeof(rpl));
			strlcat(rpl, " ", sizeof(rpl));
		}
	}
	ngt_TrimLastChr(rpl, ' ');

	return IRC_WriteStrClient(Client, rpl, Client_ID(Client));
} /* IRC_USERHOST */

/**
 * Handler for the IRC command "USERS".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_USERS(CLIENT * Client, UNUSED REQUEST * Req)
{
	assert(Client != NULL);

	return IRC_WriteErrClient(Client, ERR_USERSDISABLED_MSG,
				  Client_ID(Client));
} /* IRC_USERS */

/**
 * Handler for the IRC command "VERSION".
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_VERSION( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *prefix;

	assert( Client != NULL );
	assert( Req != NULL );

	_IRC_GET_SENDER_OR_RETURN_(prefix, Req, Client)
	_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 0, prefix)

	/* Forward? */
	if (target != Client_ThisServer()) {
		IRC_WriteStrClientPrefix(target, prefix, "VERSION %s",
					 Client_ID(target));
		return CONNECTED;
	}

	/* send version information */
	if (!IRC_WriteStrClient(Client, RPL_VERSION_MSG, Client_ID(prefix),
				PACKAGE_NAME, PACKAGE_VERSION,
				NGIRCd_DebugLevel, Conf_ServerName,
				NGIRCd_VersionAddition))
		return DISCONNECTED;

#ifndef STRICT_RFC
	/* send RPL_ISUPPORT(005) numerics */
	if (!IRC_Send_ISUPPORT(prefix))
		return DISCONNECTED;
#endif

	return CONNECTED;
} /* IRC_VERSION */

/**
 * Handler for the IRC "WHO" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_WHO(CLIENT *Client, REQUEST *Req)
{
	bool only_ops;
	CHANNEL *chan;

	assert (Client != NULL);
	assert (Req != NULL);

	only_ops = false;
	if (Req->argc == 2) {
		if (strcmp(Req->argv[1], "o") == 0)
			only_ops = true;
#ifdef STRICT_RFC
		else {
			return IRC_WriteErrClient(Client,
						  ERR_NEEDMOREPARAMS_MSG,
						  Client_ID(Client),
						  Req->command);
		}
#endif
	}

	if (Req->argc >= 1) {
		/* Channel or mask given */
		chan = Channel_Search(Req->argv[0]);
		if (chan) {
			/* Members of a channel have been requested */
			return IRC_WHO_Channel(Client, chan, only_ops);
		}
		if (strcmp(Req->argv[0], "0") != 0) {
			/* A mask has been given. But please note this RFC
			 * stupidity: "0" is same as no arguments ... */
			return IRC_WHO_Mask(Client, Req->argv[0], only_ops);
		}
	}

	/* No channel or (valid) mask given */
	return IRC_WHO_Mask(Client, NULL, only_ops);
} /* IRC_WHO */

/**
 * Handler for the IRC "WHOIS" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_WHOIS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *from, *target, *c;
	unsigned int match_count = 0, found = 0;
	bool has_wildcards, is_remote;
	bool got_wildcard = false;
	char mask[COMMAND_LEN], *query;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Wrong number of parameters? */
	if (Req->argc < 1)
		return IRC_WriteErrClient(Client, ERR_NONICKNAMEGIVEN_MSG,
					  Client_ID(Client));

	_IRC_ARGC_LE_OR_RETURN_(Client, Req, 2)
	_IRC_GET_SENDER_OR_RETURN_(from, Req, Client)

	/* Get target server for this command */
	if (Req->argc > 1) {
		_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 0, Client)
	} else
		target = Client_ThisServer();

	assert(target != NULL);

	/* Forward? */
	if (target != Client_ThisServer()) {
		IRC_WriteStrClientPrefix(target, from, "WHOIS %s :%s",
					 Req->argv[0], Req->argv[1]);
		return CONNECTED;
	}

	is_remote = Client_Conn(from) < 0;
	strlcpy(mask, Req->argv[Req->argc - 1], sizeof(mask));
	for (query = strtok(ngt_LowerStr(mask), ",");
			query && found < 3;
			query = strtok(NULL, ","), found++)
	{
		has_wildcards = query[strcspn(query, "*?")] != 0;
		/*
		 * follows ircd 2.10 implementation:
		 *  - handle up to 3 targets
		 *  - no wildcards for remote clients
		 *  - only one wildcard target per local client
		 *
		 *  Also, at most MAX_RPL_WHOIS matches are returned.
		 */
		if (!has_wildcards || is_remote) {
			c = Client_Search(query);
			if (c && (Client_Type(c) == CLIENT_USER
				  || Client_Type(c) == CLIENT_SERVICE)) {
				if (!IRC_WHOIS_SendReply(Client, from, c))
					return DISCONNECTED;
			} else {
				if (!IRC_WriteErrClient(Client,
							ERR_NOSUCHNICK_MSG,
							Client_ID(Client),
							query))
					return DISCONNECTED;
			}
			continue;
		}
		if (got_wildcard) {
			/* we already handled one wildcard query */
			if (!IRC_WriteErrClient(Client, ERR_NOSUCHNICK_MSG,
			     Client_ID(Client), query))
				return DISCONNECTED;
			continue;
		}
		got_wildcard = true;
		/* Increase penalty for wildcard queries */
		IRC_SetPenalty(Client, 3);

		for (c = Client_First(); c; c = Client_Next(c)) {
			if (IRC_CheckListTooBig(Client, match_count,
					    MAX_RPL_WHOIS, "WHOIS"))
				break;

			if (Client_Type(c) != CLIENT_USER)
				continue;
			if (Client_HasMode(c, 'i'))
				continue;
			if (!MatchCaseInsensitive(query, Client_ID(c)))
				continue;
			if (!IRC_WHOIS_SendReply(Client, from, c))
				return DISCONNECTED;

			match_count++;
		}

		if (match_count == 0)
			IRC_WriteErrClient(Client, ERR_NOSUCHNICK_MSG,
					   Client_ID(Client),
					   Req->argv[Req->argc - 1]);
	}
	return IRC_WriteStrClient(from, RPL_ENDOFWHOIS_MSG,
				  Client_ID(from), Req->argv[Req->argc - 1]);
} /* IRC_WHOIS */

/**
 * Handler for the IRC "WHOWAS" command.
 *
 * @param Client The client from which this command has been received.
 * @param Req Request structure with prefix and all parameters.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_WHOWAS( CLIENT *Client, REQUEST *Req )
{
	CLIENT *target, *prefix;
	WHOWAS *whowas;
	char tok_buf[COMMAND_LEN];
	int max, last, count, i, nc;
	const char *nick;

	assert( Client != NULL );
	assert( Req != NULL );

	/* Wrong number of parameters? */
	if (Req->argc < 1)
		return IRC_WriteErrClient(Client, ERR_NONICKNAMEGIVEN_MSG,
					  Client_ID(Client));

	_IRC_ARGC_LE_OR_RETURN_(Client, Req, 3)
	_IRC_GET_SENDER_OR_RETURN_(prefix, Req, Client)
	_IRC_GET_TARGET_SERVER_OR_RETURN_(target, Req, 2, prefix)

	/* Do not reveal any info on disconnected users? */
	if (Conf_MorePrivacy)
		return CONNECTED;

	/* Forward? */
	if (target != Client_ThisServer()) {
		IRC_WriteStrClientPrefix(target, prefix, "WHOWAS %s %s %s",
					 Req->argv[0], Req->argv[1],
					 Client_ID(target));
		return CONNECTED;
	}

	whowas = Client_GetWhowas( );
	last = Client_GetLastWhowasIndex( );
	if (last < 0)
		last = 0;

	max = DEF_RPL_WHOWAS;
	if (Req->argc > 1) {
		max = atoi(Req->argv[1]);
		if (max < 1)
			max = MAX_RPL_WHOWAS;
	}

	/*
	 * Break up the nick argument into a list of nicks, if applicable
	 * Can't modify Req->argv[0] because we need it for RPL_ENDOFWHOWAS_MSG.
	 */
	strlcpy(tok_buf, Req->argv[0], sizeof(tok_buf));
	nick = strtok(tok_buf, ",");

	for (i=last, count=0; nick != NULL ; nick = strtok(NULL, ",")) {
		nc = 0;
		do {
			/* Used entry? */
			if (whowas[i].time > 0 && strcasecmp(nick, whowas[i].id) == 0) {
				if (!WHOWAS_EntryWrite(prefix, &whowas[i]))
					return DISCONNECTED;
				nc++;
				count++;
			}
			/* previous entry */
			i--;

			/* "underflow", wrap around */
			if (i < 0)
				i = MAX_WHOWAS - 1;

			if (nc && count >= max)
				break;
		} while (i != last);

		if (nc == 0 && !IRC_WriteErrClient(prefix, ERR_WASNOSUCHNICK_MSG,
						Client_ID(prefix), nick))
			return DISCONNECTED;
	}
	return IRC_WriteStrClient(prefix, RPL_ENDOFWHOWAS_MSG,
				  Client_ID(prefix), Req->argv[0]);
} /* IRC_WHOWAS */

/**
 * Send LUSERS reply to a client.
 *
 * @param Client The recipient of the information.
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_Send_LUSERS(CLIENT *Client)
{
	unsigned long cnt;
#ifndef STRICT_RFC
	unsigned long max;
#endif

	assert(Client != NULL);

	/* Users, services and servers in the network */
	if (!IRC_WriteStrClient(Client, RPL_LUSERCLIENT_MSG, Client_ID(Client),
				Client_UserCount(), Client_ServiceCount(),
				Client_ServerCount()))
		return DISCONNECTED;

	/* Number of IRC operators */
	cnt = Client_OperCount( );
	if (cnt > 0) {
		if (!IRC_WriteStrClient(Client, RPL_LUSEROP_MSG,
					Client_ID(Client), cnt))
			return DISCONNECTED;
	}

	/* Unknown connections */
	cnt = Client_UnknownCount( );
	if (cnt > 0) {
		if (!IRC_WriteStrClient(Client, RPL_LUSERUNKNOWN_MSG,
					Client_ID(Client), cnt))
			return DISCONNECTED;
	}

	/* Number of created channels */
	if (!IRC_WriteStrClient(Client, RPL_LUSERCHANNELS_MSG,
				Client_ID(Client),
				Channel_CountVisible(Client)))
		return DISCONNECTED;

	/* Number of local users, services and servers */
	if (!IRC_WriteStrClient(Client, RPL_LUSERME_MSG, Client_ID(Client),
				Client_MyUserCount(), Client_MyServiceCount(),
				Client_MyServerCount()))
		return DISCONNECTED;

#ifndef STRICT_RFC
	/* Maximum number of local users */
	cnt = Client_MyUserCount();
	max = Client_MyMaxUserCount();
	if (! IRC_WriteStrClient(Client, RPL_LOCALUSERS_MSG, Client_ID(Client),
			cnt, max, cnt, max))
		return DISCONNECTED;
	/* Maximum number of users in the network */
	cnt = Client_UserCount();
	max = Client_MaxUserCount();
	if(! IRC_WriteStrClient(Client, RPL_NETUSERS_MSG, Client_ID(Client),
			cnt, max, cnt, max))
		return DISCONNECTED;
	/* Connection counters */
	if (! IRC_WriteStrClient(Client, RPL_STATSCONN_MSG, Client_ID(Client),
			Conn_CountMax(), Conn_CountAccepted()))
		return DISCONNECTED;
#endif

	return CONNECTED;
} /* IRC_Send_LUSERS */

GLOBAL bool
IRC_Show_MOTD( CLIENT *Client )
{
	const char *line;
	size_t len_tot, len_str;

	assert( Client != NULL );

	len_tot = array_bytes(&Conf_Motd);
	if (len_tot == 0 && !Conn_UsesSSL(Client_Conn(Client)))
		return IRC_WriteErrClient(Client, ERR_NOMOTD_MSG, Client_ID(Client));

	if (!IRC_WriteStrClient(Client, RPL_MOTDSTART_MSG, Client_ID(Client),
				Client_ID(Client_ThisServer())))
		return DISCONNECTED;

	line = array_start(&Conf_Motd);
	while (len_tot > 0) {
		len_str = strlen(line) + 1;

		assert(len_tot >= len_str);
		len_tot -= len_str;

		if (!IRC_WriteStrClient(Client, RPL_MOTD_MSG, Client_ID(Client), line))
			return DISCONNECTED;
		line += len_str;
	}

	if (!Show_MOTD_SSLInfo(Client))
		return DISCONNECTED;

	if (!IRC_WriteStrClient(Client, RPL_ENDOFMOTD_MSG, Client_ID(Client)))
		return DISCONNECTED;

	if (*Conf_CloakHost)
		return IRC_WriteStrClient(Client, RPL_HOSTHIDDEN_MSG,
					  Client_ID(Client),
					  Client_Hostname(Client));

	return CONNECTED;
} /* IRC_Show_MOTD */

/**
 * Send NAMES reply for a specific client and channel.
 *
 * @param Client The client requesting the NAMES information.
 * @param Chan The channel
 * @return CONNECTED or DISCONNECTED.
 */
GLOBAL bool
IRC_Send_NAMES(CLIENT * Client, CHANNEL * Chan)
{
	bool is_visible, is_member;
	char str[COMMAND_LEN];
	CL2CHAN *cl2chan;
	CLIENT *cl;
	bool secret_channel;
	char chan_symbol;

	assert(Client != NULL);
	assert(Chan != NULL);

	if (Channel_IsMemberOf(Chan, Client))
		is_member = true;
	else
		is_member = false;

	/* Do not print info on channel memberships to anyone that is not member? */
	if (Conf_MorePrivacy && !is_member)
		return CONNECTED;

	/* Secret channel? */
	secret_channel = Channel_HasMode(Chan, 's');
	if (!is_member && secret_channel)
		return CONNECTED;

	chan_symbol = secret_channel ? '@' : '=';

	snprintf(str, sizeof(str), RPL_NAMREPLY_MSG, Client_ID(Client), chan_symbol,
		 Channel_Name(Chan));
	cl2chan = Channel_FirstMember(Chan);
	while (cl2chan) {
		cl = Channel_GetClient(cl2chan);

		if (Client_HasMode(cl, 'i'))
			is_visible = false;
		else
			is_visible = true;

		if (is_member || is_visible) {
			if (str[strlen(str) - 1] != ':')
				strlcat(str, " ", sizeof(str));

			who_flags_qualifier(Client, Channel_UserModes(Chan, cl),
					    str, sizeof(str));
			strlcat(str, Client_ID(cl), sizeof(str));

			if (strlen(str) > (COMMAND_LEN - CLIENT_NICK_LEN - 4)) {
				if (!IRC_WriteStrClient(Client, "%s", str))
					return DISCONNECTED;
				snprintf(str, sizeof(str), RPL_NAMREPLY_MSG,
					 Client_ID(Client), chan_symbol,
					 Channel_Name(Chan));
			}
		}

		cl2chan = Channel_NextMember(Chan, cl2chan);
	}
	if (str[strlen(str) - 1] != ':') {
		if (!IRC_WriteStrClient(Client, "%s", str))
			return DISCONNECTED;
	}

	return CONNECTED;
} /* IRC_Send_NAMES */

/**
 * Send the ISUPPORT numeric (005).
 * This numeric indicates the features that are supported by this server.
 * See <http://www.irc.org/tech_docs/005.html> for details.
 */
GLOBAL bool
IRC_Send_ISUPPORT(CLIENT * Client)
{
	if (Conf_Network[0] && !IRC_WriteStrClient(Client, RPL_ISUPPORTNET_MSG,
						   Client_ID(Client),
						   Conf_Network))
		return DISCONNECTED;
	if (!IRC_WriteStrClient(Client, RPL_ISUPPORT1_MSG, Client_ID(Client),
				CHANTYPES, CHANTYPES, Conf_MaxJoins))
		return DISCONNECTED;
	return IRC_WriteStrClient(Client, RPL_ISUPPORT2_MSG, Client_ID(Client),
				  CHANNEL_NAME_LEN - 1, Conf_MaxNickLength - 1,
				  COMMAND_LEN - 23, CLIENT_AWAY_LEN - 1,
				  COMMAND_LEN - 113, MAX_HNDL_MODES_ARG,
				  MAX_HNDL_CHANNEL_LISTS);
} /* IRC_Send_ISUPPORT */

/* -eof- */
