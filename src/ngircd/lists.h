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
 * $Id: lists.h,v 1.12 2005/03/19 18:43:49 fw Exp $
 *
 * Management of IRC lists: ban, invite, ... (header)
 */


#ifndef __lists_h__
#define __lists_h__


GLOBAL void Lists_Init PARAMS(( void ));
GLOBAL void Lists_Exit PARAMS(( void ));

GLOBAL bool Lists_CheckInvited PARAMS(( CLIENT *Client, CHANNEL *Chan ));
GLOBAL bool Lists_AddInvited PARAMS(( char *Mask, CHANNEL *Chan, bool OnlyOnce ));
GLOBAL void Lists_DelInvited PARAMS(( char *Mask, CHANNEL *Chan ));
GLOBAL bool Lists_ShowInvites PARAMS(( CLIENT *Client, CHANNEL *Channel ));
GLOBAL bool Lists_SendInvites PARAMS(( CLIENT *Client ));
GLOBAL bool Lists_IsInviteEntry PARAMS(( char *Mask, CHANNEL *Chan ));

GLOBAL bool Lists_CheckBanned PARAMS(( CLIENT *Client, CHANNEL *Chan ));
GLOBAL bool Lists_AddBanned PARAMS(( char *Mask, CHANNEL *Chan ));
GLOBAL void Lists_DelBanned PARAMS(( char *Mask, CHANNEL *Chan ));
GLOBAL bool Lists_ShowBans PARAMS(( CLIENT *Client, CHANNEL *Channel ));
GLOBAL bool Lists_SendBans PARAMS(( CLIENT *Client ));
GLOBAL bool Lists_IsBanEntry PARAMS(( char *Mask, CHANNEL *Chan ));

GLOBAL void Lists_DeleteChannel PARAMS(( CHANNEL *Chan ));

GLOBAL char *Lists_MakeMask PARAMS(( char *Pattern ));


#endif


/* -eof- */
