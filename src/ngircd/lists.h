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
 * $Id: lists.h,v 1.6 2002/08/26 23:47:58 alex Exp $
 *
 * lists.h: Verwaltung der "IRC-Listen": Ban, Invite, ... (Header)
 */


#ifndef __lists_h__
#define __lists_h__


GLOBAL VOID Lists_Init PARAMS(( VOID ));
GLOBAL VOID Lists_Exit PARAMS(( VOID ));

GLOBAL BOOLEAN Lists_CheckInvited PARAMS(( CLIENT *Client, CHANNEL *Chan ));
GLOBAL BOOLEAN Lists_AddInvited PARAMS(( CHAR *Pattern, CHANNEL *Chan, BOOLEAN OnlyOnce ));

GLOBAL BOOLEAN Lists_CheckBanned PARAMS(( CLIENT *Client, CHANNEL *Chan ));

GLOBAL VOID Lists_DeleteChannel PARAMS(( CHANNEL *Chan ));


#endif


/* -eof- */
