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
 * ngircd.c,v 1.55 2002/10/03 21:49:59 alex Exp
 *
 * ngircd.c: Hier beginnt alles ;-)
 */


#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

#include "resolve.h"
#include "conn.h"
#include "client.h"
#include "channel.h"
#include "conf.h"
#include "defines.h"
#include "lists.h"
#include "log.h"
#include "parse.h"
#include "irc.h"

#include "exp.h"
#include "ngircd.h"


LOCAL VOID Initialize_Signal_Handler PARAMS(( VOID ));
LOCAL VOID Signal_Handler PARAMS(( INT Signal ));

LOCAL VOID Initialize_Listen_Ports PARAMS(( VOID ));

LOCAL VOID Show_Version PARAMS(( VOID ));
LOCAL VOID Show_Help PARAMS(( VOID ));


GLOBAL int
main( int argc, const char *argv[] )
{
	BOOLEAN ok, configtest = FALSE;
	INT32 pid, n;
	INT i;

	umask( 0077 );

	NGIRCd_Restart = FALSE;
	NGIRCd_Quit = FALSE;
	NGIRCd_NoDaemon = FALSE;
	NGIRCd_Passive = FALSE;
#ifdef DEBUG
	NGIRCd_Debug = FALSE;
#endif
#ifdef SNIFFER
	NGIRCd_Sniffer = FALSE;
#endif
	strcpy( NGIRCd_ConfFile, CONFIG_FILE );

	/* Kommandozeile parsen */
	for( i = 1; i < argc; i++ )
	{
		ok = FALSE;
		if(( argv[i][0] == '-' ) && ( argv[i][1] == '-' ))
		{
			/* Lange Option */

			if( strcmp( argv[i], "--config" ) == 0 )
			{
				if( i + 1 < argc )
				{
					/* Ok, danach kommt noch ein Parameter */
					strncpy( NGIRCd_ConfFile, argv[i + 1], FNAME_LEN - 1 );
					NGIRCd_ConfFile[FNAME_LEN - 1] = '\0';

					/* zum uebernaechsten Parameter */
					i++; ok = TRUE;
				}
			}
			if( strcmp( argv[i], "--configtest" ) == 0 )
			{
				configtest = TRUE;
				ok = TRUE;
			}
#ifdef DEBUG
			if( strcmp( argv[i], "--debug" ) == 0 )
			{
				NGIRCd_Debug = TRUE;
				ok = TRUE;
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
				NGIRCd_NoDaemon = TRUE;
				ok = TRUE;
			}
			if( strcmp( argv[i], "--passive" ) == 0 )
			{
				NGIRCd_Passive = TRUE;
				ok = TRUE;
			}
#ifdef SNIFFER
			if( strcmp( argv[i], "--sniffer" ) == 0 )
			{
				NGIRCd_Sniffer = TRUE;
				ok = TRUE;
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
			/* Kurze Option */
			
			for( n = 1; n < (INT32)strlen( argv[i] ); n++ )
			{
				ok = FALSE;
#ifdef DEBUG
				if( argv[i][n] == 'd' )
				{
					NGIRCd_Debug = TRUE;
					ok = TRUE;
				}
#endif
				if( argv[i][n] == 'f' )
				{
					if(( ! argv[i][n + 1] ) && ( i + 1 < argc ))
					{
						/* Ok, danach kommt ein Leerzeichen */
						strncpy( NGIRCd_ConfFile, argv[i + 1], FNAME_LEN - 1 );
						NGIRCd_ConfFile[FNAME_LEN - 1] = '\0';

						/* zum uebernaechsten Parameter */
						i++; n = (INT32)strlen( argv[i] );
						ok = TRUE;
					}
				}
				if( argv[i][n] == 'n' )
				{
					NGIRCd_NoDaemon = TRUE;
					ok = TRUE;
				}
				if( argv[i][n] == 'p' )
				{
					NGIRCd_Passive = TRUE;
					ok = TRUE;
				}
#ifdef SNIFFER
				if( argv[i][n] == 's' )
				{
					NGIRCd_Sniffer = TRUE;
					ok = TRUE;
				}
#endif

				if( ! ok )
				{
					printf( "%s: invalid option \"-%c\"!\n", PACKAGE, argv[i][n] );
					printf( "Try \"%s --help\" for more information.\n", PACKAGE );
					exit( 1 );
				}
			}

		}
		if( ! ok )
		{
			printf( "%s: invalid option \"%s\"!\n", PACKAGE, argv[i] );
			printf( "Try \"%s --help\" for more information.\n", PACKAGE );
			exit( 1 );
		}
	}

	/* Debug-Level (fuer IRC-Befehl "VERSION") ermitteln */
	strcpy( NGIRCd_DebugLevel, "" );
#ifdef DEBUG
	if( NGIRCd_Debug ) strcpy( NGIRCd_DebugLevel, "1" );
#endif
#ifdef SNIFFER
	if( NGIRCd_Sniffer )
	{
		NGIRCd_Debug = TRUE;
		strcpy( NGIRCd_DebugLevel, "2" );
	}
#endif

	/* Soll nur die Konfigurations ueberprueft und ausgegeben werden? */
	if( configtest )
	{
		Show_Version( ); puts( "" );
		exit( Conf_Test( ));
	}
	
	while( ! NGIRCd_Quit )
	{
		/* In der Regel wird ein Sub-Prozess ge-fork()'t, der
		 * nicht mehr mit dem Terminal verbunden ist. Mit der
		 * Option "--nodaemon" kann dies (z.B. zum Debuggen)
		 * verhindert werden. */
		if( ! NGIRCd_NoDaemon )
		{
			/* Daemon im Hintergrund erzeugen */
			pid = (INT32)fork( );
			if( pid > 0 )
			{
				/* "alter" Prozess */
				exit( 0 );
			}
			if( pid < 0 )
			{
				/* Fehler */
				printf( "%s: Can't fork: %s!\nFatal error, exiting now ...\n", PACKAGE, strerror( errno ));
				exit( 1 );
			}

			/* Child-Prozess initialisieren */
			(VOID)setsid( );
			chdir( "/" );
		}
	
		/* Globale Variablen initialisieren */
		NGIRCd_Start = time( NULL );
		(VOID)strftime( NGIRCd_StartStr, 64, "%a %b %d %Y at %H:%M:%S (%Z)", localtime( &NGIRCd_Start ));
		NGIRCd_Restart = FALSE;
		NGIRCd_Quit = FALSE;

		/* Module initialisieren */
		Log_Init( );
		Resolve_Init( );
		Conf_Init( );
		Lists_Init( );
		Channel_Init( );
		Client_Init( );
		Conn_Init( );

		/* Wenn als root ausgefuehrt und eine andere UID
		 * konfiguriert ist, jetzt zu dieser wechseln */
		if( getuid( ) == 0 )
		{
			if( Conf_GID != 0 )
			{
				/* Neue Group-ID setzen */
				if( setgid( Conf_GID ) != 0 ) Log( LOG_ERR, "Can't change Group-ID to %u: %s", Conf_GID, strerror( errno ));
			}
			if( Conf_UID != 0 )
			{
				/* Neue User-ID setzen */
				if( setuid( Conf_UID ) != 0 ) Log( LOG_ERR, "Can't change User-ID to %u: %s", Conf_UID, strerror( errno ));
			}
		}
		Log( LOG_INFO, "Running as user %ld, group %ld, with PID %ld.", (INT32)getuid( ), (INT32)getgid( ), (INT32)getpid( ));

		Log_InitErrorfile( );

		/* Signal-Handler initialisieren */
		Initialize_Signal_Handler( );

		/* Protokoll- und Server-Identifikation erzeugen. Die vom ngIRCd
		 * beim PASS-Befehl verwendete Syntax sowie die erweiterten Flags
		 * sind in doc/Protocol.txt beschrieben. */
#ifdef IRCPLUS
		sprintf( NGIRCd_ProtoID, "%s%s %s|%s:%s", PROTOVER, PROTOIRCPLUS, PACKAGE, VERSION, IRCPLUSFLAGS );
		if( Conf_OperCanMode ) strcat( NGIRCd_ProtoID, "o" );
#else
		sprintf( NGIRCd_ProtoID, "%s%s %s|%s", PROTOVER, PROTOIRC, PACKAGE, VERSION );
#endif
		strcat( NGIRCd_ProtoID, " P" );
		Log( LOG_DEBUG, "Protocol and server ID is \"%s\".", NGIRCd_ProtoID );

		/* Vordefinierte Channels anlegen */
		Channel_InitPredefined( );

		/* Listen-Ports initialisieren */
		Initialize_Listen_Ports( );

		/* Hauptschleife */
		Conn_Handler( );

		/* Alles abmelden */
		Conn_Exit( );
		Client_Exit( );
		Channel_Exit( );
		Lists_Exit( );
		Log_Exit( );
	}

	return 0;
} /* main */


GLOBAL CHAR *
NGIRCd_Version( VOID )
{
	STATIC CHAR version[126];

	sprintf( version, "%s %s-%s", PACKAGE, VERSION, NGIRCd_VersionAddition( ));
	return version;
} /* NGIRCd_Version */


GLOBAL CHAR *
NGIRCd_VersionAddition( VOID )
{
	STATIC CHAR txt[64];

	strcpy( txt, "" );

#ifdef USE_SYSLOG
	if( txt[0] ) strcat( txt, "+" );
	strcat( txt, "SYSLOG" );
#endif
#ifdef DEBUG
	if( txt[0] ) strcat( txt, "+" );
	strcat( txt, "DEBUG" );
#endif
#ifdef SNIFFER
	if( txt[0] ) strcat( txt, "+" );
	strcat( txt, "SNIFFER" );
#endif
#ifdef STRICT_RFC
	if( txt[0] ) strcat( txt, "+" );
	strcat( txt, "RFC" );
#endif
#ifdef IRCPLUS
	if( txt[0] ) strcat( txt, "+" );
	strcat( txt, "IRCPLUS" );
#endif
	
	if( txt[0] ) strcat( txt, "-" );
	strcat( txt, TARGET_CPU );
	strcat( txt, "/" );
	strcat( txt, TARGET_VENDOR );
	strcat( txt, "/" );
	strcat( txt, TARGET_OS );

	return txt;
} /* NGIRCd_VersionAddition */


LOCAL VOID
Initialize_Signal_Handler( VOID )
{
	/* Signal-Handler initialisieren: einige Signale
	 * werden ignoriert, andere speziell behandelt. */

#ifdef HAVE_SIGACTION
	/* sigaction() ist vorhanden */

	struct sigaction saction;

	/* Signal-Struktur initialisieren */
	memset( &saction, 0, sizeof( saction ));
	saction.sa_handler = Signal_Handler;
#ifdef SA_RESTART
	saction.sa_flags |= SA_RESTART;
#endif
#ifdef SA_NOCLDWAIT
	saction.sa_flags |= SA_NOCLDWAIT;
#endif

	/* Signal-Handler einhaengen */
	sigaction( SIGINT, &saction, NULL );
	sigaction( SIGQUIT, &saction, NULL );
	sigaction( SIGTERM, &saction, NULL);
	sigaction( SIGHUP, &saction, NULL);
	sigaction( SIGCHLD, &saction, NULL);

	/* einige Signale ignorieren */
	saction.sa_handler = SIG_IGN;
	sigaction( SIGPIPE, &saction, NULL );
#else
	/* kein sigaction() vorhanden */

	/* Signal-Handler einhaengen */
	signal( SIGINT, Signal_Handler );
	signal( SIGQUIT, Signal_Handler );
	signal( SIGTERM, Signal_Handler );
	signal( SIGHUP, Signal_Handler );
	signal( SIGCHLD, Signal_Handler );

	/* einige Signale ignorieren */
	signal( SIGPIPE, SIG_IGN );
#endif
} /* Initialize_Signal_Handler */


LOCAL VOID
Signal_Handler( INT Signal )
{
	/* Signal-Handler. Dieser wird aufgerufen, wenn eines der Signale eintrifft,
	 * fuer das wir uns registriert haben (vgl. Initialize_Signal_Handler). Die
	 * Nummer des eingetroffenen Signals wird der Funktion uebergeben. */

	switch( Signal )
	{
		case SIGTERM:
		case SIGINT:
		case SIGQUIT:
			/* wir soll(t)en uns wohl beenden ... */
			if( Signal == SIGTERM ) Log( LOG_WARNING, "Got TERM signal, terminating now ..." );
			else if( Signal == SIGINT ) Log( LOG_WARNING, "Got INT signal, terminating now ..." );
			else if( Signal == SIGQUIT ) Log( LOG_WARNING, "Got QUIT signal, terminating now ..." );
			NGIRCd_Quit = TRUE;
			break;
		case SIGHUP:
			/* neu starten */
			Log( LOG_WARNING, "Got HUP signal, restarting now ..." );
			NGIRCd_Restart = TRUE;
			break;
		case SIGCHLD:
			/* Child-Prozess wurde beendet. Zombies vermeiden: */
			while( waitpid( -1, NULL, WNOHANG ) > 0);
			break;
		default:
			/* unbekanntes bzw. unbehandeltes Signal */
			Log( LOG_NOTICE, "Got signal %d! Ignored.", Signal );
	}
} /* Signal_Handler */


LOCAL VOID
Initialize_Listen_Ports( VOID )
{
	/* Ports, auf denen der Server Verbindungen entgegennehmen
	 * soll, initialisieren */
	
	INT created, i;

	created = 0;
	for( i = 0; i < Conf_ListenPorts_Count; i++ )
	{
		if( Conn_NewListener( Conf_ListenPorts[i] )) created++;
		else Log( LOG_ERR, "Can't listen on port %u!", Conf_ListenPorts[i] );
	}

	if( created < 1 )
	{
		Log( LOG_ALERT, "Server isn't listening on a single port!" );
		Log( LOG_ALERT, "%s exiting due to fatal errors!", PACKAGE );
		exit( 1 );
	}
} /* Initialize_Listen_Ports */


LOCAL VOID
Show_Version( VOID )
{
	puts( NGIRCd_Version( ));
	puts( "Copyright (c)2001,2002 by Alexander Barton (<alex@barton.de>)." );
	puts( "Homepage: <http://arthur.ath.cx/~alex/ngircd/>\n" );
	puts( "This is free software; see the source for copying conditions. There is NO" );
	puts( "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." );
} /* Show_Version */


LOCAL VOID
Show_Help( VOID )
{
#ifdef DEBUG
	puts( "  -d, --debug        log extra debug messages" );
#endif
	puts( "  -f, --config <f>   use file <f> as configuration file" );
        puts( "  -n, --nodaemon     don't fork and don't detatch from controlling terminal" );
        puts( "  -p, --passive      disable automatic connections to other servers" );
#ifdef SNIFFER
	puts( "  -s, --sniffer      enable network sniffer and display all IRC traffic" );
#endif
	puts( "      --configtest   read, validate and display configuration; then exit" );
 	puts( "      --version      output version information and exit" );
	puts( "      --help         display this help and exit" );
} /* Show_Help */


/* -eof- */
