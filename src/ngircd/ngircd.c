/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton (alex@barton.de).
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
 * The main program, including the C function main() which is called
 * by the loader of the operating system.
 */

#include "imp.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

#if defined(DEBUG) && defined(HAVE_MTRACE)
#include <mcheck.h>
#endif

#include "defines.h"
#include "resolve.h"
#include "conn.h"
#include "conf-ssl.h"
#include "client.h"
#include "channel.h"
#include "conf.h"
#include "lists.h"
#include "log.h"
#include "parse.h"
#include "irc.h"

#ifdef ZEROCONF
#include "rendezvous.h"
#endif

#include "exp.h"
#include "ngircd.h"


static void Initialize_Signal_Handler PARAMS(( void ));
static void Signal_Handler PARAMS(( int Signal ));

static void Show_Version PARAMS(( void ));
static void Show_Help PARAMS(( void ));

static void Pidfile_Create PARAMS(( pid_t pid ));
static void Pidfile_Delete PARAMS(( void ));

static void Fill_Version PARAMS(( void ));

static void Setup_FDStreams PARAMS(( void ));

static bool NGIRCd_Init PARAMS(( bool ));

/**
 * The main() function of ngIRCd.
 * Here all starts: this function is called by the operating system loader,
 * it is the first portion of code executed of ngIRCd.
 * @param argc The number of arguments passed to ngIRCd on the command line.
 * @param argv An array containing all the arguments passed to ngIRCd.
 * @return Global exit code of ngIRCd, zero on success.
 */
