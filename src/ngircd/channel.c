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
 * $Id: channel.c,v 1.9 2002/01/27 22:47:11 alex Exp $
 *
 * channel.c: Management der Channels
 *
 * $Log: channel.c,v $
 * Revision 1.9  2002/01/27 22:47:11  alex
 * - PART wird nicht mehr an den Server verschickt, von dem es empfangen wurde.
 *
 * Revision 1.8  2002/01/27 21:56:54  alex
 * - weitere Anpassungen an Chennals, v.a. ueber Server-Links.
 *
 * Revision 1.7  2002/01/27 17:14:33  alex
 * - diverse Aenderungen fuer Channels ueber mehrere Server.
 *
 * Revision 1.6  2002/01/26 18:41:55  alex
 * - CHANNEL- und CL2CHAN-Strukturen in Header verlegt,
 * - einige neue Funktionen (Channel_GetChannel(), Channel_FirstMember(), ...)
 *
 * Revision 1.5  2002/01/21 00:12:29  alex
 * - begonnen, Channels zu implementieren :-)
 *
 * Revision 1.4  2002/01/16 22:09:07  alex
 * - neue Funktion Channel_Count().
 *
 * Revision 1.3  2002/01/02 02:42:58  alex
 * - Copyright-Texte aktualisiert.
 *
 * Revision 1.2  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 */


#define __channel_c__


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "irc.h"
#include "log.h"
#include "messages.h"

#include <exp.h>
#include "channel.h"


LOCAL CHANNEL *My_Channels;
LOCAL CL2CHAN *My_Cl2Chan;


LOCAL CHANNEL *New_Chan( CHAR *Name );
LOCAL CL2CHAN *Get_Cl2Chan( CHANNEL *Chan, CLIENT *Client );
LOCAL CL2CHAN *Add_Client( CHANNEL *Chan, CLIENT *Client );
LOCAL BOOLEAN Remove_Client( CHANNEL *Chan, CLIENT *Client, CLIENT *Origin, CHAR *Reason, BOOLEAN ServerPART );
LOCAL CL2CHAN *Get_First_Cl2Chan( CLIENT *Client, CHANNEL *Chan );
LOCAL CL2CHAN *Get_Next_Cl2Chan( CL2CHAN *Start, CLIENT *Client, CHANNEL *Chan );
LOCAL BOOLEAN Delete_Channel( CHANNEL *Chan );


GLOBAL VOID Channel_Init( VOID )
{
	My_Channels = NULL;
	My_Cl2Chan = NULL;
} /* Channel_Init */


GLOBAL VOID Channel_Exit( VOID )
{
	CHANNEL *c, *c_next;
	CL2CHAN *cl2chan, *cl2chan_next;
	
	/* Channel-Strukturen freigeben */
	c = My_Channels;
	while( c )
	{
		c_next = c->next;
		free( c );
		c = c_next;
	}

	/* Channel-Zuordnungstabelle freigeben */
	cl2chan = My_Cl2Chan;
	while( c )
	{
		cl2chan_next = cl2chan->next;
		free( cl2chan );
		cl2chan = cl2chan_next;
	}
} /* Channel_Exit */


GLOBAL BOOLEAN Channel_Join( CLIENT *Client, CHAR *Name )
{
	CHANNEL *chan;
	
	assert( Client != NULL );
	assert( Name != NULL );

	/* Valider Channel-Name? */
	if(( Name[0] != '#' ) || ( strlen( Name ) >= CHANNEL_NAME_LEN ))
	{
		IRC_WriteStrClient( Client, ERR_NOSUCHCHANNEL_MSG, Client_ID( Client ), Name );
		return FALSE;
	}

	/* Channel suchen */
	chan = Channel_Search( Name );
	if( chan )
	{
		/* Ist der Client bereits Mitglied? */
		if( Get_Cl2Chan( chan, Client )) return FALSE;
	}
	else
	{
		/* Gibt es noch nicht? Dann neu anlegen: */
		chan = New_Chan( Name );
		if( ! chan ) return FALSE;

		/* Verketten */
		chan->next = My_Channels;
		My_Channels = chan;
	}

	/* User dem Channel hinzufuegen */
	if( ! Add_Client( chan, Client )) return FALSE;
	else return TRUE;
} /* Channel_Join */


