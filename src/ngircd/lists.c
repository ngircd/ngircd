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
 * $Id: lists.c,v 1.5 2002/08/26 23:39:22 alex Exp $
 *
 * lists.c: Verwaltung der "IRC-Listen": Ban, Invite, ...
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>

#include "defines.h"
#include "conn.h"
#include "client.h"
#include "channel.h"
#include "log.h"
#include "match.h"

#include <stdlib.h>
#include <string.h>

#include "exp.h"
#include "lists.h"


#define MASK_LEN CLIENT_ID_LEN+CLIENT_HOST_LEN


typedef struct _C2C
{
	struct _C2C *next;
	CHAR mask[MASK_LEN];
	CHANNEL *channel;
	BOOLEAN onlyonce;
} C2C;


LOCAL C2C *My_Invites, *My_Bans;


LOCAL C2C *New_C2C PARAMS(( CHAR *Mask, CHANNEL *Chan, BOOLEAN OnlyOnce ));


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

	C2C *c2c, *next;

	/* Invite-Lists freigeben */
	c2c = My_Invites;
	while( c2c )
	{
		next = c2c->next;
		free( c2c );
		c2c = next;
	}

	/* Ban-Lists freigeben */
	c2c = My_Bans;
	while( c2c )
	{
		next = c2c->next;
		free( c2c );
		c2c = next;
	}
} /* Lists_Exit */


GLOBAL BOOLEAN
Lists_CheckInvited( CLIENT *Client, CHANNEL *Chan )
{
	C2C *c2c, *last;
	
	assert( Client != NULL );
	assert( Chan != NULL );

	last = NULL;
	c2c = My_Invites;
	while( c2c )
	{
		if( c2c->channel == Chan )
		{
			/* Ok, richtiger Channel. Passt die Maske? */
Log( LOG_DEBUG, "%s : %s", Client_Mask( Client ), c2c->mask );
			if( Match( Client_Mask( Client ), c2c->mask ))
			{
				/* Treffer! */
				if( c2c->onlyonce )
				{
					/* Eintrag loeschen */
					if( last ) last->next = c2c->next;
					else My_Invites = c2c->next;
					free( c2c );
				}
				return TRUE;
			}
		}
		last = c2c;
		c2c = c2c->next;
	}
	
	return FALSE;
} /* Lists_CheckInvited */


GLOBAL BOOLEAN
Lists_AddInvited( CHAR *Mask, CHANNEL *Chan, BOOLEAN OnlyOnce )
{
	C2C *c2c;

	assert( Mask != NULL );
	assert( Chan != NULL );

	c2c = New_C2C( Mask, Chan, OnlyOnce );
	if( ! c2c )
	{
		Log( LOG_ERR, "Can't add new invite list entry!" );
		return FALSE;
	}

	/* verketten */
	c2c->next = My_Invites;
	My_Invites = c2c;

	Log( LOG_DEBUG, "Added \"%s\" to invite list for \"%s\".", Mask, Channel_Name( Chan ));
	return TRUE;
} /* Lists_AddInvited */


GLOBAL BOOLEAN
Lists_CheckBanned( CLIENT *Client, CHANNEL *Chan )
{
	assert( Client != NULL );
	assert( Chan != NULL );

	return FALSE;
} /* Lists_CheckBanned */


GLOBAL VOID
Lists_DeleteChannel( CHANNEL *Chan )
{
	/* Channel wurde geloescht, Invite- und Ban-Lists aufraeumen */

	C2C *c2c, *last, *next;

	/* Invite-List */
	last = NULL;
	c2c = My_Invites;
	while( c2c )
	{
		next = c2c->next;
		if( c2c->channel == Chan )
		{
			/* dieser Eintrag muss geloescht werden */
			if( last ) last->next = next;
			else My_Invites = next;
			free( c2c );
		}
		else last = c2c;
		c2c = next;
	}

	/* Ban-List */
	last = NULL;
	c2c = My_Bans;
	while( c2c )
	{
		next = c2c->next;
		if( c2c->channel == Chan )
		{
			/* dieser Eintrag muss geloescht werden */
			if( last ) last->next = next;
			else My_Bans = next;
			free( c2c );
		}
		else last = c2c;
		c2c = next;
	}
} /* Lists_DeleteChannel */


GLOBAL CHAR *
Lists_MakeMask( CHAR *Pattern )
{
	assert( Pattern );

	/* Hier sollte aus einem "beliebigen" Pattern eine
	 * gueltige IRC-Mask erzeugt werden ... */
	
	return Pattern;
} /* Lists_MakeMask */


LOCAL C2C *
New_C2C( CHAR *Mask, CHANNEL *Chan, BOOLEAN OnlyOnce )
{
	C2C *c2c;
	
	assert( Mask != NULL );
	assert( Chan != NULL );

	/* Speicher fuer Eintrag anfordern */
	c2c = malloc( sizeof( C2C ));
	if( ! c2c )
	{
		Log( LOG_EMERG, "Can't allocate memory! [New_C2C]" );
		return NULL;
	}

	strncpy( c2c->mask, Mask, MASK_LEN );
	c2c->channel = Chan;
	c2c->onlyonce = OnlyOnce;
	
	return c2c;
} /* New_C2C */


/* -eof- */
