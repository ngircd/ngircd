/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2012 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#include "portab.h"

/**
 * @file
 * Configuration management (reading, parsing & validation)
 */

#include "imp.h"
#include <assert.h>
#include <ctype.h>
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


#include "array.h"
#include "ngircd.h"
#include "conn.h"
#include "channel.h"
#include "defines.h"
#include "log.h"
#include "match.h"
#include "tool.h"

#include "exp.h"
#include "conf.h"


static bool Use_Log = true, Using_MotdFile = true;
static CONF_SERVER New_Server;
static int New_Server_Idx;

static char Conf_MotdFile[FNAME_LEN];

static void Set_Defaults PARAMS(( bool InitServers ));
static bool Read_Config PARAMS(( bool TestOnly, bool IsStarting ));
static bool Validate_Config PARAMS(( bool TestOnly, bool Rehash ));

static void Handle_GLOBAL PARAMS(( int Line, char *Var, char *Arg ));
static void Handle_LIMITS PARAMS(( int Line, char *Var, char *Arg ));
static void Handle_OPTIONS PARAMS(( int Line, char *Var, char *Arg ));
static void Handle_OPERATOR PARAMS(( int Line, char *Var, char *Arg ));
static void Handle_SERVER PARAMS(( int Line, char *Var, char *Arg ));
static void Handle_CHANNEL PARAMS(( int Line, char *Var, char *Arg ));

static void Config_Error PARAMS(( const int Level, const char *Format, ... ));

static void Config_Error_NaN PARAMS(( const int LINE, const char *Value ));
static void Config_Error_Section PARAMS(( const int Line, const char *Item,
					  const char *Section ));
static void Config_Error_TooLong PARAMS(( const int LINE, const char *Value ));

static void Init_Server_Struct PARAMS(( CONF_SERVER *Server ));


#ifdef WANT_IPV6
#define DEFAULT_LISTEN_ADDRSTR "::,0.0.0.0"
#else
#define DEFAULT_LISTEN_ADDRSTR "0.0.0.0"
#endif


#ifdef SSL_SUPPORT

static void Handle_SSL PARAMS(( int Line, char *Var, char *Ark ));

struct SSLOptions Conf_SSLOptions;

/**
 * Initialize SSL configuration.
 */
static void
ConfSSL_Init(void)
{
	free(Conf_SSLOptions.KeyFile);
	Conf_SSLOptions.KeyFile = NULL;

	free(Conf_SSLOptions.CertFile);
	Conf_SSLOptions.CertFile = NULL;

	free(Conf_SSLOptions.DHFile);
	Conf_SSLOptions.DHFile = NULL;
	array_free_wipe(&Conf_SSLOptions.KeyFilePassword);

	array_free(&Conf_SSLOptions.ListenPorts);
}

/**
 * Make sure that a configured file is readable.
 *
 * Currently, this function is only used for SSL-related options ...
 *
 * @param Var Configuration variable
 * @param Filename Configured filename
 */
static void
CheckFileReadable(const char *Var, const char *Filename)
{
	FILE *fp;

	if (!Filename)
		return;

	fp = fopen(Filename, "r");
	if (fp)
		fclose(fp);
	else
		Config_Error(LOG_ERR, "Can't read \"%s\" (\"%s\"): %s",
			     Filename, Var, strerror(errno));
}

#endif


/**
 * Duplicate string and warn on errors.
 *
 * @returns Pointer to string on success, NULL otherwise.
 */
static char *
strdup_warn(const char *str)
{
	char *ptr = strdup(str);
	if (!ptr)
		Config_Error(LOG_ERR,
			     "Could not allocate memory for string: %s", str);
	return ptr;
}

/**
 * Output a comma separated list of ports (integer values).
 */
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

/**
 * Parse a comma separated string into an array of port numbers (integers).
 */