GLOBAL int
main( int argc, const char *argv[] )
{
	bool ok, configtest = false;
	bool NGIRCd_NoDaemon = false;
	int i;
	size_t n;

#if defined(DEBUG) && defined(HAVE_MTRACE)
	/* enable GNU libc memory tracing when running in debug mode
	 * and functionality available */
	mtrace();
#endif

	umask( 0077 );

	NGIRCd_SignalQuit = NGIRCd_SignalRestart = NGIRCd_SignalRehash = false;
	NGIRCd_Passive = false;
#ifdef DEBUG
	NGIRCd_Debug = false;
#endif
#ifdef SNIFFER
	NGIRCd_Sniffer = false;
#endif
	strlcpy( NGIRCd_ConfFile, SYSCONFDIR, sizeof( NGIRCd_ConfFile ));
	strlcat( NGIRCd_ConfFile, CONFIG_FILE, sizeof( NGIRCd_ConfFile ));

	Fill_Version( );

	/* parse conmmand line */
	for( i = 1; i < argc; i++ )
	{
		ok = false;
		if(( argv[i][0] == '-' ) && ( argv[i][1] == '-' ))
		{
			/* long option */
			if( strcmp( argv[i], "--config" ) == 0 )
			{
				if( i + 1 < argc )
				{
					/* Ok, there's an parameter left */
					strlcpy( NGIRCd_ConfFile, argv[i + 1], sizeof( NGIRCd_ConfFile ));

					/* next parameter */
					i++; ok = true;
				}
			}
			if( strcmp( argv[i], "--configtest" ) == 0 )
			{
				configtest = true;
				ok = true;
			}
#ifdef DEBUG
			if( strcmp( argv[i], "--debug" ) == 0 )
			{
				NGIRCd_Debug = true;
				ok = true;
			}
#endif
			if( strcmp( argv[i], "--help" ) == 0 )
			{
				Show_Version( );
				puts( "" ); Show_Help( ); puts( "" );
				exit( 1 );
			}
			if( strcmp( argv[i], "--nodaemon" ) == 0 )
			{
				NGIRCd_NoDaemon = true;
				ok = true;
			}
			if( strcmp( argv[i], "--passive" ) == 0 )
			{
				NGIRCd_Passive = true;
				ok = true;
			}
#ifdef SNIFFER
			if( strcmp( argv[i], "--sniffer" ) == 0 )
			{
				NGIRCd_Sniffer = true;
				ok = true;
			}
#endif
			if( strcmp( argv[i], "--version" ) == 0 )
			{
				Show_Version( );
				exit( 1 );
			}
		}
		else if(( argv[i][0] == '-' ) && ( argv[i][1] != '-' ))
		{
			/* short option */
			for( n = 1; n < strlen( argv[i] ); n++ )
			{
				ok = false;
#ifdef DEBUG
				if (argv[i][n] == 'd') {
					NGIRCd_Debug = true;
					ok = true;
				}
#endif
				if (argv[i][n] == 'f') {
					if(( ! argv[i][n + 1] ) && ( i + 1 < argc ))
					{
						/* Ok, next character is a blank */
						strlcpy( NGIRCd_ConfFile, argv[i + 1], sizeof( NGIRCd_ConfFile ));

						/* go to the following parameter */
						i++;
						n = strlen( argv[i] );
						ok = true;
					}
				}

				if (argv[i][n] == 'h') {
					Show_Version();
					puts(""); Show_Help(); puts("");
					exit(1);
				}

				if (argv[i][n] == 'n') {
					NGIRCd_NoDaemon = true;
					ok = true;
				}
				if (argv[i][n] == 'p') {
					NGIRCd_Passive = true;
					ok = true;
				}
#ifdef SNIFFER
				if (argv[i][n] == 's') {
					NGIRCd_Sniffer = true;
					ok = true;
				}
#endif
				if (argv[i][n] == 't') {
					configtest = true;
					ok = true;
				}

				if (argv[i][n] == 'V') {
					Show_Version();
					exit(1);
				}

				if (! ok) {
					printf( "%s: invalid option \"-%c\"!\n", PACKAGE_NAME, argv[i][n] );
					printf( "Try \"%s --help\" for more information.\n", PACKAGE_NAME );
					exit( 1 );
				}
			}

		}
		if( ! ok )
		{
			printf( "%s: invalid option \"%s\"!\n", PACKAGE_NAME, argv[i] );
			printf( "Try \"%s --help\" for more information.\n", PACKAGE_NAME );
			exit( 1 );
		}
	}

	/* Debug-Level (for IRCs "VERSION" command) */
	NGIRCd_DebugLevel[0] = '\0';
#ifdef DEBUG
	if( NGIRCd_Debug ) strcpy( NGIRCd_DebugLevel, "1" );
#endif
#ifdef SNIFFER
	if( NGIRCd_Sniffer )
	{
		NGIRCd_Debug = true;
		strcpy( NGIRCd_DebugLevel, "2" );
	}
#endif

	if( configtest )
	{
		Show_Version( ); puts( "" );
		exit( Conf_Test( ));
	}
	
	while( ! NGIRCd_SignalQuit )
	{
		/* Initialize global variables */
		NGIRCd_Start = time( NULL );
		(void)strftime( NGIRCd_StartStr, 64, "%a %b %d %Y at %H:%M:%S (%Z)", localtime( &NGIRCd_Start ));

		NGIRCd_SignalRehash = false;
		NGIRCd_SignalRestart = false;
		NGIRCd_SignalQuit = false;

		/* Initialize modules, part I */
		Log_Init( ! NGIRCd_NoDaemon );
		Conf_Init( );

		/* Initialize the "main program": chroot environment, user and
		 * group ID, ... */
		if (!NGIRCd_Init(NGIRCd_NoDaemon)) {
			Log(LOG_ALERT, "Fatal: Initialization failed");
			exit(1);
		}

		/* Initialize modules, part II: these functions are eventually
		 * called with already dropped privileges ... */
		Channel_Init( );
		Client_Init( );
#ifdef ZEROCONF
		Rendezvous_Init( );
#endif
		Conn_Init( );

#ifdef DEBUG
		/* Redirect stderr handle to "error file" for debugging
		 * when not running in "no daemon" mode: */
		if( ! NGIRCd_NoDaemon ) Log_InitErrorfile( );
#endif

		Initialize_Signal_Handler( );

		/*
		 * create protocol and server identification.
		 * The syntax used by ngIRCd in PASS commands and the extended flags
		 * are described in doc/Protocol.txt
		 */
#ifdef IRCPLUS
		snprintf( NGIRCd_ProtoID, sizeof NGIRCd_ProtoID, "%s%s %s|%s:%s", PROTOVER, PROTOIRCPLUS, PACKAGE_NAME, PACKAGE_VERSION, IRCPLUSFLAGS );
#ifdef ZLIB
		strcat( NGIRCd_ProtoID, "Z" );
#endif
		if( Conf_OperCanMode ) strcat( NGIRCd_ProtoID, "o" );
#else
		snprintf( NGIRCd_ProtoID, sizeof NGIRCd_ProtoID, "%s%s %s|%s", PROTOVER, PROTOIRC, PACKAGE_NAME, PACKAGE_VERSION );
#endif
		strlcat( NGIRCd_ProtoID, " P", sizeof NGIRCd_ProtoID );
#ifdef ZLIB
		strlcat( NGIRCd_ProtoID, "Z", sizeof NGIRCd_ProtoID );
#endif
		LogDebug("Protocol and server ID is \"%s\".", NGIRCd_ProtoID);

		Channel_InitPredefined( );

		if( Conn_InitListeners( ) < 1 )
		{
			Log( LOG_ALERT, "Server isn't listening on a single port!" );
			Log( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE_NAME );
			Pidfile_Delete( );
			exit( 1 );
		}
		
		/* Hauptschleife */
		Conn_Handler( );

		/* Alles abmelden */
		Conn_Exit( );
#ifdef ZEROCONF
		Rendezvous_Exit( );
#endif
		Client_Exit( );
		Channel_Exit( );
		Log_Exit( );
	}
	Pidfile_Delete( );

	return 0;
} /* main */


