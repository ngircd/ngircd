/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2024 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#define GLOBAL_INIT
#include "portab.h"

/**
 * @file
 * The main program, including the C function main() which is called
 * by the loader of the operating system.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

#if defined(DEBUG) && defined(HAVE_MTRACE)
#include <mcheck.h>
#endif

#include "conn.h"
#include "class.h"
#include "channel.h"
#include "conf.h"
#include "log.h"
#include "sighandlers.h"
#include "io.h"

#include "ngircd.h"

static void Show_Version PARAMS(( void ));
static void Show_Help PARAMS(( void ));

static void Pidfile_Create PARAMS(( pid_t pid ));
static void Pidfile_Delete PARAMS(( void ));

static void Fill_Version PARAMS(( void ));

static void Random_Init PARAMS(( void ));

static void Setup_FDStreams PARAMS(( int fd ));

static bool NGIRCd_Init PARAMS(( bool ));


/**
 * The main() function of ngIRCd.
 *
 * Here all starts: this function is called by the operating system loader,
 * it is the first portion of code executed of ngIRCd.
 *
 * @param argc The number of arguments passed to ngIRCd on the command line.
 * @param argv An array containing all the arguments passed to ngIRCd.
 * @return Global exit code of ngIRCd, zero on success.
 */
GLOBAL int
main(int argc, const char *argv[])
{
	bool ok, configtest = false;
	bool NGIRCd_NoDaemon = false, NGIRCd_NoSyslog = false;
	int i;
	size_t n;

#if defined(DEBUG) && defined(HAVE_MTRACE)
	/* enable GNU libc memory tracing when running in debug mode
	 * and functionality available */
	mtrace();
#endif

	umask(0077);

	NGIRCd_SignalQuit = NGIRCd_SignalRestart = false;
	NGIRCd_Passive = false;
	NGIRCd_Debug = false;
#ifdef SNIFFER
	NGIRCd_Sniffer = false;
#endif

	Fill_Version();

	/* parse conmmand line */
	for (i = 1; i < argc; i++) {
		ok = false;
		if (argv[i][0] == '-' && argv[i][1] == '-') {
			/* long option */
			if (strcmp(argv[i], "--config") == 0) {
				if (i + 1 < argc) {
					/* Ok, there's an parameter left */
					strlcpy(NGIRCd_ConfFile, argv[i+1],
						sizeof(NGIRCd_ConfFile));
					/* next parameter */
					i++; ok = true;
				}
			}
			if (strcmp(argv[i], "--configtest") == 0) {
				configtest = true;
				ok = true;
			}
			if (strcmp(argv[i], "--debug") == 0) {
				NGIRCd_Debug = true;
				ok = true;
			}
			if (strcmp(argv[i], "--help") == 0) {
				Show_Version();
				puts(""); Show_Help( ); puts( "" );
				exit(0);
			}
			if (strcmp(argv[i], "--nodaemon") == 0) {
				NGIRCd_NoDaemon = true;
				NGIRCd_NoSyslog = true;
				ok = true;
			}
			if (strcmp(argv[i], "--passive") == 0) {
				NGIRCd_Passive = true;
				ok = true;
			}
#ifdef SNIFFER
			if (strcmp(argv[i], "--sniffer") == 0) {
				NGIRCd_Sniffer = true;
				ok = true;
			}
#endif
#ifdef SYSLOG
			if (strcmp(argv[i], "--syslog") == 0) {
				NGIRCd_NoSyslog = false;
				ok = true;
			}
#endif
			if (strcmp(argv[i], "--version") == 0) {
				Show_Version();
				exit(0);
			}
		}
		else if(argv[i][0] == '-' && argv[i][1] != '-') {
			/* short option */
			for (n = 1; n < strlen(argv[i]); n++) {
				ok = false;
				if (argv[i][n] == 'd') {
					NGIRCd_Debug = true;
					ok = true;
				}
				if (argv[i][n] == 'f') {
					if (!argv[i][n+1] && i+1 < argc) {
						/* Ok, next character is a blank */
						strlcpy(NGIRCd_ConfFile, argv[i+1],
							sizeof(NGIRCd_ConfFile));

						/* go to the following parameter */
						i++;
						n = strlen(argv[i]);
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
					NGIRCd_NoSyslog = true;
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
#ifdef SYSLOG
				if (argv[i][n] == 'y') {
					NGIRCd_NoSyslog = false;
					ok = true;
				}
#endif

				if (!ok) {
					fprintf(stderr,
						"%s: invalid option \"-%c\"!\n",
						PACKAGE_NAME, argv[i][n]);
					fprintf(stderr,
						"Try \"%s --help\" for more information.\n",
						PACKAGE_NAME);
					exit(2);
				}
			}

		}
		if (!ok) {
			fprintf(stderr, "%s: invalid option \"%s\"!\n",
				PACKAGE_NAME, argv[i]);
			fprintf(stderr, "Try \"%s --help\" for more information.\n",
				PACKAGE_NAME);
			exit(2);
		}
	}

	/* Debug level for "VERSION" command */
	NGIRCd_DebugLevel[0] = '\0';
	if (NGIRCd_Debug)
		strcpy(NGIRCd_DebugLevel, "1");
#ifdef SNIFFER
	if (NGIRCd_Sniffer) {
		NGIRCd_Debug = true;
		strcpy(NGIRCd_DebugLevel, "2");
	}
#endif

	if (configtest) {
		Show_Version(); puts("");
		exit(Conf_Test());
	}

	while (!NGIRCd_SignalQuit) {
		/* Initialize global variables */
		NGIRCd_Start = time(NULL);
		(void)strftime(NGIRCd_StartStr, 64,
			       "%a %b %d %Y at %H:%M:%S (%Z)",
			       localtime(&NGIRCd_Start));

		NGIRCd_SignalRestart = false;
		NGIRCd_SignalQuit = false;

		Log_Init(!NGIRCd_NoSyslog);
		Random_Init();
		Conf_Init();
		Log_ReInit();

		/* Initialize the "main program":
		 * chroot environment, user and group ID, ... */
		if (!NGIRCd_Init(NGIRCd_NoDaemon)) {
			Log(LOG_ALERT, "Fatal: Initialization failed, exiting!");
			exit(1);
		}

		if (!io_library_init(CONNECTION_POOL)) {
			Log(LOG_ALERT,
			    "Fatal: Could not initialize IO routines: %s",
			    strerror(errno));
			exit(1);
		}

		if (!Signals_Init()) {
			Log(LOG_ALERT,
			    "Fatal: Could not set up signal handlers: %s",
			    strerror(errno));
			exit(1);
		}

		Channel_Init();
		Conn_Init();
		Class_Init();
		Client_Init();

		/* Create protocol and server identification. The syntax
		 * used by ngIRCd in PASS commands and the known "extended
		 * flags" are described in doc/Protocol.txt. */
#ifdef IRCPLUS
		snprintf(NGIRCd_ProtoID, sizeof NGIRCd_ProtoID, "%s%s %s|%s:%s",
			 PROTOVER, PROTOIRCPLUS, PACKAGE_NAME, PACKAGE_VERSION,
			 IRCPLUSFLAGS);
#ifdef ZLIB
		strlcat(NGIRCd_ProtoID, "Z", sizeof NGIRCd_ProtoID);
#endif
		if (Conf_OperCanMode)
			strlcat(NGIRCd_ProtoID, "o", sizeof NGIRCd_ProtoID);
#else /* IRCPLUS */
		snprintf(NGIRCd_ProtoID, sizeof NGIRCd_ProtoID, "%s%s %s|%s",
			 PROTOVER, PROTOIRC, PACKAGE_NAME, PACKAGE_VERSION);
#endif /* IRCPLUS */
		strlcat(NGIRCd_ProtoID, " P", sizeof NGIRCd_ProtoID);
#ifdef ZLIB
		strlcat(NGIRCd_ProtoID, "Z", sizeof NGIRCd_ProtoID);
#endif
		LogDebug("Protocol and server ID is \"%s\".", NGIRCd_ProtoID);

		Channel_InitPredefined();

		if (Conn_InitListeners() < 1) {
			Log(LOG_ALERT,
			    "Server isn't listening on a single port!" );
			Log(LOG_ALERT,
			    "%s exiting due to fatal errors!", PACKAGE_NAME);
			Pidfile_Delete();
			exit(1);
		}

		/* Main Run Loop */
		Conn_Handler();

		Conn_Exit();
		Client_Exit();
		Channel_Exit();
		Class_Exit();
		Log_Exit();
		Signals_Exit();
	}
	Pidfile_Delete();

	return 0;
} /* main */


/**
 * Generate ngIRCd "version strings".
 *
 * The ngIRCd version information is generated once and then stored in the
 * NGIRCd_Version and NGIRCd_VersionAddition string variables for further
 * usage, for example by the IRC command "VERSION" and the --version command
 * line switch.
 */
static void
Fill_Version(void)
{
	NGIRCd_VersionAddition[0] = '\0';

#ifdef ICONV
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof NGIRCd_VersionAddition);
	strlcat(NGIRCd_VersionAddition, "CHARCONV",
		sizeof NGIRCd_VersionAddition);
#endif
#ifdef DEBUG
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof NGIRCd_VersionAddition);
	strlcat(NGIRCd_VersionAddition, "DEBUG",
		sizeof NGIRCd_VersionAddition);
#endif
#ifdef IDENTAUTH
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof NGIRCd_VersionAddition);
	strlcat(NGIRCd_VersionAddition, "IDENT",
		sizeof NGIRCd_VersionAddition);
