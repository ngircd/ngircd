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
 * $Id: client.c,v 1.56 2002/05/30 16:52:21 alex Exp $
 *
 * client.c: Management aller Clients
 *
 * Der Begriff "Client" ist in diesem Fall evtl. etwas verwirrend: Clients sind
 * alle Verbindungen, die im gesamten(!) IRC-Netzwerk bekannt sind. Das sind IRC-
 * Clients (User), andere Server und IRC-Services.
 * Ueber welchen IRC-Server die Verbindung nun tatsaechlich in das Netzwerk her-
 * gestellt wurde, muss der jeweiligen Struktur entnommen werden. Ist es dieser
 * Server gewesen, so existiert eine entsprechende CONNECTION-Struktur.
 */


#define __client_c__


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include "conn.h"

#include "exp.h"
#include "client.h"

#include <imp.h>
#include "ngircd.h"
#include "channel.h"
#include "resolve.h"
#include "conf.h"
#include "hash.h"
#include "irc-write.h"
#include "log.h"
#include "messages.h"

#include <exp.h>


LOCAL CLIENT *This_Server, *My_Clients;
LOCAL CHAR GetID_Buffer[CLIENT_ID_LEN];


LOCAL INT Count PARAMS(( CLIENT_TYPE Type ));
LOCAL INT MyCount PARAMS(( CLIENT_TYPE Type ));

LOCAL CLIENT *New_Client_Struct PARAMS(( VOID ));
LOCAL VOID Generate_MyToken PARAMS(( CLIENT *Client ));


GLOBAL VOID
Client_Init( VOID )
{
	struct hostent *h;
	
	This_Server = New_Client_Struct( );
	if( ! This_Server )
	{
		Log( LOG_EMERG, "Can't allocate client structure for server! Going down." );
		Log( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE );
		exit( 1 );
	}

	/* Client-Struktur dieses Servers */
	This_Server->next = NULL;
	This_Server->type = CLIENT_SERVER;
	This_Server->conn_id = NONE;
	This_Server->introducer = This_Server;
	This_Server->mytoken = 1;
	This_Server->hops = 0;

	gethostname( This_Server->host, CLIENT_HOST_LEN );
	h = gethostbyname( This_Server->host );
	if( h ) strcpy( This_Server->host, h->h_name );

	Client_SetID( This_Server, Conf_ServerName );
	Client_SetInfo( This_Server, Conf_ServerInfo );

	My_Clients = This_Server;
} /* Client_Init */


GLOBAL VOID
Client_Exit( VOID )
{
	CLIENT *c, *next;
	INT cnt;

	Client_Destroy( This_Server, "Server going down.", NULL, FALSE );
	
	cnt = 0;
	c = My_Clients;
	while( c )
	{
		cnt++;
		next = (CLIENT *)c->next;
		free( c );
		c = next;
	}
	if( cnt ) Log( LOG_INFO, "Freed %d client structure%s.", cnt, cnt == 1 ? "" : "s" );
} /* Client_Exit */


GLOBAL CLIENT *
Client_ThisServer( VOID )
{
	return This_Server;
} /* Client_ThisServer */


GLOBAL CLIENT *
Client_NewLocal( CONN_ID Idx, CHAR *Hostname, INT Type, BOOLEAN Idented )
{
	/* Neuen lokalen Client erzeugen: Wrapper-Funktion fuer Client_New(). */
	return Client_New( Idx, This_Server, NULL, Type, NULL, NULL, Hostname, NULL, 0, 0, NULL, Idented );
} /* Client_NewLocal */


GLOBAL CLIENT *
Client_NewRemoteServer( CLIENT *Introducer, CHAR *Hostname, CLIENT *TopServer, INT Hops, INT Token, CHAR *Info, BOOLEAN Idented )
{
	/* Neuen Remote-Client erzeugen: Wrapper-Funktion fuer Client_New (). */
	return Client_New( NONE, Introducer, TopServer, CLIENT_SERVER, Hostname, NULL, Hostname, Info, Hops, Token, NULL, Idented );
} /* Client_NewRemoteServer */


