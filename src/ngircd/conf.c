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
 * $Id: conf.c,v 1.22 2002/03/29 23:33:05 alex Exp $
 *
 * conf.h: Konfiguration des ngircd
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ngircd.h"
#include "client.h"
#include "defines.h"
#include "log.h"
#include "tool.h"

#include "exp.h"
#include "conf.h"


LOCAL BOOLEAN Use_Log = TRUE;


LOCAL VOID Set_Defaults( VOID );
LOCAL VOID Read_Config( VOID );
LOCAL VOID Validate_Config( VOID );

GLOBAL VOID Handle_GLOBAL( INT Line, CHAR *Var, CHAR *Arg );
GLOBAL VOID Handle_OPERATOR( INT Line, CHAR *Var, CHAR *Arg );
GLOBAL VOID Handle_SERVER( INT Line, CHAR *Var, CHAR *Arg );

LOCAL VOID Config_Error( CONST INT Level, CONST CHAR *Format, ... );


GLOBAL VOID Conf_Init( VOID )
{
	Set_Defaults( );
	Read_Config( );
	Validate_Config( );
} /* Config_Init */


GLOBAL INT Conf_Test( VOID )
{
	/* Konfiguration einlesen, ueberpruefen und ausgeben. */

	UINT i;

	Use_Log = FALSE;
	Set_Defaults( );

	printf( "Using \"%s\" as configuration file ...\n", NGIRCd_ConfFile );
	Read_Config( );

	/* Wenn stdin ein ein TTY ist: auf Taste warten */
	if( isatty( fileno( stdout )))
	{
		puts( "OK, press enter to see a dump of your service configuration ..." );
		getchar( );
	}
	else puts( "Ok, dump of your server configuration follows:\n" );

	puts( "[GLOBAL]" );
	printf( "  ServerName = %s\n", Conf_ServerName );
	printf( "  ServerInfo = %s\n", Conf_ServerInfo );
	printf( "  ServerPwd = %s\n", Conf_ServerPwd );
	printf( "  MotdFile = %s\n", Conf_MotdFile );
	printf( "  ListenPorts = " );
	for( i = 0; i < Conf_ListenPorts_Count; i++ )
	{
		if( i != 0 ) printf( ", " );
		printf( "%u", Conf_ListenPorts[i] );
	}
	puts( "" );
	printf( "  ServerUID = %ld\n", (INT32)Conf_UID );
	printf( "  ServerGID = %ld\n", (INT32)Conf_GID );
	printf( "  PingTimeout = %d\n", Conf_PingTimeout );
	printf( "  PongTimeout = %d\n", Conf_PongTimeout );
	printf( "  ConnectRetry = %d\n", Conf_ConnectRetry );
	puts( "" );

	for( i = 0; i < Conf_Oper_Count; i++ )
	{
		puts( "[OPERATOR]" );
		printf( "  Name = %s\n", Conf_Oper[i].name );
		printf( "  Password = %s\n", Conf_Oper[i].pwd );
		puts( "" );
	}

	for( i = 0; i < Conf_Server_Count; i++ )
	{
		puts( "[SERVER]" );
		printf( "  Name = %s\n", Conf_Server[i].name );
		printf( "  Host = %s\n", Conf_Server[i].host );
		printf( "  Port = %d\n", Conf_Server[i].port );
		printf( "  Password = %s\n", Conf_Server[i].pwd );
		printf( "  Group = %d\n", Conf_Server[i].group );
		puts( "" );
	}
	
	return 0;
} /* Conf_Test */


GLOBAL VOID Conf_Exit( VOID )
{
	/* ... */
} /* Config_Exit */


