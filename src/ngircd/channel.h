/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2024 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __channel_h__
#define __channel_h__

/**
 * @file
 * Channel management (header)
 */

#if defined(__channel_c__)

#include "lists.h"
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
	time_t creation_time;		/* Channel creation time */
	time_t topic_time;		/* Time when topic was set */
	char topic_who[CLIENT_NICK_LEN];/* Nickname of user that set topic */
#endif
	char key[CLIENT_PASS_LEN];	/* Channel key ("password", mode "k" ) */
	unsigned long maxusers;		/* Maximum number of members (mode "l") */
	struct list_head list_bans;	/* list head of banned users */
	struct list_head list_excepts;	/* list head of (ban) exception list */
	struct list_head list_invites;	/* list head of invited users */
	array keyfile;			/* Name of the channel key file */
} CHANNEL;

typedef struct _CLIENT2CHAN
{
	struct _CLIENT2CHAN *next;
	CLIENT *client;
	CHANNEL *channel;
	char modes[CHANNEL_MODE_LEN];	/* User-Modes in Channel */
} CL2CHAN;

#else

typedef POINTER CHANNEL;
typedef POINTER CL2CHAN;

#endif

GLOBAL struct list_head *Channel_GetListBans PARAMS((CHANNEL *c));
GLOBAL struct list_head *Channel_GetListExcepts PARAMS((CHANNEL *c));
GLOBAL struct list_head *Channel_GetListInvites PARAMS((CHANNEL *c));

GLOBAL void Channel_Init PARAMS(( void ));
GLOBAL void Channel_InitPredefined PARAMS((  void ));
GLOBAL void Channel_Exit PARAMS(( void ));

GLOBAL bool Channel_Join PARAMS(( CLIENT *Client, const char *Name ));
GLOBAL bool Channel_Part PARAMS(( CLIENT *Client, CLIENT *Origin, const char *Name, const char *Reason ));

GLOBAL void Channel_Quit PARAMS(( CLIENT *Client, const char *Reason ));

GLOBAL void Channel_Kick PARAMS((CLIENT *Peer, CLIENT *Target, CLIENT *Origin,
				 const char *Name, const char *Reason));

GLOBAL unsigned long Channel_CountVisible PARAMS((CLIENT *Client));
GLOBAL unsigned long Channel_MemberCount PARAMS(( CHANNEL *Chan ));
GLOBAL int Channel_CountForUser PARAMS(( CLIENT *Client ));

GLOBAL const char *Channel_Name PARAMS(( const CHANNEL *Chan ));
GLOBAL char *Channel_Topic PARAMS(( CHANNEL *Chan ));
GLOBAL char *Channel_Key PARAMS(( CHANNEL *Chan ));
GLOBAL unsigned long Channel_MaxUsers PARAMS(( CHANNEL *Chan ));

GLOBAL void Channel_SetTopic PARAMS(( CHANNEL *Chan, CLIENT *Client, const char *Topic ));
GLOBAL void Channel_SetModes PARAMS(( CHANNEL *Chan, const char *Modes ));
GLOBAL void Channel_SetKey PARAMS(( CHANNEL *Chan, const char *Key ));
GLOBAL void Channel_SetMaxUsers PARAMS(( CHANNEL *Chan, unsigned long Count ));

GLOBAL CHANNEL *Channel_Search PARAMS(( const char *Name ));

GLOBAL CHANNEL *Channel_First PARAMS(( void ));
GLOBAL CHANNEL *Channel_Next PARAMS(( CHANNEL *Chan ));

GLOBAL CL2CHAN *Channel_FirstMember PARAMS(( CHANNEL *Chan ));
GLOBAL CL2CHAN *Channel_NextMember PARAMS(( CHANNEL *Chan, CL2CHAN *Cl2Chan ));
GLOBAL CL2CHAN *Channel_FirstChannelOf PARAMS(( CLIENT *Client ));
GLOBAL CL2CHAN *Channel_NextChannelOf PARAMS(( CLIENT *Client, CL2CHAN *Cl2Chan ));

GLOBAL CLIENT *Channel_GetClient PARAMS(( CL2CHAN *Cl2Chan ));
GLOBAL CHANNEL *Channel_GetChannel PARAMS(( CL2CHAN *Cl2Chan ));

GLOBAL bool Channel_IsValidName PARAMS(( const char *Name ));

GLOBAL bool Channel_ModeAdd PARAMS(( CHANNEL *Chan, char Mode ));
GLOBAL bool Channel_ModeDel PARAMS(( CHANNEL *Chan, char Mode ));
GLOBAL bool Channel_HasMode PARAMS(( CHANNEL *Chan, char Mode ));
GLOBAL char *Channel_Modes PARAMS(( CHANNEL *Chan ));

GLOBAL bool Channel_UserModeAdd PARAMS(( CHANNEL *Chan, CLIENT *Client, char Mode ));
GLOBAL bool Channel_UserModeDel PARAMS(( CHANNEL *Chan, CLIENT *Client, char Mode ));
GLOBAL bool Channel_UserHasMode PARAMS(( CHANNEL *Chan, CLIENT *Client, char Mode ));
GLOBAL char *Channel_UserModes PARAMS(( CHANNEL *Chan, CLIENT *Client ));

GLOBAL bool Channel_IsMemberOf PARAMS(( CHANNEL *Chan, CLIENT *Client ));

GLOBAL bool Channel_Write PARAMS((CHANNEL *Chan, CLIENT *From, CLIENT *Client,
				  const char *Command, bool SendErrors,
				  const char *Text));

GLOBAL CHANNEL *Channel_Create PARAMS(( const char *Name ));

#ifndef STRICT_RFC
GLOBAL unsigned int Channel_TopicTime PARAMS(( CHANNEL *Chan ));
GLOBAL char *Channel_TopicWho PARAMS(( CHANNEL *Chan ));
GLOBAL unsigned int Channel_CreationTime PARAMS(( CHANNEL *Chan ));
#endif

GLOBAL bool Channel_AddBan PARAMS((CHANNEL *c, const char *Mask, const char *who));
GLOBAL bool Channel_AddExcept PARAMS((CHANNEL *c, const char *Mask, const char *who));
GLOBAL bool Channel_AddInvite PARAMS((CHANNEL *c, const char *Mask,
				      bool OnlyOnce, const char *who));

GLOBAL bool Channel_ShowBans PARAMS((CLIENT *client, CHANNEL *c));
GLOBAL bool Channel_ShowExcepts PARAMS((CLIENT *client, CHANNEL *c));
GLOBAL bool Channel_ShowInvites PARAMS((CLIENT *client, CHANNEL *c));

GLOBAL void Channel_LogServer PARAMS((const char *msg));

GLOBAL bool Channel_CheckKey PARAMS((CHANNEL *Chan, CLIENT *Client,
				     const char *Key));

GLOBAL void Channel_CheckAdminRights PARAMS((CHANNEL *Chan, CLIENT *Client,
					     CLIENT *Origin, bool *OnChannel,
					     bool *AdminOk, bool *UseServerMode));

#define Channel_IsLocal(c) (Channel_Name(c)[0] == '&')
#define Channel_IsModeless(c) (Channel_Name(c)[0] == '+')

#endif

/* -eof- */
