/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an comBase beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: channel.h,v 1.2 2001/12/23 21:54:30 alex Exp $
 *
 * channel.h: Management der Channels (Header)
 *
 * $Log: channel.h,v $
 * Revision 1.2  2001/12/23 21:54:30  alex
 * - Konstanten um Prefix "CHANNEL_" erweitert.
 *
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 *
 */


#ifndef __channel_h__
#define __channel_h__


#define CHANNEL_NAME_LEN 50			/* vgl. RFC 2812, 1.3 */


typedef struct _CHANNEL
{
	CHAR name[CHANNEL_NAME_LEN + 1];	/* Name */
} CHANNEL;


GLOBAL VOID Channel_Init( VOID );
GLOBAL VOID Channel_Exit( VOID );


#endif


/* -eof- */