LOCAL VOID Set_Defaults( VOID )
{
	/* Konfigurationsvariablen initialisieren, d.h. auf Default-Werte setzen. */

	strcpy( Conf_ServerName, "" );
	strcpy( Conf_ServerInfo, PACKAGE" "VERSION );
	strcpy( Conf_ServerPwd, "" );

	strcpy( Conf_MotdFile, MOTD_FILE );

	Conf_ListenPorts_Count = 0;

	Conf_UID = Conf_GID = 0;
	
	Conf_PingTimeout = 120;
	Conf_PongTimeout = 20;

	Conf_ConnectRetry = 60;

	Conf_Oper_Count = 0;

	Conf_Server_Count = 0;
} /* Set_Defaults */


LOCAL VOID Read_Config( VOID )
{
	/* Konfigurationsdatei einlesen. */

	CHAR section[LINE_LEN], str[LINE_LEN], *var, *arg, *ptr;
	INT line;
	FILE *fd;

	fd = fopen( NGIRCd_ConfFile, "r" );
	if( ! fd )
	{
		/* Keine Konfigurationsdatei gefunden */
		Config_Error( LOG_ALERT, "Can't read configuration \"%s\": %s", NGIRCd_ConfFile, strerror( errno ));
		Config_Error( LOG_ALERT, PACKAGE" exiting due to fatal errors!" );
		exit( 1 );
	}

	line = 0;
	strcpy( section, "" );
	while( TRUE )
	{
		if( ! fgets( str, LINE_LEN, fd )) break;
		ngt_TrimStr( str );
		line++;

		/* Kommentarzeilen und leere Zeilen ueberspringen */
		if( str[0] == ';' || str[0] == '#' || str[0] == '\0' ) continue;

		/* Anfang eines Abschnittes? */
		if(( str[0] == '[' ) && ( str[strlen( str ) - 1] == ']' ))
		{
			strcpy( section, str );
			if( strcasecmp( section, "[GLOBAL]" ) == 0 ) continue;
			if( strcasecmp( section, "[OPERATOR]" ) == 0 )
			{
				if( Conf_Oper_Count + 1 > MAX_OPERATORS ) Config_Error( LOG_ERR, "Too many operators configured." );
				else
				{
					/* neuen Operator initialisieren */
					strcpy( Conf_Oper[Conf_Oper_Count].name, "" );
					strcpy( Conf_Oper[Conf_Oper_Count].pwd, "" );
					Conf_Oper_Count++;
				}
				continue;
			}
			if( strcasecmp( section, "[SERVER]" ) == 0 )
			{
				if( Conf_Server_Count + 1 > MAX_SERVERS ) Config_Error( LOG_ERR, "Too many servers configured." );
				else
				{
					/* neuen Server ("Peer") initialisieren */
					strcpy( Conf_Server[Conf_Server_Count].host, "" );
					strcpy( Conf_Server[Conf_Server_Count].ip, "" );
					strcpy( Conf_Server[Conf_Server_Count].name, "" );
					strcpy( Conf_Server[Conf_Server_Count].pwd, "" );
					Conf_Server[Conf_Server_Count].port = 0;
					Conf_Server[Conf_Server_Count].group = -1;
					Conf_Server[Conf_Server_Count].lasttry = time( NULL ) - Conf_ConnectRetry + STARTUP_DELAY;
					Conf_Server[Conf_Server_Count].res_stat = NULL;
					Conf_Server_Count++;
				}
				continue;
			}
			Config_Error( LOG_ERR, "%s, line %d: Unknown section \"%s\"!", NGIRCd_ConfFile, line, section );
			section[0] = 0x1;
		}
		if( section[0] == 0x1 ) continue;

		/* In Variable und Argument zerlegen */
		ptr = strchr( str, '=' );
		if( ! ptr )
		{
			Config_Error( LOG_ERR, "%s, line %d: Syntax error!", NGIRCd_ConfFile, line );
			continue;
		}
		*ptr = '\0';
		var = str; ngt_TrimStr( var );
		arg = ptr + 1; ngt_TrimStr( arg );

		if( strcasecmp( section, "[GLOBAL]" ) == 0 ) Handle_GLOBAL( line, var, arg );
		else if( strcasecmp( section, "[OPERATOR]" ) == 0 ) Handle_OPERATOR( line, var, arg );
		else if( strcasecmp( section, "[SERVER]" ) == 0 ) Handle_SERVER( line, var, arg );
		else Config_Error( LOG_ERR, "%s, line %d: Variable \"%s\" outside section!", NGIRCd_ConfFile, line, var );
	}
	
	fclose( fd );
} /* Read_Config */


