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
 * $Id: client.c,v 1.24 2002/01/06 15:18:14 alex Exp $
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
 * Revision 1.24  2002/01/06 15:18:14  alex
 * - Loglevel und Meldungen nochmals geaendert. Level passen nun besser.
 *
 * Revision 1.23  2002/01/05 23:26:05  alex
 * - Vorbereitungen fuer Ident-Abfragen in Client-Strukturen.
 *
 * Revision 1.22  2002/01/05 20:08:17  alex
 * - neue Funktion Client_NextHop().
 *
 * Revision 1.21  2002/01/05 19:15:03  alex
 * - Fehlerpruefung bei select() in der "Hauptschleife" korrigiert.
 *
 * Revision 1.20  2002/01/04 17:57:08  alex
 * - Client_Destroy() an Server-Links angepasst.
 *
 * Revision 1.19  2002/01/04 01:21:22  alex
 * - Client-Strukturen koennen von anderen Modulen nun nur noch ueber die
 *   enstprechenden (zum Teil neuen) Funktionen angesprochen werden.
 *
 * Revision 1.18  2002/01/03 02:28:06  alex
 * - neue Funktion Client_CheckID(), diverse Aenderungen fuer Server-Links.
 *
 * Revision 1.17  2002/01/02 02:42:58  alex
 * - Copyright-Texte aktualisiert.
 *
 * Revision 1.16  2002/01/01 18:25:44  alex
 * - #include's fuer stdlib.h ergaenzt.
 *
 * Revision 1.15  2001/12/31 15:33:13  alex
 * - neuer Befehl NAMES, kleinere Bugfixes.
 * - Bug bei PING behoben: war zu restriktiv implementiert :-)
 *
 * Revision 1.14  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.13  2001/12/30 19:26:11  alex
 * - Unterstuetzung fuer die Konfigurationsdatei eingebaut.
 *
 * Revision 1.12  2001/12/29 20:18:18  alex
 * - neue Funktion Client_SetHostname().
 *
 * Revision 1.11  2001/12/29 03:10:47  alex
 * - Client-Modes implementiert; Loglevel mal wieder angepasst.
 *
 * Revision 1.10  2001/12/27 19:13:47  alex
 * - neue Funktion Client_Search(), besseres Logging.
 *
 * Revision 1.9  2001/12/27 17:15:29  alex
 * - der eigene Hostname wird nun komplet (als FQDN) ermittelt.
 *
 * Revision 1.8  2001/12/27 16:54:51  alex
 * - neue Funktion Client_GetID(), liefert die "Client ID".
 *
 * Revision 1.7  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.6  2001/12/26 03:19:16  alex
 * - neue Funktion Client_Nick().
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


#define __client_c__


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include <exp.h>
#include "client.h"

#include <imp.h>
#include "ngircd.h"
#include "channel.h"
#include "conf.h"
#include "conn.h"
#include "irc.h"
#include "log.h"
#include "messages.h"

#include <exp.h>


LOCAL CLIENT *This_Server, *My_Clients;
LOCAL CHAR GetID_Buffer[CLIENT_ID_LEN];


LOCAL CLIENT *New_Client_Struct( VOID );


GLOBAL VOID Client_Init( VOID )
{
	struct hostent *h;
	
	This_Server = New_Client_Struct( );
	if( ! This_Server )
	{
		Log( LOG_EMERG, "Can't allocate client structure for server! Going down." );
		Log( LOG_ALERT, PACKAGE" exiting due to fatal errors!" );
		exit( 1 );
	}

	/* Client-Struktur dieses Servers */
	This_Server->next = NULL;
	This_Server->type = CLIENT_SERVER;
	This_Server->conn_id = NONE;
	This_Server->introducer = This_Server;

	gethostname( This_Server->host, CLIENT_HOST_LEN );
	h = gethostbyname( This_Server->host );
	if( h ) strcpy( This_Server->host, h->h_name );

	strcpy( This_Server->id, Conf_ServerName );
	strcpy( This_Server->info, Conf_ServerInfo );

	My_Clients = This_Server;
} /* Client_Init */