GLOBAL CLIENT *
Client_NewRemoteUser( CLIENT *Introducer, CHAR *Nick, INT Hops, CHAR *User, CHAR *Hostname, INT Token, CHAR *Modes, CHAR *Info, BOOLEAN Idented )
{
	/* Neuen Remote-Client erzeugen: Wrapper-Funktion fuer Client_New (). */
	return Client_New( NONE, Introducer, NULL, CLIENT_USER, Nick, User, Hostname, Info, Hops, Token, Modes, Idented );
} /* Client_NewRemoteUser */


GLOBAL CLIENT *
Client_New( CONN_ID Idx, CLIENT *Introducer, CLIENT *TopServer, INT Type, CHAR *ID, CHAR *User, CHAR *Hostname, CHAR *Info, INT Hops, INT Token, CHAR *Modes, BOOLEAN Idented )
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
	client->topserver = TopServer;
	client->type = Type;
	if( ID ) Client_SetID( client, ID );
	if( User ) Client_SetUser( client, User, Idented );
	if( Hostname ) Client_SetHostname( client, Hostname );
	if( Info ) Client_SetInfo( client, Info );
	client->hops = Hops;
	client->token = Token;
	if( Modes ) Client_SetModes( client, Modes );
	if( Type == CLIENT_SERVER ) Generate_MyToken( client );

	/* ist der User away? */
	if( strchr( client->modes, 'a' )) strcpy( client->away, DEFAULT_AWAY_MSG );

	/* Verketten */
	client->next = (POINTER *)My_Clients;
	My_Clients = client;

	return client;
} /* Client_New */


GLOBAL VOID
Client_Destroy( CLIENT *Client, CHAR *LogMsg, CHAR *FwdMsg, BOOLEAN SendQuit )
{
	/* Client entfernen. */
	
	CLIENT *last, *c;
	CHAR msg[LINE_LEN], *txt;

	assert( Client != NULL );

	if( LogMsg ) txt = LogMsg;
	else txt = FwdMsg;
	if( ! txt ) txt = "Reason unknown.";

	/* Netz-Split-Nachricht vorbereiten (noch nicht optimal) */
	if( Client->type == CLIENT_SERVER ) sprintf( msg, "%s: lost server %s", This_Server->id, Client->id );

	last = NULL;
	c = My_Clients;
	while( c )
	{
		if(( Client->type == CLIENT_SERVER ) && ( c->introducer == Client ) && ( c != Client ))
		{
			/* der Client, der geloescht wird ist ein Server. Der Client, den wir gerade
			 * pruefen, ist ein Child von diesem und muss daher auch entfernt werden */
			Client_Destroy( c, NULL, msg, FALSE );
			last = NULL;
			c = My_Clients;
			continue;
		}
		if( c == Client )
		{
			/* Wir haben den Client gefunden: entfernen */
			if( last ) last->next = c->next;
			else My_Clients = (CLIENT *)c->next;

			if( c->type == CLIENT_USER )
			{
				if( c->conn_id != NONE )
				{
					/* Ein lokaler User */
					Log( LOG_NOTICE, "User \"%s\" unregistered (connection %d): %s", Client_Mask( c ), c->conn_id, txt );

					if( SendQuit )
					{
						/* Alle andere Server informieren! */
						if( FwdMsg ) IRC_WriteStrServersPrefix( NULL, c, "QUIT :%s", FwdMsg );
						else IRC_WriteStrServersPrefix( NULL, c, "QUIT :" );
					}
				}
				else
				{
					/* Remote User */
					Log( LOG_DEBUG, "User \"%s\" unregistered: %s", Client_Mask( c ), txt );

					if( SendQuit )
					{
						/* Andere Server informieren, ausser denen, die "in
						 * Richtung dem liegen", auf dem der User registriert
						 * ist. Von denen haben wir das QUIT ja wohl bekommen. */
						if( FwdMsg ) IRC_WriteStrServersPrefix( Client_NextHop( c ), c, "QUIT :%s", FwdMsg );
						else IRC_WriteStrServersPrefix( Client_NextHop( c ), c, "QUIT :" );
					}
				}
				Channel_RemoveClient( c, FwdMsg ? FwdMsg : c->id );
			}
			else if( c->type == CLIENT_SERVER )
			{
				if( c != This_Server )
				{
					if( c->conn_id != NONE ) Log( LOG_NOTICE|LOG_snotice, "Server \"%s\" unregistered (connection %d): %s", c->id, c->conn_id, txt );
					else Log( LOG_NOTICE|LOG_snotice, "Server \"%s\" unregistered: %s", c->id, txt );
				}

				/* andere Server informieren */
				if( ! NGIRCd_Quit )
				{
					if( FwdMsg ) IRC_WriteStrServersPrefix( Client_NextHop( c ), c, "SQUIT %s :%s", c->id, FwdMsg );
					else IRC_WriteStrServersPrefix( Client_NextHop( c ), c, "SQUIT %s :", c->id );
				}
			}
			else
			{
				if( c->conn_id != NONE )
				{
					if( c->id[0] ) Log( LOG_NOTICE, "Client \"%s\" unregistered (connection %d): %s", c->id, c->conn_id, txt );
					else Log( LOG_NOTICE, "Client unregistered (connection %d): %s", c->conn_id, txt );
				}
				else
				{
					if( c->id[0] ) Log( LOG_WARNING, "Unregistered unknown client \"%s\": %s", c->id, txt );
					else Log( LOG_WARNING, "Unregistered unknown client: %s", c->id, txt );
				}
			}

			free( c );
			break;
		}
		last = c;
		c = (CLIENT *)c->next;
	}
} /* Client_Destroy */


