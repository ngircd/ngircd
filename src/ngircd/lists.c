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
 * $Id: lists.c,v 1.1 2002/05/27 11:22:39 alex Exp $
 *
 * lists.c: Verwaltung der "IRC-Listen": Ban, Invite, ...
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>

#include "conn.h"
#include "client.h"
#include "channel.h"

#include "exp.h"
#include "lists.h"


typedef struct _C2C
{
	struct _C2C *next;
	CLIENT *client;
	CHANNEL *channel;
} C2C;


LOCAL C2C *My_Invites, *My_Bans;


LOCAL C2C *New_C2C  PARAMS(( CLIENT *Client, CHANNEL *Chan ));


GLOBAL VOID
Lists_Init( VOID )
{
	/* Modul initialisieren */

	My_Invites = My_Bans = NULL;
} /* Lists_Init */


GLOBAL VOID
Lists_Exit( VOID )
{
	/* Modul abmelden */
} /* Lists_Exit */


GLOBAL BOOLEAN
Lists_CheckInvited( CLIENT *Client, CHANNEL *Chan )
{
	assert( Client != NULL );
	assert( Chan != NULL );

	return FALSE;
} /* Lists_CheckInvited */


GLOBAL BOOLEAN
Lists_CheckBanned( CLIENT *Client, CHANNEL *Chan )
{
	assert( Client != NULL );
	assert( Chan != NULL );

	return FALSE;
} /* Lists_CheckBanned */


LOCAL C2C *
New_C2C( CLIENT *Client, CHANNEL *Chan )
{
	assert( Client != NULL );
	assert( Chan != NULL );

	return NULL;
} /* New_C2C */


/* -eof- */
