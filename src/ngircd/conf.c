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

static char UNUSED id[] = "$Id: conf.c,v 1.105 2008/03/18 20:12:47 fw Exp $";

#include "imp.h"
#include <assert.h>
#include <errno.h>
#ifdef PROTOTYPES
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif

#include "array.h"
#include "ngircd.h"
#include "conn.h"
#include "client.h"
#include "defines.h"
#include "log.h"
#include "resolve.h"
#include "tool.h"

#include "exp.h"
#include "conf.h"


static bool Use_Log = true;
static CONF_SERVER New_Server;
static int New_Server_Idx;


static void Set_Defaults PARAMS(( bool InitServers ));
static bool Read_Config PARAMS(( bool ngircd_starting ));
static void Validate_Config PARAMS(( bool TestOnly, bool Rehash ));

static void Handle_GLOBAL PARAMS(( int Line, char *Var, char *Arg ));
static void Handle_OPERATOR PARAMS(( int Line, char *Var, char *Arg ));
static void Handle_SERVER PARAMS(( int Line, char *Var, char *Arg ));
static void Handle_CHANNEL PARAMS(( int Line, char *Var, char *Arg ));

static void Config_Error PARAMS(( const int Level, const char *Format, ... ));

static void Config_Error_NaN PARAMS(( const int LINE, const char *Value ));
static void Config_Error_TooLong PARAMS(( const int LINE, const char *Value ));

static void Init_Server_Struct PARAMS(( CONF_SERVER *Server ));


static char *
strdup_warn(const char *str)
{
	char *ptr = strdup(str);
	if (!ptr)
		Config_Error(LOG_ERR, "Could not allocate mem for string: %s", str);
	return ptr;
}


static void
ports_puts(array *a)
{
	size_t len;
	UINT16 *ports;
	len = array_length(a, sizeof(UINT16));
	if (len--) {
		ports = (UINT16*) array_start(a);
		printf("%u", (unsigned int) *ports);
		while (len--) {
			ports++;
			printf(", %u", (unsigned int) *ports);
		}
	}
	putc('\n', stdout);
}


static void
ports_parse(array *a, int Line, char *Arg)
{
	char *ptr;
	int port;
	UINT16 port16;

	array_trunc(a);

	/* Ports on that the server should listen. More port numbers
	 * must be separated by "," */
	ptr = strtok( Arg, "," );
	while (ptr) {
		ngt_TrimStr( ptr );
		port = atol( ptr );
		if (port > 0 && port < 0xFFFF) {
			port16 = (UINT16) port;
			if (!array_catb(a, (char*)&port16, sizeof port16))
				Config_Error(LOG_ERR, "%s, line %d Could not add port number %ld: %s",
							NGIRCd_ConfFile, Line, port, strerror(errno));
		} else {
			Config_Error( LOG_ERR, "%s, line %d (section \"Global\"): Illegal port number %ld!",
									NGIRCd_ConfFile, Line, port );
		}

		ptr = strtok( NULL, "," );
	}
}


GLOBAL void
Conf_Init( void )
{
	Read_Config( true );
	Validate_Config(false, false);
} /* Config_Init */


GLOBAL bool
Conf_Rehash( void )
{
	if (!Read_Config(false))
		return false;
	Validate_Config(false, true);

	/* Update CLIENT structure of local server */
	Client_SetInfo(Client_ThisServer(), Conf_ServerInfo);
	return true;
} /* Config_Rehash */