GLOBAL VOID
Client_SetHostname( CLIENT *Client, CHAR *Hostname )
{
	/* Hostname eines Clients setzen */
	
	assert( Client != NULL );
	assert( Hostname != NULL );
	
	strncpy( Client->host, Hostname, CLIENT_HOST_LEN - 1 );
	Client->host[CLIENT_HOST_LEN - 1] = '\0';
} /* Client_SetHostname */


GLOBAL VOID
Client_SetID( CLIENT *Client, CHAR *ID )
{
	/* Hostname eines Clients setzen, Hash-Wert berechnen */

	assert( Client != NULL );
	assert( ID != NULL );
	
	strncpy( Client->id, ID, CLIENT_ID_LEN - 1 );
	Client->id[CLIENT_ID_LEN - 1] = '\0';

	/* Hash */
	Client->hash = Hash( Client->id );
} /* Client_SetID */


GLOBAL VOID
Client_SetUser( CLIENT *Client, CHAR *User, BOOLEAN Idented )
{
	/* Username eines Clients setzen */

	assert( Client != NULL );
	assert( User != NULL );
	
	if( Idented ) strncpy( Client->user, User, CLIENT_USER_LEN - 1 );
	else
	{
		Client->user[0] = '~';
		strncpy( Client->user + 1, User, CLIENT_USER_LEN - 2 );
	}
	Client->user[CLIENT_USER_LEN - 1] = '\0';
} /* Client_SetUser */


GLOBAL VOID
Client_SetInfo( CLIENT *Client, CHAR *Info )
{
	/* Hostname eines Clients setzen */

	assert( Client != NULL );
	assert( Info != NULL );
	
	strncpy( Client->info, Info, CLIENT_INFO_LEN - 1 );
	Client->info[CLIENT_INFO_LEN - 1] = '\0';
} /* Client_SetInfo */


GLOBAL VOID
Client_SetModes( CLIENT *Client, CHAR *Modes )
{
	/* Hostname eines Clients setzen */

	assert( Client != NULL );
	assert( Modes != NULL );

	strncpy( Client->modes, Modes, CLIENT_MODE_LEN - 1 );
	Client->modes[CLIENT_MODE_LEN - 1] = '\0';
} /* Client_SetModes */


GLOBAL VOID
Client_SetPassword( CLIENT *Client, CHAR *Pwd )
{
	/* Von einem Client geliefertes Passwort */

	assert( Client != NULL );
	assert( Pwd != NULL );
	
	strncpy( Client->pwd, Pwd, CLIENT_PASS_LEN - 1 );
	Client->pwd[CLIENT_PASS_LEN - 1] = '\0';
} /* Client_SetPassword */


GLOBAL VOID
Client_SetAway( CLIENT *Client, CHAR *Txt )
{
	/* Von einem Client gelieferte AWAY-Nachricht */

	assert( Client != NULL );

	if( Txt )
	{
		/* Client AWAY setzen */
		strncpy( Client->away, Txt, CLIENT_AWAY_LEN - 1 );
		Client->away[CLIENT_AWAY_LEN - 1] = '\0';
		Client_ModeAdd( Client, 'a' );
		Log( LOG_DEBUG, "User \"%s\" is away: %s", Client_Mask( Client ), Txt );
	}
	else
	{
		/* AWAY loeschen */
		Client_ModeDel( Client, 'a' );
		Log( LOG_DEBUG, "User \"%s\" is no longer away.", Client_Mask( Client ));
	}
} /* Client_SetAway */