#endif
#ifdef WANT_IPV6
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof(NGIRCd_VersionAddition));
	strlcat(NGIRCd_VersionAddition, "IPv6",
		sizeof(NGIRCd_VersionAddition));
#endif
#ifdef IRCPLUS
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof NGIRCd_VersionAddition);
	strlcat(NGIRCd_VersionAddition, "IRCPLUS",
		sizeof NGIRCd_VersionAddition);
#endif
#ifdef PAM
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof NGIRCd_VersionAddition);
	strlcat(NGIRCd_VersionAddition, "PAM",
		sizeof NGIRCd_VersionAddition);
#endif
#ifdef STRICT_RFC
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof NGIRCd_VersionAddition);
	strlcat(NGIRCd_VersionAddition, "RFC",
		sizeof NGIRCd_VersionAddition);
#endif
#ifdef SNIFFER
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof NGIRCd_VersionAddition);
	strlcat(NGIRCd_VersionAddition, "SNIFFER",
		sizeof NGIRCd_VersionAddition);
#endif
#ifdef SSL_SUPPORT
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof NGIRCd_VersionAddition);
	strlcat(NGIRCd_VersionAddition, "SSL",
		sizeof NGIRCd_VersionAddition);
#endif
#ifdef SYSLOG
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof NGIRCd_VersionAddition);
	strlcat(NGIRCd_VersionAddition, "SYSLOG",
		sizeof NGIRCd_VersionAddition);