/**
 * Generate ngIRCd "version string".
 * This string is generated once and then stored in NGIRCd_Version for
 * further usage, for example by the IRC command VERSION and the --version
 * command line switch.
 */
static void
Fill_Version( void )
{
	NGIRCd_VersionAddition[0] = '\0';

#ifdef SYSLOG
	strlcpy( NGIRCd_VersionAddition, "SYSLOG", sizeof NGIRCd_VersionAddition );
#endif
#ifdef ZLIB
	if( NGIRCd_VersionAddition[0] )
		strlcat( NGIRCd_VersionAddition, "+", sizeof NGIRCd_VersionAddition );

	strlcat( NGIRCd_VersionAddition, "ZLIB", sizeof NGIRCd_VersionAddition );
#endif
#ifdef SSL_SUPPORT
	if ( NGIRCd_VersionAddition[0] ) strlcat( NGIRCd_VersionAddition, "+", sizeof NGIRCd_VersionAddition );
	strlcat( NGIRCd_VersionAddition, "SSL", sizeof NGIRCd_VersionAddition );
#endif
#ifdef TCPWRAP
	if( NGIRCd_VersionAddition[0] )
			strlcat( NGIRCd_VersionAddition, "+", sizeof NGIRCd_VersionAddition );

	strlcat( NGIRCd_VersionAddition, "TCPWRAP", sizeof NGIRCd_VersionAddition );
#endif
#ifdef ZEROCONF
	if( NGIRCd_VersionAddition[0] )
		strlcat( NGIRCd_VersionAddition, "+", sizeof NGIRCd_VersionAddition );

	strlcat( NGIRCd_VersionAddition, "ZEROCONF", sizeof NGIRCd_VersionAddition );
#endif
#ifdef IDENTAUTH
	if( NGIRCd_VersionAddition[0] )
		strlcat( NGIRCd_VersionAddition, "+", sizeof NGIRCd_VersionAddition );

	strlcat( NGIRCd_VersionAddition, "IDENT", sizeof NGIRCd_VersionAddition );
#endif
#ifdef DEBUG
	if( NGIRCd_VersionAddition[0] )
		strlcat( NGIRCd_VersionAddition, "+", sizeof NGIRCd_VersionAddition );

	strlcat( NGIRCd_VersionAddition, "DEBUG", sizeof NGIRCd_VersionAddition );
#endif
#ifdef SNIFFER
	if( NGIRCd_VersionAddition[0] )
		strlcat( NGIRCd_VersionAddition, "+", sizeof NGIRCd_VersionAddition );

	strlcat( NGIRCd_VersionAddition, "SNIFFER", sizeof NGIRCd_VersionAddition );
#endif
#ifdef STRICT_RFC
	if( NGIRCd_VersionAddition[0] )
		strlcat( NGIRCd_VersionAddition, "+", sizeof NGIRCd_VersionAddition );

	strlcat( NGIRCd_VersionAddition, "RFC", sizeof NGIRCd_VersionAddition );
#endif
#ifdef IRCPLUS
	if( NGIRCd_VersionAddition[0] )
		strlcat( NGIRCd_VersionAddition, "+", sizeof NGIRCd_VersionAddition );

	strlcat( NGIRCd_VersionAddition, "IRCPLUS", sizeof NGIRCd_VersionAddition );
#endif
#ifdef WANT_IPV6
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+", sizeof(NGIRCd_VersionAddition));

	strlcat(NGIRCd_VersionAddition, "IPv6", sizeof(NGIRCd_VersionAddition));
#endif
	if( NGIRCd_VersionAddition[0] )
		strlcat( NGIRCd_VersionAddition, "-", sizeof( NGIRCd_VersionAddition ));

	strlcat( NGIRCd_VersionAddition, TARGET_CPU, sizeof( NGIRCd_VersionAddition ));
	strlcat( NGIRCd_VersionAddition, "/", sizeof( NGIRCd_VersionAddition ));
	strlcat( NGIRCd_VersionAddition, TARGET_VENDOR, sizeof( NGIRCd_VersionAddition ));
	strlcat( NGIRCd_VersionAddition, "/", sizeof( NGIRCd_VersionAddition ));
	strlcat( NGIRCd_VersionAddition, TARGET_OS, sizeof( NGIRCd_VersionAddition ));

	snprintf(NGIRCd_Version, sizeof NGIRCd_Version, "%s %s-%s",
		 PACKAGE_NAME, PACKAGE_VERSION, NGIRCd_VersionAddition);
	} /* Fill_Version */