GLOBAL int
Conf_Test( void )
{
	/* Read configuration, validate and output it. */

	struct passwd *pwd;
	struct group *grp;
	unsigned int i;
	char *topic;

	Use_Log = false;

	Read_Config( true );
	Validate_Config(true, false);

	/* If stdin and stdout ("you can read our nice message and we can
	 * read in your keypress") are valid tty's, wait for a key: */
	if( isatty( fileno( stdin )) && isatty( fileno( stdout ))) {
		puts( "OK, press enter to see a dump of your service configuration ..." );
		getchar( );
	} else {
		puts( "Ok, dump of your server configuration follows:\n" );
	}

	puts( "[GLOBAL]" );
	printf( "  Name = %s\n", Conf_ServerName );
	printf( "  Info = %s\n", Conf_ServerInfo );
	printf( "  Password = %s\n", Conf_ServerPwd );
	printf( "  AdminInfo1 = %s\n", Conf_ServerAdmin1 );
	printf( "  AdminInfo2 = %s\n", Conf_ServerAdmin2 );
	printf( "  AdminEMail = %s\n", Conf_ServerAdminMail );
	printf( "  MotdFile = %s\n", Conf_MotdFile );
	printf( "  MotdPhrase = %s\n", Conf_MotdPhrase );
	printf( "  ChrootDir = %s\n", Conf_Chroot );
	printf( "  PidFile = %s\n", Conf_PidFile);
	fputs("  Ports = ", stdout);

	ports_puts(&Conf_ListenPorts);

	printf( "  Listen = %s\n", Conf_ListenAddress );
	pwd = getpwuid( Conf_UID );
	if( pwd ) printf( "  ServerUID = %s\n", pwd->pw_name );
	else printf( "  ServerUID = %ld\n", (long)Conf_UID );
	grp = getgrgid( Conf_GID );
	if( grp ) printf( "  ServerGID = %s\n", grp->gr_name );
	else printf( "  ServerGID = %ld\n", (long)Conf_GID );
	printf( "  PingTimeout = %d\n", Conf_PingTimeout );
	printf( "  PongTimeout = %d\n", Conf_PongTimeout );
	printf( "  ConnectRetry = %d\n", Conf_ConnectRetry );
	printf( "  OperCanUseMode = %s\n", Conf_OperCanMode == true ? "yes" : "no" );
	printf( "  OperServerMode = %s\n", Conf_OperServerMode == true? "yes" : "no" );
	printf( "  PredefChannelsOnly = %s\n", Conf_PredefChannelsOnly == true ? "yes" : "no" );
	printf( "  NoDNS = %s\n", Conf_NoDNS ? "yes" : "no");
	printf( "  MaxConnections = %ld\n", Conf_MaxConnections);
	printf( "  MaxConnectionsIP = %d\n", Conf_MaxConnectionsIP);
	printf( "  MaxJoins = %d\n", Conf_MaxJoins>0 ? Conf_MaxJoins : -1);
	printf( "  MaxNickLength = %u\n\n", Conf_MaxNickLength - 1);

	for( i = 0; i < Conf_Oper_Count; i++ ) {
		if( ! Conf_Oper[i].name[0] ) continue;

		/* Valid "Operator" section */
		puts( "[OPERATOR]" );
		printf( "  Name = %s\n", Conf_Oper[i].name );
		printf( "  Password = %s\n", Conf_Oper[i].pwd );
		if ( Conf_Oper[i].mask ) printf( "  Mask = %s\n", Conf_Oper[i].mask );
		puts( "" );
	}

	for( i = 0; i < MAX_SERVERS; i++ ) {
		if( ! Conf_Server[i].name[0] ) continue;

		/* Valid "Server" section */
		puts( "[SERVER]" );
		printf( "  Name = %s\n", Conf_Server[i].name );
		printf( "  Host = %s\n", Conf_Server[i].host );
		printf( "  Port = %u\n", (unsigned int)Conf_Server[i].port );
		printf( "  MyPassword = %s\n", Conf_Server[i].pwd_in );
		printf( "  PeerPassword = %s\n", Conf_Server[i].pwd_out );
		printf( "  Group = %d\n", Conf_Server[i].group );
		printf( "  Passive = %s\n\n", Conf_Server[i].flags & CONF_SFLAG_DISABLED ? "yes" : "no");
	}

	for( i = 0; i < Conf_Channel_Count; i++ ) {
		if( ! Conf_Channel[i].name[0] ) continue;

		/* Valid "Channel" section */
		puts( "[CHANNEL]" );
		printf( "  Name = %s\n", Conf_Channel[i].name );
		printf( "  Modes = %s\n", Conf_Channel[i].modes );
		printf( "  Key = %s\n", Conf_Channel[i].key );
		printf( "  MaxUsers = %lu\n", Conf_Channel[i].maxusers );

		topic = (char*)array_start(&Conf_Channel[i].topic);
		printf( "  Topic = %s\n\n", topic ? topic : "");
	}

	return 0;
} /* Conf_Test */


GLOBAL void
Conf_UnsetServer( CONN_ID Idx )
{
	/* Set next time for next connection attempt, if this is a server
	 * link that is (still) configured here. If the server is set as
	 * "once", delete it from our configuration.
	 * Non-Server-Connections will be silently ignored. */

	int i;
	time_t t;

	/* Check all our configured servers */
	for( i = 0; i < MAX_SERVERS; i++ ) {
		if( Conf_Server[i].conn_id != Idx ) continue;

		/* Gotcha! Mark server configuration as "unused": */
		Conf_Server[i].conn_id = NONE;

		if( Conf_Server[i].flags & CONF_SFLAG_ONCE ) {
			/* Delete configuration here */
			Init_Server_Struct( &Conf_Server[i] );
		} else {
			/* Set time for next connect attempt */
			t = time(NULL);
			if (Conf_Server[i].lasttry < t - Conf_ConnectRetry) {
				/* The connection has been "long", so we don't
				 * require the next attempt to be delayed. */
				Conf_Server[i].lasttry =
					t - Conf_ConnectRetry + RECONNECT_DELAY;
			} else
				Conf_Server[i].lasttry = t;
		}
	}
} /* Conf_UnsetServer */


GLOBAL void
Conf_SetServer( int ConfServer, CONN_ID Idx )
{
	/* Set connection for specified configured server */

	assert( ConfServer > NONE );
	assert( Idx > NONE );

	Conf_Server[ConfServer].conn_id = Idx;
} /* Conf_SetServer */


GLOBAL int
Conf_GetServer( CONN_ID Idx )
{
	/* Get index of server in configuration structure */

	int i = 0;

	assert( Idx > NONE );

	for( i = 0; i < MAX_SERVERS; i++ ) {
		if( Conf_Server[i].conn_id == Idx ) return i;
	}
	return NONE;
} /* Conf_GetServer */