#endif
#ifdef TCPWRAP
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof NGIRCd_VersionAddition);
	strlcat(NGIRCd_VersionAddition, "TCPWRAP",
		sizeof NGIRCd_VersionAddition);
#endif
#ifdef ZLIB
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "+",
			sizeof NGIRCd_VersionAddition);
	strlcat(NGIRCd_VersionAddition, "ZLIB",
		sizeof NGIRCd_VersionAddition);
#endif
	if (NGIRCd_VersionAddition[0])
		strlcat(NGIRCd_VersionAddition, "-",
			sizeof(NGIRCd_VersionAddition));

	strlcat(NGIRCd_VersionAddition, HOST_CPU,
		sizeof(NGIRCd_VersionAddition));
	strlcat(NGIRCd_VersionAddition, "/", sizeof(NGIRCd_VersionAddition));
	strlcat(NGIRCd_VersionAddition, HOST_VENDOR,
		sizeof(NGIRCd_VersionAddition));
	strlcat(NGIRCd_VersionAddition, "/", sizeof(NGIRCd_VersionAddition));
	strlcat(NGIRCd_VersionAddition, HOST_OS,
		sizeof(NGIRCd_VersionAddition));

	snprintf(NGIRCd_Version, sizeof NGIRCd_Version, "%s %s-%s",
		 PACKAGE_NAME, PACKAGE_VERSION, NGIRCd_VersionAddition);
} /* Fill_Version */


/**
 * Display copyright and version information of ngIRCd on the console.
 */
static void
Show_Version( void )
{
	puts( NGIRCd_Version );
	puts( "Copyright (c)2001-2024 Alexander Barton (<alex@barton.de>) and Contributors." );
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
	puts( "  -d, --debug        log extra debug messages" );
	puts( "  -f, --config <f>   use file <f> as configuration file" );
	puts( "  -n, --nodaemon     don't fork and don't detach from controlling terminal" );
	puts( "  -p, --passive      disable automatic connections to other servers" );
#ifdef SNIFFER
	puts( "  -s, --sniffer      enable network sniffer and display all IRC traffic" );
#endif
	puts( "  -t, --configtest   read, validate and display configuration; then exit" );
	puts( "  -V, --version      output version information and exit" );
#ifdef SYSLOG
	puts( "  -y, --syslog       log to syslog even when running in the foreground (-n)" );
#endif
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

	LogDebug( "Removing PID file (%s) ...", Conf_PidFile );

	if( unlink( Conf_PidFile ))
		Log( LOG_ERR, "Error unlinking PID file (%s): %s", Conf_PidFile, strerror( errno ));
} /* Pidfile_Delete */


