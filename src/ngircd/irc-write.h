/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: irc-write.h,v 1.7 2005/03/19 18:43:49 fw Exp $
 *
 * Sending IRC commands over the network (header)
 */


#ifndef __irc_write_h__
#define __irc_write_h__


GLOBAL bool IRC_WriteStrClient PARAMS(( CLIENT *Client, char *Format, ... ));
GLOBAL bool IRC_WriteStrClientPrefix PARAMS(( CLIENT *Client, CLIENT *Prefix, char *Format, ... ));

GLOBAL bool IRC_WriteStrChannel PARAMS(( CLIENT *Client, CHANNEL *Chan, bool Remote, char *Format, ... ));
GLOBAL bool IRC_WriteStrChannelPrefix PARAMS(( CLIENT *Client, CHANNEL *Chan, CLIENT *Prefix, bool Remote, char *Format, ... ));

GLOBAL void IRC_WriteStrServers PARAMS(( CLIENT *ExceptOf, char *Format, ... ));
GLOBAL void IRC_WriteStrServersPrefix PARAMS(( CLIENT *ExceptOf, CLIENT *Prefix, char *Format, ... ));
GLOBAL void IRC_WriteStrServersPrefixFlag PARAMS(( CLIENT *ExceptOf, CLIENT *Prefix, char Flag, char *Format, ... ));

GLOBAL bool IRC_WriteStrRelatedPrefix PARAMS(( CLIENT *Client, CLIENT *Prefix, bool Remote, char *Format, ... ));

GLOBAL void IRC_SetPenalty PARAMS(( CLIENT *Client, int Seconds ));


#endif


/* -eof- */