static void
ports_parse(array *a, int Line, char *Arg)
{
	char *ptr;
	int port;
	UINT16 port16;

	array_trunc(a);

	ptr = strtok( Arg, "," );
	while (ptr) {
		ngt_TrimStr(ptr);
		port = atoi(ptr);
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

/**
 * Initialize configuration module.
 */
GLOBAL void
Conf_Init( void )
{
	Read_Config(false, true);
	Validate_Config(false, false);
}

/**
 * "Rehash" (reload) server configuration.
 *
 * @returns true if configuration has been re-read, false on errors.
 */
GLOBAL bool
Conf_Rehash( void )
{
	if (!Read_Config(false, false))
		return false;
	Validate_Config(false, true);

	/* Update CLIENT structure of local server */
	Client_SetInfo(Client_ThisServer(), Conf_ServerInfo);
	return true;
}

/**
 * Output a boolean value as "yes/no" string.
 */
static const char*
yesno_to_str(int boolean_value)
{
	if (boolean_value)
		return "yes";
	return "no";
}

/**
 * Free all IRC operator configuration structures.
 */
static void
opers_free(void)
{
	struct Conf_Oper *op;
	size_t len;

	len = array_length(&Conf_Opers, sizeof(*op));
	op = array_start(&Conf_Opers);
	while (len--) {
		free(op->mask);
		op++;
	}
	array_free(&Conf_Opers);
}

/**
 * Output all IRC operator configuration structures.
 */
static void
opers_puts(void)
{
	struct Conf_Oper *op;
	size_t count, i;

	count = array_length(&Conf_Opers, sizeof(*op));
	op = array_start(&Conf_Opers);
	for (i = 0; i < count; i++, op++) {
		if (!op->name[0])
			continue;

		puts("[OPERATOR]");
		printf("  Name = %s\n", op->name);
		printf("  Password = %s\n", op->pwd);
		printf("  Mask = %s\n\n", op->mask ? op->mask : "");
	}
}

/**
 * Read configuration, validate and output it.
 *
 * This function waits for a keypress of the user when stdin/stdout are valid
 * tty's ("you can read our nice message and we can read in your keypress").
 *
 * @return	0 on succes, 1 on failure(s); therefore the result code can
 * 		directly be used by exit() when running "ngircd --configtest".
 */
GLOBAL int
Conf_Test( void )
{
	struct passwd *pwd;
	struct group *grp;
	unsigned int i;
	bool config_valid;
	size_t predef_channel_count;
	struct Conf_Channel *predef_chan;

	Use_Log = false;

	if (!Read_Config(true, true))
		return 1;

	config_valid = Validate_Config(true, false);

	/* Valid tty? */
	if(isatty(fileno(stdin)) && isatty(fileno(stdout))) {
		puts("OK, press enter to see a dump of your server configuration ...");
		getchar();
	} else
		puts("Ok, dump of your server configuration follows:\n");

	puts("[GLOBAL]");
	printf("  Name = %s\n", Conf_ServerName);
	printf("  AdminInfo1 = %s\n", Conf_ServerAdmin1);
	printf("  AdminInfo2 = %s\n", Conf_ServerAdmin2);
	printf("  AdminEMail = %s\n", Conf_ServerAdminMail);
	printf("  Info = %s\n", Conf_ServerInfo);
	printf("  Listen = %s\n", Conf_ListenAddress);
	if (Using_MotdFile) {
		printf("  MotdFile = %s\n", Conf_MotdFile);
		printf("  MotdPhrase =\n");
	} else {
		printf("  MotdFile = \n");
		printf("  MotdPhrase = %s\n", array_bytes(&Conf_Motd)
		       ? (const char*) array_start(&Conf_Motd) : "");
	}
#ifndef PAM
	printf("  Password = %s\n", Conf_ServerPwd);
#endif
	printf("  PidFile = %s\n", Conf_PidFile);
	printf("  Ports = ");
	ports_puts(&Conf_ListenPorts);
	grp = getgrgid(Conf_GID);
	if (grp)
		printf("  ServerGID = %s\n", grp->gr_name);
	else
		printf("  ServerGID = %ld\n", (long)Conf_GID);
	pwd = getpwuid(Conf_UID);
	if (pwd)
		printf("  ServerUID = %s\n", pwd->pw_name);
	else
		printf("  ServerUID = %ld\n", (long)Conf_UID);
	puts("");

	puts("[LIMITS]");
	printf("  ConnectRetry = %d\n", Conf_ConnectRetry);
	printf("  MaxConnections = %d\n", Conf_MaxConnections);
	printf("  MaxConnectionsIP = %d\n", Conf_MaxConnectionsIP);
	printf("  MaxJoins = %d\n", Conf_MaxJoins > 0 ? Conf_MaxJoins : -1);
	printf("  MaxNickLength = %u\n", Conf_MaxNickLength - 1);
	printf("  PingTimeout = %d\n", Conf_PingTimeout);
	printf("  PongTimeout = %d\n", Conf_PongTimeout);
	puts("");

	puts("[OPTIONS]");
	printf("  AllowRemoteOper = %s\n", yesno_to_str(Conf_AllowRemoteOper));
	printf("  ChrootDir = %s\n", Conf_Chroot);
	printf("  CloakHost = %s\n", Conf_CloakHost);
	printf("  CloakHostModeX = %s\n", Conf_CloakHostModeX);
	printf("  CloakHostSalt = %s\n", Conf_CloakHostSalt);
	printf("  CloakUserToNick = %s\n", yesno_to_str(Conf_CloakUserToNick));
#ifdef WANT_IPV6
	printf("  ConnectIPv4 = %s\n", yesno_to_str(Conf_ConnectIPv6));
	printf("  ConnectIPv6 = %s\n", yesno_to_str(Conf_ConnectIPv4));
#endif
	printf("  DNS = %s\n", yesno_to_str(Conf_DNS));
#ifdef IDENT
	printf("  Ident = %s\n", yesno_to_str(Conf_Ident));
#endif
	printf("  MorePrivacy = %s\n", yesno_to_str(Conf_MorePrivacy));
	printf("  NoticeAuth = %s\n", yesno_to_str(Conf_NoticeAuth));
	printf("  OperCanUseMode = %s\n", yesno_to_str(Conf_OperCanMode));
	printf("  OperChanPAutoOp = %s\n", yesno_to_str(Conf_OperChanPAutoOp));
	printf("  OperServerMode = %s\n", yesno_to_str(Conf_OperServerMode));
#ifdef PAM
	printf("  PAM = %s\n", yesno_to_str(Conf_PAM));
	printf("  PAMIsOptional = %s\n", yesno_to_str(Conf_PAMIsOptional));
#endif
	printf("  PredefChannelsOnly = %s\n", yesno_to_str(Conf_PredefChannelsOnly));
#ifndef STRICT_RFC
	printf("  RequireAuthPing = %s\n", yesno_to_str(Conf_AuthPing));
#endif
	printf("  ScrubCTCP = %s\n", yesno_to_str(Conf_ScrubCTCP));
#ifdef SYSLOG
	printf("  SyslogFacility = %s\n",
	       ngt_SyslogFacilityName(Conf_SyslogFacility));
#endif
	printf("  WebircPassword = %s\n", Conf_WebircPwd);
	puts("");

#ifdef SSL_SUPPORT
	puts("[SSL]");
	printf("  CertFile = %s\n", Conf_SSLOptions.CertFile
					? Conf_SSLOptions.CertFile : "");
	printf("  DHFile = %s\n", Conf_SSLOptions.DHFile
					? Conf_SSLOptions.DHFile : "");
	printf("  KeyFile = %s\n", Conf_SSLOptions.KeyFile
					? Conf_SSLOptions.KeyFile : "");
	if (array_bytes(&Conf_SSLOptions.KeyFilePassword))
		puts("  KeyFilePassword = <secret>");
	else
		puts("  KeyFilePassword = ");
	array_free_wipe(&Conf_SSLOptions.KeyFilePassword);
	printf("  Ports = ");
	ports_puts(&Conf_SSLOptions.ListenPorts);
	puts("");
#endif

	opers_puts();

	for( i = 0; i < MAX_SERVERS; i++ ) {
		if( ! Conf_Server[i].name[0] ) continue;

		/* Valid "Server" section */
		puts( "[SERVER]" );
		printf( "  Name = %s\n", Conf_Server[i].name );
		printf( "  Host = %s\n", Conf_Server[i].host );
		printf( "  Port = %u\n", (unsigned int)Conf_Server[i].port );
#ifdef SSL_SUPPORT
		printf( "  SSLConnect = %s\n", Conf_Server[i].SSLConnect?"yes":"no");
#endif
		printf( "  MyPassword = %s\n", Conf_Server[i].pwd_in );
		printf( "  PeerPassword = %s\n", Conf_Server[i].pwd_out );
		printf( "  ServiceMask = %s\n", Conf_Server[i].svs_mask);
		printf( "  Group = %d\n", Conf_Server[i].group );
		printf( "  Passive = %s\n\n", Conf_Server[i].flags & CONF_SFLAG_DISABLED ? "yes" : "no");
	}

	predef_channel_count = array_length(&Conf_Channels, sizeof(*predef_chan));
	predef_chan = array_start(&Conf_Channels);

	for (i = 0; i < predef_channel_count; i++, predef_chan++) {
		if (!predef_chan->name[0])
			continue;

		/* Valid "Channel" section */
		puts( "[CHANNEL]" );
		printf("  Name = %s\n", predef_chan->name);
		printf("  Modes = %s\n", predef_chan->modes);
		printf("  Key = %s\n", predef_chan->key);
		printf("  MaxUsers = %lu\n", predef_chan->maxusers);
		printf("  Topic = %s\n", predef_chan->topic);
		printf("  KeyFile = %s\n\n", predef_chan->keyfile);
	}

	return (config_valid ? 0 : 1);
}

/**
 * Remove connection information from configured server.
 *
 * If the server is set as "once", delete it from our configuration;
 * otherwise set the time for the next connection attempt.
 *
 * Non-server connections will be silently ignored.
 */
GLOBAL void
Conf_UnsetServer( CONN_ID Idx )
{
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
}

/**
 * Set connection information for specified configured server.
 */
GLOBAL void
Conf_SetServer( int ConfServer, CONN_ID Idx )
{
	assert( ConfServer > NONE );
	assert( Idx > NONE );

	if (Conf_Server[ConfServer].conn_id > NONE &&
	    Conf_Server[ConfServer].conn_id != Idx) {
		Log(LOG_ALERT,
			"Trying to update connection index for already registered server \"%s\": %d/%d - ignored.",
			Conf_Server[ConfServer].name,
			Conf_Server[ConfServer].conn_id, Idx);
		return;
	}
	Conf_Server[ConfServer].conn_id = Idx;
}

/**
 * Get index of server in configuration structure.
 */
GLOBAL int
Conf_GetServer( CONN_ID Idx )
{
	int i = 0;

	assert( Idx > NONE );

	for( i = 0; i < MAX_SERVERS; i++ ) {
		if( Conf_Server[i].conn_id == Idx ) return i;
	}
	return NONE;
}

/**
 * Enable a server by name and adjust its port number.
 *
 * @returns	true if a server has been enabled and now has a valid port
 *		number and host name for outgoing connections.
 */
GLOBAL bool
Conf_EnableServer( const char *Name, UINT16 Port )
{
	int i;

	assert( Name != NULL );
	for( i = 0; i < MAX_SERVERS; i++ ) {
		if( strcasecmp( Conf_Server[i].name, Name ) == 0 ) {
			/* Gotcha! Set port and enable server: */
			Conf_Server[i].port = Port;
			Conf_Server[i].flags &= ~CONF_SFLAG_DISABLED;
			return (Conf_Server[i].port && Conf_Server[i].host[0]);
		}
	}
	return false;
}

/**
 * Enable a server by name.
 *
 * The server is only usable as outgoing server, if it has set a valid port
 * number for outgoing connections!
 * If not, you have to use Conf_EnableServer() function to make it available.
 *
 * @returns	true if a server has been enabled; false otherwise.
 */
GLOBAL bool
Conf_EnablePassiveServer(const char *Name)
{
	int i;

	assert( Name != NULL );
	for (i = 0; i < MAX_SERVERS; i++) {
		if ((strcasecmp( Conf_Server[i].name, Name ) == 0)
		    && (Conf_Server[i].port > 0)) {
			/* BINGO! Enable server */
			Conf_Server[i].flags &= ~CONF_SFLAG_DISABLED;
			return true;
		}
	}
	return false;
}

/**
 * Disable a server by name.
 * An already established connection will be disconnected.
 *
 * @returns	true if a server was found and has been disabled.
 */
GLOBAL bool
Conf_DisableServer( const char *Name )
{
	int i;

	assert( Name != NULL );
	for( i = 0; i < MAX_SERVERS; i++ ) {
		if( strcasecmp( Conf_Server[i].name, Name ) == 0 ) {
			/* Gotcha! Disable and disconnect server: */
			Conf_Server[i].flags |= CONF_SFLAG_DISABLED;
			if( Conf_Server[i].conn_id > NONE )
				Conn_Close(Conf_Server[i].conn_id, NULL,
					   "Server link terminated on operator request",
					   true);
			return true;
		}
	}
	return false;
}

/**
 * Add a new remote server to our configuration.
 *
 * @param Name		Name of the new server.
 * @param Port		Port number to connect to or 0 for incoming connections.
 * @param Host		Host name to connect to.
 * @param MyPwd		Password that will be sent to the peer.
 * @param PeerPwd	Password that must be received from the peer.
 * @returns		true if the new server has been added; false otherwise.
 */
GLOBAL bool
Conf_AddServer(const char *Name, UINT16 Port, const char *Host,
	       const char *MyPwd, const char *PeerPwd)
{
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
}

/**
 * Check if the given nick name is reserved for services on a particular server.
 *
 * @param ConfServer The server index to check.
 * @param Nick The nick name to check.
 * @returns true if the given nick name belongs to an "IRC service".
 */
GLOBAL bool
Conf_NickIsService(int ConfServer, const char *Nick)
{
	assert (ConfServer >= 0);
	assert (ConfServer < MAX_SERVERS);

	return MatchCaseInsensitiveList(Conf_Server[ConfServer].svs_mask,
					Nick, ",");
}

/**
 * Check if the given nick name is blocked for "normal client" use.
 *
 * @param ConfServer The server index or NONE to check all configured servers.
 * @param Nick The nick name to check.
 * @returns true if the given nick name belongs to an "IRC service".
 */
GLOBAL bool
Conf_NickIsBlocked(const char *Nick)
{
	int i;

	for(i = 0; i < MAX_SERVERS; i++) {
		if (!Conf_Server[i].name[0])
			continue;
		if (Conf_NickIsService(i, Nick))
			return true;
	}
	return false;
}

/**
 * Initialize configuration settings with their default values.
 */
static void
Set_Defaults(bool InitServers)
{
	int i;
	char random[RANDOM_SALT_LEN + 1];

	/* Global */
	strcpy(Conf_ServerName, "");
	strcpy(Conf_ServerAdmin1, "");
	strcpy(Conf_ServerAdmin2, "");
	strcpy(Conf_ServerAdminMail, "");
	snprintf(Conf_ServerInfo, sizeof Conf_ServerInfo, "%s %s",
		 PACKAGE_NAME, PACKAGE_VERSION);
	free(Conf_ListenAddress);
	Conf_ListenAddress = NULL;
	array_free(&Conf_ListenPorts);
	array_free(&Conf_Motd);
	strlcpy(Conf_MotdFile, SYSCONFDIR, sizeof(Conf_MotdFile));
	strlcat(Conf_MotdFile, MOTD_FILE, sizeof(Conf_MotdFile));
	strcpy(Conf_ServerPwd, "");
	strlcpy(Conf_PidFile, PID_FILE, sizeof(Conf_PidFile));
	Conf_UID = Conf_GID = 0;

	/* Limits */
	Conf_ConnectRetry = 60;
	Conf_MaxConnections = 0;
	Conf_MaxConnectionsIP = 5;
	Conf_MaxJoins = 10;
	Conf_MaxNickLength = CLIENT_NICK_LEN_DEFAULT;
	Conf_PingTimeout = 120;
	Conf_PongTimeout = 20;

	/* Options */
	Conf_AllowRemoteOper = false;
#ifndef STRICT_RFC
	Conf_AuthPing = false;
#endif
	strlcpy(Conf_Chroot, CHROOT_DIR, sizeof(Conf_Chroot));
	strcpy(Conf_CloakHost, "");
	strcpy(Conf_CloakHostModeX, "");
	strcpy(Conf_CloakHostSalt, ngt_RandomStr(random, RANDOM_SALT_LEN));
	Conf_CloakUserToNick = false;
	Conf_ConnectIPv4 = true;
#ifdef WANT_IPV6
	Conf_ConnectIPv6 = true;
#else
	Conf_ConnectIPv6 = false;
#endif
	Conf_DNS = true;
#ifdef IDENTAUTH
	Conf_Ident = true;
#else
	Conf_Ident = false;
#endif
	Conf_MorePrivacy = false;
	Conf_NoticeAuth = false;
	Conf_OperCanMode = false;
	Conf_OperChanPAutoOp = true;
	Conf_OperServerMode = false;
#ifdef PAM
	Conf_PAM = true;
#else
	Conf_PAM = false;
#endif
	Conf_PAMIsOptional = false;
	Conf_PredefChannelsOnly = false;
#ifdef SYSLOG
	Conf_ScrubCTCP = false;
#ifdef LOG_LOCAL5
	Conf_SyslogFacility = LOG_LOCAL5;
#else
	Conf_SyslogFacility = 0;
#endif
#endif

	/* Initialize server configuration structures */
	if (InitServers) {
		for (i = 0; i < MAX_SERVERS;
		     Init_Server_Struct(&Conf_Server[i++]));
	}
}

/**
 * Get number of configured listening ports.
 *
 * @returns The number of ports (IPv4+IPv6) on which the server should listen.
 */
static bool
no_listenports(void)
{
	size_t cnt = array_bytes(&Conf_ListenPorts);
#ifdef SSL_SUPPORT
	cnt += array_bytes(&Conf_SSLOptions.ListenPorts);
#endif
	return cnt == 0;
}

/**
 * Read MOTD ("message of the day") file.
 *
 * @param filename	Name of the file to read.
 */
static void
Read_Motd(const char *filename)
{
	char line[127];
	FILE *fp;

	if (*filename == '\0')
		return;

	fp = fopen(filename, "r");
	if (!fp) {
		Config_Error(LOG_WARNING, "Can't read MOTD file \"%s\": %s",
					filename, strerror(errno));
		return;
	}

	array_free(&Conf_Motd);
	Using_MotdFile = true;

	while (fgets(line, (int)sizeof line, fp)) {
		ngt_TrimLastChr( line, '\n');

		/* add text including \0 */
		if (!array_catb(&Conf_Motd, line, strlen(line) + 1)) {
			Log(LOG_WARNING, "Cannot add MOTD text: %s", strerror(errno));
			break;
		}
	}
	fclose(fp);
}

/**
 * Read ngIRCd configuration file.
 *
 * Please note that this function uses exit(1) on fatal errors and therefore
 * can result in ngIRCd terminating!
 *
 * @param ngircd_starting	Flag indicating if ngIRCd is starting or not.
 * @returns			true when the configuration file has been read
 *				successfully; false otherwise.
 */
static bool
Read_Config(bool TestOnly, bool IsStarting)
{
	char section[LINE_LEN], str[LINE_LEN], *var, *arg, *ptr;
	const UINT16 defaultport = 6667;
	int line, i, n;
	size_t count;
	FILE *fd;

	/* Open configuration file */
	fd = fopen( NGIRCd_ConfFile, "r" );
	if( ! fd ) {
		/* No configuration file found! */
		Config_Error( LOG_ALERT, "Can't read configuration \"%s\": %s",
					NGIRCd_ConfFile, strerror( errno ));
		if (!IsStarting)
			return false;
		Config_Error( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE_NAME );
		exit( 1 );
	}

	opers_free();
	Set_Defaults(IsStarting);

	if (TestOnly)
		Config_Error(LOG_INFO,
			     "Reading configuration from \"%s\" ...",
			     NGIRCd_ConfFile );

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
#ifdef SSL_SUPPORT
	ConfSSL_Init();
#endif
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
			if (strcasecmp(section, "[GLOBAL]") == 0
			    || strcasecmp(section, "[LIMITS]") == 0
			    || strcasecmp(section, "[OPTIONS]") == 0
#ifdef SSL_SUPPORT
			    || strcasecmp(section, "[SSL]") == 0
#endif
			    )
				continue;

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

			if (strcasecmp(section, "[CHANNEL]") == 0) {
				count = array_length(&Conf_Channels,
						     sizeof(struct Conf_Channel));
				if (!array_alloc(&Conf_Channels,
						 sizeof(struct Conf_Channel),
						 count)) {
					Config_Error(LOG_ERR,
						     "Could not allocate memory for new operator (line %d)",
						     line);
				}
				continue;
			}

			if (strcasecmp(section, "[OPERATOR]") == 0) {
				count = array_length(&Conf_Opers,
						     sizeof(struct Conf_Oper));
				if (!array_alloc(&Conf_Opers,
						 sizeof(struct Conf_Oper),
						 count)) {
					Config_Error(LOG_ERR,
						     "Could not allocate memory for new channel (line &d)",
						     line);
				}
				continue;
			}

			Config_Error(LOG_ERR,
				     "%s, line %d: Unknown section \"%s\"!",
				     NGIRCd_ConfFile, line, section);
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

		if(strcasecmp(section, "[GLOBAL]") == 0)
			Handle_GLOBAL(line, var, arg);
		else if(strcasecmp(section, "[LIMITS]") == 0)
			Handle_LIMITS(line, var, arg);
		else if(strcasecmp(section, "[OPTIONS]") == 0)
			Handle_OPTIONS(line, var, arg);
#ifdef SSL_SUPPORT
		else if(strcasecmp(section, "[SSL]") == 0)
			Handle_SSL(line, var, arg);
#endif
		else if(strcasecmp(section, "[OPERATOR]") == 0)
			Handle_OPERATOR(line, var, arg);
		else if(strcasecmp(section, "[SERVER]") == 0)
			Handle_SERVER(line, var, arg);
		else if(strcasecmp(section, "[CHANNEL]") == 0)
			Handle_CHANNEL(line, var, arg);
		else
			Config_Error(LOG_ERR,
				     "%s, line %d: Variable \"%s\" outside section!",
				     NGIRCd_ConfFile, line, var);
	}

	/* Close configuration file */
	fclose( fd );

	/* Check if there is still a server to add */
	if( New_Server.name[0] ) {
		/* Copy data to "real" server structure */
		assert( New_Server_Idx > NONE );
		Conf_Server[New_Server_Idx] = New_Server;
	}

	/* not a single listening port? Add default. */
	if (no_listenports() &&
		!array_copyb(&Conf_ListenPorts, (char*) &defaultport, sizeof defaultport))
	{
		Config_Error(LOG_ALERT, "Could not add default listening Port %u: %s",
					(unsigned int) defaultport, strerror(errno));

		exit(1);
	}

	if (!Conf_ListenAddress)
		Conf_ListenAddress = strdup_warn(DEFAULT_LISTEN_ADDRSTR);

	if (!Conf_ListenAddress) {
		Config_Error(LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE_NAME);
		exit(1);
	}

	/* No MOTD phrase configured? (re)try motd file. */
	if (array_bytes(&Conf_Motd) == 0)
		Read_Motd(Conf_MotdFile);

#ifdef SSL_SUPPORT
	/* Make sure that all SSL-related files are readable */
	CheckFileReadable("CertFile", Conf_SSLOptions.CertFile);
	CheckFileReadable("DHFile", Conf_SSLOptions.DHFile);
	CheckFileReadable("KeyFile", Conf_SSLOptions.KeyFile);
#endif

	return true;
}

/**
 * Check whether a string argument is "true" or "false".
 *
 * @param Arg	Input string.
 * @returns	true if the input string has been parsed as "yes", "true"
 *		(case insensitive) or a non-zero integer value.
 */
static bool
Check_ArgIsTrue(const char *Arg)
{
	if (strcasecmp(Arg, "yes") == 0)
		return true;
	if (strcasecmp(Arg, "true") == 0)
		return true;
	if (atoi(Arg) != 0)
		return true;

	return false;
}

/**
 * Handle setting of "MaxNickLength".
 *
 * @param Line	Line number in configuration file.
 * @raram Arg	Input string.
 * @returns	New configured maximum nick name length.
 */
static unsigned int
Handle_MaxNickLength(int Line, const char *Arg)
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
}

