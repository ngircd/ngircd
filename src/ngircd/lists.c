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
 * $Id: lists.c,v 1.7 2002/09/08 00:55:45 alex Exp $
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
#include "messages.h"
#include "irc-write.h"

#include <stdlib.h>
#include <string.h>

#include "exp.h"
#include "lists.h"


#define MASK_LEN 2*CLIENT_HOST_LEN


typedef struct _C2C
{
	struct _C2C *next;
	CHAR mask[MASK_LEN];
	CHANNEL *channel;
	BOOLEAN onlyonce;
} C2C;


LOCAL C2C *My_Invites, *My_Bans;


LOCAL C2C *New_C2C PARAMS(( CHAR *Mask, CHANNEL *Chan, BOOLEAN OnlyOnce ));

LOCAL BOOLEAN Check_List PARAMS(( C2C **Cl2Chan, CLIENT *Client, CHANNEL *Chan ));



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
	return Check_List( &My_Invites, Client, Chan );
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


GLOBAL VOID
Lists_DelInvited( CHAR *Mask, CHANNEL *Chan )
{
	C2C *c2c, *last, *next;

	assert( Mask != NULL );
	assert( Chan != NULL );

	last = NULL;
	c2c = My_Invites;
	while( c2c )
	{
		next = c2c->next;
		if(( c2c->channel == Chan ) && ( strcasecmp( c2c->mask, Mask ) == 0 ))
		{
			/* dieser Eintrag muss geloescht werden */
			Log( LOG_DEBUG, "Deleted \"%s\" from invite list for \"%s\"." , c2c->mask, Channel_Name( Chan ));
			if( last ) last->next = next;
			else My_Invites = next;
			free( c2c );
		}
		else last = c2c;
		c2c = next;
	}
} /* Lists_DelInvited */


GLOBAL BOOLEAN
Lists_ShowInvites( CLIENT *Client, CHANNEL *Channel )
{
	C2C *c2c;

	assert( Client != NULL );
	assert( Channel != NULL );

	c2c = My_Invites;
	while( c2c )
	{
		if( c2c->channel == Channel )
		{
			/* Eintrag fuer Channel gefunden; ausgeben: */
			if( ! IRC_WriteStrClient( Client, RPL_INVITELIST_MSG, Client_ID( Client ), Channel_Name( Channel ), c2c->mask )) return DISCONNECTED;
		}
		c2c = c2c->next;
	}
	return IRC_WriteStrClient( Client, RPL_ENDOFINVITELIST_MSG, Client_ID( Client ), Channel_Name( Channel ));
} /* Lists_ShowInvites */


GLOBAL BOOLEAN
Lists_CheckBanned( CLIENT *Client, CHANNEL *Chan )
{
	return Check_List( &My_Bans, Client, Chan );
} /* Lists_CheckBanned */


GLOBAL BOOLEAN
Lists_AddBanned( CHAR *Mask, CHANNEL *Chan )
{
	C2C *c2c;

	assert( Mask != NULL );
	assert( Chan != NULL );

	c2c = New_C2C( Mask, Chan, FALSE );
	if( ! c2c )
	{
		Log( LOG_ERR, "Can't add new ban list entry!" );
		return FALSE;
	}

	/* verketten */
	c2c->next = My_Bans;
	My_Bans = c2c;

	Log( LOG_DEBUG, "Added \"%s\" to ban list for \"%s\".", Mask, Channel_Name( Chan ));
	return TRUE;
} /* Lists_AddBanned */


GLOBAL VOID
Lists_DelBanned( CHAR *Mask, CHANNEL *Chan )
{
	C2C *c2c, *last, *next;

	assert( Mask != NULL );
	assert( Chan != NULL );

	last = NULL;
	c2c = My_Bans;
	while( c2c )
	{
		next = c2c->next;
		if(( c2c->channel == Chan ) && ( strcasecmp( c2c->mask, Mask ) == 0 ))
		{
			/* dieser Eintrag muss geloescht werden */
			Log( LOG_DEBUG, "Deleted \"%s\" from ban list for \"%s\"." , c2c->mask, Channel_Name( Chan ));
			if( last ) last->next = next;
			else My_Bans = next;
			free( c2c );
		}
		else last = c2c;
		c2c = next;
	}
} /* Lists_DelBanned */


