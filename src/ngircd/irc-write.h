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

#ifndef __irc_write_h__
#define __irc_write_h__

/**
 * @file
 * Sending IRC commands over the network (header)
 */

GLOBAL bool IRC_WriteErrClient PARAMS((CLIENT *Client, const char *Format, ...));

GLOBAL bool IRC_WriteStrClient PARAMS((CLIENT *Client, const char *Format, ...));
GLOBAL bool IRC_WriteStrClientPrefix PARAMS((CLIENT *Client, CLIENT *Prefix,
		const char *Format, ...));

GLOBAL void IRC_WriteStrChannel PARAMS((CLIENT *Client, CHANNEL *Chan,
		bool Remote, const char *Format, ...));
GLOBAL void IRC_WriteStrChannelPrefix PARAMS((CLIENT *Client, CHANNEL *Chan,
		CLIENT *Prefix, bool Remote, const char *Format, ...));

GLOBAL void IRC_WriteStrServers PARAMS((CLIENT *ExceptOf,
		const char *Format, ...));
GLOBAL void IRC_WriteStrServersPrefix PARAMS((CLIENT *ExceptOf, CLIENT *Prefix,
		const char *Format, ...));
GLOBAL void IRC_WriteStrServersPrefixFlag PARAMS((CLIENT *ExceptOf,
		CLIENT *Prefix, char Flag, const char *Format, ...));
GLOBAL void IRC_WriteStrServersPrefixFlag_CB PARAMS((CLIENT *ExceptOf,
		CLIENT *Prefix, char Flag,
		void (*callback)(CLIENT *, CLIENT *, void *), void *cb_data));

GLOBAL void IRC_WriteStrRelatedPrefix PARAMS((CLIENT *Client, CLIENT *Prefix,
		bool Remote, const char *Format, ...));

GLOBAL void IRC_SendWallops PARAMS((CLIENT *Client, CLIENT *From,
		const char *Format, ...));

GLOBAL void IRC_SetPenalty PARAMS((CLIENT *Client, time_t Seconds));

#endif

/* -eof- */