/**
 * Output a warning messages if IDENT is configured but not compiled in.
 */
static void
WarnIdent(int UNUSED Line)
{
#ifndef IDENTAUTH
	if (Conf_Ident) {
		/* user has enabled ident lookups explicitly, but ... */
		Config_Error(LOG_WARNING,
			"%s: line %d: \"Ident = yes\", but ngircd was built without IDENT support!",
			NGIRCd_ConfFile, Line);
	}
#endif
}

/**
 * Output a warning messages if IPv6 is configured but not compiled in.
 */
static void
WarnIPv6(int UNUSED Line)
{
#ifndef WANT_IPV6
	if (Conf_ConnectIPv6) {
		/* user has enabled IPv6 explicitly, but ... */
		Config_Error(LOG_WARNING,
			"%s: line %d: \"ConnectIPv6 = yes\", but ngircd was built without IPv6 support!",
			NGIRCd_ConfFile, Line);
	}
#endif
}

/**
 * Output a warning messages if PAM is configured but not compiled in.
 */
static void
WarnPAM(int UNUSED Line)
{
#ifndef PAM
	if (Conf_PAM) {
		Config_Error(LOG_WARNING,
			"%s: line %d: \"PAM = yes\", but ngircd was built without PAM support!",
			NGIRCd_ConfFile, Line);
	}
#endif
}