GLOBAL bool
Conf_EnableServer( char *Name, UINT16 Port )
{
	/* Enable specified server and adjust port */

	int i;

	assert( Name != NULL );

	for( i = 0; i < MAX_SERVERS; i++ ) {
		if( strcasecmp( Conf_Server[i].name, Name ) == 0 ) {
			/* Gotcha! Set port and enable server: */
			Conf_Server[i].port = Port;
			Conf_Server[i].flags &= ~CONF_SFLAG_DISABLED;
			return true;
		}
	}
	return false;
} /* Conf_EnableServer */


GLOBAL bool
Conf_EnablePassiveServer(const char *Name)
{
	/* Enable specified server */
	int i;

	assert( Name != NULL );
	for (i = 0; i < MAX_SERVERS; i++) {
		if ((strcasecmp( Conf_Server[i].name, Name ) == 0) && (Conf_Server[i].port > 0)) {
			/* BINGO! Enable server */
			Conf_Server[i].flags &= ~CONF_SFLAG_DISABLED;
			return true;
		}
	}
	return false;
} /* Conf_EnablePassiveServer */


GLOBAL bool
Conf_DisableServer( char *Name )
{
	/* Enable specified server and adjust port */

	int i;

	assert( Name != NULL );

	for( i = 0; i < MAX_SERVERS; i++ ) {
		if( strcasecmp( Conf_Server[i].name, Name ) == 0 ) {
			/* Gotcha! Disable and disconnect server: */
			Conf_Server[i].flags |= CONF_SFLAG_DISABLED;
			if( Conf_Server[i].conn_id > NONE ) Conn_Close( Conf_Server[i].conn_id, NULL, "Server link terminated on operator request", true);
			return true;
		}
	}
	return false;
} /* Conf_DisableServer */


GLOBAL bool
Conf_AddServer( char *Name, UINT16 Port, char *Host, char *MyPwd, char *PeerPwd )
{
	/* Add new server to configuration */

	int i;

	assert( Name != NULL );
	assert( Host != NULL );
	assert( MyPwd != NULL );
	assert( PeerPwd != NULL );

	/* Search unused item in server configuration structure */
	for( i = 0; i < MAX_SERVERS; i++ ) {
		/* Is this item used? */
		if( ! Conf_Server[i].name[0] ) break;
	}
	if( i >= MAX_SERVERS ) return false;

	Init_Server_Struct( &Conf_Server[i] );
	strlcpy( Conf_Server[i].name, Name, sizeof( Conf_Server[i].name ));
	strlcpy( Conf_Server[i].host, Host, sizeof( Conf_Server[i].host ));
	strlcpy( Conf_Server[i].pwd_out, MyPwd, sizeof( Conf_Server[i].pwd_out ));
	strlcpy( Conf_Server[i].pwd_in, PeerPwd, sizeof( Conf_Server[i].pwd_in ));
	Conf_Server[i].port = Port;
	Conf_Server[i].flags = CONF_SFLAG_ONCE;

	return true;
} /* Conf_AddServer */


static void
Set_Defaults( bool InitServers )
{
	/* Initialize configuration variables with default values. */

	int i;

	strcpy( Conf_ServerName, "" );
	snprintf( Conf_ServerInfo, sizeof Conf_ServerInfo, "%s %s", PACKAGE_NAME, PACKAGE_VERSION );
	strcpy( Conf_ServerPwd, "" );

	strcpy( Conf_ServerAdmin1, "" );
	strcpy( Conf_ServerAdmin2, "" );
	strcpy( Conf_ServerAdminMail, "" );

	strlcpy( Conf_MotdFile, SYSCONFDIR, sizeof( Conf_MotdFile ));
	strlcat( Conf_MotdFile, MOTD_FILE, sizeof( Conf_MotdFile ));

	strlcpy( Conf_MotdPhrase, MOTD_PHRASE, sizeof( Conf_MotdPhrase ));

	strlcpy( Conf_Chroot, CHROOT_DIR, sizeof( Conf_Chroot ));

	strlcpy( Conf_PidFile, PID_FILE, sizeof( Conf_PidFile ));

	strcpy( Conf_ListenAddress, "" );

	Conf_UID = Conf_GID = 0;

	Conf_PingTimeout = 120;
	Conf_PongTimeout = 20;

	Conf_ConnectRetry = 60;

	Conf_Oper_Count = 0;
	Conf_Channel_Count = 0;

	Conf_OperCanMode = false;
	Conf_NoDNS = false;
	Conf_PredefChannelsOnly = false;
	Conf_OperServerMode = false;

	Conf_MaxConnections = 0;
	Conf_MaxConnectionsIP = 5;
	Conf_MaxJoins = 10;
	Conf_MaxNickLength = CLIENT_NICK_LEN_DEFAULT;

	/* Initialize server configuration structures */
	if( InitServers ) for( i = 0; i < MAX_SERVERS; Init_Server_Struct( &Conf_Server[i++] ));
} /* Set_Defaults */


