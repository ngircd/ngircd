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
 * $Id: conf.c,v 1.29.2.3 2002/10/04 13:12:46 alex Exp $
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
#include "conn.h"
#include "client.h"
#include "defines.h"
#include "log.h"
#include "resolve.h"
#include "tool.h"

#include "exp.h"
#include "conf.h"


LOCAL BOOLEAN Use_Log = TRUE;


LOCAL VOID Set_Defaults PARAMS(( VOID ));
LOCAL VOID Read_Config PARAMS(( VOID ));
LOCAL VOID Validate_Config PARAMS(( VOID ));

LOCAL VOID Handle_GLOBAL PARAMS(( INT Line, CHAR *Var, CHAR *Arg ));
LOCAL VOID Handle_OPERATOR PARAMS(( INT Line, CHAR *Var, CHAR *Arg ));
LOCAL VOID Handle_SERVER PARAMS(( INT Line, CHAR *Var, CHAR *Arg ));
LOCAL VOID Handle_CHANNEL PARAMS(( INT Line, CHAR *Var, CHAR *Arg ));

LOCAL VOID Config_Error PARAMS(( CONST INT Level, CONST CHAR *Format, ... ));


GLOBAL VOID
Conf_Init( VOID )
{
	Set_Defaults( );
	Read_Config( );
	Validate_Config( );
} /* Config_Init */


GLOBAL INT
Conf_Test( VOID )
{
	/* Konfiguration einlesen, ueberpruefen und ausgeben. */

	INT i;

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
	printf( "  AdminInfo1 = %s\n", Conf_ServerAdmin1 );
	printf( "  AdminInfo2 = %s\n", Conf_ServerAdmin2 );
	printf( "  AdminEMail = %s\n", Conf_ServerAdminMail );
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
	printf( "  OperCanUseMode = %s\n", Conf_OperCanMode == TRUE ? "yes" : "no" );
	puts( "" );

	for( i = 0; i < Conf_Oper_Count; i++ )
	{
		if( ! Conf_Oper[i].name[0] ) continue;
		
		/* gueltiger Operator-Block: ausgeben */
		puts( "[OPERATOR]" );
		printf( "  Name = %s\n", Conf_Oper[i].name );
		printf( "  Password = %s\n", Conf_Oper[i].pwd );
		puts( "" );
	}

	for( i = 0; i < Conf_Server_Count; i++ )
	{
		if( ! Conf_Server[i].name[0] ) continue;
		if( ! Conf_Server[i].host[0] ) continue;
		
		/* gueltiger Server-Block: ausgeben */
		puts( "[SERVER]" );
		printf( "  Name = %s\n", Conf_Server[i].name );
		printf( "  Host = %s\n", Conf_Server[i].host );
		printf( "  Port = %d\n", Conf_Server[i].port );
		printf( "  Password = %s\n", Conf_Server[i].pwd );
		printf( "  Group = %d\n", Conf_Server[i].group );
		puts( "" );
	}

	for( i = 0; i < Conf_Channel_Count; i++ )
	{
		if( ! Conf_Channel[i].name[0] ) continue;
		
		/* gueltiger Channel-Block: ausgeben */
		puts( "[CHANNEL]" );
		printf( "  Name = %s\n", Conf_Channel[i].name );
		printf( "  Modes = %s\n", Conf_Channel[i].modes );
		printf( "  Topic = %s\n", Conf_Channel[i].topic );
		puts( "" );
	}
	
	return 0;
} /* Conf_Test */


LOCAL VOID
Set_Defaults( VOID )
{
	/* Konfigurationsvariablen initialisieren, d.h. auf Default-Werte setzen. */

	strcpy( Conf_ServerName, "" );
	sprintf( Conf_ServerInfo, "%s %s", PACKAGE, VERSION );
	strcpy( Conf_ServerPwd, "" );

	strcpy( Conf_ServerAdmin1, "" );
	strcpy( Conf_ServerAdmin2, "" );
	strcpy( Conf_ServerAdminMail, "" );

	strcpy( Conf_MotdFile, MOTD_FILE );

	Conf_ListenPorts_Count = 0;

	Conf_UID = Conf_GID = 0;
	
	Conf_PingTimeout = 120;
	Conf_PongTimeout = 20;

	Conf_ConnectRetry = 60;

	Conf_Oper_Count = 0;
	Conf_Server_Count = 0;
	Conf_Channel_Count = 0;

	Conf_OperCanMode = FALSE;
} /* Set_Defaults */


LOCAL VOID
Read_Config( VOID )
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
		Config_Error( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE );
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
			if( strcasecmp( section, "[CHANNEL]" ) == 0 )
			{
				if( Conf_Channel_Count + 1 > MAX_DEFCHANNELS ) Config_Error( LOG_ERR, "Too many pre-defined channels configured." );
				else
				{
					/* neuen vordefinierten Channel initialisieren */
					strcpy( Conf_Channel[Conf_Channel_Count].name, "" );
					strcpy( Conf_Channel[Conf_Channel_Count].modes, "" );
					strcpy( Conf_Channel[Conf_Channel_Count].topic, "" );
					Conf_Channel_Count++;
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
		else if( strcasecmp( section, "[CHANNEL]" ) == 0 ) Handle_CHANNEL( line, var, arg );
		else Config_Error( LOG_ERR, "%s, line %d: Variable \"%s\" outside section!", NGIRCd_ConfFile, line, var );
	}
	
	fclose( fd );
	
	/* Wenn kein Port definiert wurde, Port 6667 als Default benutzen */
	if( Conf_ListenPorts_Count < 1 )
	{
		Conf_ListenPorts_Count = 1;
		Conf_ListenPorts[0] = 6667;
	}
} /* Read_Config */