/**
 * Reload the server configuration file.
 */
GLOBAL void
NGIRCd_Rehash( void )
{
	char old_name[CLIENT_ID_LEN];
	unsigned old_nicklen;

	Log( LOG_NOTICE|LOG_snotice, "Re-reading configuration NOW!" );
	NGIRCd_SignalRehash = false;

	/* Remember old server name and nick name length */
	strlcpy( old_name, Conf_ServerName, sizeof old_name );
	old_nicklen = Conf_MaxNickLength;

	/* Re-read configuration ... */
	if (!Conf_Rehash( ))
		return;

	/* Close down all listening sockets */
	Conn_ExitListeners( );

	/* Recover old server name and nick name length: these values can't
	 * be changed during run-time */
	if (strcmp(old_name, Conf_ServerName) != 0 ) {
		strlcpy(Conf_ServerName, old_name, sizeof Conf_ServerName);
		Log(LOG_ERR, "Can't change \"ServerName\" on runtime! Ignored new name.");
	}
	if (old_nicklen != Conf_MaxNickLength) {
		Conf_MaxNickLength = old_nicklen;
		Log(LOG_ERR, "Can't change \"MaxNickLength\" on runtime! Ignored new value.");
	}

	/* Create new pre-defined channels */
	Channel_InitPredefined( );

	if (!ConnSSL_InitLibrary())
		Log(LOG_WARNING, "Re-Initializing SSL failed, using old keys");

	/* Start listening on sockets */
	Conn_InitListeners( );

	/* Sync configuration with established connections */
	Conn_SyncServerStruct( );

	Log( LOG_NOTICE|LOG_snotice, "Re-reading of configuration done." );
} /* NGIRCd_Rehash */


