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
 * $Id: client.c,v 1.2 2001/12/23 22:04:37 alex Exp $
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
 * Revision 1.2  2001/12/23 22:04:37  alex
 * - einige neue Funktionen,
 * - CLIENT-Struktur erweitert.
 *
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 *
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "channel.h"
#include "conn.h"
#include "log.h"

#include <exp.h>
#include "client.h"


LOCAL CLIENT *My_Clients;


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
	if( cnt ) Log( LOG_DEBUG, "Freed %d client structure%s.", cnt, cnt == 1 ? "" : "s" );
} /* Client Exit */


GLOBAL CLIENT *Client_New_Local( CONN_ID Idx, CHAR *Hostname )
{
	/* Neuen lokalen Client erzeugen. */
	
	CLIENT *client;

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
} /* Client_New_Local */


GLOBAL VOID Client_Destroy( CLIENT *Client )
{
	/* Client entfernen. */
	
	CLIENT *last, *c;

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

	c = My_Clients;
	while( c )
	{
		if( c->conn_id == Idx ) return c;
		c = c->next;
	}
	return NULL;
} /* Client_GetFromConn */


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
