/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: channel.h,v 1.21.2.1 2002/11/04 19:18:39 alex Exp $
 *
 * channel.h: Management der Channels (Header)
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


GLOBAL VOID Channel_Init PARAMS((VOID ));
GLOBAL VOID Channel_InitPredefined PARAMS(( VOID ));
GLOBAL VOID Channel_Exit PARAMS((VOID ));

GLOBAL BOOLEAN Channel_Join PARAMS((CLIENT *Client, CHAR *Name ));
GLOBAL BOOLEAN Channel_Part PARAMS((CLIENT *Client, CLIENT *Origin, CHAR *Name, CHAR *Reason ));

GLOBAL VOID Channel_Quit PARAMS((CLIENT *Client, CHAR *Reason ));

GLOBAL VOID Channel_Kick PARAMS(( CLIENT *Client, CLIENT *Origin, CHAR *Name, CHAR *Reason ));

GLOBAL LONG Channel_Count PARAMS((VOID ));
GLOBAL LONG Channel_MemberCount PARAMS((CHANNEL *Chan ));

GLOBAL CHAR *Channel_Name PARAMS((CHANNEL *Chan ));
GLOBAL CHAR *Channel_Modes PARAMS((CHANNEL *Chan ));
GLOBAL CHAR *Channel_Topic PARAMS((CHANNEL *Chan ));

GLOBAL VOID Channel_SetTopic PARAMS((CHANNEL *Chan, CHAR *Topic ));
GLOBAL VOID Channel_SetModes PARAMS((CHANNEL *Chan, CHAR *Modes ));

GLOBAL CHANNEL *Channel_Search PARAMS((CHAR *Name ));

GLOBAL CHANNEL *Channel_First PARAMS((VOID ));
GLOBAL CHANNEL *Channel_Next PARAMS((CHANNEL *Chan ));

GLOBAL CL2CHAN *Channel_FirstMember PARAMS((CHANNEL *Chan ));
GLOBAL CL2CHAN *Channel_NextMember PARAMS((CHANNEL *Chan, CL2CHAN *Cl2Chan ));
GLOBAL CL2CHAN *Channel_FirstChannelOf PARAMS((CLIENT *Client ));
GLOBAL CL2CHAN *Channel_NextChannelOf PARAMS((CLIENT *Client, CL2CHAN *Cl2Chan ));

GLOBAL CLIENT *Channel_GetClient PARAMS((CL2CHAN *Cl2Chan ));
GLOBAL CHANNEL *Channel_GetChannel PARAMS((CL2CHAN *Cl2Chan ));

GLOBAL BOOLEAN Channel_IsValidName PARAMS((CHAR *Name ));

GLOBAL BOOLEAN Channel_ModeAdd PARAMS((CHANNEL *Chan, CHAR Mode ));
GLOBAL BOOLEAN Channel_ModeDel PARAMS((CHANNEL *Chan, CHAR Mode ));

GLOBAL BOOLEAN Channel_UserModeAdd PARAMS((CHANNEL *Chan, CLIENT *Client, CHAR Mode ));
GLOBAL BOOLEAN Channel_UserModeDel PARAMS((CHANNEL *Chan, CLIENT *Client, CHAR Mode ));
GLOBAL CHAR *Channel_UserModes PARAMS((CHANNEL *Chan, CLIENT *Client ));

GLOBAL BOOLEAN Channel_IsMemberOf PARAMS((CHANNEL *Chan, CLIENT *Client ));

GLOBAL BOOLEAN Channel_Write PARAMS((CHANNEL *Chan, CLIENT *From, CLIENT *Client, CHAR *Text ));

GLOBAL CHANNEL *Channel_Create PARAMS((CHAR *Name ));


#endif


/* -eof- */