static bool
Read_Config( bool ngircd_starting )
{
	/* Read configuration file. */

	char section[LINE_LEN], str[LINE_LEN], *var, *arg, *ptr;
	const UINT16 defaultport = 6667;
	int line, i, n;
	FILE *fd;

	/* Open configuration file */
	fd = fopen( NGIRCd_ConfFile, "r" );
	if( ! fd ) {
		/* No configuration file found! */
		Config_Error( LOG_ALERT, "Can't read configuration \"%s\": %s",
					NGIRCd_ConfFile, strerror( errno ));
		if (!ngircd_starting)
			return false;
		Config_Error( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE_NAME );
		exit( 1 );
	}

	Set_Defaults( ngircd_starting );

	Config_Error( LOG_INFO, "Reading configuration from \"%s\" ...", NGIRCd_ConfFile );

	/* Clean up server configuration structure: mark all already
	 * configured servers as "once" so that they are deleted
	 * after the next disconnect and delete all unused servers.
	 * And delete all servers which are "duplicates" of servers
	 * that are already marked as "once" (such servers have been
	 * created by the last rehash but are now useless). */
	for( i = 0; i < MAX_SERVERS; i++ ) {
		if( Conf_Server[i].conn_id == NONE ) Init_Server_Struct( &Conf_Server[i] );
		else {
			/* This structure is in use ... */
			if( Conf_Server[i].flags & CONF_SFLAG_ONCE ) {
				/* Check for duplicates */
				for( n = 0; n < MAX_SERVERS; n++ ) {
					if( n == i ) continue;

					if( Conf_Server[i].conn_id == Conf_Server[n].conn_id ) {
						Init_Server_Struct( &Conf_Server[n] );
#ifdef DEBUG
						Log(LOG_DEBUG,"Deleted unused duplicate server %d (kept %d).",
												n, i );
#endif
					}
				}
			} else {
				/* Mark server as "once" */
				Conf_Server[i].flags |= CONF_SFLAG_ONCE;
				Log( LOG_DEBUG, "Marked server %d as \"once\"", i );
			}
		}
	}

	/* Initialize variables */
	line = 0;
	strcpy( section, "" );
	Init_Server_Struct( &New_Server );
	New_Server_Idx = NONE;

	/* Read configuration file */
	while( true ) {
		if( ! fgets( str, LINE_LEN, fd )) break;
		ngt_TrimStr( str );
		line++;

		/* Skip comments and empty lines */
		if( str[0] == ';' || str[0] == '#' || str[0] == '\0' ) continue;

		/* Is this the beginning of a new section? */
		if(( str[0] == '[' ) && ( str[strlen( str ) - 1] == ']' )) {
			strlcpy( section, str, sizeof( section ));
			if( strcasecmp( section, "[GLOBAL]" ) == 0 )
				continue;

			if( strcasecmp( section, "[OPERATOR]" ) == 0 ) {
				if( Conf_Oper_Count + 1 > MAX_OPERATORS )
					Config_Error( LOG_ERR, "Too many operators configured.");
				else {
					/* Initialize new operator structure */
					Conf_Oper[Conf_Oper_Count].name[0] = '\0';
					Conf_Oper[Conf_Oper_Count].pwd[0] = '\0';
					if (Conf_Oper[Conf_Oper_Count].mask) {
						free(Conf_Oper[Conf_Oper_Count].mask );
						Conf_Oper[Conf_Oper_Count].mask = NULL;
					}
					Conf_Oper_Count++;
				}
				continue;
			}
			if( strcasecmp( section, "[SERVER]" ) == 0 ) {
				/* Check if there is already a server to add */
				if( New_Server.name[0] ) {
					/* Copy data to "real" server structure */
					assert( New_Server_Idx > NONE );
					Conf_Server[New_Server_Idx] = New_Server;
				}

				/* Re-init structure for new server */
				Init_Server_Struct( &New_Server );

				/* Search unused item in server configuration structure */
				for( i = 0; i < MAX_SERVERS; i++ ) {
					/* Is this item used? */
					if( ! Conf_Server[i].name[0] ) break;
				}
				if( i >= MAX_SERVERS ) {
					/* Oops, no free item found! */
					Config_Error( LOG_ERR, "Too many servers configured." );
					New_Server_Idx = NONE;
				}
				else New_Server_Idx = i;
				continue;
			}
			if( strcasecmp( section, "[CHANNEL]" ) == 0 ) {
				if( Conf_Channel_Count + 1 > MAX_DEFCHANNELS ) {
					Config_Error( LOG_ERR, "Too many pre-defined channels configured." );
				} else {
					/* Initialize new channel structure */
					strcpy( Conf_Channel[Conf_Channel_Count].name, "" );
					strcpy( Conf_Channel[Conf_Channel_Count].modes, "" );
					strcpy( Conf_Channel[Conf_Channel_Count].key, "" );
					Conf_Channel[Conf_Channel_Count].maxusers = 0;
					array_free(&Conf_Channel[Conf_Channel_Count].topic);
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
		if( ! ptr ) {
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

	/* Close configuration file */
	fclose( fd );

	/* Check if there is still a server to add */
	if( New_Server.name[0] ) {
		/* Copy data to "real" server structure */
		assert( New_Server_Idx > NONE );
		Conf_Server[New_Server_Idx] = New_Server;
	}

	if (0 == array_length(&Conf_ListenPorts, sizeof(UINT16))) {
		if (!array_copyb(&Conf_ListenPorts, (char*) &defaultport, sizeof defaultport)) {
			Config_Error( LOG_ALERT, "Could not add default listening Port %u: %s",
							(unsigned int) defaultport, strerror(errno));
			exit( 1 );
		}
	}
	return true;
} /* Read_Config */


static bool
Check_ArgIsTrue( const char *Arg )
{
	if( strcasecmp( Arg, "yes" ) == 0 ) return true;
	if( strcasecmp( Arg, "true" ) == 0 ) return true;
	if( atoi( Arg ) != 0 ) return true;

	return false;
} /* Check_ArgIsTrue */


static unsigned int Handle_MaxNickLength(int Line, const char *Arg)
{
	unsigned new;

	new = (unsigned) atoi(Arg) + 1;
	if (new > CLIENT_NICK_LEN) {
		Config_Error(LOG_WARNING,
			     "%s, line %d: Value of \"MaxNickLength\" exceeds %u!",
			     NGIRCd_ConfFile, Line, CLIENT_NICK_LEN - 1);
		return CLIENT_NICK_LEN;
	}
	if (new < 2) {
		Config_Error(LOG_WARNING,
			     "%s, line %d: Value of \"MaxNickLength\" must be at least 1!",
			     NGIRCd_ConfFile, Line);
		return 2;
	}
	return new;
} /* Handle_MaxNickLength */


static void
Handle_GLOBAL( int Line, char *Var, char *Arg )
{
	struct passwd *pwd;
	struct group *grp;
	size_t len;
	
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );
	
	if( strcasecmp( Var, "Name" ) == 0 ) {
		/* Server name */
		len = strlcpy( Conf_ServerName, Arg, sizeof( Conf_ServerName ));
		if (len >= sizeof( Conf_ServerName ))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "Info" ) == 0 ) {
		/* Info text of server */
		len = strlcpy( Conf_ServerInfo, Arg, sizeof( Conf_ServerInfo ));
		if (len >= sizeof( Conf_ServerInfo ))
			Config_Error_TooLong ( Line, Var );
		return;
	}
	if( strcasecmp( Var, "Password" ) == 0 ) {
		/* Global server password */
		len = strlcpy( Conf_ServerPwd, Arg, sizeof( Conf_ServerPwd ));
		if (len >= sizeof( Conf_ServerPwd ))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "AdminInfo1" ) == 0 ) {
		/* Administrative info #1 */
		len = strlcpy( Conf_ServerAdmin1, Arg, sizeof( Conf_ServerAdmin1 ));
		if (len >= sizeof( Conf_ServerAdmin1 ))
			Config_Error_TooLong ( Line, Var );
		return;
	}
	if( strcasecmp( Var, "AdminInfo2" ) == 0 ) {
		/* Administrative info #2 */
		len = strlcpy( Conf_ServerAdmin2, Arg, sizeof( Conf_ServerAdmin2 ));
		if (len >= sizeof( Conf_ServerAdmin2 ))
			Config_Error_TooLong ( Line, Var );
		return;
	}
	if( strcasecmp( Var, "AdminEMail" ) == 0 ) {
		/* Administrative email contact */
		len = strlcpy( Conf_ServerAdminMail, Arg, sizeof( Conf_ServerAdminMail ));
		if (len >= sizeof( Conf_ServerAdminMail ))
			Config_Error_TooLong( Line, Var );
		return;
	}

	if( strcasecmp( Var, "Ports" ) == 0 ) {
		ports_parse(&Conf_ListenPorts, Line, Arg);
		return;
	}
	if( strcasecmp( Var, "MotdFile" ) == 0 ) {
		/* "Message of the day" (MOTD) file */
		len = strlcpy( Conf_MotdFile, Arg, sizeof( Conf_MotdFile ));
		if (len >= sizeof( Conf_MotdFile ))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "MotdPhrase" ) == 0 ) {
		/* "Message of the day" phrase (instead of file) */
		len = strlcpy( Conf_MotdPhrase, Arg, sizeof( Conf_MotdPhrase ));
		if (len >= sizeof( Conf_MotdPhrase ))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "ChrootDir" ) == 0 ) {
		/* directory for chroot() */
		len = strlcpy( Conf_Chroot, Arg, sizeof( Conf_Chroot ));
		if (len >= sizeof( Conf_Chroot ))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if ( strcasecmp( Var, "PidFile" ) == 0 ) {
		/* name of pidfile */
		len = strlcpy( Conf_PidFile, Arg, sizeof( Conf_PidFile ));
		if (len >= sizeof( Conf_PidFile ))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "ServerUID" ) == 0 ) {
		/* UID the daemon should switch to */
		pwd = getpwnam( Arg );
		if( pwd ) Conf_UID = pwd->pw_uid;
		else {
#ifdef HAVE_ISDIGIT
			if( ! isdigit( (int)*Arg )) Config_Error_NaN( Line, Var );
			else
#endif
			Conf_UID = (unsigned int)atoi( Arg );
		}
		return;
	}
	if( strcasecmp( Var, "ServerGID" ) == 0 ) {
		/* GID the daemon should use */
		grp = getgrnam( Arg );
		if( grp ) Conf_GID = grp->gr_gid;
		else {
#ifdef HAVE_ISDIGIT
			if( ! isdigit( (int)*Arg )) Config_Error_NaN( Line, Var );
			else
#endif
			Conf_GID = (unsigned int)atoi( Arg );
		}
		return;
	}
	if( strcasecmp( Var, "PingTimeout" ) == 0 ) {
		/* PING timeout */
		Conf_PingTimeout = atoi( Arg );
		if( Conf_PingTimeout < 5 ) {
			Config_Error( LOG_WARNING, "%s, line %d: Value of \"PingTimeout\" too low!",
									NGIRCd_ConfFile, Line );
			Conf_PingTimeout = 5;
		}
		return;
	}
	if( strcasecmp( Var, "PongTimeout" ) == 0 ) {
		/* PONG timeout */
		Conf_PongTimeout = atoi( Arg );
		if( Conf_PongTimeout < 5 ) {
			Config_Error( LOG_WARNING, "%s, line %d: Value of \"PongTimeout\" too low!",
									NGIRCd_ConfFile, Line );
			Conf_PongTimeout = 5;
		}
		return;
	}
	if( strcasecmp( Var, "ConnectRetry" ) == 0 ) {
		/* Seconds between connection attempts to other servers */
		Conf_ConnectRetry = atoi( Arg );
		if( Conf_ConnectRetry < 5 ) {
			Config_Error( LOG_WARNING, "%s, line %d: Value of \"ConnectRetry\" too low!",
									NGIRCd_ConfFile, Line );
			Conf_ConnectRetry = 5;
		}
		return;
	}
	if( strcasecmp( Var, "PredefChannelsOnly" ) == 0 ) {
		/* Should we only allow pre-defined-channels? (i.e. users cannot create their own channels) */
		Conf_PredefChannelsOnly = Check_ArgIsTrue( Arg );
		return;
	}
	if( strcasecmp( Var, "NoDNS" ) == 0 ) {
		/* don't do reverse dns lookups when clients connect? */
		Conf_NoDNS = Check_ArgIsTrue( Arg );
		return;
	}
	if( strcasecmp( Var, "OperCanUseMode" ) == 0 ) {
		/* Are IRC operators allowed to use MODE in channels they aren't Op in? */
		Conf_OperCanMode = Check_ArgIsTrue( Arg );
		return;
	}
	if( strcasecmp( Var, "OperServerMode" ) == 0 ) {
		/* Mask IRC operator as if coming from the server? (ircd-irc2 compat hack) */
		Conf_OperServerMode = Check_ArgIsTrue( Arg );
		return;
	}
	if( strcasecmp( Var, "MaxConnections" ) == 0 ) {
		/* Maximum number of connections. 0 -> "no limit". */
#ifdef HAVE_ISDIGIT
		if( ! isdigit( (int)*Arg )) Config_Error_NaN( Line, Var);
		else
#endif
		Conf_MaxConnections = atol( Arg );
		return;
	}
	if( strcasecmp( Var, "MaxConnectionsIP" ) == 0 ) {
		/* Maximum number of simultaneous connections from one IP. 0 -> "no limit" */
#ifdef HAVE_ISDIGIT
		if( ! isdigit( (int)*Arg )) Config_Error_NaN( Line, Var );
		else
#endif
		Conf_MaxConnectionsIP = atoi( Arg );
		return;
	}
	if( strcasecmp( Var, "MaxJoins" ) == 0 ) {
		/* Maximum number of channels a user can join. 0 -> "no limit". */
#ifdef HAVE_ISDIGIT
		if( ! isdigit( (int)*Arg )) Config_Error_NaN( Line, Var );
		else
#endif
		Conf_MaxJoins = atoi( Arg );
		return;
	}
	if( strcasecmp( Var, "MaxNickLength" ) == 0 ) {
		/* Maximum length of a nick name; must be same on all servers
		 * within the IRC network! */
		Conf_MaxNickLength = Handle_MaxNickLength(Line, Arg);
		return;
	}

	if( strcasecmp( Var, "Listen" ) == 0 ) {
		/* IP-Address to bind sockets */
		len = strlcpy( Conf_ListenAddress, Arg, sizeof( Conf_ListenAddress ));
		if (len >= sizeof( Conf_ListenAddress ))
			Config_Error_TooLong( Line, Var );
		return;
	}

	Config_Error( LOG_ERR, "%s, line %d (section \"Global\"): Unknown variable \"%s\"!",
								NGIRCd_ConfFile, Line, Var );
} /* Handle_GLOBAL */


