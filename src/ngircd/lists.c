/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2005 Alexander Barton (alex@barton.de)
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

static char UNUSED id[] = "$Id: lists.c,v 1.19 2006/08/12 11:56:24 fw Exp $";

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
#include <strings.h>

#include "exp.h"
#include "lists.h"


#define MASK_LEN	(2*CLIENT_HOST_LEN)


typedef struct _C2C
{
	struct _C2C *next;
	char mask[MASK_LEN];
	CHANNEL *channel;
	bool onlyonce;
} C2C;


static C2C *My_Invites, *My_Bans;


static C2C *New_C2C PARAMS(( char *Mask, CHANNEL *Chan, bool OnlyOnce ));

static bool Check_List PARAMS(( C2C **Cl2Chan, CLIENT *Client, CHANNEL *Chan ));
static bool Already_Registered PARAMS(( C2C *Cl2Chan, char *Mask, CHANNEL *Chan ));



GLOBAL void
Lists_Init( void )
{
	/* Modul initialisieren */

	My_Invites = My_Bans = NULL;
} /* Lists_Init */


GLOBAL void
Lists_Exit( void )
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


GLOBAL bool
Lists_CheckInvited( CLIENT *Client, CHANNEL *Chan )
{
	return Check_List( &My_Invites, Client, Chan );
} /* Lists_CheckInvited */


GLOBAL bool
Lists_IsInviteEntry( char *Mask, CHANNEL *Chan )
{
	assert( Mask != NULL );
	assert( Chan != NULL );

	return Already_Registered( My_Invites, Mask, Chan );
} /* Lists_IsInviteEntry */


GLOBAL bool
Lists_AddInvited( char *Mask, CHANNEL *Chan, bool OnlyOnce )
{
	C2C *c2c;

	assert( Mask != NULL );
	assert( Chan != NULL );

	if( Already_Registered( My_Invites, Mask, Chan )) return true;
	
	c2c = New_C2C( Mask, Chan, OnlyOnce );
	if( ! c2c )
	{
		Log( LOG_ERR, "Can't add new invite list entry!" );
		return false;
	}

	/* verketten */
	c2c->next = My_Invites;
	My_Invites = c2c;

	Log( LOG_DEBUG, "Added \"%s\" to invite list for \"%s\".", Mask, Channel_Name( Chan ));
	return true;
} /* Lists_AddInvited */


GLOBAL void
Lists_DelInvited( char *Mask, CHANNEL *Chan )
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


GLOBAL bool
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


GLOBAL bool
Lists_SendInvites( CLIENT *Client )
{
	C2C *c2c;
	
	assert( Client != NULL );
	
	c2c = My_Invites;
	while( c2c )
	{
		if( ! IRC_WriteStrClient( Client, "MODE %s +I %s", Channel_Name( c2c->channel ), c2c->mask )) return DISCONNECTED;
		c2c = c2c->next;
	}
	return CONNECTED;
} /* Lists_SendInvites */


GLOBAL bool
Lists_SendBans( CLIENT *Client )
{
	C2C *c2c;
	
	assert( Client != NULL );
	
	c2c = My_Bans;
	while( c2c )
	{
		if( ! IRC_WriteStrClient( Client, "MODE %s +b %s", Channel_Name( c2c->channel ), c2c->mask )) return DISCONNECTED;
		c2c = c2c->next;
	}
	return CONNECTED;
} /* Lists_SendBans */


GLOBAL bool
Lists_CheckBanned( CLIENT *Client, CHANNEL *Chan )
{
	return Check_List( &My_Bans, Client, Chan );
} /* Lists_CheckBanned */


GLOBAL bool
Lists_IsBanEntry( char *Mask, CHANNEL *Chan )
{
	assert( Mask != NULL );
	assert( Chan != NULL );
	
	return Already_Registered( My_Bans, Mask, Chan );
} /* Lists_IsBanEntry */


GLOBAL bool
Lists_AddBanned( char *Mask, CHANNEL *Chan )
{
	C2C *c2c;

	assert( Mask != NULL );
	assert( Chan != NULL );

	if( Already_Registered( My_Bans, Mask, Chan )) return true;

	c2c = New_C2C( Mask, Chan, false );
	if( ! c2c )
	{
		Log( LOG_ERR, "Can't add new ban list entry!" );
		return false;
	}

	/* verketten */
	c2c->next = My_Bans;
	My_Bans = c2c;

	Log( LOG_DEBUG, "Added \"%s\" to ban list for \"%s\".", Mask, Channel_Name( Chan ));
	return true;
} /* Lists_AddBanned */


GLOBAL void
Lists_DelBanned( char *Mask, CHANNEL *Chan )
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


GLOBAL bool
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


GLOBAL void
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


GLOBAL char *
Lists_MakeMask( char *Pattern )
{
	/* This function generats a valid IRC mask of "any" string. This
	 * mask is only valid until the next call to Lists_MakeMask(),
	 * because a single global buffer is used. You have to copy the
	 * generated mask to some sane location yourself! */

	static char TheMask[MASK_LEN];
	char *excl, *at;

	assert( Pattern != NULL );

	excl = strchr( Pattern, '!' );
	at = strchr( Pattern, '@' );

	if(( at ) && ( at < excl )) excl = NULL;

	if(( ! at ) && ( ! excl ))
	{
		/* Neither "!" nor "@" found: use string as nick name */
		strlcpy( TheMask, Pattern, sizeof( TheMask ) - 5 );
		strlcat( TheMask, "!*@*", sizeof( TheMask ));
		return TheMask;
	}

	if(( ! at ) && ( excl ))
	{
		/* Domain part is missing */
		strlcpy( TheMask, Pattern, sizeof( TheMask ) - 3 );
		strlcat( TheMask, "@*", sizeof( TheMask ));
		return TheMask;
	}

	if(( at ) && ( ! excl ))
	{
		/* User name is missing */
		*at = '\0'; at++;
		strlcpy( TheMask, Pattern, sizeof( TheMask ) - 5 );
		strlcat( TheMask, "!*@", sizeof( TheMask ));
		strlcat( TheMask, at, sizeof( TheMask ));
		return TheMask;
	}

	/* All parts (nick, user and domain name) are given */
	strlcpy( TheMask, Pattern, sizeof( TheMask ));
	return TheMask;
} /* Lists_MakeMask */


static C2C *
New_C2C( char *Mask, CHANNEL *Chan, bool OnlyOnce )
{
	C2C *c2c;
	
	assert( Mask != NULL );
	assert( Chan != NULL );

	/* Speicher fuer Eintrag anfordern */
	c2c = (C2C *)malloc( sizeof( C2C ));
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


static bool
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
				return true;
			}
		}
		last = c2c;
		c2c = c2c->next;
	}

	return false;
} /* Check_List */


static bool
Already_Registered( C2C *List, char *Mask, CHANNEL *Chan )
{
	C2C *c2c;

	c2c = List;
	while( c2c )
	{
		if(( c2c->channel == Chan ) && ( strcasecmp( c2c->mask, Mask ) == 0 )) return true;
		c2c = c2c->next;
	}
	return false;
} /* Already_Registered */


/* -eof- */