/**
 * Handle legacy "NoXXX" options in [GLOBAL] section.
 *
 * TODO: This function and support for "NoXXX" could be removed starting
 * with ngIRCd release 19 (one release after marking it "deprecated").
 *
 * @param Var	Variable name.
 * @param Arg	Argument string.
 * @returns	true if a NoXXX option has been processed; false otherwise.
 */
static bool
CheckLegacyNoOption(const char *Var, const char *Arg)
{
	if(strcasecmp(Var, "NoDNS") == 0) {
		Conf_DNS = !Check_ArgIsTrue( Arg );
		return true;
	}
	if (strcasecmp(Var, "NoIdent") == 0) {
		Conf_Ident = !Check_ArgIsTrue(Arg);
		return true;
	}
	if(strcasecmp(Var, "NoPAM") == 0) {
		Conf_PAM = !Check_ArgIsTrue(Arg);
		return true;
	}
	return false;
}

/**
 * Handle deprecated legacy options in [GLOBAL] section.
 *
 * TODO: This function and support for these options in the [Global] section
 * could be removed starting with ngIRCd release 19 (one release after
 * marking it "deprecated").
 *
 * @param Var	Variable name.
 * @param Arg	Argument string.
 * @returns	true if a legacy option has been processed; false otherwise.
 */