GLOBAL BOOLEAN
Lists_ShowBans( CLIENT *Client, CHANNEL *Channel )
{
	C2C *c2c;

	assert( Client != NULL );
	assert( Channel != NULL );

	c2c = My_Bans;
	while( c2c )
	{
		if( c2c->channel == Channel )
		{
			/* Eintrag fuer Channel gefunden; ausgeben: */
			if( ! IRC_WriteStrClient( Client, RPL_BANLIST_MSG, Client_ID( Client ), Channel_Name( Channel ), c2c->mask )) return DISCONNECTED;
		}
		c2c = c2c->next;
	}
	return IRC_WriteStrClient( Client, RPL_ENDOFBANLIST_MSG, Client_ID( Client ), Channel_Name( Channel ));
} /* Lists_ShowBans */


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
			Log( LOG_DEBUG, "Deleted \"%s\" from invite list for \"%s\"." , c2c->mask, Channel_Name( Chan ));
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
			Log( LOG_DEBUG, "Deleted \"%s\" from ban list for \"%s\"." , c2c->mask, Channel_Name( Chan ));
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
	/* Hier wird aus einem "beliebigen" Pattern eine gueltige IRC-Mask erzeugt.
	* Diese ist aber nur bis zum naechsten Aufruf von Lists_MakeMask() gueltig,
	* da ein einziger globaler Puffer verwendet wird. ->Umkopieren!*/

	STATIC CHAR TheMask[MASK_LEN];
	CHAR *excl, *at;

	assert( Pattern );

	excl = strchr( Pattern, '!' );
	at = strchr( Pattern, '@' );

	if(( at ) && ( at < excl )) excl = NULL;

	if(( ! at ) && ( ! excl ))
	{
		/* weder ! noch @Êvorhanden: als Nick annehmen */
		strncpy( TheMask, Pattern, MASK_LEN - 5 );
		TheMask[MASK_LEN - 5] = '\0';
		strcat( TheMask, "!*@*" );
		return TheMask;
	}

	if(( ! at ) && ( excl ))
	{
		/* Domain fehlt */
		strncpy( TheMask, Pattern, MASK_LEN - 3 );
		TheMask[MASK_LEN - 3] = '\0';
		strcat( TheMask, "@*" );
		return TheMask;
	}

	if(( at ) && ( ! excl ))
	{
		/* User fehlt */
		*at = '\0'; at++;
		strncpy( TheMask, Pattern, MASK_LEN - 4 );
		TheMask[MASK_LEN - 4] = '\0';
		strcat( TheMask, "!*@" );
		strncat( TheMask, at, strlen( TheMask ) - MASK_LEN - 1 );
		TheMask[MASK_LEN - 1] = '\0';
		return TheMask;
	}

	/* alle Teile vorhanden */
	strncpy( TheMask, Pattern, MASK_LEN - 1 );
	TheMask[MASK_LEN - 1] = '\0';
	return TheMask;
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


LOCAL BOOLEAN
Check_List( C2C **Cl2Chan, CLIENT *Client, CHANNEL *Chan )
{
	C2C *c2c, *last;

	assert( Cl2Chan != NULL );
	assert( Client != NULL );
	assert( Chan != NULL );

	c2c = *Cl2Chan;
	last = NULL;

	while( c2c )
	{
		if( c2c->channel == Chan )
		{
			/* Ok, richtiger Channel. Passt die Maske? */
			if( Match( c2c->mask, Client_Mask( Client )))
			{
				/* Treffer! */
				if( c2c->onlyonce )
				{
					/* Eintrag loeschen */
					Log( LOG_DEBUG, "Deleted \"%s\" from %s list for \"%s\".", c2c->mask, *Cl2Chan == My_Invites ? "invite" : "ban", Channel_Name( Chan ));
					if( last ) last->next = c2c->next;
					else *Cl2Chan = c2c->next;
					free( c2c );
				}
				return TRUE;
			}
		}
		last = c2c;
		c2c = c2c->next;
	}

	return FALSE;
} /* Check_List */


/* -eof- */