GLOBAL BOOLEAN Channel_Part( CLIENT *Client, CLIENT *Origin, CHAR *Name, CHAR *Reason )
{
	CHANNEL *chan;

	assert( Client != NULL );
	assert( Name != NULL );

	/* Channel suchen */
	chan = Channel_Search( Name );
	if(( ! chan ) || ( ! Get_Cl2Chan( chan, Client )))
	{
		IRC_WriteStrClient( Client, ERR_NOSUCHCHANNEL_MSG, Client_ID( Client ), Name );
		return FALSE;
	}

	/* User aus Channel entfernen */
	if( ! Remove_Client( chan, Client, Origin, Reason, TRUE )) return FALSE;
	else return TRUE;
} /* Channel_Part */


GLOBAL VOID Channel_RemoveClient( CLIENT *Client, CHAR *Reason )
{
	CHANNEL *c, *next_c;

	assert( Client != NULL );

	c = My_Channels;
	while( c )
	{
		next_c = c->next;
		Remove_Client( c, Client, Client_ThisServer( ), Reason, FALSE );
		c = next_c;
	}
} /* Channel_RemoveClient */


GLOBAL INT Channel_Count( VOID )
{
	CHANNEL *c;
	INT count;
	
	count = 0;
	c = My_Channels;
	while( c )
	{
		count++;
		c = c->next;
	}
	return count;
} /* Channel_Count */


GLOBAL CHANNEL *Channel_Search( CHAR *Name )
{
	/* Channel-Struktur suchen */
	
	CHANNEL *c;

	assert( Name != NULL );
	c = My_Channels;
	while( c )
	{
		if( strcasecmp( Name, c->name ) == 0 ) return c;
		c = c->next;
	}
	return NULL;
} /* Channel_Search */


GLOBAL CL2CHAN *Channel_FirstMember( CHANNEL *Chan )
{
	assert( Chan != NULL );
	return Get_First_Cl2Chan( NULL, Chan );
} /* Channel_IsMember */


GLOBAL CL2CHAN *Channel_NextMember( CHANNEL *Chan, CL2CHAN *Cl2Chan )
{
	assert( Chan != NULL );
	assert( Cl2Chan != NULL );
	return Get_Next_Cl2Chan( Cl2Chan->next, NULL, Chan );
} /* Channel_NextMember */


GLOBAL CLIENT *Channel_GetClient( CL2CHAN *Cl2Chan )
{
	assert( Cl2Chan != NULL );
	return Cl2Chan->client;
} /* Channel_GetClient */


GLOBAL CHANNEL *Channel_GetChannel( CL2CHAN *Cl2Chan )
{
	assert( Cl2Chan != NULL );
	return Cl2Chan->channel;
} /* Channel_GetChannel */


LOCAL CHANNEL *New_Chan( CHAR *Name )
{
	/* Neue Channel-Struktur anlegen */
	
	CHANNEL *c;

	assert( Name != NULL );
	
	c = malloc( sizeof( CHANNEL ));
	if( ! c )
	{
		Log( LOG_EMERG, "Can't allocate memory!" );
		return NULL;
	}
	c->next = NULL;
	strncpy( c->name, Name, CHANNEL_NAME_LEN );
	c->name[CHANNEL_NAME_LEN - 1] = '\0';
	strcpy( c->modes, "" );

	Log( LOG_DEBUG, "Created new channel structure for \"%s\".", Name );
	
	return c;
} /* New_Chan */


LOCAL CL2CHAN *Get_Cl2Chan( CHANNEL *Chan, CLIENT *Client )
{
	CL2CHAN *cl2chan;

	assert( Chan != NULL );
	assert( Client != NULL );

	cl2chan = My_Cl2Chan;
	while( cl2chan )
	{
		if(( cl2chan->channel == Chan ) && ( cl2chan->client == Client )) return cl2chan;
		cl2chan = cl2chan->next;
	}
	return NULL;
} /* Get_Cl2Chan */


