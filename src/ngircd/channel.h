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
 * $Id: channel.h,v 1.4 2002/01/02 02:42:58 alex Exp $
 *
 * channel.h: Management der Channels (Header)
 *
 * $Log: channel.h,v $
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
 *
 */


#ifndef __channel_h__
#define __channel_h__


typedef struct _CHANNEL
{
	CHAR name[CHANNEL_NAME_LEN];	/* Name */
} CHANNEL;


GLOBAL VOID Channel_Init( VOID );
GLOBAL VOID Channel_Exit( VOID );


#endif


/* -eof- */
