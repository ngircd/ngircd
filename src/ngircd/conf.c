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
 * $Id: conf.c,v 1.12 2002/01/05 23:26:24 alex Exp $
 *
 * conf.h: Konfiguration des ngircd
 *
 * $Log: conf.c,v $
 * Revision 1.12  2002/01/05 23:26:24  alex
 * - Fehlermeldungen korrigiert.
 *
 * Revision 1.11  2002/01/05 16:51:49  alex
 * - Bug bei Remote-Server-Namen entfernt: diese wurden falsch gekuerzt.
 *
 * Revision 1.10  2002/01/03 02:27:20  alex
 * - das Server-Passwort kann nun konfiguriert werden.
 *
 * Revision 1.9  2002/01/02 02:49:15  alex
 * - Konfigurationsdatei "Samba like" umgestellt.
 * - es koennen nun mehrere Server und Oprtatoren konfiguriert werden.
 *
 * Revision 1.7  2002/01/01 18:25:44  alex
 * - #include's fuer stdlib.h ergaenzt.
 *
 * Revision 1.6  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.5  2001/12/30 19:26:11  alex
 * - Unterstuetzung fuer die Konfigurationsdatei eingebaut.
 *
 * Revision 1.4  2001/12/26 22:48:53  alex
 * - MOTD-Datei ist nun konfigurierbar und wird gelesen.
 *
 * Revision 1.3  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.2  2001/12/26 03:19:57  alex
 * - erste Konfigurations-Variablen definiert: PING/PONG-Timeout.
 *
 * Revision 1.1  2001/12/12 17:18:20  alex
 * - Modul fuer Server-Konfiguration begonnen.
 */


#include <portab.h>
#include "global.h"

#include <imp.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "log.h"
#include "tool.h"

#include <exp.h>
#include "conf.h"


LOCAL VOID Read_Config( VOID );

GLOBAL VOID Handle_GLOBAL( INT Line, CHAR *Var, CHAR *Arg );
GLOBAL VOID Handle_OPERATOR( INT Line, CHAR *Var, CHAR *Arg );
GLOBAL VOID Handle_SERVER( INT Line, CHAR *Var, CHAR *Arg );

LOCAL VOID Validate_Config( VOID );


GLOBAL VOID Conf_Init( VOID )
{
	/* Konfigurationsvariablen initialisieren: zunaechst Default-
	 * Werte setzen, dann Konfigurationsdtaei einlesen. */

	strcpy( Conf_File, "/usr/local/etc/ngircd.conf" );

	strcpy( Conf_ServerName, "" );
	strcpy( Conf_ServerInfo, PACKAGE" "VERSION );
	strcpy( Conf_ServerPwd, "" );

	strcpy( Conf_MotdFile, "/usr/local/etc/ngircd.motd" );

	Conf_ListenPorts_Count = 0;
	
	Conf_PingTimeout = 120;
	Conf_PongTimeout = 10;

	Conf_ConnectRetry = 60;

	Conf_Oper_Count = 0;

	Conf_Server_Count = 0;

	/* Konfigurationsdatei einlesen und validieren */
	Read_Config( );
	Validate_Config( );
} /* Config_Init */


GLOBAL VOID Conf_Exit( VOID )
{
	/* ... */
} /* Config_Exit */


LOCAL VOID Read_Config( VOID )
{
	/* Konfigurationsdatei einlesen. */

	CHAR section[LINE_LEN], str[LINE_LEN], *var, *arg, *ptr;
	INT line;
	FILE *fd;

	fd = fopen( Conf_File, "r" );
	if( ! fd )
	{
		/* Keine Konfigurationsdatei gefunden */
		Log( LOG_ALERT, "Can't read configuration \"%s\": %s", Conf_File, strerror( errno ));
		Log( LOG_ALERT, PACKAGE" exiting due to fatal errors!" );
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
				if( Conf_Oper_Count + 1 > MAX_OPERATORS ) Log( LOG_ERR, "Too many operators configured." );
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
				if( Conf_Server_Count + 1 > MAX_SERVERS ) Log( LOG_ERR, "Too many servers configured." );
				else
				{
					/* neuen Server ("Peer") initialisieren */
					strcpy( Conf_Server[Conf_Server_Count].host, "" );
					strcpy( Conf_Server[Conf_Server_Count].ip, "" );
					strcpy( Conf_Server[Conf_Server_Count].name, "" );
					strcpy( Conf_Server[Conf_Server_Count].pwd, "" );
					Conf_Server[Conf_Server_Count].port = 0;
					Conf_Server[Conf_Server_Count].lasttry = 0;
					Conf_Server[Conf_Server_Count].res_stat = NULL;
					Conf_Server_Count++;
				}
				continue;
			}
			Log( LOG_ERR, "%s, line %d: Unknown section \"%s\"!", Conf_File, line, section );
			section[0] = 0x1;
		}
		if( section[0] == 0x1 ) continue;

		/* In Variable und Argument zerlegen */
		ptr = strchr( str, '=' );
		if( ! ptr )
		{
			Log( LOG_ERR, "%s, line %d: Syntax error!", Conf_File, line );
			continue;
		}
		*ptr = '\0';
		var = str; ngt_TrimStr( var );
		arg = ptr + 1; ngt_TrimStr( arg );

		if( strcasecmp( section, "[GLOBAL]" ) == 0 ) Handle_GLOBAL( line, var, arg );
		else if( strcasecmp( section, "[OPERATOR]" ) == 0 ) Handle_OPERATOR( line, var, arg );
		else if( strcasecmp( section, "[SERVER]" ) == 0 ) Handle_SERVER( line, var, arg );
		else Log( LOG_ERR, "%s, line %d: Variable \"%s\" outside section!", Conf_File, line, var );
	}
	
	fclose( fd );
} /* Read_Config */