GLOBAL VOID Client_Exit( VOID )
{
	CLIENT *c, *next;
	INT cnt;

	Client_Destroy( This_Server, "Server going down.", NULL );
	
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
} /* Client_Exit */


GLOBAL CLIENT *Client_ThisServer( VOID )
{
	return This_Server;
} /* Client_ThisServer */


GLOBAL CLIENT *Client_NewLocal( CONN_ID Idx, CHAR *Hostname, INT Type, BOOLEAN Idented )
{
	/* Neuen lokalen Client erzeugen: Wrapper-Funktion fuer Client_New(). */
	return Client_New( Idx, This_Server, Type, NULL, NULL, Hostname, NULL, 0, 0, NULL, Idented );
} /* Client_NewLocal */


GLOBAL CLIENT *Client_NewRemoteServer( CLIENT *Introducer, CHAR *Hostname, INT Hops, INT Token, CHAR *Info, BOOLEAN Idented )
{
	/* Neuen Remote-Client erzeugen: Wrapper-Funktion fuer Client_New (). */
	return Client_New( NONE, Introducer, CLIENT_SERVER, Hostname, NULL, Hostname, Info, Hops, Token, NULL, Idented );
} /* Client_NewRemoteServer */


GLOBAL CLIENT *Client_NewRemoteUser( CLIENT *Introducer, CHAR *Nick, INT Hops, CHAR *User, CHAR *Hostname, INT Token, CHAR *Modes, CHAR *Info, BOOLEAN Idented )
{
	/* Neuen Remote-Client erzeugen: Wrapper-Funktion fuer Client_New (). */
	return Client_New( NONE, Introducer, CLIENT_USER, Nick, User, Hostname, Info, Hops, Token, NULL, Idented );
} /* Client_NewRemoteUser */


GLOBAL CLIENT *Client_New( CONN_ID Idx, CLIENT *Introducer, INT Type, CHAR *ID, CHAR *User, CHAR *Hostname, CHAR *Info, INT Hops, INT Token, CHAR *Modes, BOOLEAN Idented )
{
	CLIENT *client;

	assert( Idx >= NONE );
	assert( Introducer != NULL );
	assert( Hostname != NULL );

	client = New_Client_Struct( );
	if( ! client ) return NULL;

	/* Initialisieren */
	client->conn_id = Idx;
	client->introducer = Introducer;
	client->type = Type;
	if( ID ) Client_SetID( client, ID );
	if( User ) Client_SetUser( client, User, Idented );
	if( Hostname ) Client_SetHostname( client, Hostname );
	if( Info ) Client_SetInfo( client, Info );
	client->hops = Hops;
	client->token = Token;
	if( Modes ) Client_SetModes( client, Modes );

	/* Verketten */
	client->next = My_Clients;
	My_Clients = client;

	return client;
} /* Client_New */


GLOBAL VOID Client_Destroy( CLIENT *Client, CHAR *LogMsg, CHAR *FwdMsg )
{
	/* Client entfernen. */
	
	CLIENT *last, *c;
	CHAR *txt;

	assert( Client != NULL );

	if( LogMsg ) txt = LogMsg;
	else txt = FwdMsg;
	if( ! txt ) txt = "Reason unknown.";

	last = NULL;
	c = My_Clients;
	while( c )
	{
		if(( Client->type == CLIENT_SERVER ) && ( c->introducer == Client ) && ( c != Client ))
		{
			Client_Destroy( c, LogMsg, FwdMsg );
			last = NULL;
			c = My_Clients;
			continue;
		}
		if( c == Client )
		{
			if( last ) last->next = c->next;
			else My_Clients = c->next;

			if( c->type == CLIENT_USER )
			{
				if( c->conn_id != NONE )
				{
					/* Ein lokaler User. Andere Server informieren! */
					Log( LOG_NOTICE, "User \"%s\" unregistered (connection %d): %s", c->id, c->conn_id, txt );

					if( FwdMsg ) IRC_WriteStrServersPrefix( NULL, c, "QUIT :%s", FwdMsg );
					else IRC_WriteStrServersPrefix( NULL, c, "QUIT :" );
				}
				else
				{
					Log( LOG_DEBUG, "User \"%s\" unregistered: %s", c->id, txt );
				}
			}
			else if( c->type == CLIENT_SERVER )
			{
				if( c != This_Server ) Log( LOG_NOTICE, "Server \"%s\" unregistered: %s", c->id, txt );
			}
			else Log( LOG_NOTICE, "Unknown client \"%s\" unregistered: %s", c->id, txt );

			free( c );
			break;
		}
		last = c;
		c = c->next;
	}
} /* Client_Destroy */