LOCAL CL2CHAN *Add_Client( CHANNEL *Chan, CLIENT *Client )
{
	CL2CHAN *cl2chan;

	assert( Chan != NULL );
	assert( Client != NULL );

	cl2chan = malloc( sizeof( CL2CHAN ));
	if( ! cl2chan )
	{
		Log( LOG_EMERG, "Can't allocate memory!" );
		return NULL;
	}
	cl2chan->channel = Chan;
	cl2chan->client = Client;

	/* Verketten */
	cl2chan->next = My_Cl2Chan;
	My_Cl2Chan = cl2chan;

	Log( LOG_DEBUG, "User \"%s\" joined channel \"%s\".", Client_Mask( Client ), Chan->name );

	return cl2chan;
} /* Add_Client */


LOCAL BOOLEAN Remove_Client( CHANNEL *Chan, CLIENT *Client, CLIENT *Origin, CHAR *Reason, BOOLEAN ServerPART )
{
	CL2CHAN *cl2chan, *last_cl2chan;
	CHANNEL *c;
	
	assert( Chan != NULL );
	assert( Client != NULL );
	assert( Origin != NULL );
	assert( Reason != NULL );

	last_cl2chan = NULL;
	cl2chan = My_Cl2Chan;
	while( cl2chan )
	{
		if(( cl2chan->channel == Chan ) && ( cl2chan->client == Client )) break;
		last_cl2chan = cl2chan;
		cl2chan = cl2chan->next;
	}
	if( ! cl2chan ) return FALSE;

	c = cl2chan->channel;
	assert( c != NULL );

	/* Aus Verkettung loesen und freigeben */
	if( last_cl2chan ) last_cl2chan->next = cl2chan->next;
	else My_Cl2Chan = cl2chan->next;
	free( cl2chan );

	if( ServerPART ) IRC_WriteStrServersPrefix( Origin, Client, "PART %s :%s", c->name, Reason );
	IRC_WriteStrChannelPrefix( Origin, c, Client, FALSE, "PART %s :%s", c->name, Reason );
	if(( Client_Conn( Origin ) > NONE ) && ( Client_Type( Origin ) == CLIENT_USER )) IRC_WriteStrClientPrefix( Origin, Client, "PART %s :%s", c->name, Reason );

	Log( LOG_DEBUG, "User \"%s\" left channel \"%s\" (%s).", Client_Mask( Client ), c->name, Reason );

	/* Wenn Channel nun leer: loeschen */
	if( ! Get_First_Cl2Chan( NULL, Chan )) Delete_Channel( Chan );
		
	return TRUE;
} /* Remove_Client */


LOCAL CL2CHAN *Get_First_Cl2Chan( CLIENT *Client, CHANNEL *Chan )
{
	return Get_Next_Cl2Chan( My_Cl2Chan, Client, Chan );
} /* Get_First_Cl2Chan */


LOCAL CL2CHAN *Get_Next_Cl2Chan( CL2CHAN *Start, CLIENT *Client, CHANNEL *Channel )
{
	CL2CHAN *cl2chan;

	assert( Client != NULL || Channel != NULL );
	
	cl2chan = Start;
	while( cl2chan )
	{
		if(( Client ) && ( cl2chan->client == Client )) return cl2chan;
		if(( Channel ) && ( cl2chan->channel == Channel )) return cl2chan;
		cl2chan = cl2chan->next;
	}
	return NULL;
} /* Get_Next_Cl2Chan */


LOCAL BOOLEAN Delete_Channel( CHANNEL *Chan )
{
	/* Channel-Struktur loeschen */
	
	CHANNEL *chan, *last_chan;

	last_chan = NULL;
	chan = My_Channels;
	while( chan )
	{
		if( chan == Chan ) break;
		last_chan = chan;
		chan = chan->next;
	}
	if( ! chan ) return FALSE;

	Log( LOG_DEBUG, "Freed channel structure for \"%s\".", Chan->name );

	/* Neu verketten und freigeben */
	if( last_chan ) last_chan->next = chan->next;
	else My_Channels = chan->next;
	free( chan );
		
	return TRUE;
} /* Delete_Channel */


/* -eof- */