static const char*
CheckLegacyGlobalOption(int Line, char *Var, char *Arg)
{
	if (strcasecmp(Var, "AllowRemoteOper") == 0
	    || strcasecmp(Var, "ChrootDir") == 0
	    || strcasecmp(Var, "ConnectIPv4") == 0
	    || strcasecmp(Var, "ConnectIPv6") == 0
	    || strcasecmp(Var, "OperCanUseMode") == 0
	    || strcasecmp(Var, "OperChanPAutoOp") == 0
	    || strcasecmp(Var, "OperServerMode") == 0
	    || strcasecmp(Var, "PredefChannelsOnly") == 0
	    || strcasecmp(Var, "SyslogFacility") == 0
	    || strcasecmp(Var, "WebircPassword") == 0) {
		Handle_OPTIONS(Line, Var, Arg);
		return "[Options]";
	}
	if (strcasecmp(Var, "ConnectRetry") == 0
	    || strcasecmp(Var, "MaxConnections") == 0
	    || strcasecmp(Var, "MaxConnectionsIP") == 0
	    || strcasecmp(Var, "MaxJoins") == 0
	    || strcasecmp(Var, "MaxNickLength") == 0
	    || strcasecmp(Var, "PingTimeout") == 0
	    || strcasecmp(Var, "PongTimeout") == 0) {
		Handle_LIMITS(Line, Var, Arg);
		return "[Limits]";
	}
#ifdef SSL_SUPPORT
	if (strcasecmp(Var, "SSLCertFile") == 0
	    || strcasecmp(Var, "SSLDHFile") == 0
	    || strcasecmp(Var, "SSLKeyFile") == 0
	    || strcasecmp(Var, "SSLKeyFilePassword") == 0
	    || strcasecmp(Var, "SSLPorts") == 0) {
		Handle_SSL(Line, Var + 3, Arg);
		return "[SSL]";
	}
#endif

	return NULL;
}

/**
 * Strip "no" prefix of a string.
 *
 * TODO: This function and support for "NoXXX" should be removed starting
 * with ngIRCd release 19! (One release after marking it "deprecated").
 *
 * @param str	Pointer to input string starting with "no".
 * @returns	New pointer to string without "no" prefix.
 */
static const char *
NoNo(const char *str)
{
	assert(strncasecmp("no", str, 2) == 0 && str[2]);
	return str + 2;
}

/**
 * Invert "boolean" string.
 *
 * TODO: This function and support for "NoXXX" should be removed starting
 * with ngIRCd release 19! (One release after marking it "deprecated").
 *
 * @param arg	"Boolean" input string.
 * @returns	Pointer to inverted "boolean string".
 */
static const char *
InvertArg(const char *arg)
{
	return yesno_to_str(!Check_ArgIsTrue(arg));
}

/**
 * Handle variable in [Global] configuration section.
 *
 * @param Line	Line numer in configuration file.
 * @param Var	Variable name.
 * @param Arg	Variable argument.
 */
