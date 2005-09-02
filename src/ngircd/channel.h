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
 * $Id: channel.h,v 1.29 2005/09/02 12:50:25 alex Exp $
 *
 * Channel management (header)
 */


#ifndef __channel_h__
#define __channel_h__


#if defined(__channel_c__) | defined(S_SPLINT_S)

#include "defines.h"
#include "array.h"

typedef struct _CHANNEL
{
	struct _CHANNEL *next;
	char name[CHANNEL_NAME_LEN];	/* Name of the channel */
	UINT32 hash;			/* Hash of the (lowecase!) name */
	char modes[CHANNEL_MODE_LEN];	/* Channel modes */
	array topic;			/* Topic of the channel */
#ifndef STRICT_RFC
	time_t topic_time;		/* Time when topic was set */
	char topic_who[CLIENT_NICK_LEN];/* Nickname of user that set topic */
#endif
	char key[CLIENT_PASS_LEN];	/* Channel key ("password", mode "k" ) */
	long maxusers;			/* Maximum number of members (mode "l") */
} CHANNEL;

typedef struct _CLIENT2CHAN
{
	struct _CLIENT2CHAN *next;
	CLIENT *client;
	CHANNEL *channel;
	char modes[CHANNEL_MODE_LEN];	/* User-Modes in dem Channel */
} CL2CHAN;

#else

typedef POINTER CHANNEL;
typedef POINTER CL2CHAN;

#endif


GLOBAL void Channel_Init PARAMS(( void ));
GLOBAL void Channel_InitPredefined PARAMS((  void ));
GLOBAL void Channel_Exit PARAMS(( void ));

GLOBAL bool Channel_Join PARAMS(( CLIENT *Client, char *Name ));
GLOBAL bool Channel_Part PARAMS(( CLIENT *Client, CLIENT *Origin, char *Name, char *Reason ));

GLOBAL void Channel_Quit PARAMS(( CLIENT *Client, char *Reason ));

GLOBAL void Channel_Kick PARAMS((  CLIENT *Client, CLIENT *Origin, char *Name, char *Reason ));

GLOBAL long Channel_Count PARAMS(( void ));
GLOBAL long Channel_MemberCount PARAMS(( CHANNEL *Chan ));
GLOBAL int Channel_CountForUser PARAMS(( CLIENT *Client ));
GLOBAL int Channel_PCount PARAMS(( void ));

GLOBAL char *Channel_Name PARAMS(( CHANNEL *Chan ));
GLOBAL char *Channel_Modes PARAMS(( CHANNEL *Chan ));
GLOBAL char *Channel_Topic PARAMS(( CHANNEL *Chan ));
GLOBAL char *Channel_Key PARAMS(( CHANNEL *Chan ));
GLOBAL long Channel_MaxUsers PARAMS(( CHANNEL *Chan ));

GLOBAL void Channel_SetTopic PARAMS(( CHANNEL *Chan, CLIENT *Client, char *Topic ));
GLOBAL void Channel_SetModes PARAMS(( CHANNEL *Chan, char *Modes ));
GLOBAL void Channel_SetKey PARAMS(( CHANNEL *Chan, char *Key ));
GLOBAL void Channel_SetMaxUsers PARAMS(( CHANNEL *Chan, long Count ));

GLOBAL CHANNEL *Channel_Search PARAMS(( char *Name ));

GLOBAL CHANNEL *Channel_First PARAMS(( void ));
GLOBAL CHANNEL *Channel_Next PARAMS(( CHANNEL *Chan ));

GLOBAL CL2CHAN *Channel_FirstMember PARAMS(( CHANNEL *Chan ));
GLOBAL CL2CHAN *Channel_NextMember PARAMS(( CHANNEL *Chan, CL2CHAN *Cl2Chan ));
GLOBAL CL2CHAN *Channel_FirstChannelOf PARAMS(( CLIENT *Client ));
GLOBAL CL2CHAN *Channel_NextChannelOf PARAMS(( CLIENT *Client, CL2CHAN *Cl2Chan ));

GLOBAL CLIENT *Channel_GetClient PARAMS(( CL2CHAN *Cl2Chan ));
GLOBAL CHANNEL *Channel_GetChannel PARAMS(( CL2CHAN *Cl2Chan ));

GLOBAL bool Channel_IsValidName PARAMS(( char *Name ));

GLOBAL bool Channel_ModeAdd PARAMS(( CHANNEL *Chan, char Mode ));
GLOBAL bool Channel_ModeDel PARAMS(( CHANNEL *Chan, char Mode ));

GLOBAL bool Channel_UserModeAdd PARAMS(( CHANNEL *Chan, CLIENT *Client, char Mode ));
GLOBAL bool Channel_UserModeDel PARAMS(( CHANNEL *Chan, CLIENT *Client, char Mode ));
GLOBAL char *Channel_UserModes PARAMS(( CHANNEL *Chan, CLIENT *Client ));

GLOBAL bool Channel_IsMemberOf PARAMS(( CHANNEL *Chan, CLIENT *Client ));

GLOBAL bool Channel_Write PARAMS(( CHANNEL *Chan, CLIENT *From, CLIENT *Client, char *Text ));

GLOBAL CHANNEL *Channel_Create PARAMS(( char *Name ));

#ifndef STRICT_RFC
GLOBAL unsigned int Channel_TopicTime PARAMS(( CHANNEL *Chan ));
GLOBAL char *Channel_TopicWho PARAMS(( CHANNEL *Chan ));
#endif


#endif


/* -eof- */