GLOBAL VOID Client_SetHostname( CLIENT *Client, CHAR *Hostname )
{
	/* Hostname eines Clients setzen */
	
	assert( Client != NULL );
	strncpy( Client->host, Hostname, CLIENT_HOST_LEN );
	Client->host[CLIENT_HOST_LEN - 1] = '\0';
} /* Client_SetHostname */


GLOBAL VOID Client_SetID( CLIENT *Client, CHAR *ID )
{
	/* Hostname eines Clients setzen */

	assert( Client != NULL );
	strncpy( Client->id, ID, CLIENT_ID_LEN );
	Client->id[CLIENT_ID_LEN - 1] = '\0';
} /* Client_SetID */


GLOBAL VOID Client_SetUser( CLIENT *Client, CHAR *User, BOOLEAN Idented )
{
	/* Username eines Clients setzen */

	assert( Client != NULL );
	if( Idented ) strncpy( Client->user, User, CLIENT_USER_LEN );
	else
	{
		Client->user[0] = '~';
		strncpy( Client->user + 1, User, CLIENT_USER_LEN - 1 );
	}
	Client->user[CLIENT_USER_LEN - 1] = '\0';
} /* Client_SetUser */


GLOBAL VOID Client_SetInfo( CLIENT *Client, CHAR *Info )
{
	/* Hostname eines Clients setzen */

	assert( Client != NULL );
	strncpy( Client->info, Info, CLIENT_INFO_LEN );
	Client->info[CLIENT_INFO_LEN - 1] = '\0';
} /* Client_SetInfo */


GLOBAL VOID Client_SetModes( CLIENT *Client, CHAR *Modes )
{
	/* Hostname eines Clients setzen */

	assert( Client != NULL );
	strncpy( Client->modes, Modes, CLIENT_MODE_LEN );
	Client->info[CLIENT_MODE_LEN - 1] = '\0';
} /* Client_SetModes */


GLOBAL VOID Client_SetPassword( CLIENT *Client, CHAR *Pwd )
{
	/* Von einem Client geliefertes Passwort */

	assert( Client != NULL );
	strncpy( Client->pwd, Pwd, CLIENT_PASS_LEN );
	Client->pwd[CLIENT_PASS_LEN - 1] = '\0';
} /* Client_SetPassword */


GLOBAL VOID Client_SetType( CLIENT *Client, INT Type )
{
	assert( Client != NULL );
	Client->type = Type;
} /* Client_SetType */


GLOBAL VOID Client_SetHops( CLIENT *Client, INT Hops )
{
	assert( Client != NULL );
	Client->hops = Hops;
} /* Client_SetHops */


GLOBAL VOID Client_SetToken( CLIENT *Client, INT Token )
{
	assert( Client != NULL );
	Client->token = Token;
} /* Client_SetToken */


GLOBAL VOID Client_SetIntroducer( CLIENT *Client, CLIENT *Introducer )
{
	assert( Client != NULL );
	Client->introducer = Introducer;
} /* Client_SetIntroducer */


GLOBAL VOID Client_SetOperByMe( CLIENT *Client, BOOLEAN OperByMe )
{
	assert( Client != NULL );
	Client->oper_by_me = OperByMe;
} /* Client_SetOperByMe */