LOCAL VOID
Handle_GLOBAL( INT Line, CHAR *Var, CHAR *Arg )
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
	if( strcasecmp( Var, "AdminInfo1" ) == 0 )
	{
		/* Server-Info-Text */
		strncpy( Conf_ServerAdmin1, Arg, CLIENT_INFO_LEN - 1 );
		Conf_ServerAdmin1[CLIENT_INFO_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "AdminInfo2" ) == 0 )
	{
		/* Server-Info-Text */
		strncpy( Conf_ServerAdmin2, Arg, CLIENT_INFO_LEN - 1 );
		Conf_ServerAdmin2[CLIENT_INFO_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "AdminEMail" ) == 0 )
	{
		/* Server-Info-Text */
		strncpy( Conf_ServerAdminMail, Arg, CLIENT_INFO_LEN - 1 );
		Conf_ServerAdminMail[CLIENT_INFO_LEN - 1] = '\0';
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
	if( strcasecmp( Var, "OperCanUseMode" ) == 0 )
	{
		/* Koennen IRC-Operatoren immer MODE benutzen? */
		if( strcasecmp( Arg, "yes" ) == 0 ) Conf_OperCanMode = TRUE;
		else if( strcasecmp( Arg, "true" ) == 0 ) Conf_OperCanMode = TRUE;
		else if( atoi( Arg ) != 0 ) Conf_OperCanMode = TRUE;
		else Conf_OperCanMode = FALSE;
		return;
	}
		
	Config_Error( LOG_ERR, "%s, line %d (section \"Global\"): Unknown variable \"%s\"!", NGIRCd_ConfFile, Line, Var );
} /* Handle_GLOBAL */


LOCAL VOID
Handle_OPERATOR( INT Line, CHAR *Var, CHAR *Arg )
{
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );
	assert( Conf_Oper_Count > 0 );

	if( strcasecmp( Var, "Name" ) == 0 )
	{
		/* Name des IRC Operator */
		strncpy( Conf_Oper[Conf_Oper_Count - 1].name, Arg, CLIENT_PASS_LEN - 1 );
		Conf_Oper[Conf_Oper_Count - 1].name[CLIENT_PASS_LEN - 1] = '\0';
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


LOCAL VOID
Handle_SERVER( INT Line, CHAR *Var, CHAR *Arg )
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


LOCAL VOID
Handle_CHANNEL( INT Line, CHAR *Var, CHAR *Arg )
{
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );

	if( strcasecmp( Var, "Name" ) == 0 )
	{
		/* Hostname des Servers */
		strncpy( Conf_Channel[Conf_Channel_Count - 1].name, Arg, CHANNEL_NAME_LEN - 1 );
		Conf_Channel[Conf_Channel_Count - 1].name[CHANNEL_NAME_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Modes" ) == 0 )
	{
		/* Name des Servers ("Nick") */
		strncpy( Conf_Channel[Conf_Channel_Count - 1].modes, Arg, CHANNEL_MODE_LEN - 1 );
		Conf_Channel[Conf_Channel_Count - 1].modes[CHANNEL_MODE_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Topic" ) == 0 )
	{
		/* Passwort des Servers */
		strncpy( Conf_Channel[Conf_Channel_Count - 1].topic, Arg, CHANNEL_TOPIC_LEN - 1 );
		Conf_Channel[Conf_Channel_Count - 1].topic[CHANNEL_TOPIC_LEN - 1] = '\0';
		return;
	}

	Config_Error( LOG_ERR, "%s, line %d (section \"Channel\"): Unknown variable \"%s\"!", NGIRCd_ConfFile, Line, Var );
} /* Handle_CHANNEL */


LOCAL VOID
Validate_Config( VOID )
{
	/* Konfiguration ueberpruefen */
	
	if( ! Conf_ServerName[0] )
	{
		/* Kein Servername konfiguriert */
		Config_Error( LOG_ALERT, "No server name configured in \"%s\" ('ServerName')!", NGIRCd_ConfFile );
		Config_Error( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE );
		exit( 1 );
	}

#ifdef STRICT_RFC
	if( ! ConfAdminMail[0] )
	{
		/* Keine Server-Information konfiguriert */
		Config_Error( LOG_ALERT, "No administrator email address configured in \"%s\" ('AdminEMail')!", NGIRCd_ConfFile );
		Config_Error( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE );
		exit( 1 );
	}
#endif

	if( ! Conf_ServerAdmin1[0] && ! Conf_ServerAdmin2[0] && ! Conf_ServerAdminMail[0] )
	{
		/* Keine Server-Information konfiguriert */
		Log( LOG_WARNING, "No server information configured but required by RFC!" );
	}
} /* Validate_Config */


#ifdef PROTOTYPES
LOCAL VOID Config_Error( CONST INT Level, CONST CHAR *Format, ... )
#else
LOCAL VOID Config_Error( Level, Format, va_alist )
CONST INT Level;
CONST CHAR *Format;
va_dcl
#endif
{
	/* Fehler! Auf Console und/oder ins Log schreiben */

	CHAR msg[MAX_LOG_MSG_LEN];
	va_list ap;

	assert( Format != NULL );

	/* String mit variablen Argumenten zusammenbauen ... */
#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( msg, MAX_LOG_MSG_LEN, Format, ap );
	va_end( ap );

	/* Im "normalen Betrieb" soll der Log-Mechanismus des ngIRCd verwendet
	 * werden, beim Testen der Konfiguration jedoch nicht, hier sollen alle
	 * Meldungen direkt auf die Konsole ausgegeben werden: */
	if( Use_Log ) Log( Level, "%s", msg );
	else puts( msg );
} /* Config_Error */


/* -eof- */
