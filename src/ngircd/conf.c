/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Configuration management (reading, parsing & validation)
 */


#include "portab.h"

static char UNUSED id[] = "$Id: conf.c,v 1.46 2002/12/18 02:52:51 alex Exp $";

#include "imp.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif

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
LOCAL VOID Validate_Config PARAMS(( BOOLEAN TestOnly ));

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
	Validate_Config( FALSE );
} /* Config_Init */


GLOBAL INT
Conf_Test( VOID )
{
	/* Read configuration, validate and output it. */

	struct passwd *pwd;
	struct group *grp;
	INT i;

	Use_Log = FALSE;
	Set_Defaults( );

	Read_Config( );
	Validate_Config( TRUE );

	/* If stdin is a valid tty wait for a key: */
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
	printf( "  Ports = " );
	for( i = 0; i < Conf_ListenPorts_Count; i++ )
	{
		if( i != 0 ) printf( ", " );
		printf( "%u", Conf_ListenPorts[i] );
	}
	puts( "" );
	pwd = getpwuid( Conf_UID );
	if( pwd ) printf( "  ServerUID = %s\n", pwd->pw_name );
	else printf( "  ServerUID = %ld\n", (LONG)Conf_UID );
	grp = getgrgid( Conf_GID );
	if( grp ) printf( "  ServerGID = %s\n", grp->gr_name );
	else printf( "  ServerGID = %ld\n", (LONG)Conf_GID );
	printf( "  PingTimeout = %d\n", Conf_PingTimeout );
	printf( "  PongTimeout = %d\n", Conf_PongTimeout );
	printf( "  ConnectRetry = %d\n", Conf_ConnectRetry );
	printf( "  OperCanUseMode = %s\n", Conf_OperCanMode == TRUE ? "yes" : "no" );
	if( Conf_MaxConnections > 0 ) printf( "  MaxConnections = %ld\n", Conf_MaxConnections );
	else printf( "  MaxConnections = -1\n" );
	if( Conf_MaxJoins > 0 ) printf( "  MaxJoins = %d\n", Conf_MaxJoins );
	else printf( "  MaxJoins = -1\n" );
	puts( "" );

	for( i = 0; i < Conf_Oper_Count; i++ )
	{
		if( ! Conf_Oper[i].name[0] ) continue;
		
		/* Valid "Operator" section */
		puts( "[OPERATOR]" );
		printf( "  Name = %s\n", Conf_Oper[i].name );
		printf( "  Password = %s\n", Conf_Oper[i].pwd );
		puts( "" );
	}

	for( i = 0; i < Conf_Server_Count; i++ )
	{
		if( ! Conf_Server[i].name[0] ) continue;
		
		/* Valid "Server" section */
		puts( "[SERVER]" );
		printf( "  Name = %s\n", Conf_Server[i].name );
		printf( "  Host = %s\n", Conf_Server[i].host );
		printf( "  Port = %d\n", Conf_Server[i].port );
		printf( "  MyPassword = %s\n", Conf_Server[i].pwd_in );
		printf( "  PeerPassword = %s\n", Conf_Server[i].pwd_out );
		printf( "  Group = %d\n", Conf_Server[i].group );
		puts( "" );
	}

	for( i = 0; i < Conf_Channel_Count; i++ )
	{
		if( ! Conf_Channel[i].name[0] ) continue;
		
		/* Valid "Channel" section */
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
	/* Initialize configuration variables with default values. */

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
	
	Conf_MaxConnections = -1;
	Conf_MaxJoins = 10;
} /* Set_Defaults */


LOCAL VOID
Read_Config( VOID )
{
	/* Read configuration file. */

	CHAR section[LINE_LEN], str[LINE_LEN], *var, *arg, *ptr;
	INT line;
	FILE *fd;

	fd = fopen( NGIRCd_ConfFile, "r" );
	if( ! fd )
	{
		/* No configuration file found! */
		Config_Error( LOG_ALERT, "Can't read configuration \"%s\": %s", NGIRCd_ConfFile, strerror( errno ));
		Config_Error( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE );
		exit( 1 );
	}

	Config_Error( LOG_INFO, "Reading configuration from \"%s\" ...", NGIRCd_ConfFile );

	line = 0;
	strcpy( section, "" );
	while( TRUE )
	{
		if( ! fgets( str, LINE_LEN, fd )) break;
		ngt_TrimStr( str );
		line++;

		/* Skip comments and empty lines */
		if( str[0] == ';' || str[0] == '#' || str[0] == '\0' ) continue;

		/* Is this the beginning of a new section? */
		if(( str[0] == '[' ) && ( str[strlen( str ) - 1] == ']' ))
		{
			strcpy( section, str );
			if( strcasecmp( section, "[GLOBAL]" ) == 0 ) continue;
			if( strcasecmp( section, "[OPERATOR]" ) == 0 )
			{
				if( Conf_Oper_Count + 1 > MAX_OPERATORS ) Config_Error( LOG_ERR, "Too many operators configured." );
				else
				{
					/* Initialize new operator structure */
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
					/* Initialize new server structure */
					strcpy( Conf_Server[Conf_Server_Count].host, "" );
					strcpy( Conf_Server[Conf_Server_Count].ip, "" );
					strcpy( Conf_Server[Conf_Server_Count].name, "" );
					strcpy( Conf_Server[Conf_Server_Count].pwd_in, "" );
					strcpy( Conf_Server[Conf_Server_Count].pwd_out, "" );
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
					/* Initialize new channel structure */
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

		/* Split line into variable name and parameters */
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
	
	/* If there are no ports configured use the default: 6667 */
	if( Conf_ListenPorts_Count < 1 )
	{
		Conf_ListenPorts_Count = 1;
		Conf_ListenPorts[0] = 6667;
	}
} /* Read_Config */


LOCAL VOID
Handle_GLOBAL( INT Line, CHAR *Var, CHAR *Arg )
{
	struct passwd *pwd;
	struct group *grp;
	CHAR *ptr;
	LONG port;
	
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );
	
	if( strcasecmp( Var, "Name" ) == 0 )
	{
		/* Server name */
		strncpy( Conf_ServerName, Arg, CLIENT_ID_LEN - 1 );
		Conf_ServerName[CLIENT_ID_LEN - 1] = '\0';
		if( strlen( Arg ) > CLIENT_ID_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"Name\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "Info" ) == 0 )
	{
		/* Info text of server */
		strncpy( Conf_ServerInfo, Arg, CLIENT_INFO_LEN - 1 );
		Conf_ServerInfo[CLIENT_INFO_LEN - 1] = '\0';
		if( strlen( Arg ) > CLIENT_INFO_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"Info\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "Password" ) == 0 )
	{
		/* Global server password */
		strncpy( Conf_ServerPwd, Arg, CLIENT_PASS_LEN - 1 );
		Conf_ServerPwd[CLIENT_PASS_LEN - 1] = '\0';
		if( strlen( Arg ) > CLIENT_PASS_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"Password\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "AdminInfo1" ) == 0 )
	{
		/* Administrative info #1 */
		strncpy( Conf_ServerAdmin1, Arg, CLIENT_INFO_LEN - 1 );
		Conf_ServerAdmin1[CLIENT_INFO_LEN - 1] = '\0';
		if( strlen( Arg ) > CLIENT_INFO_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"AdminInfo1\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "AdminInfo2" ) == 0 )
	{
		/* Administrative info #2 */
		strncpy( Conf_ServerAdmin2, Arg, CLIENT_INFO_LEN - 1 );
		Conf_ServerAdmin2[CLIENT_INFO_LEN - 1] = '\0';
		if( strlen( Arg ) > CLIENT_INFO_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"AdminInfo2\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "AdminEMail" ) == 0 )
	{
		/* Administrative email contact */
		strncpy( Conf_ServerAdminMail, Arg, CLIENT_INFO_LEN - 1 );
		Conf_ServerAdminMail[CLIENT_INFO_LEN - 1] = '\0';
		if( strlen( Arg ) > CLIENT_INFO_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"AdminEMail\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "Ports" ) == 0 )
	{
		/* Ports on that the server should listen. More port numbers
		 * must be separated by "," */
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
		/* "Message of the day" (MOTD) file */
		strncpy( Conf_MotdFile, Arg, FNAME_LEN - 1 );
		Conf_MotdFile[FNAME_LEN - 1] = '\0';
		if( strlen( Arg ) > FNAME_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"MotdFile\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "ServerUID" ) == 0 )
	{
		/* UID the daemon should switch to */
		pwd = getpwnam( Arg );
		if( pwd ) Conf_UID = pwd->pw_uid;
		else
		{
#ifdef HAVE_ISDIGIT
			if( ! isdigit( *Arg )) Config_Error( LOG_WARNING, "%s, line %d: Value of \"ServerUID\" is not a number!", NGIRCd_ConfFile, Line );
			else
#endif
			Conf_UID = (UINT)atoi( Arg );
		}
		return;
	}
	if( strcasecmp( Var, "ServerGID" ) == 0 )
	{
		/* GID the daemon should use */
		grp = getgrnam( Arg );
		if( grp ) Conf_GID = grp->gr_gid;
		else
		{
#ifdef HAVE_ISDIGIT
			if( ! isdigit( *Arg )) Config_Error( LOG_WARNING, "%s, line %d: Value of \"ServerGID\" is not a number!", NGIRCd_ConfFile, Line );
			else
#endif
			Conf_GID = (UINT)atoi( Arg );
		}
		return;
	}
	if( strcasecmp( Var, "PingTimeout" ) == 0 )
	{
		/* PING timeout */
		Conf_PingTimeout = atoi( Arg );
		if( Conf_PingTimeout < 5 )
		{
			Config_Error( LOG_WARNING, "%s, line %d: Value of \"PingTimeout\" too low!", NGIRCd_ConfFile, Line );
			Conf_PingTimeout = 5;
		}
		return;
	}
	if( strcasecmp( Var, "PongTimeout" ) == 0 )
	{
		/* PONG timeout */
		Conf_PongTimeout = atoi( Arg );
		if( Conf_PongTimeout < 5 )
		{
			Config_Error( LOG_WARNING, "%s, line %d: Value of \"PongTimeout\" too low!", NGIRCd_ConfFile, Line );
			Conf_PongTimeout = 5;
		}
		return;
	}
	if( strcasecmp( Var, "ConnectRetry" ) == 0 )
	{
		/* Seconds between connection attempts to other servers */
		Conf_ConnectRetry = atoi( Arg );
		if( Conf_ConnectRetry < 5 )
		{
			Config_Error( LOG_WARNING, "%s, line %d: Value of \"ConnectRetry\" too low!", NGIRCd_ConfFile, Line );
			Conf_ConnectRetry = 5;
		}
		return;
	}
	if( strcasecmp( Var, "OperCanUseMode" ) == 0 )
	{
		/* Are IRC operators allowed to use MODE in channels they aren't Op in? */
		if( strcasecmp( Arg, "yes" ) == 0 ) Conf_OperCanMode = TRUE;
		else if( strcasecmp( Arg, "true" ) == 0 ) Conf_OperCanMode = TRUE;
		else if( atoi( Arg ) != 0 ) Conf_OperCanMode = TRUE;
		else Conf_OperCanMode = FALSE;
		return;
	}
	if( strcasecmp( Var, "MaxConnections" ) == 0 )
	{
		/* Maximum number of connections. Values <= 0 are equal to "no limit". */
#ifdef HAVE_ISDIGIT
		if( ! isdigit( *Arg )) Config_Error( LOG_WARNING, "%s, line %d: Value of \"MaxConnections\" is not a number!", NGIRCd_ConfFile, Line );
		else
#endif
		Conf_MaxConnections = atol( Arg );
		return;
	}
	if( strcasecmp( Var, "MaxJoins" ) == 0 )
	{
		/* Maximum number of channels a user can join. Values <= 0 are equal to "no limit". */
#ifdef HAVE_ISDIGIT
		if( ! isdigit( *Arg )) Config_Error( LOG_WARNING, "%s, line %d: Value of \"MaxJoins\" is not a number!", NGIRCd_ConfFile, Line );
		else
#endif
		Conf_MaxJoins = atoi( Arg );
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
		/* Name of IRC operator */
		strncpy( Conf_Oper[Conf_Oper_Count - 1].name, Arg, CLIENT_PASS_LEN - 1 );
		Conf_Oper[Conf_Oper_Count - 1].name[CLIENT_PASS_LEN - 1] = '\0';
		if( strlen( Arg ) > CLIENT_PASS_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"Name\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "Password" ) == 0 )
	{
		/* Password of IRC operator */
		strncpy( Conf_Oper[Conf_Oper_Count - 1].pwd, Arg, CLIENT_PASS_LEN - 1 );
		Conf_Oper[Conf_Oper_Count - 1].pwd[CLIENT_PASS_LEN - 1] = '\0';
		if( strlen( Arg ) > CLIENT_PASS_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"Password\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	
	Config_Error( LOG_ERR, "%s, line %d (section \"Operator\"): Unknown variable \"%s\"!", NGIRCd_ConfFile, Line, Var );
} /* Handle_OPERATOR */


LOCAL VOID
Handle_SERVER( INT Line, CHAR *Var, CHAR *Arg )
{
	LONG port;
	
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );

	if( strcasecmp( Var, "Host" ) == 0 )
	{
		/* Hostname of the server */
		strncpy( Conf_Server[Conf_Server_Count - 1].host, Arg, HOST_LEN - 1 );
		Conf_Server[Conf_Server_Count - 1].host[HOST_LEN - 1] = '\0';
		if( strlen( Arg ) > HOST_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"Host\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "Name" ) == 0 )
	{
		/* Name of the server ("Nick"/"ID") */
		strncpy( Conf_Server[Conf_Server_Count - 1].name, Arg, CLIENT_ID_LEN - 1 );
		Conf_Server[Conf_Server_Count - 1].name[CLIENT_ID_LEN - 1] = '\0';
		if( strlen( Arg ) > CLIENT_ID_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"Name\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "MyPassword" ) == 0 )
	{
		/* Password of this server which is sent to the peer */
		strncpy( Conf_Server[Conf_Server_Count - 1].pwd_in, Arg, CLIENT_PASS_LEN - 1 );
		Conf_Server[Conf_Server_Count - 1].pwd_in[CLIENT_PASS_LEN - 1] = '\0';
		if( strlen( Arg ) > CLIENT_PASS_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"MyPassword\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "PeerPassword" ) == 0 )
	{
		/* Passwort of the peer which must be received */
		strncpy( Conf_Server[Conf_Server_Count - 1].pwd_out, Arg, CLIENT_PASS_LEN - 1 );
		Conf_Server[Conf_Server_Count - 1].pwd_out[CLIENT_PASS_LEN - 1] = '\0';
		if( strlen( Arg ) > CLIENT_PASS_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"PeerPassword\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "Port" ) == 0 )
	{
		/* Port to which this server should connect */
		port = atol( Arg );
		if( port > 0 && port < 0xFFFF ) Conf_Server[Conf_Server_Count - 1].port = (INT)port;
		else Config_Error( LOG_ERR, "%s, line %d (section \"Server\"): Illegal port number %ld!", NGIRCd_ConfFile, Line, port );
		return;
	}
	if( strcasecmp( Var, "Group" ) == 0 )
	{
		/* Server group */
#ifdef HAVE_ISDIGIT
		if( ! isdigit( *Arg )) Config_Error( LOG_WARNING, "%s, line %d: Value of \"Group\" is not a number!", NGIRCd_ConfFile, Line );
		else
#endif
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
		/* Name of the channel */
		strncpy( Conf_Channel[Conf_Channel_Count - 1].name, Arg, CHANNEL_NAME_LEN - 1 );
		Conf_Channel[Conf_Channel_Count - 1].name[CHANNEL_NAME_LEN - 1] = '\0';
		if( strlen( Arg ) > CHANNEL_NAME_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"Name\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "Modes" ) == 0 )
	{
		/* Initial modes */
		strncpy( Conf_Channel[Conf_Channel_Count - 1].modes, Arg, CHANNEL_MODE_LEN - 1 );
		Conf_Channel[Conf_Channel_Count - 1].modes[CHANNEL_MODE_LEN - 1] = '\0';
		if( strlen( Arg ) > CHANNEL_MODE_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"Modes\" too long!", NGIRCd_ConfFile, Line );
		return;
	}
	if( strcasecmp( Var, "Topic" ) == 0 )
	{
		/* Initial topic */
		strncpy( Conf_Channel[Conf_Channel_Count - 1].topic, Arg, CHANNEL_TOPIC_LEN - 1 );
		Conf_Channel[Conf_Channel_Count - 1].topic[CHANNEL_TOPIC_LEN - 1] = '\0';
		if( strlen( Arg ) > CHANNEL_TOPIC_LEN - 1 ) Config_Error( LOG_WARNING, "%s, line %d: Value of \"Topic\" too long!", NGIRCd_ConfFile, Line );
		return;
	}

	Config_Error( LOG_ERR, "%s, line %d (section \"Channel\"): Unknown variable \"%s\"!", NGIRCd_ConfFile, Line, Var );
} /* Handle_CHANNEL */


LOCAL VOID
Validate_Config( BOOLEAN Configtest )
{
	/* Validate configuration settings. */
	
	INT i;
	
	if( ! Conf_ServerName[0] )
	{
		/* No server name configured! */
		Config_Error( LOG_ALERT, "No server name configured in \"%s\" ('ServerName')!", NGIRCd_ConfFile );
		if( ! Configtest )
		{
			Config_Error( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE );
			exit( 1 );
		}
	}

#ifdef STRICT_RFC
	if( ! Conf_ServerAdminMail[0] )
	{
		/* No administrative contact configured! */
		Config_Error( LOG_ALERT, "No administrator email address configured in \"%s\" ('AdminEMail')!", NGIRCd_ConfFile );
		if( ! Configtest )
		{
			Config_Error( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE );
			exit( 1 );
		}
	}
#endif

	if( ! Conf_ServerAdmin1[0] && ! Conf_ServerAdmin2[0] && ! Conf_ServerAdminMail[0] )
	{
		/* No administrative information configured! */
		Config_Error( LOG_WARNING, "No administrative information configured but required by RFC!" );
	}
#ifdef FD_SETSIZE	
	if(( Conf_MaxConnections > (LONG)FD_SETSIZE ) || ( Conf_MaxConnections < 1 ))
	{
		Conf_MaxConnections = (LONG)FD_SETSIZE;
		Config_Error( LOG_ERR, "Setting MaxConnections to %ld, select() can't handle more file descriptors!", Conf_MaxConnections );
	}
#else
	Config_Error( LOG_WARN, "Don't know how many file descriptors select() can handle on this system, don't set MaxConnections too high!" );
#endif
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
	/* Error! Write to console and/or logfile. */

	CHAR msg[MAX_LOG_MSG_LEN];
	va_list ap;

	assert( Format != NULL );

#ifdef PROTOTYPES
	va_start( ap, Format );
#else
	va_start( ap );
#endif
	vsnprintf( msg, MAX_LOG_MSG_LEN, Format, ap );
	va_end( ap );
	
	/* During "normal operations" the log functions of the daemon should
	 * be used, but during testing of the configuration file, all messages
	 * should go directly to the console: */
	if( Use_Log ) Log( Level, "%s", msg );
	else puts( msg );
} /* Config_Error */


/* -eof- */