GLOBAL BOOLEAN Client_ModeAdd( CLIENT *Client, CHAR Mode )
{
	/* Mode soll gesetzt werden. TRUE wird geliefert, wenn der
	 * Mode neu gesetzt wurde, FALSE, wenn der Client den Mode
	 * bereits hatte. */

	CHAR x[2];
	
	assert( Client != NULL );

	x[0] = Mode; x[1] = '\0';
	if( ! strchr( Client->modes, x[0] ))
	{
		/* Client hat den Mode noch nicht -> setzen */
		strcat( Client->modes, x );
		return TRUE;
	}
	else return FALSE;
} /* Client_ModeAdd */


GLOBAL BOOLEAN Client_ModeDel( CLIENT *Client, CHAR Mode )
{
	/* Mode soll geloescht werden. TRUE wird geliefert, wenn der
	* Mode entfernt wurde, FALSE, wenn der Client den Mode
	* ueberhaupt nicht hatte. */

	CHAR x[2], *p;

	assert( Client != NULL );

	x[0] = Mode; x[1] = '\0';

	p = strchr( Client->modes, x[0] );
	if( ! p ) return FALSE;

	/* Client hat den Mode -> loeschen */
	while( *p )
	{
		*p = *(p + 1);
		p++;
	}
	return TRUE;
} /* Client_ModeDel */


GLOBAL CLIENT *Client_GetFromConn( CONN_ID Idx )
{
	/* Client-Struktur, die zur lokalen Verbindung Idx gehoert,
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


GLOBAL CLIENT *Client_GetFromID( CHAR *Nick )
{
	/* Client-Struktur, die den entsprechenden Nick hat,
	* liefern. Wird keine gefunden, so wird NULL geliefert. */

	CLIENT *c;

	assert( Nick != NULL );

	c = My_Clients;
	while( c )
	{
		if( strcasecmp( c->id, Nick ) == 0 ) return c;
		c = c->next;
	}
	return NULL;
} /* Client_GetFromID */


GLOBAL CLIENT *Client_GetFromToken( CLIENT *Client, INT Token )
{
	/* Client-Struktur, die den entsprechenden Introducer (=Client)
	 * und das gegebene Token hat, liefern. Wird keine gefunden,
	 * so wird NULL geliefert. */

	CLIENT *c;

	assert( Client != NULL );
	assert( Token > 0 );

	c = My_Clients;
	while( c )
	{
		if(( c->type == CLIENT_SERVER ) && ( c->introducer == Client ) && ( c->token == Token )) return c;
		c = c->next;
	}
	return NULL;
} /* Client_GetFromToken */


GLOBAL INT Client_Type( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->type;
} /* Client_Type */


GLOBAL CONN_ID Client_Conn( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->conn_id;
} /* Client_Conn */


GLOBAL CHAR *Client_ID( CLIENT *Client )
{
	assert( Client != NULL );

	if( Client->id[0] ) return Client->id;
	else return "*";
} /* Client_ID */


GLOBAL CHAR *Client_Info( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->info;
} /* Client_Info */


GLOBAL CHAR *Client_User( CLIENT *Client )
{
	assert( Client != NULL );
	if( Client->user ) return Client->user;
	else return "~";
} /* Client_User */


GLOBAL CHAR *Client_Hostname( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->host;
} /* Client_Hostname */


GLOBAL CHAR *Client_Password( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->pwd;
} /* Client_Password */


GLOBAL CHAR *Client_Modes( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->modes;
} /* Client_Modes */


GLOBAL BOOLEAN Client_OperByMe( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->oper_by_me;
} /* Client_OperByMe */


GLOBAL INT Client_Hops( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->hops;
} /* Client_Hops */


GLOBAL INT Client_Token( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->token;
} /* Client_Token */


GLOBAL CLIENT *Client_NextHop( CLIENT *Client )
{
	CLIENT *c;
	
	assert( Client != NULL );

	c = Client;
	while( c->introducer && ( c->introducer != c ) && ( c->introducer != This_Server )) c = c->introducer;
	return c;
} /* Client_NextHop */