/**
 * Initialize the signal handler.
 */
static void
Initialize_Signal_Handler( void )
{
#ifdef HAVE_SIGACTION
	struct sigaction saction;

	memset( &saction, 0, sizeof( saction ));
	saction.sa_handler = Signal_Handler;
#ifdef SA_RESTART
	saction.sa_flags |= SA_RESTART;
#endif
#ifdef SA_NOCLDWAIT
	saction.sa_flags |= SA_NOCLDWAIT;
#endif

	sigaction(SIGINT, &saction, NULL);
	sigaction(SIGQUIT, &saction, NULL);
	sigaction(SIGTERM, &saction, NULL);
	sigaction(SIGHUP, &saction, NULL);
	sigaction(SIGCHLD, &saction, NULL);

	/* we handle write errors properly; ignore SIGPIPE */
	saction.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &saction, NULL);
#else
	signal(SIGINT, Signal_Handler);
	signal(SIGQUIT, Signal_Handler);
	signal(SIGTERM, Signal_Handler);
	signal(SIGHUP, Signal_Handler);
	signal(SIGCHLD, Signal_Handler);

	signal(SIGPIPE, SIG_IGN);
#endif
} /* Initialize_Signal_Handler */


/**
 * Signal handler of ngIRCd.
 * This function is called whenever ngIRCd catches a signal sent by the
 * user and/or the system to it. For example SIGTERM and SIGHUP.
 * @param Signal Number of the signal to handle.
 */
static void
Signal_Handler( int Signal )
{
	switch( Signal )
	{
		case SIGTERM:
		case SIGINT:
		case SIGQUIT:
			/* shut down sever */
			NGIRCd_SignalQuit = true;
			break;
		case SIGHUP:
			/* re-read configuration */
			NGIRCd_SignalRehash = true;
			break;
		case SIGCHLD:
			/* child-process exited, avoid zombies */
			while (waitpid( -1, NULL, WNOHANG) > 0)
				;
			break;
#ifdef DEBUG
		default:
			/* unbekanntes bzw. unbehandeltes Signal */
			Log( LOG_DEBUG, "Got signal %d! Ignored.", Signal );
#endif
	}
} /* Signal_Handler */


/**
 * Display copyright and version information of ngIRCd on the console.
 */
static void
Show_Version( void )
{
	puts( NGIRCd_Version );
	puts( "Copyright (c)2001-2010 Alexander Barton (<alex@barton.de>) and Contributors." );
	puts( "Homepage: <http://ngircd.barton.de/>\n" );
	puts( "This is free software; see the source for copying conditions. There is NO" );
	puts( "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." );
} /* Show_Version */


/**
 * Display a short help text on the console.
 * This help depends on the configuration of the executable and only shows
 * options that are actually enabled.
 */
static void
Show_Help( void )
{
#ifdef DEBUG
	puts( "  -d, --debug        log extra debug messages" );
#endif
	puts( "  -f, --config <f>   use file <f> as configuration file" );
	puts( "  -n, --nodaemon     don't fork and don't detach from controlling terminal" );
	puts( "  -p, --passive      disable automatic connections to other servers" );
#ifdef SNIFFER
	puts( "  -s, --sniffer      enable network sniffer and display all IRC traffic" );
#endif
	puts( "  -t, --configtest   read, validate and display configuration; then exit" );
	puts( "  -V, --version      output version information and exit" );
	puts( "  -h, --help         display this help and exit" );
} /* Show_Help */


/**
 * Delete the file containing the process ID (PID).
 */
