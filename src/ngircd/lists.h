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
 * $Id: lists.h,v 1.10 2004/04/09 21:41:52 alex Exp $
 *
 * Management of IRC lists: ban, invite, ... (header)
 */


#ifndef __lists_h__
#define __lists_h__


GLOBAL VOID Lists_Init PARAMS(( VOID ));
GLOBAL VOID Lists_Exit PARAMS(( VOID ));

GLOBAL BOOLEAN Lists_CheckInvited PARAMS(( CLIENT *Client, CHANNEL *Chan ));
GLOBAL BOOLEAN Lists_AddInvited PARAMS(( CHAR *Mask, CHANNEL *Chan, BOOLEAN OnlyOnce ));
GLOBAL VOID Lists_DelInvited PARAMS(( CHAR *Mask, CHANNEL *Chan ));
GLOBAL BOOLEAN Lists_ShowInvites PARAMS(( CLIENT *Client, CHANNEL *Channel ));

GLOBAL BOOLEAN Lists_CheckBanned PARAMS(( CLIENT *Client, CHANNEL *Chan ));
GLOBAL BOOLEAN Lists_AddBanned PARAMS(( CHAR *Mask, CHANNEL *Chan ));
GLOBAL VOID Lists_DelBanned PARAMS(( CHAR *Mask, CHANNEL *Chan ));
GLOBAL BOOLEAN Lists_ShowBans PARAMS(( CLIENT *Client, CHANNEL *Channel ));

GLOBAL VOID Lists_DeleteChannel PARAMS(( CHANNEL *Chan ));

GLOBAL CHAR *Lists_MakeMask PARAMS(( CHAR *Pattern ));


#endif


/* -eof- */