static void
Handle_OPERATOR( int Line, char *Var, char *Arg )
{
	unsigned int opercount;
	size_t len;
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );
	assert( Conf_Oper_Count > 0 );

	if ( Conf_Oper_Count == 0 )
		return;

	opercount = Conf_Oper_Count - 1;

	if( strcasecmp( Var, "Name" ) == 0 ) {
		/* Name of IRC operator */
		len = strlcpy( Conf_Oper[opercount].name, Arg, sizeof( Conf_Oper[opercount].name ));
		if (len >= sizeof( Conf_Oper[opercount].name ))
				Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "Password" ) == 0 ) {
		/* Password of IRC operator */
		len = strlcpy( Conf_Oper[opercount].pwd, Arg, sizeof( Conf_Oper[opercount].pwd ));
		if (len >= sizeof( Conf_Oper[opercount].pwd ))
				Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "Mask" ) == 0 ) {
		if (Conf_Oper[opercount].mask) return; /* Hostname already configured */

		Conf_Oper[opercount].mask = strdup_warn( Arg );
		return;
	}
	Config_Error( LOG_ERR, "%s, line %d (section \"Operator\"): Unknown variable \"%s\"!",
								NGIRCd_ConfFile, Line, Var );
} /* Handle_OPERATOR */


static void
Handle_SERVER( int Line, char *Var, char *Arg )
{
	long port;
	size_t len;
	
	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );

	/* Ignore server block if no space is left in server configuration structure */
	if( New_Server_Idx <= NONE ) return;

	if( strcasecmp( Var, "Host" ) == 0 ) {
		/* Hostname of the server */
		len = strlcpy( New_Server.host, Arg, sizeof( New_Server.host ));
		if (len >= sizeof( New_Server.host ))
			Config_Error_TooLong ( Line, Var );
		return;
	}
	if( strcasecmp( Var, "Name" ) == 0 ) {
		/* Name of the server ("Nick"/"ID") */
		len = strlcpy( New_Server.name, Arg, sizeof( New_Server.name ));
		if (len >= sizeof( New_Server.name ))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if (strcasecmp(Var, "Bind") == 0) {
		if (ng_ipaddr_init(&New_Server.bind_addr, Arg, 0))
			return;

		Config_Error(LOG_ERR, "%s, line %d (section \"Server\"): Can't parse IP address \"%s\"",
				NGIRCd_ConfFile, Line, Arg);
		return;
	}
	if( strcasecmp( Var, "MyPassword" ) == 0 ) {
		/* Password of this server which is sent to the peer */
		if (*Arg == ':') {
			Config_Error(LOG_ERR,
				"%s, line %d (section \"Server\"): MyPassword must not start with ':'!",
										NGIRCd_ConfFile, Line);
		}
		len = strlcpy( New_Server.pwd_in, Arg, sizeof( New_Server.pwd_in ));
		if (len >= sizeof( New_Server.pwd_in ))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "PeerPassword" ) == 0 ) {
		/* Passwort of the peer which must be received */
		len = strlcpy( New_Server.pwd_out, Arg, sizeof( New_Server.pwd_out ));
		if (len >= sizeof( New_Server.pwd_out ))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "Port" ) == 0 ) {
		/* Port to which this server should connect */
		port = atol( Arg );
		if( port > 0 && port < 0xFFFF )
			New_Server.port = (UINT16)port;
		else
			Config_Error( LOG_ERR, "%s, line %d (section \"Server\"): Illegal port number %ld!",
										NGIRCd_ConfFile, Line, port );
		return;
	}
	if( strcasecmp( Var, "Group" ) == 0 ) {
		/* Server group */
#ifdef HAVE_ISDIGIT
		if( ! isdigit( (int)*Arg ))
			Config_Error_NaN( Line, Var );
		else
#endif
		New_Server.group = atoi( Arg );
		return;
	}
	if( strcasecmp( Var, "Passive" ) == 0 ) {
		if (Check_ArgIsTrue(Arg))
			New_Server.flags |= CONF_SFLAG_DISABLED;
		return;
	}
	
	Config_Error( LOG_ERR, "%s, line %d (section \"Server\"): Unknown variable \"%s\"!",
								NGIRCd_ConfFile, Line, Var );
} /* Handle_SERVER */