GLOBAL VOID
Client_SetType( CLIENT *Client, INT Type )
{
	assert( Client != NULL );
	Client->type = Type;
	if( Type == CLIENT_SERVER ) Generate_MyToken( Client );
} /* Client_SetType */


GLOBAL VOID
Client_SetHops( CLIENT *Client, INT Hops )
{
	assert( Client != NULL );
	Client->hops = Hops;
} /* Client_SetHops */


GLOBAL VOID
Client_SetToken( CLIENT *Client, INT Token )
{
	assert( Client != NULL );
	Client->token = Token;
} /* Client_SetToken */


GLOBAL VOID
Client_SetIntroducer( CLIENT *Client, CLIENT *Introducer )
{
	assert( Client != NULL );
	assert( Introducer != NULL );
	Client->introducer = Introducer;
} /* Client_SetIntroducer */


GLOBAL VOID
Client_SetOperByMe( CLIENT *Client, BOOLEAN OperByMe )
{
	assert( Client != NULL );
	Client->oper_by_me = OperByMe;
} /* Client_SetOperByMe */


GLOBAL BOOLEAN
Client_ModeAdd( CLIENT *Client, CHAR Mode )
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


GLOBAL BOOLEAN
Client_ModeDel( CLIENT *Client, CHAR Mode )
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


GLOBAL CLIENT *
Client_GetFromConn( CONN_ID Idx )
{
	/* Client-Struktur, die zur lokalen Verbindung Idx gehoert,
	 * liefern. Wird keine gefunden, so wird NULL geliefert. */

	CLIENT *c;

	assert( Idx >= 0 );
	
	c = My_Clients;
	while( c )
	{
		if( c->conn_id == Idx ) return c;
		c = (CLIENT *)c->next;
	}
	return NULL;
} /* Client_GetFromConn */


GLOBAL CLIENT *
Client_Search( CHAR *Nick )
{
	/* Client-Struktur, die den entsprechenden Nick hat, liefern.
	 * Wird keine gefunden, so wird NULL geliefert. */

	CHAR search_id[CLIENT_ID_LEN], *ptr;
	CLIENT *c = NULL;
	UINT32 search_hash;

	assert( Nick != NULL );

	/* Nick kopieren und ggf. Host-Mask abschneiden */
	strncpy( search_id, Nick, CLIENT_ID_LEN - 1 );
	search_id[CLIENT_ID_LEN - 1] = '\0';
	ptr = strchr( search_id, '!' );
	if( ptr ) *ptr = '\0';

	search_hash = Hash( search_id );

	c = My_Clients;
	while( c )
	{
		if( c->hash == search_hash )
		{
			/* lt. Hash-Wert: Treffer! */
			if( strcasecmp( c->id, search_id ) == 0 ) return c;
		}
		c = (CLIENT *)c->next;
	}
	return NULL;
} /* Client_Search */


GLOBAL CLIENT *
Client_GetFromToken( CLIENT *Client, INT Token )
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
		c = (CLIENT *)c->next;
	}
	return NULL;
} /* Client_GetFromToken */


GLOBAL INT
Client_Type( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->type;
} /* Client_Type */


GLOBAL CONN_ID
Client_Conn( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->conn_id;
} /* Client_Conn */


GLOBAL CHAR *
Client_ID( CLIENT *Client )
{
	assert( Client != NULL );

#ifdef DEBUG
	if( Client->type == CLIENT_USER ) assert( strlen( Client->id ) < CLIENT_NICK_LEN );
#endif
						   
	if( Client->id[0] ) return Client->id;
	else return "*";
} /* Client_ID */


GLOBAL CHAR *
Client_Info( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->info;
} /* Client_Info */


GLOBAL CHAR *
Client_User( CLIENT *Client )
{
	assert( Client != NULL );
	if( Client->user[0] ) return Client->user;
	else return "~";
} /* Client_User */


