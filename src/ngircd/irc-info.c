/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2013 Alexander Barton (alex@barton.de) and Contributors.
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

#include "imp.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "ngircd.h"
#include "conn-func.h"
#include "conn-zip.h"
#include "channel.h"
#include "class.h"
#include "conf.h"
#include "defines.h"
#include "lists.h"
#include "log.h"
#include "messages.h"
#include "match.h"
#include "tool.h"
#include "parse.h"
#include "irc.h"
#include "irc-write.h"
#include "client-cap.h"

#include "exp.h"
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

static const char *
who_flags_status(const char *client_modes)
{
	if (strchr(client_modes, 'a'))
		return "G"; /* away */
	return "H";
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
	bool is_visible, is_member, is_ircop;
	CL2CHAN *cl2chan;
	const char *client_modes;
	char flags[10];
	CLIENT *c;
	int count = 0;

	assert( Client != NULL );
	assert( Chan != NULL );

	IRC_SetPenalty(Client, 1);

	is_member = Channel_IsMemberOf(Chan, Client);

	/* Secret channel? */
	if (!is_member && strchr(Channel_Modes(Chan), 's'))
		return IRC_WriteStrClient(Client, RPL_ENDOFWHO_MSG,
					  Client_ID(Client), Channel_Name(Chan));

	cl2chan = Channel_FirstMember(Chan);
	for (; cl2chan ; cl2chan = Channel_NextMember(Chan, cl2chan)) {
		c = Channel_GetClient(cl2chan);

		client_modes = Client_Modes(c);
		is_ircop = strchr(client_modes, 'o') != NULL;
		if (OnlyOps && !is_ircop)
			continue;

		is_visible = strchr(client_modes, 'i') == NULL;
		if (is_member || is_visible) {
			strlcpy(flags, who_flags_status(client_modes),
				sizeof(flags));
			if (is_ircop)
				strlcat(flags, "*", sizeof(flags));

			who_flags_qualifier(Client, Channel_UserModes(Chan, c),
					    flags, sizeof(flags));

			if (!write_whoreply(Client, c, Channel_Name(Chan),
					    flags))
				return DISCONNECTED;
			count++;
		}
	}

	/* If there are a lot of clients, augment penalty a bit */
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
	char flags[4];
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

		strlcpy(flags, who_flags_status(Client_Modes(c)), sizeof(flags));
		if (strchr(Client_Modes(c), 'o'))
			strlcat(flags, "*", sizeof(flags));

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
	char str[LINE_LEN + 1];
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

	/* Channels */
	snprintf(str, sizeof(str), RPL_WHOISCHANNELS_MSG,
		 Client_ID(from), Client_ID(c));
	cl2chan = Channel_FirstChannelOf(c);
	while (cl2chan) {
		chan = Channel_GetChannel(cl2chan);
		assert(chan != NULL);

		/* next */
		cl2chan = Channel_NextChannelOf(c, cl2chan);

		/* Secret channel? */
		if (strchr(Channel_Modes(chan), 's')
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

		if (strlen(str) > (LINE_LEN - CHANNEL_NAME_LEN - 4)) {
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

	/* IRC-Operator? */
	if (Client_HasMode(c, 'o') &&
	    !IRC_WriteStrClient(from, RPL_WHOISOPERATOR_MSG,
				Client_ID(from), Client_ID(c)))
		return DISCONNECTED;

	/* IRC-Bot? */
	if (Client_HasMode(c, 'B') &&
	    !IRC_WriteStrClient(from, RPL_WHOISBOT_MSG,
				Client_ID(from), Client_ID(c)))
		return DISCONNECTED;

	/* Connected using SSL? */
	if (Conn_UsesSSL(Client_Conn(c)) &&
	    !IRC_WriteStrClient(from, RPL_WHOISSSL_MSG, Client_ID(from),
				Client_ID(c)))
		return DISCONNECTED;

	/* Registered nickname? */
	if (Client_HasMode(c, 'R') &&
	    !IRC_WriteStrClient(from, RPL_WHOISREGNICK_MSG,
				Client_ID(from), Client_ID(c)))
		return DISCONNECTED;

	/* Local client and requester is the user itself or an IRC Op? */
	if (Client_Conn(c) > NONE &&
	    (from == c || (!Conf_MorePrivacy && Client_HasMode(from, 'o')))) {
		/* Client hostname */
		if (!IRC_WriteStrClient(from, RPL_WHOISHOST_MSG,
					Client_ID(from), Client_ID(c), Client_Hostname(c),
					Conn_GetIPAInfo(Client_Conn(c))))
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

static bool
Show_MOTD_Start(CLIENT *Client)
{
	return IRC_WriteStrClient(Client, RPL_MOTDSTART_MSG,
				  Client_ID( Client ), Client_ID( Client_ThisServer( )));
}

static bool
Show_MOTD_Sendline(CLIENT *Client, const char *msg)
{
	return IRC_WriteStrClient(Client, RPL_MOTD_MSG, Client_ID( Client ), msg);
}

static bool
Show_MOTD_End(CLIENT *Client)
{
	if (!IRC_WriteStrClient(Client, RPL_ENDOFMOTD_MSG, Client_ID(Client)))
		return DISCONNECTED;

	if (*Conf_CloakHost)
		return IRC_WriteStrClient(Client, RPL_HOSTHIDDEN_MSG,
					  Client_ID(Client),
					  Client_Hostname(Client));

	return CONNECTED;
}

#ifdef SSL_SUPPORT
static bool Show_MOTD_SSLInfo(CLIENT *Client)
{
	bool ret = true;
	char buf[COMMAND_LEN] = "Connected using Cipher ";

	if (!Conn_GetCipherInfo(Client_Conn(Client), buf + 23, sizeof buf - 23))
		return true;

	if (!Show_MOTD_Sendline(Client, buf))
		ret = false;

	return ret;
}
#else
static inline bool
Show_MOTD_SSLInfo(UNUSED CLIENT *c)
{ return true; }
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

	if(( Req->argc > 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* find target ... */
	if( Req->argc == 1 ) target = Client_Search( Req->argv[0] );
	else target = Client_ThisServer( );

	/* find Prefix */
	if( Client_Type( Client ) == CLIENT_SERVER ) prefix = Client_Search( Req->prefix );
	else prefix = Client;
	if( ! prefix ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* forwad message to another server? */
	if( target != Client_ThisServer( ))
	{
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( prefix, ERR_NOSUCHSERVER_MSG, Client_ID( prefix ), Req->argv[0] );

		/* forward */
		IRC_WriteStrClientPrefix( target, prefix, "ADMIN %s", Req->argv[0] );
		return CONNECTED;
	}

	/* mit Versionsinfo antworten */
	if( ! IRC_WriteStrClient( Client, RPL_ADMINME_MSG, Client_ID( prefix ), Conf_ServerName )) return DISCONNECTED;
	if( ! IRC_WriteStrClient( Client, RPL_ADMINLOC1_MSG, Client_ID( prefix ), Conf_ServerAdmin1 )) return DISCONNECTED;
	if( ! IRC_WriteStrClient( Client, RPL_ADMINLOC2_MSG, Client_ID( prefix ), Conf_ServerAdmin2 )) return DISCONNECTED;
	if( ! IRC_WriteStrClient( Client, RPL_ADMINEMAIL_MSG, Client_ID( prefix ), Conf_ServerAdminMail )) return DISCONNECTED;

	IRC_SetPenalty( Client, 1 );
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

	/* Wrong number of parameters? */
	if (Req->argc > 1)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	/* Determine prefix */
	if (Client_Type(Client) == CLIENT_SERVER)
		prefix = Client_Search(Req->prefix);
	else
		prefix = Client;
	if (!prefix)
		return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->prefix);

	/* Look for a target */
	if (Req->argc > 0)
		target = Client_Search(Req->argv[0]);
	else
		target = Client_ThisServer();

	/* Make sure that the target is a server */
	if (target && Client_Type(target) != CLIENT_SERVER)
		target = Client_Introducer(target);

	if (!target)
		return IRC_WriteStrClient(prefix, ERR_NOSUCHSERVER_MSG,
					  Client_ID(prefix), Req->argv[0]);

	/* Pass on to another server? */
	if (target != Client_ThisServer()) {
		IRC_WriteStrClientPrefix(target, prefix, "INFO %s",
					 Req->argv[0]);
		return CONNECTED;
	}

	if (!IRC_WriteStrClient(Client, RPL_INFO_MSG, Client_ID(prefix),
				NGIRCd_Version))
		return DISCONNECTED;

#if defined(__DATE__) && defined(__TIME__)
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

	IRC_SetPenalty(Client, 2);
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

	/* Bad number of arguments? */
	if (Req->argc < 1)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

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

	IRC_SetPenalty(Client, 1);

	if (Req->argc > 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	/* Get pointer to server mask or "*", if none given */
	if (Req->argc > 0)
		mask = Req->argv[Req->argc - 1];
	else
		mask = "*";

	if (Client_Type(Client) == CLIENT_SERVER)
		from = Client_Search(Req->prefix);
	else
		from = Client;
	if (!from)
		return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->prefix);

	/* Forward? */
	if (Req->argc == 2) {
		target = Client_Search(Req->argv[0]);
		if (! target || Client_Type(target) != CLIENT_SERVER)
			return IRC_WriteStrClient(from, ERR_NOSUCHSERVER_MSG,
						  Client_ID(from),
						  Req->argv[0] );
		else
			if (target != Client_ThisServer())
				return IRC_WriteStrClientPrefix(target, from,
						"LINKS %s %s", Req->argv[0],
						Req->argv[1]);
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

	if(( Req->argc > 2 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Absender ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* An anderen Server forwarden? */
	if( Req->argc == 2 )
	{
		target = Client_Search( Req->argv[1] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[1] );
		else if( target != Client_ThisServer( )) return IRC_WriteStrClientPrefix( target, from, "LUSERS %s %s", Req->argv[0], Req->argv[1] );
	}

	/* Wer ist der Absender? */
	if( Client_Type( Client ) == CLIENT_SERVER ) target = Client_Search( Req->prefix );
	else target = Client;
	if( ! target ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	IRC_Send_LUSERS( target );

	IRC_SetPenalty( target, 1 );
	return CONNECTED;
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

	if (Req->argc > 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

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

	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* From aus Prefix ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	if( Req->argc == 1 )
	{
		/* forward? */
		target = Client_Search( Req->argv[0] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[0] );

		if( target != Client_ThisServer( ))
		{
			/* Ok, anderer Server ist das Ziel: forwarden */
			return IRC_WriteStrClientPrefix( target, from, "MOTD %s", Req->argv[0] );
		}
	}

	IRC_SetPenalty( from, 3 );
	return IRC_Show_MOTD( from );
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

	if( Req->argc > 2 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* use prefix to determine "From" */
	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	if( Req->argc == 2 )
	{
		/* forward to another server? */
		target = Client_Search( Req->argv[1] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( from, ERR_NOSUCHSERVER_MSG, Client_ID( from ), Req->argv[1] );

		if( target != Client_ThisServer( )) {
			/* target is another server, forward */
			return IRC_WriteStrClientPrefix( target, from, "NAMES %s :%s", Req->argv[0], Req->argv[1] );
		}
	}

	if( Req->argc > 0 )
	{
		/* bestimmte Channels durchgehen */
		ptr = strtok( Req->argv[0], "," );
		while( ptr )
		{
			chan = Channel_Search( ptr );
			if( chan )
			{
				/* print name */
				if( ! IRC_Send_NAMES( from, chan )) return DISCONNECTED;
			}
			if( ! IRC_WriteStrClient( from, RPL_ENDOFNAMES_MSG, Client_ID( from ), ptr )) return DISCONNECTED;

			/* get next channel name */
			ptr = strtok( NULL, "," );
		}
		return CONNECTED;
	}

	chan = Channel_First( );
	while( chan )
	{
		if( ! IRC_Send_NAMES( from, chan )) return DISCONNECTED;

		chan = Channel_Next( chan );
	}

	/* Now print all clients which are not in any channel */
	c = Client_First( );
	snprintf( rpl, sizeof( rpl ), RPL_NAMREPLY_MSG, Client_ID( from ), "*", "*" );
	while( c )
	{
		if(( Client_Type( c ) == CLIENT_USER ) && ( Channel_FirstChannelOf( c ) == NULL ) && ( ! strchr( Client_Modes( c ), 'i' )))
		{
			/* its a user, concatenate ... */
			if( rpl[strlen( rpl ) - 1] != ':' ) strlcat( rpl, " ", sizeof( rpl ));
			strlcat( rpl, Client_ID( c ), sizeof( rpl ));

			if( strlen( rpl ) > ( LINE_LEN - CLIENT_NICK_LEN - 4 ))
			{
				/* Line is gwoing too long, send now */
				if( ! IRC_WriteStrClient( from, "%s", rpl )) return DISCONNECTED;
				snprintf( rpl, sizeof( rpl ), RPL_NAMREPLY_MSG, Client_ID( from ), "*", "*" );
			}
		}

		c = Client_Next( c );
	}
	if( rpl[strlen( rpl ) - 1] != ':')
	{
		if( ! IRC_WriteStrClient( from, "%s", rpl )) return DISCONNECTED;
	}

	IRC_SetPenalty( from, 1 );
	return IRC_WriteStrClient( from, RPL_ENDOFNAMES_MSG, Client_ID( from ), "*" );
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

	assert(Client != NULL);
	assert(Req != NULL);

	if (Req->argc > 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	/* use prefix to determine "From" */
	if (Client_Type(Client) == CLIENT_SERVER)
		from = Client_Search(Req->prefix);
	else
		from = Client;

	if (!from)
		return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->prefix);

	if (Req->argc == 2) {
		/* forward to another server? */
		target = Client_Search(Req->argv[1]);
		if ((!target) || (Client_Type(target) != CLIENT_SERVER))
			return IRC_WriteStrClient(from, ERR_NOSUCHSERVER_MSG,
						 Client_ID(from), Req->argv[1]);

		if (target != Client_ThisServer()) {
			/* forward to another server */
			return IRC_WriteStrClientPrefix(target, from,
				     "STATS %s %s", Req->argv[0], Req->argv[1]);
		}
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
		    return IRC_WriteStrClient(from, ERR_NOPRIVILEGES_MSG,
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
	case 'l':	/* Link status (servers and own link) */
	case 'L':
		time_now = time(NULL);
		for (con = Conn_First(); con != NONE; con = Conn_Next(con)) {
			cl = Conn_GetClient(con);
			if (!cl)
				continue;
			if ((Client_Type(cl) == CLIENT_SERVER)
			    || (cl == Client)) {
				/* Server link or our own connection */
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

	IRC_SetPenalty(from, 2);
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
	return IRC_WriteStrClient(Client, ERR_SUMMONDISABLED_MSG,
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

	assert( Client != NULL );
	assert( Req != NULL );

	if( Req->argc > 1 ) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	if( Client_Type( Client ) == CLIENT_SERVER ) from = Client_Search( Req->prefix );
	else from = Client;
	if( ! from ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	if( Req->argc == 1 )
	{
		target = Client_Search( Req->argv[0] );
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( Client, ERR_NOSUCHSERVER_MSG, Client_ID( Client ), Req->argv[0] );

		if( target != Client_ThisServer( ))
		{
			return IRC_WriteStrClientPrefix( target, from, "TIME %s", Req->argv[0] );
		}
	}

	t = time( NULL );
	(void)strftime( t_str, 60, "%A %B %d %Y -- %H:%M %Z", localtime( &t ));
	return IRC_WriteStrClient( from, RPL_TIME_MSG, Client_ID( from ), Client_ID( Client_ThisServer( )), t_str );
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

	if ((Req->argc < 1))
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

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
	return IRC_WriteStrClient(Client, ERR_USERSDISABLED_MSG,
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

	if(( Req->argc > 1 )) return IRC_WriteStrClient( Client, ERR_NEEDMOREPARAMS_MSG, Client_ID( Client ), Req->command );

	/* Ziel suchen */
	if( Req->argc == 1 ) target = Client_Search( Req->argv[0] );
	else target = Client_ThisServer( );

	/* Prefix ermitteln */
	if( Client_Type( Client ) == CLIENT_SERVER ) prefix = Client_Search( Req->prefix );
	else prefix = Client;
	if( ! prefix ) return IRC_WriteStrClient( Client, ERR_NOSUCHNICK_MSG, Client_ID( Client ), Req->prefix );

	/* An anderen Server weiterleiten? */
	if( target != Client_ThisServer( ))
	{
		if(( ! target ) || ( Client_Type( target ) != CLIENT_SERVER )) return IRC_WriteStrClient( prefix, ERR_NOSUCHSERVER_MSG, Client_ID( prefix ), Req->argv[0] );

		/* forwarden */
		IRC_WriteStrClientPrefix( target, prefix, "VERSION %s", Req->argv[0] );
		return CONNECTED;
	}

	/* send version information */
	IRC_SetPenalty(Client, 1);
	return IRC_WriteStrClient(Client, RPL_VERSION_MSG, Client_ID(prefix),
				  PACKAGE_NAME, PACKAGE_VERSION,
				  NGIRCd_DebugLevel, Conf_ServerName,
				  NGIRCd_VersionAddition);
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

	if (Req->argc > 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	only_ops = false;

	if (Req->argc == 2) {
		if (strcmp(Req->argv[1], "o") == 0)
			only_ops = true;
#ifdef STRICT_RFC
		else
			return IRC_WriteStrClient(Client,
						  ERR_NEEDMOREPARAMS_MSG,
						  Client_ID(Client),
						  Req->command);
#endif
	}

	IRC_SetPenalty(Client, 1);
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
	IRC_SetPenalty(Client, 2);
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

	/* Bad number of parameters? */
	if (Req->argc < 1 || Req->argc > 2)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					  Client_ID(Client), Req->command);

	/* Search sender of the WHOIS */
	if (Client_Type(Client) == CLIENT_SERVER) {
		from = Client_Search(Req->prefix);
	} else {
		IRC_SetPenalty(Client, 1);
		from = Client;
	}
	if (!from)
		return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
					  Client_ID(Client), Req->prefix);

	/* Get target server for this command */
	if (Req->argc > 1) {
		/* Search the target server, which can be specified as a
		 * nickname on that server as well: */
		target = Client_Search(Req->argv[0]);
		if (!target)
			return IRC_WriteStrClient(from, ERR_NOSUCHSERVER_MSG,
						Client_ID(from), Req->argv[0]);
	} else
		target = Client_ThisServer();
	assert(target != NULL);

	/* Forward to other server? */
	if (Client_NextHop(target) != Client_ThisServer() &&
	    Client_Type(Client_NextHop(target)) == CLIENT_SERVER)
		return IRC_WriteStrClientPrefix(target, from,
						"WHOIS %s :%s",
						Req->argv[0], Req->argv[1]);

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
			if (c && Client_Type(c) == CLIENT_USER) {
				if (!IRC_WHOIS_SendReply(Client, from, c))
					return DISCONNECTED;
			} else {
				if (!IRC_WriteStrClient(Client,
							ERR_NOSUCHNICK_MSG,
							Client_ID(Client),
							query))
					return DISCONNECTED;
			}
			continue;
		}
		if (got_wildcard) {
			/* we already handled one wildcard query */
			if (!IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
			     Client_ID(Client), query))
				return DISCONNECTED;
			continue;
		}
		got_wildcard = true;
		IRC_SetPenalty(Client, 3);

		for (c = Client_First(); c; c = Client_Next(c)) {
			if (IRC_CheckListTooBig(Client, match_count,
					    MAX_RPL_WHOIS, "WHOIS"))
				break;

			if (Client_Type(c) != CLIENT_USER)
				continue;
			if (!MatchCaseInsensitive(query, Client_ID(c)))
				continue;
			if (!IRC_WHOIS_SendReply(Client, from, c))
				return DISCONNECTED;

			match_count++;
		}

		if (match_count == 0)
			IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
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

	/* Do not reveal any info on disconnected users? */
	if (Conf_MorePrivacy)
		return CONNECTED;

	/* Wrong number of parameters? */
	if (Req->argc > 3)
		return IRC_WriteStrClient(Client, ERR_NEEDMOREPARAMS_MSG,
					Client_ID(Client), Req->command);
	if (Req->argc < 1)
		return IRC_WriteStrClient(Client, ERR_NONICKNAMEGIVEN_MSG, Client_ID(Client));

	/* Search target */
	if (Req->argc == 3)
		target = Client_Search(Req->argv[2]);
	else
		target = Client_ThisServer();

	/* Get prefix */
	if (Client_Type(Client) == CLIENT_SERVER)
		prefix = Client_Search(Req->prefix);
	else
		prefix = Client;

	if (!prefix)
		return IRC_WriteStrClient(Client, ERR_NOSUCHNICK_MSG,
						Client_ID(Client), Req->prefix);

	/* Forward to other server? */
	if (target != Client_ThisServer()) {
		if (!target || (Client_Type(target) != CLIENT_SERVER))
			return IRC_WriteStrClient(prefix, ERR_NOSUCHSERVER_MSG,
					Client_ID(prefix), Req->argv[2]);

		/* Forward */
		IRC_WriteStrClientPrefix( target, prefix, "WHOWAS %s %s %s",
					  Req->argv[0], Req->argv[1],
					  Req->argv[2] );
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

		if (nc == 0 && !IRC_WriteStrClient(prefix, ERR_WASNOSUCHNICK_MSG,
						Client_ID(prefix), nick))
			return DISCONNECTED;
	}
	return IRC_WriteStrClient(prefix, RPL_ENDOFWHOWAS_MSG, Client_ID(prefix), Req->argv[0]);
} /* IRC_WHOWAS */

/**
 * Send LUSERS reply to a client.
 *
 * @param Client The receipient of the information.
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

	/* Users, services and serevers in the network */
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
		return IRC_WriteStrClient(Client, ERR_NOMOTD_MSG, Client_ID(Client));

	if (!Show_MOTD_Start(Client))
		return DISCONNECTED;

	line = array_start(&Conf_Motd);
	while (len_tot > 0) {
		len_str = strlen(line) + 1;

		assert(len_tot >= len_str);
		len_tot -= len_str;

		if (!Show_MOTD_Sendline(Client, line))
			return DISCONNECTED;
		line += len_str;
	}

	if (!Show_MOTD_SSLInfo(Client))
		return DISCONNECTED;
	return Show_MOTD_End(Client);
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
	char str[LINE_LEN + 1];
	CL2CHAN *cl2chan;
	CLIENT *cl;

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
	if (!is_member && strchr(Channel_Modes(Chan), 's'))
		return CONNECTED;

	snprintf(str, sizeof(str), RPL_NAMREPLY_MSG, Client_ID(Client), "=",
		 Channel_Name(Chan));
	cl2chan = Channel_FirstMember(Chan);
	while (cl2chan) {
		cl = Channel_GetClient(cl2chan);

		if (strchr(Client_Modes(cl), 'i'))
			is_visible = false;
		else
			is_visible = true;

		if (is_member || is_visible) {
			if (str[strlen(str) - 1] != ':')
				strlcat(str, " ", sizeof(str));

			who_flags_qualifier(Client, Channel_UserModes(Chan, cl),
					    str, sizeof(str));
			strlcat(str, Client_ID(cl), sizeof(str));

			if (strlen(str) > (LINE_LEN - CLIENT_NICK_LEN - 4)) {
				if (!IRC_WriteStrClient(Client, "%s", str))
					return DISCONNECTED;
				snprintf(str, sizeof(str), RPL_NAMREPLY_MSG,
					 Client_ID(Client), "=",
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
	if (!IRC_WriteStrClient(Client, RPL_ISUPPORT1_MSG, Client_ID(Client),
				Conf_MaxJoins))
		return DISCONNECTED;
	return IRC_WriteStrClient(Client, RPL_ISUPPORT2_MSG, Client_ID(Client),
				  CHANNEL_NAME_LEN - 1, Conf_MaxNickLength - 1,
				  COMMAND_LEN - 23, CLIENT_AWAY_LEN - 1,
				  COMMAND_LEN - 113, MAX_HNDL_MODES_ARG,
				  MAX_HNDL_CHANNEL_LISTS);
} /* IRC_Send_ISUPPORT */

/* -eof- */