static bool
Handle_Channelname(size_t chancount, const char *name)
{
	size_t size = sizeof( Conf_Channel[chancount].name );
	char *dest = Conf_Channel[chancount].name;

	if (*name && *name != '#') {
		*dest = '#';
		--size;
		++dest;
	}
	return size > strlcpy(dest, name, size);
}


static void
Handle_CHANNEL( int Line, char *Var, char *Arg )
{
	size_t len;
	size_t chancount = 0;

	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );
	if (Conf_Channel_Count > 0)
		chancount = Conf_Channel_Count - 1;

	if( strcasecmp( Var, "Name" ) == 0 ) {
		if (!Handle_Channelname(chancount, Arg))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "Modes" ) == 0 ) {
		/* Initial modes */
		len = strlcpy( Conf_Channel[chancount].modes, Arg, sizeof( Conf_Channel[chancount].modes ));
		if (len >= sizeof( Conf_Channel[chancount].modes ))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "Topic" ) == 0 ) {
		/* Initial topic */
		if (!array_copys( &Conf_Channel[chancount].topic, Arg))
			Config_Error_TooLong( Line, Var );
		return;
	}

	if( strcasecmp( Var, "Key" ) == 0 ) {
		/* Initial Channel Key (mode k) */
		len = strlcpy(Conf_Channel[chancount].key, Arg, sizeof(Conf_Channel[chancount].key));
		if (len >= sizeof( Conf_Channel[chancount].key ))
			Config_Error_TooLong(Line, Var);
		return;
	}

	if( strcasecmp( Var, "MaxUsers" ) == 0 ) {
		/* maximum user limit, mode l */
		Conf_Channel[chancount].maxusers = (unsigned long) atol(Arg);
		if (Conf_Channel[chancount].maxusers == 0)
			Config_Error_NaN(Line, Var);
		return;
	}

	Config_Error( LOG_ERR, "%s, line %d (section \"Channel\"): Unknown variable \"%s\"!",
								NGIRCd_ConfFile, Line, Var );
} /* Handle_CHANNEL */


