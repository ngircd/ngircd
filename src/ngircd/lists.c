/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Management of IRC lists: ban, invite, ...
 */


#include "portab.h"

static char UNUSED id[] = "$Id: lists.c,v 1.11 2002/12/26 16:25:43 alex Exp $";

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
LOCAL BOOLEAN Already_Registered PARAMS(( C2C *Cl2Chan, CHAR *Mask, CHANNEL *Chan ));



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
Lists_AddInvited( CLIENT *From, CHAR *Mask, CHANNEL *Chan, BOOLEAN OnlyOnce )
{
	C2C *c2c;

	assert( Mask != NULL );
	assert( Chan != NULL );

	if( Already_Registered( My_Invites, Mask, Chan ))
	{
		/* Eintrag ist bereits vorhanden */
		IRC_WriteStrClient( From, RPL_INVITELIST_MSG, Client_ID( From ), Channel_Name( Chan ), Mask );
		return FALSE;
	}
	
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
Lists_AddBanned( CLIENT *From, CHAR *Mask, CHANNEL *Chan )
{
	C2C *c2c;

	assert( Mask != NULL );
	assert( Chan != NULL );

	if( Already_Registered( My_Bans, Mask, Chan ))
	{
		/* Eintrag ist bereits vorhanden */
		IRC_WriteStrClient( From, RPL_BANLIST_MSG, Client_ID( From ), Channel_Name( Chan ), Mask );
		return FALSE;
	}

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

	assert( Pattern != NULL );

	excl = strchr( Pattern, '!' );
	at = strchr( Pattern, '@' );

	if(( at ) && ( at < excl )) excl = NULL;

	if(( ! at ) && ( ! excl ))
	{
		/* weder ! noch @ vorhanden: als Nick annehmen */
		strlcpy( TheMask, Pattern, sizeof( TheMask ) - 5 );
		strlcat( TheMask, "!*@*", sizeof( TheMask ));
		return TheMask;
	}

	if(( ! at ) && ( excl ))
	{
		/* Domain fehlt */
		strlcpy( TheMask, Pattern, sizeof( TheMask ) - 3 );
		strlcat( TheMask, "@*", sizeof( TheMask ));
		return TheMask;
	}

	if(( at ) && ( ! excl ))
	{
		/* User fehlt */
		*at = '\0'; at++;
		strlcpy( TheMask, Pattern, sizeof( TheMask ) - strlen( at ) - 4 );
		strlcat( TheMask, "!*@", sizeof( TheMask ));
		strlcat( TheMask, at, sizeof( TheMask ));
		return TheMask;
	}

	/* alle Teile vorhanden */
	strlcpy( TheMask, Pattern, sizeof( TheMask ));
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

	strlcpy( c2c->mask, Mask, sizeof( c2c->mask ));
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


LOCAL BOOLEAN
Already_Registered( C2C *List, CHAR *Mask, CHANNEL *Chan )
{
	C2C *c2c;

	c2c = List;
	while( c2c )
	{
		if(( c2c->channel == Chan ) && ( strcasecmp( c2c->mask, Mask ) == 0 )) return TRUE;
		c2c = c2c->next;
	}
	return FALSE;
} /* Already_Registered */


/* -eof- */
