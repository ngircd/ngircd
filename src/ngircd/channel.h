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
 * $Id: channel.h,v 1.24 2002/12/13 17:22:57 alex Exp $
 *
 * Channel management (header)
 */


#ifndef __channel_h__
#define __channel_h__


#if defined(__channel_c__) | defined(S_SPLINT_S)

#include "defines.h"

typedef struct _CHANNEL
{
	struct _CHANNEL *next;
	CHAR name[CHANNEL_NAME_LEN];	/* Name des Channel */
	UINT32 hash;			/* Hash ueber (kleingeschrieben) Namen */
	CHAR modes[CHANNEL_MODE_LEN];	/* Channel-Modes */
	CHAR topic[CHANNEL_TOPIC_LEN];	/* Topic des Channels */
} CHANNEL;

typedef struct _CLIENT2CHAN
{
	struct _CLIENT2CHAN *next;
	CLIENT *client;
	CHANNEL *channel;
	CHAR modes[CHANNEL_MODE_LEN];	/* User-Modes in dem Channel */
} CL2CHAN;

#else

typedef POINTER CHANNEL;
typedef POINTER CL2CHAN;

#endif


GLOBAL VOID Channel_Init PARAMS(( VOID ));
GLOBAL VOID Channel_InitPredefined PARAMS((  VOID ));
GLOBAL VOID Channel_Exit PARAMS(( VOID ));

GLOBAL BOOLEAN Channel_Join PARAMS(( CLIENT *Client, CHAR *Name ));
GLOBAL BOOLEAN Channel_Part PARAMS(( CLIENT *Client, CLIENT *Origin, CHAR *Name, CHAR *Reason ));

GLOBAL VOID Channel_Quit PARAMS(( CLIENT *Client, CHAR *Reason ));

GLOBAL VOID Channel_Kick PARAMS((  CLIENT *Client, CLIENT *Origin, CHAR *Name, CHAR *Reason ));

GLOBAL LONG Channel_Count PARAMS(( VOID ));
GLOBAL LONG Channel_MemberCount PARAMS(( CHANNEL *Chan ));
GLOBAL INT Channel_CountForUser PARAMS(( CHANNEL *Chan, CLIENT *Client ));

GLOBAL CHAR *Channel_Name PARAMS(( CHANNEL *Chan ));
GLOBAL CHAR *Channel_Modes PARAMS(( CHANNEL *Chan ));
GLOBAL CHAR *Channel_Topic PARAMS(( CHANNEL *Chan ));

GLOBAL VOID Channel_SetTopic PARAMS(( CHANNEL *Chan, CHAR *Topic ));
GLOBAL VOID Channel_SetModes PARAMS(( CHANNEL *Chan, CHAR *Modes ));

GLOBAL CHANNEL *Channel_Search PARAMS(( CHAR *Name ));

GLOBAL CHANNEL *Channel_First PARAMS(( VOID ));
GLOBAL CHANNEL *Channel_Next PARAMS(( CHANNEL *Chan ));

GLOBAL CL2CHAN *Channel_FirstMember PARAMS(( CHANNEL *Chan ));
GLOBAL CL2CHAN *Channel_NextMember PARAMS(( CHANNEL *Chan, CL2CHAN *Cl2Chan ));
GLOBAL CL2CHAN *Channel_FirstChannelOf PARAMS(( CLIENT *Client ));
GLOBAL CL2CHAN *Channel_NextChannelOf PARAMS(( CLIENT *Client, CL2CHAN *Cl2Chan ));

GLOBAL CLIENT *Channel_GetClient PARAMS(( CL2CHAN *Cl2Chan ));
GLOBAL CHANNEL *Channel_GetChannel PARAMS(( CL2CHAN *Cl2Chan ));

GLOBAL BOOLEAN Channel_IsValidName PARAMS(( CHAR *Name ));

GLOBAL BOOLEAN Channel_ModeAdd PARAMS(( CHANNEL *Chan, CHAR Mode ));
GLOBAL BOOLEAN Channel_ModeDel PARAMS(( CHANNEL *Chan, CHAR Mode ));

GLOBAL BOOLEAN Channel_UserModeAdd PARAMS(( CHANNEL *Chan, CLIENT *Client, CHAR Mode ));
GLOBAL BOOLEAN Channel_UserModeDel PARAMS(( CHANNEL *Chan, CLIENT *Client, CHAR Mode ));
GLOBAL CHAR *Channel_UserModes PARAMS(( CHANNEL *Chan, CLIENT *Client ));

GLOBAL BOOLEAN Channel_IsMemberOf PARAMS(( CHANNEL *Chan, CLIENT *Client ));

GLOBAL BOOLEAN Channel_Write PARAMS(( CHANNEL *Chan, CLIENT *From, CLIENT *Client, CHAR *Text ));

GLOBAL CHANNEL *Channel_Create PARAMS(( CHAR *Name ));


#endif


/* -eof- */