static void
Validate_Config(bool Configtest, bool Rehash)
{
	/* Validate configuration settings. */

#ifdef DEBUG
	int i, servers, servers_once;
#endif
	char *ptr;

	/* Validate configured server name, see RFC 2812 section 2.3.1 */
	ptr = Conf_ServerName;
	do {
		if (*ptr >= 'a' && *ptr <= 'z') continue;
		if (*ptr >= 'A' && *ptr <= 'Z') continue;
		if (*ptr >= '0' && *ptr <= '9') continue;
		if (ptr > Conf_ServerName) {
			if (*ptr == '.' || *ptr == '-')
				continue;
		}
		Conf_ServerName[0] = '\0';
		break;
	} while (*(++ptr));

	if (!Conf_ServerName[0]) {
		/* No server name configured! */
		Config_Error(LOG_ALERT,
			     "No (valid) server name configured in \"%s\" (section 'Global': 'Name')!",
			     NGIRCd_ConfFile);
		if (!Configtest && !Rehash) {
			Config_Error(LOG_ALERT,
				     "%s exiting due to fatal errors!",
				     PACKAGE_NAME);
			exit(1);
		}
	}

	if (Conf_ServerName[0] && !strchr(Conf_ServerName, '.')) {
		/* No dot in server name! */
		Config_Error(LOG_ALERT,
			     "Invalid server name configured in \"%s\" (section 'Global': 'Name'): Dot missing!",
			     NGIRCd_ConfFile);
		if (!Configtest) {
			Config_Error(LOG_ALERT,
				     "%s exiting due to fatal errors!",
				     PACKAGE_NAME);
			exit(1);
		}
	}

#ifdef STRICT_RFC
	if (!Conf_ServerAdminMail[0]) {
		/* No administrative contact configured! */
		Config_Error(LOG_ALERT,
			     "No administrator email address configured in \"%s\" ('AdminEMail')!",
			     NGIRCd_ConfFile);
		if (!Configtest) {
			Config_Error(LOG_ALERT,
				     "%s exiting due to fatal errors!",
				     PACKAGE_NAME);
			exit(1);
		}
	}
#endif

	if (!Conf_ServerAdmin1[0] && !Conf_ServerAdmin2[0]
	    && !Conf_ServerAdminMail[0]) {
		/* No administrative information configured! */
		Config_Error(LOG_WARNING,
			     "No administrative information configured but required by RFC!");
	}

#ifdef DEBUG
	servers = servers_once = 0;
	for (i = 0; i < MAX_SERVERS; i++) {
		if (Conf_Server[i].name[0]) {
			servers++;
			if (Conf_Server[i].flags & CONF_SFLAG_ONCE)
				servers_once++;
		}
	}
	Log(LOG_DEBUG,
	    "Configuration: Operators=%d, Servers=%d[%d], Channels=%d",
	    Conf_Oper_Count, servers, servers_once, Conf_Channel_Count);
#endif
} /* Validate_Config */