GLOBAL VOID Handle_GLOBAL( INT Line, CHAR *Var, CHAR *Arg )
{
	CHAR *ptr;
	INT port;
	
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );
	
	if( strcasecmp( Var, "Name" ) == 0 )
	{
		/* Der Server-Name */
		strncpy( Conf_ServerName, Arg, CLIENT_ID_LEN );
		Conf_ServerName[CLIENT_ID_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Info" ) == 0 )
	{
		/* Server-Info-Text */
		strncpy( Conf_ServerInfo, Arg, CLIENT_INFO_LEN );
		Conf_ServerInfo[CLIENT_INFO_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Password" ) == 0 )
	{
		/* Der Server-Name */
		strncpy( Conf_ServerPwd, Arg, CLIENT_PASS_LEN );
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
			if( Conf_ListenPorts_Count + 1 > MAX_LISTEN_PORTS ) Log( LOG_ERR, "Too many listen ports configured. Port %ld ignored.", port );
			else
			{
				if( port > 0 && port < 0xFFFF ) Conf_ListenPorts[Conf_ListenPorts_Count++] = port;
				else Log( LOG_ERR, "%s, line %d (section \"Global\"): Illegal port number %ld!", Conf_File, Line, port );
			}
			ptr = strtok( NULL, "," );
		}
		return;
	}
	if( strcasecmp( Var, "MotdFile" ) == 0 )
	{
		/* Datei mit der "message of the day" (MOTD) */
		strncpy( Conf_MotdFile, Arg, FNAME_LEN );
		Conf_MotdFile[FNAME_LEN - 1] = '\0';
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
		
	Log( LOG_ERR, "%s, line %d (section \"Global\"): Unknown variable \"%s\"!", Conf_File, Line, Var );
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
		strncpy( Conf_Oper[Conf_Oper_Count - 1].name, Arg, CLIENT_ID_LEN );
		Conf_Oper[Conf_Oper_Count - 1].name[CLIENT_ID_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Password" ) == 0 )
	{
		/* Passwort des IRC Operator */
		strncpy( Conf_Oper[Conf_Oper_Count - 1].pwd, Arg, CLIENT_PASS_LEN );
		Conf_Oper[Conf_Oper_Count - 1].pwd[CLIENT_PASS_LEN - 1] = '\0';
		return;
	}
	
	Log( LOG_ERR, "%s, line %d (section \"Operator\"): Unknown variable \"%s\"!", Conf_File, Line, Var );
} /* Handle_OPERATOR */


GLOBAL VOID Handle_SERVER( INT Line, CHAR *Var, CHAR *Arg )
{
	INT port;
	
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );

	if( strcasecmp( Var, "Host" ) == 0 )
	{
		/* Hostname des Servers */
		strncpy( Conf_Server[Conf_Server_Count - 1].host, Arg, HOST_LEN );
		Conf_Server[Conf_Server_Count - 1].host[HOST_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Name" ) == 0 )
	{
		/* Name des Servers ("Nick") */
		strncpy( Conf_Server[Conf_Server_Count - 1].name, Arg, CLIENT_ID_LEN );
		Conf_Server[Conf_Server_Count - 1].name[CLIENT_ID_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Password" ) == 0 )
	{
		/* Passwort des Servers */
		strncpy( Conf_Server[Conf_Server_Count - 1].pwd, Arg, CLIENT_PASS_LEN );
		Conf_Server[Conf_Server_Count - 1].pwd[CLIENT_PASS_LEN - 1] = '\0';
		return;
	}
	if( strcasecmp( Var, "Port" ) == 0 )
	{
		/* Port, zu dem Verbunden werden soll */
		port = atol( Arg );
		if( port > 0 && port < 0xFFFF ) Conf_Server[Conf_Server_Count - 1].port = port;
		else Log( LOG_ERR, "%s, line %d (section \"Server\"): Illegal port number %ld!", Conf_File, Line, port );
		return;
	}
	
	Log( LOG_ERR, "%s, line %d (section \"Server\"): Unknown variable \"%s\"!", Conf_File, Line, Var );
} /* Handle_SERVER */


LOCAL VOID Validate_Config( VOID )
{
	/* Konfiguration ueberpruefen */
	
	if( ! Conf_ServerName[0] )
	{
		/* Kein Servername konfiguriert */
		Log( LOG_ALERT, "No server name configured in \"%s\"!", Conf_File );
		Log( LOG_ALERT, PACKAGE" exiting due to fatal errors!" );
		exit( 1 );
	}
} /* Validate_Config */

/* -eof- */
