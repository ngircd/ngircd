/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: channel.c,v 1.2 2001/12/31 02:18:51 alex Exp $
 *
 * channel.c: Management der Channels
 *
 * $Log: channel.c,v $
 * Revision 1.2  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 *
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>

#include <exp.h>
#include "channel.h"


LOCAL CHANNEL *My_Channels;


GLOBAL VOID Channel_Init( VOID )
{
	My_Channels = NULL;
} /* Channel_Init */


GLOBAL VOID Channel_Exit( VOID )
{
} /* Channel_Exit */


/* -eof- */
