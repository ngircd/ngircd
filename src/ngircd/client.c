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
 * $Id: client.c,v 1.1 2001/12/14 08:13:43 alex Exp $
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
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 *
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>

#include "channel.h"
#include "conn.h"
#include "log.h"

#include <exp.h>
#include "client.h"


LOCAL CLIENT *This_Server, *My_Clients;


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
	strcpy( This_Server->nick, "ngircd" );	/* FIXME! */
	
	My_Clients = This_Server;
} /* Client_Init */


GLOBAL VOID Client_Exit( VOID )
{
} /* Client Exit */


LOCAL CLIENT *New_Client_Struct( VOID )
{
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
	for( i = 0; i < MAX_CHANNELS; c->channels[i++] = NULL );
	
	return c;
} /* New_Client */


/* -eof- */
