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
 * $Id: conf.c,v 1.5 2001/12/30 19:26:11 alex Exp $
 *
 * conf.h: Konfiguration des ngircd
 *
 * $Log: conf.c,v $
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
#include <string.h>

#include "client.h"
#include "log.h"
#include "tool.h"

#include <exp.h>
#include "conf.h"


#define MAX_LINE_LEN 246		/* max. Laenge einer Konfigurationszeile */


LOCAL VOID Read_Config( VOID );
LOCAL VOID Validate_Config( VOID );


GLOBAL VOID Conf_Init( VOID )
{
	/* Konfigurationsvariablen initialisieren: zunaechst Default-
	 * Werte setzen, dann Konfigurationsdtaei einlesen. */

	strcpy( Conf_File, "/usr/local/etc/ngircd.conf" );

	strcpy( Conf_ServerName, "" );

	strcpy( Conf_MotdFile, "/usr/local/etc/ngircd.motd" );

	Conf_ListenPorts_Count = 0;
	
	Conf_PingTimeout = 120;
	Conf_PongTimeout = 10;

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

	CHAR str[MAX_LINE_LEN], *var, *arg, *ptr;
	BOOLEAN ok;
	INT32 port;
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
	while( TRUE )
	{
		if( ! fgets( str, MAX_LINE_LEN, fd )) break;
		ngt_TrimStr( str );
		line++;

		/* Kommentarzeilen und leere Zeilen ueberspringen */
		if( str[0] == ';' || str[0] == '#' || str[0] == '\0' ) continue;

		ok = FALSE;

		ptr = strchr( str, '=' );
		if( ! ptr )
		{
			Log( LOG_ERR, "%s, line %d: Syntax error!", Conf_File, line );
			continue;
		}
		*ptr = '\0';

		var = str; ngt_TrimStr( var );
		arg = ptr + 1; ngt_TrimStr( arg );
		
		if( strcasecmp( str, "ServerName" ) == 0 )
		{
			/* Der Server-Name */
			strncpy( Conf_ServerName, arg, CLIENT_ID_LEN );
			Conf_ServerName[CLIENT_ID_LEN] = '\0';
			ok = TRUE;
		}
		else if( strcasecmp( str, "ListenPorts" ) == 0 )
		{
			/* Ports, durch "," getrennt, auf denen der Server
			 * Verbindungen annehmen soll */
			ptr = strtok( arg, "," );
			while( ptr )
			{
				ngt_TrimStr( ptr );
				port = atol( ptr );
				if( Conf_ListenPorts_Count + 1 > LISTEN_PORTS )	Log( LOG_ERR, "Too many listen ports configured. Port %ld ignored.", port );
				if( port > 0 && port < 0xFFFF ) Conf_ListenPorts[Conf_ListenPorts_Count++] = port;
				else Log( LOG_ERR, "Illegal port number: %ld. Ignored.", port );
				ptr = strtok( NULL, "," );
			}
			ok = TRUE;
		}
		else if( strcasecmp( str, "MotdFile" ) == 0 )
		{
			/* Datei mit der "message of the day" (MOTD) */
			strncpy( Conf_MotdFile, arg, FNAME_LEN );
			Conf_MotdFile[FNAME_LEN] = '\0';
			ok = TRUE;
		}
		else if( strcasecmp( str, "PingTimeout" ) == 0 )
		{
			/* PING-Timeout */
			Conf_PingTimeout = atoi( arg );
			if(( Conf_PingTimeout ) < 5 ) Conf_PingTimeout = 5;
			ok = TRUE;
		}
		else if( strcasecmp( str, "PongTimeout" ) == 0 )
		{
			/* PONG-Timeout */
			Conf_PongTimeout = atoi( arg );
			if(( Conf_PongTimeout ) < 5 ) Conf_PongTimeout = 5;
			ok = TRUE;
		}
		
		if( ! ok ) Log( LOG_ERR, "%s, line %d: Unknown variable \"%s\"!", Conf_File, line, var );
	}
	
	fclose( fd );
} /* Read_Config */


LOCAL VOID Validate_Config( VOID )
{
	/* Konfiguration ueberpruefen */
	
	if( ! Conf_ServerName[0] )
	{
		/* Kein Servername konfiguriert */
		Log( LOG_ALERT, "No server name configured (use \"ServerName\")!", Conf_File, strerror( errno ));
		Log( LOG_ALERT, PACKAGE" exiting due to fatal errors!" );
		exit( 1 );
	}
} /* Validate_Config */

/* -eof- */
