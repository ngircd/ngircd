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
 * $Id: client.c,v 1.7 2001/12/26 14:45:37 alex Exp $
 *
 * client.c: Management aller Clients
 *
 * Der Begriff "Client" ist in diesem Fall evtl. etwas verwirrend: Clients sind
 * alle Verbindungen, die im gesamten(!) IRC-Netzwerk bekannt sind. Das sind IRC-
 * Clients (User), andere Server und IRC-Services.
 * Ueber welchen IRC-Server die Verbindung nun tatsaechlich in das Netzwerk her-
 * gestellt wurde, muss der jeweiligen Struktur entnommen werden. Ist es dieser
 * Server gewesen, so existiert eine entsprechende CONNECTION-Struktur.
 *
 * $Log: client.c,v $
 * Revision 1.7  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.6  2001/12/26 03:19:16  alex
 * - neue Funktion Client_Name().
 *
 * Revision 1.5  2001/12/25 22:04:26  alex
 * - Aenderungen an den Debug- und Logging-Funktionen.
 *
 * Revision 1.4  2001/12/25 19:21:26  alex
 * - Client-Typ ("Status") besser unterteilt, My_Clients ist zudem nun global.
 *
 * Revision 1.3  2001/12/24 01:31:14  alex
 * - einige assert()'s eingestraeut.
 *
 * Revision 1.2  2001/12/23 22:04:37  alex
 * - einige neue Funktionen,
 * - CLIENT-Struktur erweitert.
 *
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include <exp.h>
#include "client.h"

#include <imp.h>
#include "channel.h"
#include "conn.h"
#include "irc.h"
#include "log.h"
#include "messages.h"

#include <exp.h>


GLOBAL CLIENT *My_Clients;


LOCAL CLIENT *New_Client_Struct( VOID );


GLOBAL VOID Client_Init( VOID )
{
	This_Server = New_Client_Struct( );
	if( ! This_Server )
	{
		Log( LOG_EMERG, "Can't allocate client structure for server! Going down." );
		exit( 1 );
	}

	/* Client-Struktur dieses Servers */
	This_Server->next = NULL;
	This_Server->type = CLIENT_SERVER;
	This_Server->conn_id = NONE;
	This_Server->introducer = This_Server;
	gethostname( This_Server->host, CLIENT_HOST_LEN );
	strcpy( This_Server->nick, This_Server->host );

	My_Clients = This_Server;
} /* Client_Init */


GLOBAL VOID Client_Exit( VOID )
{
	CLIENT *c, *next;
	INT cnt;

	Client_Destroy( This_Server );
	
	cnt = 0;
	c = My_Clients;
	while( c )
	{
		cnt++;
		next = c->next;
		free( c );
		c = next;
	}
	if( cnt ) Log( LOG_INFO, "Freed %d client structure%s.", cnt, cnt == 1 ? "" : "s" );
} /* Client Exit */


GLOBAL CLIENT *Client_NewLocal( CONN_ID Idx, CHAR *Hostname )
{
	/* Neuen lokalen Client erzeugen. */
	
	CLIENT *client;

	assert( Idx >= 0 );
	assert( Hostname != NULL );
	
	client = New_Client_Struct( );
	if( ! client ) return NULL;

	/* Initgialisieren */
	client->conn_id = Idx;
	client->introducer = This_Server;
	strncpy( client->host, Hostname, CLIENT_HOST_LEN );
	client->host[CLIENT_HOST_LEN] = '\0';

	/* Verketten */
	client->next = My_Clients;
	My_Clients = client;
	
	return client;
} /* Client_NewLocal */


GLOBAL VOID Client_Destroy( CLIENT *Client )
{
	/* Client entfernen. */
	
	CLIENT *last, *c;

	assert( Client != NULL );
	
	last = NULL;
	c = My_Clients;
	while( c )
	{
		if( c == Client )
		{
			if( last ) last->next = c->next;
			else My_Clients = c->next;
			free( c );
			break;
		}
		last = c;
		c = c->next;
	}
} /* Client_Destroy */


GLOBAL CLIENT *Client_GetFromConn( CONN_ID Idx )
{
	/* Client-Struktur, die zur lokalen Verbindung Idx gehoert
	 * liefern. Wird keine gefunden, so wird NULL geliefert. */

	CLIENT *c;

	assert( Idx >= 0 );
	
	c = My_Clients;
	while( c )
	{
		if( c->conn_id == Idx ) return c;
		c = c->next;
	}
	return NULL;
} /* Client_GetFromConn */


GLOBAL CHAR *Client_Name( CLIENT *Client )
{
	assert( Client != NULL );

	if( Client->nick[0] ) return Client->nick;
	else return "*";
} /* Client_Name */


GLOBAL BOOLEAN Client_CheckNick( CLIENT *Client, CHAR *Nick )
{
	/* Nick ueberpruefen */

	CLIENT *c;
	
	assert( Client != NULL );
	assert( Nick != NULL );
	
	/* Nick zu lang? */
	if( strlen( Nick ) > CLIENT_NICK_LEN ) return IRC_WriteStrClient( Client, This_Server, ERR_ERRONEUSNICKNAME_MSG, Client_Name( Client ), Nick );

	/* Nick bereits vergeben? */
	c = My_Clients;
	while( c )
	{
		if( strcasecmp( c->nick, Nick ) == 0 )
		{
			/* den Nick gibt es bereits */
			IRC_WriteStrClient( Client, This_Server, ERR_NICKNAMEINUSE_MSG, Client_Name( Client ), Nick );
			return FALSE;
		}
		c = c->next;
	}

	return TRUE;
} /* Client_CheckNick */


LOCAL CLIENT *New_Client_Struct( VOID )
{
	/* Neue CLIENT-Struktur pre-initialisieren */
	
	CLIENT *c;
	INT i;
	
	c = malloc( sizeof( CLIENT ));
	if( ! c )
	{
		Log( LOG_EMERG, "Can't allocate memory!" );
		return NULL;
	}

	c->next = NULL;
	c->type = CLIENT_UNKNOWN;
	c->conn_id = NONE;
	c->introducer = NULL;
	strcpy( c->nick, "" );
	strcpy( c->pass, "" );
	strcpy( c->host, "" );
	strcpy( c->user, "" );
	strcpy( c->name, "" );
	for( i = 0; i < MAX_CHANNELS; c->channels[i++] = NULL );

	return c;
} /* New_Client */


/* -eof- */