GLOBAL CHAR *
Client_Hostname( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->host;
} /* Client_Hostname */


GLOBAL CHAR *
Client_Password( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->pwd;
} /* Client_Password */


GLOBAL CHAR *
Client_Modes( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->modes;
} /* Client_Modes */


GLOBAL BOOLEAN
Client_OperByMe( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->oper_by_me;
} /* Client_OperByMe */


GLOBAL INT
Client_Hops( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->hops;
} /* Client_Hops */


GLOBAL INT
Client_Token( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->token;
} /* Client_Token */


GLOBAL INT
Client_MyToken( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->mytoken;
} /* Client_MyToken */


GLOBAL CLIENT *
Client_NextHop( CLIENT *Client )
{
	CLIENT *c;
	
	assert( Client != NULL );

	c = Client;
	while( c->introducer && ( c->introducer != c ) && ( c->introducer != This_Server )) c = c->introducer;
	return c;
} /* Client_NextHop */


GLOBAL CHAR *
Client_Mask( CLIENT *Client )
{
	/* Client-"ID" liefern, wie sie z.B. fuer
	 * Prefixe benoetigt wird. */

	assert( Client != NULL );
	
	if( Client->type == CLIENT_SERVER ) return Client->id;

	sprintf( GetID_Buffer, "%s!%s@%s", Client->id, Client->user, Client->host );
	return GetID_Buffer;
} /* Client_Mask */


GLOBAL CLIENT *
Client_Introducer( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->introducer;
} /* Client_Introducer */


GLOBAL CLIENT *
Client_TopServer( CLIENT *Client )
{
	assert( Client != NULL );
	return Client->topserver;
} /* Client_TopServer */


GLOBAL BOOLEAN
Client_HasMode( CLIENT *Client, CHAR Mode )
{
	assert( Client != NULL );
	return strchr( Client->modes, Mode ) != NULL;
} /* Client_HasMode */


GLOBAL CHAR *
Client_Away( CLIENT *Client )
{
	/* AWAY-Text liefern */

	assert( Client != NULL );
	return Client->away;
} /* Client_Away */


GLOBAL BOOLEAN
Client_CheckNick( CLIENT *Client, CHAR *Nick )
{
	/* Nick ueberpruefen */

	assert( Client != NULL );
	assert( Nick != NULL );
	
	/* Nick ungueltig? */
	if( ! Client_IsValidNick( Nick ))
	{
		IRC_WriteStrClient( Client, ERR_ERRONEUSNICKNAME_MSG, Client_ID( Client ), Nick );
		return FALSE;
	}

	/* Nick bereits vergeben? */
	if( Client_Search( Nick ))
	{
		/* den Nick gibt es bereits */
		IRC_WriteStrClient( Client, ERR_NICKNAMEINUSE_MSG, Client_ID( Client ), Nick );
		return FALSE;
	}

	return TRUE;
} /* Client_CheckNick */


GLOBAL BOOLEAN
Client_CheckID( CLIENT *Client, CHAR *ID )
{
	/* Nick ueberpruefen */

	CHAR str[COMMAND_LEN];
	CLIENT *c;

	assert( Client != NULL );
	assert( Client->conn_id > NONE );
	assert( ID != NULL );

	/* Nick zu lang? */
	if( strlen( ID ) > CLIENT_ID_LEN )
	{
		IRC_WriteStrClient( Client, ERR_ERRONEUSNICKNAME_MSG, Client_ID( Client ), ID );
		return FALSE;
	}

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
		c = (CLIENT *)c->next;
	}

	return TRUE;
} /* Client_CheckID */


GLOBAL CLIENT *
Client_First( VOID )
{
	/* Ersten Client liefern. */

	return My_Clients;
} /* Client_First */


GLOBAL CLIENT *
Client_Next( CLIENT *c )
{
	/* Naechsten Client liefern. Existiert keiner,
	 * so wird NULL geliefert. */

	assert( c != NULL );
	return (CLIENT *)c->next;
} /* Client_Next */


GLOBAL INT
Client_UserCount( VOID )
{
	return Count( CLIENT_USER );
} /* Client_UserCount */


GLOBAL INT
Client_ServiceCount( VOID )
{
	return Count( CLIENT_SERVICE );;
} /* Client_ServiceCount */


