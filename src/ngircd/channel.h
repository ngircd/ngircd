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
 * $Id: channel.h,v 1.13 2002/02/27 20:32:10 alex Exp $
 *
 * channel.h: Management der Channels (Header)
 *
 * $Log: channel.h,v $
 * Revision 1.13  2002/02/27 20:32:10  alex
 * - neue Funktionen Channel_Topic() und Channel_SetTopic().
 *
 * Revision 1.12  2002/02/27 15:21:21  alex
 * - neue Funktion Channel_IsMemberOf() implementiert.
 *
 * Revision 1.11  2002/02/11 01:00:22  alex
 * - neue Funktionen Channel_ModeAdd(), Channel_ModeDel(), Channel_UserModes(),
 *   Channel_UserModeAdd(), Channel_UserModeDel().
 *
 * Revision 1.10  2002/02/06 16:49:10  alex
 * - neue Funktionen Channel_Modes() und Channel_IsValidName().
 *
 * Revision 1.9  2002/01/29 00:11:19  alex
 * - neue Funktionen Channel_FirstChannelOf() und Channel_NextChannelOf().
 *
 * Revision 1.8  2002/01/28 01:16:15  alex
 * - neue Funktionen Channel_Name(), Channel_First() und Channel_Next().
 *
 * Revision 1.7  2002/01/26 18:41:55  alex
 * - CHANNEL- und CL2CHAN-Strukturen in Header verlegt,
 * - einige neue Funktionen (Channel_GetChannel(), Channel_FirstMember(), ...)
 *
 * Revision 1.6  2002/01/21 00:11:59  alex
 * - Definition der CHANNEL-Struktur aus Header entfernt,
 * - neue Funktionen Channel_Join(), Channel_Part() und Channel_RemoveClient().
 *
 * Revision 1.5  2002/01/16 22:09:07  alex
 * - neue Funktion Channel_Count().
 *
 * Revision 1.4  2002/01/02 02:42:58  alex
 * - Copyright-Texte aktualisiert.
 *
 * Revision 1.3  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.2  2001/12/23 21:54:30  alex
 * - Konstanten um Prefix "CHANNEL_" erweitert.
 *
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 */


#ifndef __channel_h__
#define __channel_h__

#include "client.h"


#ifdef __channel_c__

typedef struct _CHANNEL
{
	struct _CHANNEL *next;
	CHAR name[CHANNEL_NAME_LEN];	/* Name des Channel */
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


#endif


/* -eof- */