GLOBAL CHAR *Client_Mask( CLIENT *Client )
{
	/* Client-"ID" liefern, wie sie z.B. fuer
	 * Prefixe benoetigt wird. */

	assert( Client != NULL );
	
	if( Client->type == CLIENT_SERVER ) return Client->id;

	sprintf( GetID_Buffer, "%s!%s@%s", Client->id, Client->user, Client->host );
	return GetID_Buffer;
} /* Client_Mask */


GLOBAL CLIENT *Client_Introducer( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->introducer;
} /* Client_Introducer */


GLOBAL BOOLEAN Client_HasMode( CLIENT *Client, CHAR Mode )
{
	assert( Client != NULL );
	return strchr( Client->modes, Mode ) != NULL;
} /* Client_HasMode */


GLOBAL BOOLEAN Client_CheckNick( CLIENT *Client, CHAR *Nick )
{
	/* Nick ueberpruefen */

	CLIENT *c;
	
	assert( Client != NULL );
	assert( Nick != NULL );
	
	/* Nick zu lang? */
	if( strlen( Nick ) > CLIENT_NICK_LEN ) return IRC_WriteStrClient( Client, ERR_ERRONEUSNICKNAME_MSG, Client_ID( Client ), Nick );

	/* Nick bereits vergeben? */
	c = My_Clients;
	while( c )
	{
		if( strcasecmp( c->id, Nick ) == 0 )
		{
			/* den Nick gibt es bereits */
			IRC_WriteStrClient( Client, ERR_NICKNAMEINUSE_MSG, Client_ID( Client ), Nick );
			return FALSE;
		}
		c = c->next;
	}

	return TRUE;
} /* Client_CheckNick */


GLOBAL BOOLEAN Client_CheckID( CLIENT *Client, CHAR *ID )
{
	/* Nick ueberpruefen */

	CHAR str[COMMAND_LEN];
	CLIENT *c;

	assert( Client != NULL );
	assert( Client->conn_id > NONE );
	assert( ID != NULL );

	/* Nick zu lang? */
	if( strlen( ID ) > CLIENT_ID_LEN ) return IRC_WriteStrClient( Client, ERR_ERRONEUSNICKNAME_MSG, Client_ID( Client ), ID );

	/* ID bereits vergeben? */
	c = My_Clients;
	while( c )
	{
		if( strcasecmp( c->id, ID ) == 0 )
		{
			/* die Server-ID gibt es bereits */
			sprintf( str, "ID \"%s\" already registered!", ID );
			Log( LOG_ERR, "%s (on connection %d)", str, Client->conn_id );
			Conn_Close( Client->conn_id, str, str, TRUE );
			return FALSE;
		}
		c = c->next;
	}

	return TRUE;
} /* Client_CheckID */


GLOBAL CLIENT *Client_Search( CHAR *ID )
{
	/* Client suchen, auf den ID passt */

	CLIENT *c;

	assert( ID != NULL );

	c = My_Clients;
	while( c )
	{
		if( strcasecmp( c->id, ID ) == 0 ) return c;
		c = c->next;
	}
	
	return NULL;
} /* Client_Search */


GLOBAL CLIENT *Client_First( VOID )
{
	/* Ersten Client liefern. */

	return My_Clients;
} /* Client_First */


GLOBAL CLIENT *Client_Next( CLIENT *c )
{
	/* Naechsten Client liefern. Existiert keiner,
	 * so wird NULL geliefert. */

	assert( c != NULL );
	return c->next;
} /* Client_Next */


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
	strcpy( c->id, "" );
	strcpy( c->pwd, "" );
	strcpy( c->host, "" );
	strcpy( c->user, "" );
	strcpy( c->info, "" );
	for( i = 0; i < MAX_CHANNELS; c->channels[i++] = NULL );
	strcpy( c->modes, "" );
	c->oper_by_me = FALSE;
	c->hops = -1;
	c->token = -1;

	return c;
} /* New_Client */


/* -eof- */