GLOBAL INT
Client_ServerCount( VOID )
{
	return Count( CLIENT_SERVER );
} /* Client_ServerCount */


GLOBAL INT
Client_MyUserCount( VOID )
{
	return MyCount( CLIENT_USER );
} /* Client_MyUserCount */


GLOBAL INT
Client_MyServiceCount( VOID )
{
	return MyCount( CLIENT_SERVICE );
} /* Client_MyServiceCount */


GLOBAL INT
Client_MyServerCount( VOID )
{
	CLIENT *c;
	INT cnt;

	cnt = 0;
	c = My_Clients;
	while( c )
	{
		if(( c->type == CLIENT_SERVER ) && ( c->hops == 1 )) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* Client_MyServerCount */


GLOBAL INT
Client_OperCount( VOID )
{
	CLIENT *c;
	INT cnt;

	cnt = 0;
	c = My_Clients;
	while( c )
	{
		if( c && ( c->type == CLIENT_USER ) && ( strchr( c->modes, 'o' ))) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* Client_OperCount */


GLOBAL INT
Client_UnknownCount( VOID )
{
	CLIENT *c;
	INT cnt;

	cnt = 0;
	c = My_Clients;
	while( c )
	{
		if( c && ( c->type != CLIENT_USER ) && ( c->type != CLIENT_SERVICE ) && ( c->type != CLIENT_SERVER )) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* Client_UnknownCount */


GLOBAL BOOLEAN
Client_IsValidNick( CHAR *Nick )
{
	/* Ist der Nick gueltig? */

	CHAR *ptr, goodchars[20];
	
	assert( Nick != NULL );

	strcpy( goodchars, ";0123456789-" );

	if( Nick[0] == '#' ) return FALSE;
	if( strchr( goodchars, Nick[0] )) return FALSE;
	if( strlen( Nick ) >= CLIENT_NICK_LEN ) return FALSE;

	ptr = Nick;
	while( *ptr )
	{
		if(( *ptr < 'A' ) && ( ! strchr( goodchars, *ptr ))) return FALSE;
		if(( *ptr > '}' ) && ( ! strchr( goodchars, *ptr ))) return FALSE;
		ptr++;
	}
	
	return TRUE;
} /* Client_IsValidNick */


LOCAL INT
Count( CLIENT_TYPE Type )
{
	CLIENT *c;
	INT cnt;

	cnt = 0;
	c = My_Clients;
	while( c )
	{
		if( c->type == Type ) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* Count */


LOCAL INT
MyCount( CLIENT_TYPE Type )
{
	CLIENT *c;
	INT cnt;

	cnt = 0;
	c = My_Clients;
	while( c )
	{
		if(( c->introducer == This_Server ) && ( c->type == Type )) cnt++;
		c = (CLIENT *)c->next;
	}
	return cnt;
} /* MyCount */


LOCAL CLIENT *
New_Client_Struct( VOID )
{
	/* Neue CLIENT-Struktur pre-initialisieren */
	
	CLIENT *c;
	
	c = malloc( sizeof( CLIENT ));
	if( ! c )
	{
		Log( LOG_EMERG, "Can't allocate memory!" );
		return NULL;
	}

	c->next = NULL;
	c->hash = 0;
	c->type = CLIENT_UNKNOWN;
	c->conn_id = NONE;
	c->introducer = NULL;
	c->topserver = NULL;
	strcpy( c->id, "" );
	strcpy( c->pwd, "" );
	strcpy( c->host, "" );
	strcpy( c->user, "" );
	strcpy( c->info, "" );
	strcpy( c->modes, "" );
	c->oper_by_me = FALSE;
	c->hops = -1;
	c->token = -1;
	c->mytoken = -1;
	strcpy( c->away, "" );

	return c;
} /* New_Client */


LOCAL VOID
Generate_MyToken( CLIENT *Client )
{
	CLIENT *c;
	INT token;

	c = My_Clients;
	token = 2;
	while( c )
	{
		if( c->mytoken == token )
		{
			/* Das Token wurde bereits vergeben */
			token++;
			c = My_Clients;
			continue;
		}
		else c = (CLIENT *)c->next;
	}
	Client->mytoken = token;
	Log( LOG_DEBUG, "Assigned token %d to server \"%s\".", token, Client->id );
} /* Generate_MyToken */


/* -eof- */