static void
Handle_GLOBAL( int Line, char *Var, char *Arg )
{
	struct passwd *pwd;
	struct group *grp;
	size_t len;
	const char *section;

	assert(Line > 0);
	assert(Var != NULL);
	assert(Arg != NULL);

	if (strcasecmp(Var, "Name") == 0) {
		len = strlcpy(Conf_ServerName, Arg, sizeof(Conf_ServerName));
		if (len >= sizeof(Conf_ServerName))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "AdminInfo1") == 0) {
		len = strlcpy(Conf_ServerAdmin1, Arg, sizeof(Conf_ServerAdmin1));
		if (len >= sizeof(Conf_ServerAdmin1))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "AdminInfo2") == 0) {
		len = strlcpy(Conf_ServerAdmin2, Arg, sizeof(Conf_ServerAdmin2));
		if (len >= sizeof(Conf_ServerAdmin2))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "AdminEMail") == 0) {
		len = strlcpy(Conf_ServerAdminMail, Arg,
			sizeof(Conf_ServerAdminMail));
		if (len >= sizeof(Conf_ServerAdminMail))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "Info") == 0) {
		len = strlcpy(Conf_ServerInfo, Arg, sizeof(Conf_ServerInfo));
		if (len >= sizeof(Conf_ServerInfo))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "Listen") == 0) {
		if (Conf_ListenAddress) {
			Config_Error(LOG_ERR,
				     "Multiple Listen= options, ignoring: %s",
				     Arg);
			return;
		}
		Conf_ListenAddress = strdup_warn(Arg);
		/* If allocation fails, we're in trouble: we cannot ignore the
		 * error -- otherwise ngircd would listen on all interfaces. */
		if (!Conf_ListenAddress) {
			Config_Error(LOG_ALERT,
				     "%s exiting due to fatal errors!",
				     PACKAGE_NAME);
			exit(1);
		}
		return;
	}
	if (strcasecmp(Var, "MotdFile") == 0) {
		len = strlcpy(Conf_MotdFile, Arg, sizeof(Conf_MotdFile));
		if (len >= sizeof(Conf_MotdFile))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "MotdPhrase") == 0) {
		len = strlen(Arg);
		if (len == 0)
			return;
		if (len >= LINE_LEN) {
			Config_Error_TooLong(Line, Var);
			return;
		}
		if (!array_copyb(&Conf_Motd, Arg, len + 1))
			Config_Error(LOG_WARNING,
				     "%s, line %d: Could not append MotdPhrase: %s",
				     NGIRCd_ConfFile, Line, strerror(errno));
		Using_MotdFile = false;
		return;
	}
	if(strcasecmp(Var, "Password") == 0) {
		len = strlcpy(Conf_ServerPwd, Arg, sizeof(Conf_ServerPwd));
		if (len >= sizeof(Conf_ServerPwd))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "PidFile") == 0) {
		len = strlcpy(Conf_PidFile, Arg, sizeof(Conf_PidFile));
		if (len >= sizeof(Conf_PidFile))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "Ports") == 0) {
		ports_parse(&Conf_ListenPorts, Line, Arg);
		return;
	}
	if (strcasecmp(Var, "ServerGID") == 0) {
		grp = getgrnam(Arg);
		if (grp)
			Conf_GID = grp->gr_gid;
		else {
			Conf_GID = (unsigned int)atoi(Arg);
			if (!Conf_GID && strcmp(Arg, "0"))
				Config_Error(LOG_WARNING,
					     "%s, line %d: Value of \"%s\" is not a valid group name or ID!",
					     NGIRCd_ConfFile, Line, Var);
		}
		return;
	}
	if (strcasecmp(Var, "ServerUID") == 0) {
		pwd = getpwnam(Arg);
		if (pwd)
			Conf_UID = pwd->pw_uid;
		else {
			Conf_UID = (unsigned int)atoi(Arg);
			if (!Conf_UID && strcmp(Arg, "0"))
				Config_Error(LOG_WARNING,
					     "%s, line %d: Value of \"%s\" is not a valid user name or ID!",
					     NGIRCd_ConfFile, Line, Var);
		}
		return;
	}

	if (CheckLegacyNoOption(Var, Arg)) {
		/* TODO: This function and support for "NoXXX" could be
		 * be removed starting with ngIRCd release 19 (one release
		 * after marking it "deprecated"). */
		Config_Error(LOG_WARNING,
			     "%s, line %d (section \"Global\"): \"No\"-Prefix is deprecated, use \"%s = %s\" in [Options] section!",
			     NGIRCd_ConfFile, Line, NoNo(Var), InvertArg(Arg));
		if (strcasecmp(Var, "NoIdent") == 0)
			WarnIdent(Line);
		else if (strcasecmp(Var, "NoPam") == 0)
			WarnPAM(Line);
		return;
	}
	if ((section = CheckLegacyGlobalOption(Line, Var, Arg))) {
		/** TODO: This function and support for these options in the
		 * [Global] section could be removed starting with ngIRCd
		 * release 19 (one release after marking it "deprecated"). */
		if (strncasecmp(Var, "SSL", 3) == 0) {
			Config_Error(LOG_WARNING,
				     "%s, line %d (section \"Global\"): \"%s\" is deprecated here, move it to %s and rename to \"%s\"!",
				     NGIRCd_ConfFile, Line, Var, section,
				     Var + 3);
		} else {
			Config_Error(LOG_WARNING,
				     "%s, line %d (section \"Global\"): \"%s\" is deprecated here, move it to %s!",
				     NGIRCd_ConfFile, Line, Var, section);
		}
		return;
	}

	Config_Error_Section(Line, Var, "Global");
}

/**
 * Handle variable in [Limits] configuration section.
 *
 * @param Line	Line numer in configuration file.
 * @param Var	Variable name.
 * @param Arg	Variable argument.
 */
static void
Handle_LIMITS(int Line, char *Var, char *Arg)
{
	assert(Line > 0);
	assert(Var != NULL);
	assert(Arg != NULL);

	if (strcasecmp(Var, "ConnectRetry") == 0) {
		Conf_ConnectRetry = atoi(Arg);
		if (Conf_ConnectRetry < 5) {
			Config_Error(LOG_WARNING,
				     "%s, line %d: Value of \"ConnectRetry\" too low!",
				     NGIRCd_ConfFile, Line);
			Conf_ConnectRetry = 5;
		}
		return;
	}
	if (strcasecmp(Var, "MaxConnections") == 0) {
		Conf_MaxConnections = atoi(Arg);
		if (!Conf_MaxConnections && strcmp(Arg, "0"))
			Config_Error_NaN(Line, Var);
		return;
	}
	if (strcasecmp(Var, "MaxConnectionsIP") == 0) {
		Conf_MaxConnectionsIP = atoi(Arg);
		if (!Conf_MaxConnectionsIP && strcmp(Arg, "0"))
			Config_Error_NaN(Line, Var);
		return;
	}
	if (strcasecmp(Var, "MaxJoins") == 0) {
		Conf_MaxJoins = atoi(Arg);
		if (!Conf_MaxJoins && strcmp(Arg, "0"))
			Config_Error_NaN(Line, Var);
		return;
	}
	if (strcasecmp(Var, "MaxNickLength") == 0) {
		Conf_MaxNickLength = Handle_MaxNickLength(Line, Arg);
		return;
	}
	if (strcasecmp(Var, "PingTimeout") == 0) {
		Conf_PingTimeout = atoi(Arg);
		if (Conf_PingTimeout < 5) {
			Config_Error(LOG_WARNING,
				     "%s, line %d: Value of \"PingTimeout\" too low!",
				     NGIRCd_ConfFile, Line);
			Conf_PingTimeout = 5;
		}
		return;
	}
	if (strcasecmp(Var, "PongTimeout") == 0) {
		Conf_PongTimeout = atoi(Arg);
		if (Conf_PongTimeout < 5) {
			Config_Error(LOG_WARNING,
				     "%s, line %d: Value of \"PongTimeout\" too low!",
				     NGIRCd_ConfFile, Line);
			Conf_PongTimeout = 5;
		}
		return;
	}

	Config_Error_Section(Line, Var, "Limits");
}

/**
 * Handle variable in [Options] configuration section.
 *
 * @param Line	Line numer in configuration file.
 * @param Var	Variable name.
 * @param Arg	Variable argument.
 */