GLOBAL VOID Handle_GLOBAL( INT Line, CHAR *Var, CHAR *Arg )
{
	CHAR *ptr;
	INT32 port;
	
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );
	
	if( strcasecmp( Var, "Name" ) == 0 )
	{
		/* Der Server-Name */
		strncpy( Conf_ServerName, Arg, CLIENT_ID_LEN - 1 );
		Conf_ServerName[CLIENT_ID_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Info" ) == 0 )
	{
		/* Server-Info-Text */
		strncpy( Conf_ServerInfo, Arg, CLIENT_INFO_LEN - 1 );
		Conf_ServerInfo[CLIENT_INFO_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Password" ) == 0 )
	{
		/* Server-Passwort */
		strncpy( Conf_ServerPwd, Arg, CLIENT_PASS_LEN - 1 );
		Conf_ServerPwd[CLIENT_PASS_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Ports" ) == 0 )
	{
		/* Ports, durch "," getrennt, auf denen der Server
		* Verbindungen annehmen soll */
		ptr = strtok( Arg, "," );
		while( ptr )
		{
			ngt_TrimStr( ptr );
			port = atol( ptr );
			if( Conf_ListenPorts_Count + 1 > MAX_LISTEN_PORTS ) Config_Error( LOG_ERR, "Too many listen ports configured. Port %ld ignored.", port );
			else
			{
				if( port > 0 && port < 0xFFFF ) Conf_ListenPorts[Conf_ListenPorts_Count++] = (UINT)port;
				else Config_Error( LOG_ERR, "%s, line %d (section \"Global\"): Illegal port number %ld!", NGIRCd_ConfFile, Line, port );
			}
			ptr = strtok( NULL, "," );
		}
		return;
	}
	if( strcasecmp( Var, "MotdFile" ) == 0 )
	{
		/* Datei mit der "message of the day" (MOTD) */
		strncpy( Conf_MotdFile, Arg, FNAME_LEN - 1 );
		Conf_MotdFile[FNAME_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "ServerUID" ) == 0 )
	{
		/* UID, mit der der Daemon laufen soll */
		Conf_UID = (UINT)atoi( Arg );
		return;
	}
	if( strcasecmp( Var, "ServerGID" ) == 0 )
	{
		/* GID, mit der der Daemon laufen soll */
		Conf_GID = (UINT)atoi( Arg );
		return;
	}
	if( strcasecmp( Var, "PingTimeout" ) == 0 )
	{
		/* PING-Timeout */
		Conf_PingTimeout = atoi( Arg );
		if(( Conf_PingTimeout ) < 5 ) Conf_PingTimeout = 5;
		return;
	}
	if( strcasecmp( Var, "PongTimeout" ) == 0 )
	{
		/* PONG-Timeout */
		Conf_PongTimeout = atoi( Arg );
		if(( Conf_PongTimeout ) < 5 ) Conf_PongTimeout = 5;
		return;
	}
	if( strcasecmp( Var, "ConnectRetry" ) == 0 )
	{
		/* Sekunden zwischen Verbindungsversuchen zu anderen Servern */
		Conf_ConnectRetry = atoi( Arg );
		if(( Conf_ConnectRetry ) < 5 ) Conf_ConnectRetry = 5;
		return;
	}
		
	Config_Error( LOG_ERR, "%s, line %d (section \"Global\"): Unknown variable \"%s\"!", NGIRCd_ConfFile, Line, Var );
} /* Handle_GLOBAL */


GLOBAL VOID Handle_OPERATOR( INT Line, CHAR *Var, CHAR *Arg )
{
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );
	assert( Conf_Oper_Count > 0 );

	if( strcasecmp( Var, "Name" ) == 0 )
	{
		/* Name des IRC Operator */
		strncpy( Conf_Oper[Conf_Oper_Count - 1].name, Arg, CLIENT_ID_LEN - 1 );
		Conf_Oper[Conf_Oper_Count - 1].name[CLIENT_ID_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Password" ) == 0 )
	{
		/* Passwort des IRC Operator */
		strncpy( Conf_Oper[Conf_Oper_Count - 1].pwd, Arg, CLIENT_PASS_LEN - 1 );
		Conf_Oper[Conf_Oper_Count - 1].pwd[CLIENT_PASS_LEN - 1] = '\0';
		return;
	}
	
	Config_Error( LOG_ERR, "%s, line %d (section \"Operator\"): Unknown variable \"%s\"!", NGIRCd_ConfFile, Line, Var );
} /* Handle_OPERATOR */


GLOBAL VOID Handle_SERVER( INT Line, CHAR *Var, CHAR *Arg )
{
	INT32 port;
	
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );

	if( strcasecmp( Var, "Host" ) == 0 )
	{
		/* Hostname des Servers */
		strncpy( Conf_Server[Conf_Server_Count - 1].host, Arg, HOST_LEN - 1 );
		Conf_Server[Conf_Server_Count - 1].host[HOST_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Name" ) == 0 )
	{
		/* Name des Servers ("Nick") */
		strncpy( Conf_Server[Conf_Server_Count - 1].name, Arg, CLIENT_ID_LEN - 1 );
		Conf_Server[Conf_Server_Count - 1].name[CLIENT_ID_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Password" ) == 0 )
	{
		/* Passwort des Servers */
		strncpy( Conf_Server[Conf_Server_Count - 1].pwd, Arg, CLIENT_PASS_LEN - 1 );
		Conf_Server[Conf_Server_Count - 1].pwd[CLIENT_PASS_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Port" ) == 0 )
	{
		/* Port, zu dem Verbunden werden soll */
		port = atol( Arg );
		if( port > 0 && port < 0xFFFF ) Conf_Server[Conf_Server_Count - 1].port = (INT)port;
		else Config_Error( LOG_ERR, "%s, line %d (section \"Server\"): Illegal port number %ld!", NGIRCd_ConfFile, Line, port );
		return;
	}
	if( strcasecmp( Var, "Group" ) == 0 )
	{
		/* Server-Gruppe */
		Conf_Server[Conf_Server_Count - 1].group = atoi( Arg );
		return;
	}
	
	Config_Error( LOG_ERR, "%s, line %d (section \"Server\"): Unknown variable \"%s\"!", NGIRCd_ConfFile, Line, Var );
} /* Handle_SERVER */


LOCAL VOID Validate_Config( VOID )
{
	/* Konfiguration ueberpruefen */
	
	if( ! Conf_ServerName[0] )
	{
		/* Kein Servername konfiguriert */
		Config_Error( LOG_ALERT, "No server name configured in \"%s\"!", NGIRCd_ConfFile );
		Config_Error( LOG_ALERT, PACKAGE" exiting due to fatal errors!" );
		exit( 1 );
	}
} /* Validate_Config */


LOCAL VOID Config_Error( CONST INT Level, CONST CHAR *Format, ... )
{
	/* Fehler! Auf Console und/oder ins Log schreiben */

	CHAR msg[MAX_LOG_MSG_LEN];
	va_list ap;

	assert( Format != NULL );

	/* String mit variablen Argumenten zusammenbauen ... */
	va_start( ap, Format );
	vsnprintf( msg, MAX_LOG_MSG_LEN, Format, ap );
	va_end( ap );

	/* Im "normalen Betrieb" soll der Log-Mechanismus des ngIRCd verwendet
	 * werden, beim Testen der Konfiguration jedoch nicht, hier sollen alle
	 * Meldungen direkt auf die Konsole ausgegeben werden: */
	if( Use_Log ) Log( Level, msg );
	else puts( msg );
} /* Config_Error */


/* -eof- */
