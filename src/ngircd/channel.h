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
 * $Id: channel.h,v 1.16 2002/03/25 19:11:01 alex Exp $
 *
 * channel.h: Management der Channels (Header)
 */


#ifndef __channel_h__
#define __channel_h__

#include "client.h"


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


GLOBAL VOID Channel_Init( VOID );
GLOBAL VOID Channel_Exit( VOID );

GLOBAL BOOLEAN Channel_Join( CLIENT *Client, CHAR *Name );
GLOBAL BOOLEAN Channel_Part( CLIENT *Client, CLIENT *Origin, CHAR *Name, CHAR *Reason );

GLOBAL VOID Channel_RemoveClient( CLIENT *Client, CHAR *Reason );

GLOBAL INT Channel_Count( VOID );

GLOBAL CHAR *Channel_Name( CHANNEL *Chan );
GLOBAL CHAR *Channel_Modes( CHANNEL *Chan );
GLOBAL CHAR *Channel_Topic( CHANNEL *Chan );

GLOBAL VOID Channel_SetTopic( CHANNEL *Chan, CHAR *Topic );

GLOBAL CHANNEL *Channel_Search( CHAR *Name );

GLOBAL CHANNEL *Channel_First( VOID );
GLOBAL CHANNEL *Channel_Next( CHANNEL *Chan );

GLOBAL CL2CHAN *Channel_FirstMember( CHANNEL *Chan );
GLOBAL CL2CHAN *Channel_NextMember( CHANNEL *Chan, CL2CHAN *Cl2Chan );
GLOBAL CL2CHAN *Channel_FirstChannelOf( CLIENT *Client );
GLOBAL CL2CHAN *Channel_NextChannelOf( CLIENT *Client, CL2CHAN *Cl2Chan );

GLOBAL CLIENT *Channel_GetClient( CL2CHAN *Cl2Chan );
GLOBAL CHANNEL *Channel_GetChannel( CL2CHAN *Cl2Chan );

GLOBAL BOOLEAN Channel_IsValidName( CHAR *Name );

GLOBAL BOOLEAN Channel_ModeAdd( CHANNEL *Chan, CHAR Mode );
GLOBAL BOOLEAN Channel_ModeDel( CHANNEL *Chan, CHAR Mode );

GLOBAL BOOLEAN Channel_UserModeAdd( CHANNEL *Chan, CLIENT *Client, CHAR Mode );
GLOBAL BOOLEAN Channel_UserModeDel( CHANNEL *Chan, CLIENT *Client, CHAR Mode );
GLOBAL CHAR *Channel_UserModes( CHANNEL *Chan, CLIENT *Client );

GLOBAL BOOLEAN Channel_IsMemberOf( CHANNEL *Chan, CLIENT *Client );

GLOBAL BOOLEAN Channel_Write( CHANNEL *Chan, CLIENT *From, CLIENT *Client, CHAR *Text );


#endif


/* -eof- */