static void
Handle_OPTIONS(int Line, char *Var, char *Arg)
{
	size_t len;

	assert(Line > 0);
	assert(Var != NULL);
	assert(Arg != NULL);

	if (strcasecmp(Var, "AllowRemoteOper") == 0) {
		Conf_AllowRemoteOper = Check_ArgIsTrue(Arg);
		return;
	}
	if (strcasecmp(Var, "ChrootDir") == 0) {
		len = strlcpy(Conf_Chroot, Arg, sizeof(Conf_Chroot));
		if (len >= sizeof(Conf_Chroot))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "CloakHost") == 0) {
		len = strlcpy(Conf_CloakHost, Arg, sizeof(Conf_CloakHost));
		if (len >= sizeof(Conf_CloakHost))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "CloakHostModeX") == 0) {
		len = strlcpy(Conf_CloakHostModeX, Arg, sizeof(Conf_CloakHostModeX));
		if (len >= sizeof(Conf_CloakHostModeX))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "CloakHostSalt") == 0) {
		len = strlcpy(Conf_CloakHostSalt, Arg, sizeof(Conf_CloakHostSalt));
		if (len >= sizeof(Conf_CloakHostSalt))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "CloakUserToNick") == 0) {
		Conf_CloakUserToNick = Check_ArgIsTrue(Arg);
		return;
	}
	if (strcasecmp(Var, "ConnectIPv6") == 0) {
		Conf_ConnectIPv6 = Check_ArgIsTrue(Arg);
		WarnIPv6(Line);
		return;
	}
	if (strcasecmp(Var, "ConnectIPv4") == 0) {
		Conf_ConnectIPv4 = Check_ArgIsTrue(Arg);
		return;
	}
	if (strcasecmp(Var, "DNS") == 0) {
		Conf_DNS = Check_ArgIsTrue(Arg);
		return;
	}
	if (strcasecmp(Var, "Ident") == 0) {
		Conf_Ident = Check_ArgIsTrue(Arg);
		WarnIdent(Line);
		return;
	}
	if (strcasecmp(Var, "MorePrivacy") == 0) {
		Conf_MorePrivacy = Check_ArgIsTrue(Arg);
		return;
	}
	if (strcasecmp(Var, "NoticeAuth") == 0) {
		Conf_NoticeAuth = Check_ArgIsTrue(Arg);
		return;
	}
	if (strcasecmp(Var, "OperCanUseMode") == 0) {
		Conf_OperCanMode = Check_ArgIsTrue(Arg);
		return;
	}
	if (strcasecmp(Var, "OperChanPAutoOp") == 0) {
		Conf_OperChanPAutoOp = Check_ArgIsTrue(Arg);
		return;
	}
	if (strcasecmp(Var, "OperServerMode") == 0) {
		Conf_OperServerMode = Check_ArgIsTrue(Arg);
		return;
	}
	if (strcasecmp(Var, "PAM") == 0) {
		Conf_PAM = Check_ArgIsTrue(Arg);
		WarnPAM(Line);
		return;
	}
	if (strcasecmp(Var, "PAMIsOptional") == 0 ) {
		Conf_PAMIsOptional = Check_ArgIsTrue(Arg);
		return;
	}
	if (strcasecmp(Var, "PredefChannelsOnly") == 0) {
		Conf_PredefChannelsOnly = Check_ArgIsTrue(Arg);
		return;
	}
#ifndef STRICT_RFC
	if (strcasecmp(Var, "RequireAuthPing") == 0) {
		Conf_AuthPing = Check_ArgIsTrue(Arg);
		return;
	}
#endif
	if (strcasecmp(Var, "ScrubCTCP") == 0) {
		Conf_ScrubCTCP = Check_ArgIsTrue(Arg);
		return;
	}
#ifdef SYSLOG
	if (strcasecmp(Var, "SyslogFacility") == 0) {
		Conf_SyslogFacility = ngt_SyslogFacilityID(Arg,
							   Conf_SyslogFacility);
		return;
	}
#endif
	if (strcasecmp(Var, "WebircPassword") == 0) {
		len = strlcpy(Conf_WebircPwd, Arg, sizeof(Conf_WebircPwd));
		if (len >= sizeof(Conf_WebircPwd))
			Config_Error_TooLong(Line, Var);
		return;
	}

	Config_Error_Section(Line, Var, "Options");
}

#ifdef SSL_SUPPORT

/**
 * Handle variable in [SSL] configuration section.
 *
 * @param Line	Line numer in configuration file.
 * @param Var	Variable name.
 * @param Arg	Variable argument.
 */
static void
Handle_SSL(int Line, char *Var, char *Arg)
{
	assert(Line > 0);
	assert(Var != NULL);
	assert(Arg != NULL);

	if (strcasecmp(Var, "CertFile") == 0) {
		assert(Conf_SSLOptions.CertFile == NULL);
		Conf_SSLOptions.CertFile = strdup_warn(Arg);
		return;
	}
	if (strcasecmp(Var, "DHFile") == 0) {
		assert(Conf_SSLOptions.DHFile == NULL);
		Conf_SSLOptions.DHFile = strdup_warn(Arg);
		return;
	}
	if (strcasecmp(Var, "KeyFile") == 0) {
		assert(Conf_SSLOptions.KeyFile == NULL);
		Conf_SSLOptions.KeyFile = strdup_warn(Arg);
		return;
	}
	if (strcasecmp(Var, "KeyFilePassword") == 0) {
		assert(array_bytes(&Conf_SSLOptions.KeyFilePassword) == 0);
		if (!array_copys(&Conf_SSLOptions.KeyFilePassword, Arg))
			Config_Error(LOG_ERR,
				     "%s, line %d (section \"SSL\"): Could not copy %s: %s!",
				     NGIRCd_ConfFile, Line, Var,
				     strerror(errno));
		return;
	}
	if (strcasecmp(Var, "Ports") == 0) {
		ports_parse(&Conf_SSLOptions.ListenPorts, Line, Arg);
		return;
	}

	Config_Error_Section(Line, Var, "SSL");
}

#endif

/**
 * Handle variable in [Operator] configuration section.
 *
 * @param Line	Line numer in configuration file.
 * @param Var	Variable name.
 * @param Arg	Variable argument.
 */
static void
Handle_OPERATOR( int Line, char *Var, char *Arg )
{
	size_t len;
	struct Conf_Oper *op;

	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );

	op = array_get(&Conf_Opers, sizeof(*op),
			 array_length(&Conf_Opers, sizeof(*op)) - 1);
	if (!op)
		return;

	if (strcasecmp(Var, "Name") == 0) {
		/* Name of IRC operator */
		len = strlcpy(op->name, Arg, sizeof(op->name));
		if (len >= sizeof(op->name))
				Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "Password") == 0) {
		/* Password of IRC operator */
		len = strlcpy(op->pwd, Arg, sizeof(op->pwd));
		if (len >= sizeof(op->pwd))
				Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "Mask") == 0) {
		if (op->mask)
			return; /* Hostname already configured */
		op->mask = strdup_warn( Arg );
		return;
	}

	Config_Error_Section(Line, Var, "Operator");
}

/**
 * Handle variable in [Server] configuration section.
 *
 * @param Line	Line numer in configuration file.
 * @param Var	Variable name.
 * @param Arg	Variable argument.
 */
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
		if (port >= 0 && port < 0xFFFF)
			New_Server.port = (UINT16)port;
		else
			Config_Error(LOG_ERR,
				"%s, line %d (section \"Server\"): Illegal port number %ld!",
				NGIRCd_ConfFile, Line, port );
		return;
	}
#ifdef SSL_SUPPORT
	if( strcasecmp( Var, "SSLConnect" ) == 0 ) {
		New_Server.SSLConnect = Check_ArgIsTrue(Arg);
		return;
        }
#endif
	if( strcasecmp( Var, "Group" ) == 0 ) {
		/* Server group */
		New_Server.group = atoi( Arg );
		if (!New_Server.group && strcmp(Arg, "0"))
			Config_Error_NaN(Line, Var);
		return;
	}
	if( strcasecmp( Var, "Passive" ) == 0 ) {
		if (Check_ArgIsTrue(Arg))
			New_Server.flags |= CONF_SFLAG_DISABLED;
		return;
	}
	if (strcasecmp(Var, "ServiceMask") == 0) {
		len = strlcpy(New_Server.svs_mask, ngt_LowerStr(Arg),
			      sizeof(New_Server.svs_mask));
		if (len >= sizeof(New_Server.svs_mask))
			Config_Error_TooLong(Line, Var);
		return;
	}

	Config_Error_Section(Line, Var, "Server");
}

/**
 * Copy channel name into channel structure.
 *
 * If the channel name is not valid because of a missing prefix ('#', '&'),
 * a default prefix of '#' will be added.
 *
 * @param new_chan	New already allocated channel structure.
 * @param name		Name of the new channel.
 * @returns		true on success, false otherwise.
 */
