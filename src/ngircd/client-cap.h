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

#ifndef __client_cap_h__
#define __client_cap_h__

/**
 * @file
 * Functions to deal with IRC Capabilities (header)
 */

#define CLIENT_CAP_PENDING 1		/* Capability negotiation pending */
#define CLIENT_CAP_SUPPORTED 2		/* Client supports IRC capabilities */

#define CLIENT_CAP_MULTI_PREFIX 4	/* multi-prefix */

GLOBAL int Client_Cap PARAMS((CLIENT *Client));

GLOBAL void Client_CapSet PARAMS((CLIENT *Client, int Cap));
GLOBAL void Client_CapAdd PARAMS((CLIENT *Client, int Cap));
GLOBAL void Client_CapDel PARAMS((CLIENT *Client, int Cap));

#endif