/**
 * Create the file containing the process ID of ngIRCd ("PID file").
 *
 * @param pid	The process ID to be stored in this file.
 */
static void
Pidfile_Create(pid_t pid)
{
	int pidfd;
	char pidbuf[64];
	int len;

	/* Pidfile configured? */
	if( ! Conf_PidFile[0] ) return;

	LogDebug( "Creating PID file (%s) ...", Conf_PidFile );

	pidfd = open( Conf_PidFile, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if ( pidfd < 0 ) {
		Log( LOG_ERR, "Error writing PID file (%s): %s", Conf_PidFile, strerror( errno ));
		return;
	}

	len = snprintf(pidbuf, sizeof pidbuf, "%ld\n", (long)pid);
	if (len < 0 || len >= (int)sizeof pidbuf) {
		Log(LOG_ERR, "Error converting process ID!");
		close(pidfd);
		return;
	}

	if (write(pidfd, pidbuf, (size_t)len) != (ssize_t)len)
		Log(LOG_ERR, "Can't write PID file (%s): %s!", Conf_PidFile,
		    strerror(errno));

	if (close(pidfd) != 0)
		Log(LOG_ERR, "Error closing PID file (%s): %s!", Conf_PidFile,
		    strerror(errno));
} /* Pidfile_Create */


/**
 * Redirect stdin, stdout and stderr to appropriate file handles.
 *
 * @param fd	The file handle stdin, stdout and stderr should be redirected to.
 */
static void
Setup_FDStreams(int fd)
{
	if (fd < 0)
		return;

	fflush(stdout);
	fflush(stderr);

	/* Create new stdin(0), stdout(1) and stderr(2) descriptors */
	dup2( fd, 0 ); dup2( fd, 1 ); dup2( fd, 2 );
} /* Setup_FDStreams */


#if !defined(SINGLE_USER_OS)

/**
 * Get user and group ID of unprivileged "nobody" user.
 *
 * @param uid	User ID
 * @param gid	Group ID
 * @return	true on success.
 */
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
	if (!pwd)
		return false;

	if (!pwd->pw_uid || !pwd->pw_gid)
		return false;

	*uid = pwd->pw_uid;
	*gid = pwd->pw_gid;
	endpwent();

	return true;
} /* NGIRCd_getNobodyID */

#endif


#ifdef HAVE_ARC4RANDOM
static void
Random_Init(void)
{

}
#else
static bool
Random_Init_Kern(const char *file)
{
	unsigned int seed;
	bool ret = false;
	int fd = open(file, O_RDONLY);
	if (fd >= 0) {
		if (read(fd, &seed, sizeof(seed)) == sizeof(seed))
			ret = true;
		close(fd);
		srand(seed);
	}
	return ret;
}

/**
 * Initialize libc rand(3) number generator
 */
static void
Random_Init(void)
{
	if (Random_Init_Kern("/dev/urandom"))
		return;
	if (Random_Init_Kern("/dev/random"))
		return;
	if (Random_Init_Kern("/dev/arandom"))
		return;
	srand(rand() ^ (unsigned)getpid() ^ (unsigned)time(NULL));
}
#endif


/**
 * Initialize ngIRCd daemon.
 *
 * @param NGIRCd_NoDaemon Set to true if ngIRCd should run in the
 *		foreground (and not as a daemon).
 * @return true on success.
 */