static void
Pidfile_Delete( void )
{
	/* Pidfile configured? */
	if( ! Conf_PidFile[0] ) return;

#ifdef DEBUG
	Log( LOG_DEBUG, "Removing PID file (%s) ...", Conf_PidFile );
#endif

	if( unlink( Conf_PidFile ))
		Log( LOG_ERR, "Error unlinking PID file (%s): %s", Conf_PidFile, strerror( errno ));
} /* Pidfile_Delete */


/**
 * Create the file containing the process ID of ngIRCd ("PID file").
 * @param pid The process ID to be stored in this file.
 */
static void
Pidfile_Create(pid_t pid)
{
	int pidfd;
	char pidbuf[64];
	int len;

	/* Pidfile configured? */
	if( ! Conf_PidFile[0] ) return;

#ifdef DEBUG
	Log( LOG_DEBUG, "Creating PID file (%s) ...", Conf_PidFile );
#endif

	pidfd = open( Conf_PidFile, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if ( pidfd < 0 ) {
		Log( LOG_ERR, "Error writing PID file (%s): %s", Conf_PidFile, strerror( errno ));
		return;
	}

	len = snprintf(pidbuf, sizeof pidbuf, "%ld\n", (long)pid);
	if (len < 0 || len >= (int)sizeof pidbuf) {
		Log( LOG_ERR, "Error converting pid");
		return;
	}
	
	if (write(pidfd, pidbuf, (size_t)len) != (ssize_t)len)
		Log( LOG_ERR, "Can't write PID file (%s): %s", Conf_PidFile, strerror( errno ));

	if( close(pidfd) != 0 )
		Log( LOG_ERR, "Error closing PID file (%s): %s", Conf_PidFile, strerror( errno ));
} /* Pidfile_Create */


/**
 * Redirect stdin, stdout and stderr to apropriate file handles.
 */
static void
Setup_FDStreams( void )
{
	int fd;

	/* Test if we can open /dev/null for reading and writing. If not
	 * we are most probably chrooted already and the server has been
	 * restarted. So we simply don't try to redirect stdXXX ... */
	fd = open( "/dev/null", O_RDWR );
	if ( fd < 0 ) {
		Log(LOG_WARNING, "Could not open /dev/null: %s", strerror(errno));	
		return;
	} 

	fflush(stdout);
	fflush(stderr);

	/* Create new stdin(0), stdout(1) and stderr(2) descriptors */
	dup2( fd, 0 ); dup2( fd, 1 ); dup2( fd, 2 );

	/* Close newly opened file descriptor if not stdin/out/err */
	if( fd > 2 ) close( fd );
} /* Setup_FDStreams */


static bool
NGIRCd_getNobodyID(uid_t *uid, gid_t *gid )
{
	struct passwd *pwd;

#ifdef __CYGWIN__
	/* Cygwin kludge.
	 * It can return EINVAL instead of EPERM
	 * so, if we are already unprivileged,
	 * use id of current user.
	 */
	if (geteuid() && getuid()) {
		*uid = getuid();
		*gid = getgid();
		return true;
	}
#endif

	pwd = getpwnam("nobody");
	if (!pwd) return false;

	if ( !pwd->pw_uid || !pwd->pw_gid)
		return false;

	*uid = pwd->pw_uid;	
	*gid = pwd->pw_gid;
	endpwent();

	return true;	
}


static bool
NGIRCd_Init( bool NGIRCd_NoDaemon ) 
{
	static bool initialized;
	bool chrooted = false;
	struct passwd *pwd;
	struct group *grp;
	int real_errno;
	pid_t pid;

	if (initialized)
		return true;

	if (!ConnSSL_InitLibrary())
		Log(LOG_WARNING,
		    "Warning: Error during SSL initialization, continuing ...");

	if( Conf_Chroot[0] ) {
		if( chdir( Conf_Chroot ) != 0 ) {
			Log( LOG_ERR, "Can't chdir() in ChrootDir (%s): %s", Conf_Chroot, strerror( errno ));
			return false;
		}

		if( chroot( Conf_Chroot ) != 0 ) {
			if (errno != EPERM) {
				Log( LOG_ERR, "Can't change root directory to \"%s\": %s",
								Conf_Chroot, strerror( errno ));

				return false;
			}
		} else {
			chrooted = true;
			Log( LOG_INFO, "Changed root and working directory to \"%s\".", Conf_Chroot );
		}
	}

	if (Conf_UID == 0) {
		Log(LOG_INFO, "ServerUID must not be 0, using \"nobody\" instead.", Conf_UID);

  		if (! NGIRCd_getNobodyID(&Conf_UID, &Conf_GID)) {
			Log(LOG_WARNING, "Could not get user/group ID of user \"nobody\": %s",
					errno ? strerror(errno) : "not found" );
			return false;
		}
	}

	if (getgid() != Conf_GID) {
		/* Change group ID */
		if (setgid(Conf_GID) != 0) {
			real_errno = errno;
			Log( LOG_ERR, "Can't change group ID to %u: %s", Conf_GID, strerror( errno ));
			if (real_errno != EPERM) 
				return false;
		}
	}

	if (getuid() != Conf_UID) {
		/* Change user ID */
		if (setuid(Conf_UID) != 0) {
			real_errno = errno;
			Log(LOG_ERR, "Can't change user ID to %u: %s", Conf_UID, strerror(errno));
			if (real_errno != EPERM) 
				return false;
		}
	}

	initialized = true;

	/* Normally a child process is forked which isn't any longer
	 * connected to ther controlling terminal. Use "--nodaemon"
	 * to disable this "daemon mode" (useful for debugging). */
	if ( ! NGIRCd_NoDaemon ) {
		pid = fork( );
		if( pid > 0 ) {
			/* "Old" process: exit. */
			exit( 0 );
		}
		if( pid < 0 ) {
			/* Error!? */
			fprintf( stderr, "%s: Can't fork: %s!\nFatal error, exiting now ...\n",
								PACKAGE_NAME, strerror( errno ));
			exit( 1 );
		}

		/* New child process */
#ifndef NeXT
		(void)setsid( );
#else
		setpgrp(0, getpid());
#endif
		chdir( "/" );

		/* Detach stdin, stdout and stderr */
		Setup_FDStreams( );
	}
	pid = getpid();

	Pidfile_Create( pid );

	/* Check UID/GID we are running as, can be different from values
	 * configured (e. g. if we were already started with a UID>0. */
	Conf_UID = getuid();
	Conf_GID = getgid();

	pwd = getpwuid( Conf_UID );
	grp = getgrgid( Conf_GID );

	Log(LOG_INFO, "Running as user %s(%ld), group %s(%ld), with PID %ld.",
				pwd ? pwd->pw_name : "unknown", (long)Conf_UID,
				grp ? grp->gr_name : "unknown", (long)Conf_GID, (long)pid);

	if (chrooted) {
		Log(LOG_INFO, "Running with root directory \"%s\".",
		    Conf_Chroot );
		return true;
	} else
		Log(LOG_INFO, "Not running with changed root directory.");

	/* Change working directory to home directory of the user
	 * we are running as (only when running in daemon mode and not in chroot) */
	
	if ( pwd ) {
		if (!NGIRCd_NoDaemon ) {
			if( chdir( pwd->pw_dir ) == 0 ) 
				Log( LOG_DEBUG, "Changed working directory to \"%s\" ...", pwd->pw_dir );
			else 
				Log( LOG_INFO, "Notice: Can't change working directory to \"%s\": %s",
								pwd->pw_dir, strerror( errno ));
		}
	} else {
		Log( LOG_ERR, "Can't get user informaton for UID %d!?", Conf_UID );
	}

return true;
}

/* -eof- */