static bool
Handle_Channelname(struct Conf_Channel *new_chan, const char *name)
{
	size_t size = sizeof(new_chan->name);
	char *dest = new_chan->name;

	if (!Channel_IsValidName(name)) {
		/*
		 * maybe user forgot to add a '#'.
		 * This is only here for user convenience.
		 */
		*dest = '#';
		--size;
		++dest;
	}
	return size > strlcpy(dest, name, size);
}

/**
 * Handle variable in [Channel] configuration section.
 *
 * @param Line	Line numer in configuration file.
 * @param Var	Variable name.
 * @param Arg	Variable argument.
 */
static void
Handle_CHANNEL(int Line, char *Var, char *Arg)
{
	size_t len;
	struct Conf_Channel *chan;

	assert( Line > 0 );
	assert( Var != NULL );
	assert( Arg != NULL );

	chan = array_get(&Conf_Channels, sizeof(*chan),
			 array_length(&Conf_Channels, sizeof(*chan)) - 1);
	if (!chan)
		return;

	if (strcasecmp(Var, "Name") == 0) {
		if (!Handle_Channelname(chan, Arg))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if (strcasecmp(Var, "Modes") == 0) {
		/* Initial modes */
		len = strlcpy(chan->modes, Arg, sizeof(chan->modes));
		if (len >= sizeof(chan->modes))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "Topic" ) == 0 ) {
		/* Initial topic */
		len = strlcpy(chan->topic, Arg, sizeof(chan->topic));
		if (len >= sizeof(chan->topic))
			Config_Error_TooLong( Line, Var );
		return;
	}
	if( strcasecmp( Var, "Key" ) == 0 ) {
		/* Initial Channel Key (mode k) */
		len = strlcpy(chan->key, Arg, sizeof(chan->key));
		if (len >= sizeof(chan->key))
			Config_Error_TooLong(Line, Var);
		return;
	}
	if( strcasecmp( Var, "MaxUsers" ) == 0 ) {
		/* maximum user limit, mode l */
		chan->maxusers = (unsigned long) atol(Arg);
		if (!chan->maxusers && strcmp(Arg, "0"))
			Config_Error_NaN(Line, Var);
		return;
	}
	if (strcasecmp(Var, "KeyFile") == 0) {
		/* channel keys */
		len = strlcpy(chan->keyfile, Arg, sizeof(chan->keyfile));
		if (len >= sizeof(chan->keyfile))
			Config_Error_TooLong(Line, Var);
		return;
	}

	Config_Error_Section(Line, Var, "Channel");
}

/**
 * Validate server configuration.
 *
 * Please note that this function uses exit(1) on fatal errors and therefore
 * can result in ngIRCd terminating!
 *
 * @param Configtest	true if the daemon has been called with "--configtest".
 * @param Rehash	true if re-reading configuration on runtime.
 * @returns		true if configuration is valid.
 */
static bool
Validate_Config(bool Configtest, bool Rehash)
{
	/* Validate configuration settings. */

#ifdef DEBUG
	int i, servers, servers_once;
#endif
	bool config_valid = true;
	char *ptr;

	/* Emit a warning when the config file is not a full path name */
	if (NGIRCd_ConfFile[0] && NGIRCd_ConfFile[0] != '/') {
		Config_Error(LOG_WARNING,
			"Not specifying a full path name to \"%s\" can cause problems when rehashing the server!",
			NGIRCd_ConfFile);
	}

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
		config_valid = false;
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
		config_valid = false;
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
		config_valid = false;
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

#ifdef PAM
	if (Conf_ServerPwd[0])
		Config_Error(LOG_ERR,
			     "This server uses PAM, \"Password\" in [Global] section will be ignored!");
#endif

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
	    "Configuration: Operators=%ld, Servers=%d[%d], Channels=%ld",
	    array_length(&Conf_Opers, sizeof(struct Conf_Oper)),
	    servers, servers_once,
	    array_length(&Conf_Channels, sizeof(struct Conf_Channel)));
#endif

	return config_valid;
}

/**
 * Output "line too long" warning.
 *
 * @param Line	Line number in configuration file.
 * @param Item	Affected variable name.
 */
static void
Config_Error_TooLong ( const int Line, const char *Item )
{
	Config_Error( LOG_WARNING, "%s, line %d: Value of \"%s\" too long!", NGIRCd_ConfFile, Line, Item );
}

/**
 * Output "unknown variable" warning.
 *
 * @param Line		Line number in configuration file.
 * @param Item		Affected variable name.
 * @param Section	Section name.
 */
static void
Config_Error_Section(const int Line, const char *Item, const char *Section)
{
	Config_Error(LOG_ERR, "%s, line %d (section \"%s\"): Unknown variable \"%s\"!",
		     NGIRCd_ConfFile, Line, Section, Item);
}

/**
 * Output "not a number" warning.
 *
 * @param Line	Line number in configuration file.
 * @param Item	Affected variable name.
 */
static void
Config_Error_NaN( const int Line, const char *Item )
{
	Config_Error( LOG_WARNING, "%s, line %d: Value of \"%s\" is not a number!",
						NGIRCd_ConfFile, Line, Item );
}

/**
 * Output configuration error to console and/or logfile.
 *
 * On runtime, the normal log functions of the daemon are used. But when
 * testing the configuration ("--configtest"), all messages go directly
 * to the console.
 *
 * @param Level		Severity level of the message.
 * @param Format	Format string; see printf() function.
 */
#ifdef PROTOTYPES
static void Config_Error( const int Level, const char *Format, ... )
#else
static void Config_Error( Level, Format, va_alist )
const int Level;
const char *Format;
va_dcl
#endif
{
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

	if (!Use_Log) {
		if (Level <= LOG_WARNING)
			printf(" - %s\n", msg);
		else
			puts(msg);
	} else
		Log(Level, "%s", msg);
}

#ifdef DEBUG

/**
 * Dump internal state of the "configuration module".
 */
GLOBAL void
Conf_DebugDump(void)
{
	int i;

	Log(LOG_DEBUG, "Configured servers:");
	for (i = 0; i < MAX_SERVERS; i++) {
		if (! Conf_Server[i].name[0])
			continue;
		Log(LOG_DEBUG,
		    " - %s: %s:%d, last=%ld, group=%d, flags=%d, conn=%d",
		    Conf_Server[i].name, Conf_Server[i].host,
		    Conf_Server[i].port, Conf_Server[i].lasttry,
		    Conf_Server[i].group, Conf_Server[i].flags,
		    Conf_Server[i].conn_id);
	}
}

#endif

/**
 * Initialize server configuration structur to default values.
 *
 * @param Server	Pointer to server structure to initialize.
 */
static void
Init_Server_Struct( CONF_SERVER *Server )
{
	assert( Server != NULL );

	memset( Server, 0, sizeof (CONF_SERVER) );

	Server->group = NONE;
	Server->lasttry = time( NULL ) - Conf_ConnectRetry + STARTUP_DELAY;

	if( NGIRCd_Passive ) Server->flags = CONF_SFLAG_DISABLED;

	Proc_InitStruct(&Server->res_stat);
	Server->conn_id = NONE;
	memset(&Server->bind_addr, 0, sizeof(Server->bind_addr));
}

/* -eof- */