static bool
NGIRCd_Init(bool NGIRCd_NoDaemon)
{
	static bool initialized;
	bool chrooted = false;
	struct passwd *pwd;
	struct group *grp;
	int real_errno, fd = -1;
	pid_t pid;

	if (initialized)
		return true;

	if (!NGIRCd_NoDaemon) {
		/* open /dev/null before chroot() */
		fd = open( "/dev/null", O_RDWR);
		if (fd < 0)
			Log(LOG_WARNING, "Could not open /dev/null: %s",
			    strerror(errno));
	}

	/* SSL initialization */
	if (!ConnSSL_InitLibrary()) {
		Log(LOG_ERR, "Error during SSL initialization!");
		goto out;
	}

	/* Change root */
	if (Conf_Chroot[0]) {
		if (chdir(Conf_Chroot) != 0) {
			Log(LOG_ERR, "Can't chdir() in ChrootDir (%s): %s!",
			    Conf_Chroot, strerror(errno));
			goto out;
		}

		if (chroot(Conf_Chroot) != 0) {
			Log(LOG_ERR,
			    "Can't change root directory to \"%s\": %s!",
			    Conf_Chroot, strerror(errno));
			goto out;
		} else {
			chrooted = true;
			Log(LOG_INFO,
			    "Changed root and working directory to \"%s\".",
			    Conf_Chroot);
		}
	}

#if !defined(SINGLE_USER_OS)
	/* Check user ID */
	if (Conf_UID == 0) {
		pwd = getpwuid(0);
		Log(LOG_INFO,
		    "ServerUID must not be %s(0), using \"nobody\" instead.",
		    pwd ? pwd->pw_name : "?");
		if (!NGIRCd_getNobodyID(&Conf_UID, &Conf_GID)) {
			Log(LOG_WARNING,
			    "Could not get user/group ID of user \"nobody\": %s",
			    errno ? strerror(errno) : "not found" );
			goto out;
		}
	}

	/* Change group ID */
	if (getgid() != Conf_GID) {
		if (setgid(Conf_GID) != 0) {
			real_errno = errno;
			grp = getgrgid(Conf_GID);
			Log(LOG_ERR, "Can't change group ID to %s(%u): %s!",
			    grp ? grp->gr_name : "?", Conf_GID,
			    strerror(real_errno));
			if (real_errno != EPERM && real_errno != EINVAL)
				goto out;
		}
#ifdef HAVE_SETGROUPS
		if (setgroups(0, NULL) != 0) {
			real_errno = errno;
			Log(LOG_ERR, "Can't drop supplementary group IDs: %s!",
					strerror(errno));
			if (real_errno != EPERM)
				goto out;
		}
#else
		Log(LOG_WARNING,
		    "Can't drop supplementary group IDs: setgroups(3) missing!");
#endif
	}
#endif

	/* Change user ID */
	if (getuid() != Conf_UID) {
		if (setuid(Conf_UID) != 0) {
			real_errno = errno;
			pwd = getpwuid(Conf_UID);
			Log(LOG_ERR, "Can't change user ID to %s(%u): %s!",
			    pwd ? pwd->pw_name : "?", Conf_UID,
			    strerror(real_errno));
			if (real_errno != EPERM && real_errno != EINVAL)
				goto out;
		}
	}

	initialized = true;

	/* Normally a child process is forked which isn't any longer
	 * connected to the controlling terminal. Use "--nodaemon"
	 * to disable this "daemon mode" (useful for debugging). */
	if (!NGIRCd_NoDaemon) {
		pid = fork();
		if (pid > 0) {
			/* "Old" process: exit. */
			exit(0);
		}
		if (pid < 0) {
			/* Error!? */
			fprintf(stderr,
				"%s: Can't fork: %s!\nFatal error, exiting now ...\n",
				PACKAGE_NAME, strerror(errno));
			exit(1);
		}

		/* New child process */
#ifdef HAVE_SETSID
		(void)setsid();
#else
		setpgrp(0, getpid());
#endif
		if (chdir("/") != 0)
			Log(LOG_ERR, "Can't change directory to '/': %s!",
				     strerror(errno));

		/* Detach stdin, stdout and stderr */
		Setup_FDStreams(fd);
		if (fd > 2)
			close(fd);
	}
	pid = getpid();

	Pidfile_Create(pid);

	/* Check UID/GID we are running as, can be different from values
	 * configured (e. g. if we were already started with a UID>0. */
	Conf_UID = getuid();
	Conf_GID = getgid();

	pwd = getpwuid(Conf_UID);
	grp = getgrgid(Conf_GID);

	Log(LOG_INFO, "Running as user %s(%ld), group %s(%ld), with PID %ld.",
	    pwd ? pwd->pw_name : "unknown", (long)Conf_UID,
	    grp ? grp->gr_name : "unknown", (long)Conf_GID, (long)pid);

	if (chrooted) {
		Log(LOG_INFO, "Running with root directory \"%s\".",
		    Conf_Chroot );
		return true;
	} else
		Log(LOG_INFO, "Not running with changed root directory.");

	/* Change working directory to home directory of the user we are
	 * running as (only when running in daemon mode and not in chroot) */

	if (NGIRCd_NoDaemon)
		return true;

	if (pwd) {
		if (chdir(pwd->pw_dir) == 0)
			LogDebug(
			    "Changed working directory to \"%s\" ...",
			    pwd->pw_dir);
		else
			Log(LOG_ERR,
			    "Can't change working directory to \"%s\": %s!",
			    pwd->pw_dir, strerror(errno));
	} else
		Log(LOG_ERR, "Can't get user information for UID %d!?", Conf_UID);

	return true;
 out:
	if (fd > 2)
		close(fd);
	return false;
} /* NGIRCd_Init */


/* -eof- */
