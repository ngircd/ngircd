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
 * $Id: irc-write.h,v 1.5.4.1 2003/11/07 20:51:11 alex Exp $
 *
 * Sending IRC commands over the network (header)
 */


#ifndef __irc_write_h__
#define __irc_write_h__


GLOBAL BOOLEAN IRC_WriteStrClient PARAMS(( CLIENT *Client, CHAR *Format, ... ));
GLOBAL BOOLEAN IRC_WriteStrClientPrefix PARAMS(( CLIENT *Client, CLIENT *Prefix, CHAR *Format, ... ));

GLOBAL BOOLEAN IRC_WriteStrChannel PARAMS(( CLIENT *Client, CHANNEL *Chan, BOOLEAN Remote, CHAR *Format, ... ));
GLOBAL BOOLEAN IRC_WriteStrChannelPrefix PARAMS(( CLIENT *Client, CHANNEL *Chan, CLIENT *Prefix, BOOLEAN Remote, CHAR *Format, ... ));

GLOBAL VOID IRC_WriteStrServers PARAMS(( CLIENT *ExceptOf, CHAR *Format, ... ));
GLOBAL VOID IRC_WriteStrServersPrefix PARAMS(( CLIENT *ExceptOf, CLIENT *Prefix, CHAR *Format, ... ));
GLOBAL VOID IRC_WriteStrServersPrefixFlag PARAMS(( CLIENT *ExceptOf, CLIENT *Prefix, CHAR Flag, CHAR *Format, ... ));

GLOBAL BOOLEAN IRC_WriteStrRelatedPrefix PARAMS(( CLIENT *Client, CLIENT *Prefix, BOOLEAN Remote, CHAR *Format, ... ));

GLOBAL VOID IRC_SetPenalty PARAMS(( CLIENT *Client, INT Seconds ));


#endif


/* -eof- */