static void
Config_Error_TooLong ( const int Line, const char *Item )
{
	Config_Error( LOG_WARNING, "%s, line %d: Value of \"%s\" too long!", NGIRCd_ConfFile, Line, Item );
}


static void
Config_Error_NaN( const int Line, const char *Item )
{
	Config_Error( LOG_WARNING, "%s, line %d: Value of \"%s\" is not a number!",
						NGIRCd_ConfFile, Line, Item );
}


#ifdef PROTOTYPES
static void Config_Error( const int Level, const char *Format, ... )
#else
static void Config_Error( Level, Format, va_alist )
const int Level;
const char *Format;
va_dcl
#endif
{
	/* Error! Write to console and/or logfile. */

	char msg[MAX_LOG_MSG_LEN];
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
	if (Use_Log) Log( Level, "%s", msg );
	else puts( msg );
} /* Config_Error */


static void
Init_Server_Struct( CONF_SERVER *Server )
{
	/* Initialize server configuration structur to default values */

	assert( Server != NULL );

	memset( Server, 0, sizeof (CONF_SERVER) );

	Server->group = NONE;
	Server->lasttry = time( NULL ) - Conf_ConnectRetry + STARTUP_DELAY;

	if( NGIRCd_Passive ) Server->flags = CONF_SFLAG_DISABLED;

	Resolve_Init(&Server->res_stat);
	Server->conn_id = NONE;
	memset(&Server->bind_addr, 0, sizeof(&Server->bind_addr));
} /* Init_Server_Struct */


/* -eof- */
