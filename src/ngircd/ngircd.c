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
 * $Id: ngircd.c,v 1.16 2002/01/02 02:44:37 alex Exp $
 *
 * ngircd.c: Hier beginnt alles ;-)
 *
 * $Log: ngircd.c,v $
 * Revision 1.16  2002/01/02 02:44:37  alex
 * - neue Defines fuer max. Anzahl Server und Operatoren.
 *
 * Revision 1.15  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.14  2001/12/30 19:26:12  alex
 * - Unterstuetzung fuer die Konfigurationsdatei eingebaut.
 *
 * Revision 1.13  2001/12/30 11:42:00  alex
 * - der Server meldet nun eine ordentliche "Start-Zeit".
 *
 * Revision 1.12  2001/12/29 03:07:36  alex
 * - einige Loglevel geaendert.
 *
 * Revision 1.11  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.10  2001/12/24 01:34:38  alex
 * - Signal-Handler aufgeraeumt; u.a. SIGPIPE wird nun korrekt ignoriert.
 *
 * Revision 1.9  2001/12/21 22:24:50  alex
 * - neues Modul "parse" wird initialisiert und abgemeldet.
 *
 * Revision 1.8  2001/12/14 08:15:26  alex
 * - neue Module (irc, client, channel) werden an- und abgemeldet.
 * - zweiter Listen-Socket wird zu Testzwecken konfiguriert.
 *
 * Revision 1.7  2001/12/13 01:31:46  alex
 * - Conn_Handler() wird nun mit einem Timeout aufgerufen.
 *
 * Revision 1.6  2001/12/12 23:30:42  alex
 * - Log-Meldungen an syslog angepasst.
 * - NGIRCd_Quit ist nun das Flag zum Beenden des ngircd.
 *
 * Revision 1.5  2001/12/12 17:21:21  alex
 * - mehr Unterfunktionen eingebaut, Modul besser strukturiert & dokumentiert.
 * - Anpassungen an neue Module.
 *
 * Revision 1.4  2001/12/12 01:58:53  alex
 * - Test auf socklen_t verbessert.
 *
 * Revision 1.3  2001/12/12 01:40:39  alex
 * - ein paar mehr Kommentare; Variablennamen verstaendlicher gemacht.
 * - fehlenden Header <arpa/inet.h> ergaenz.
 * - SIGINT und SIGQUIT werden nun ebenfalls behandelt.
 *
 * Revision 1.2  2001/12/11 22:04:21  alex
 * - Test auf stdint.h (HAVE_STDINT_H) hinzugefuegt.
 *
 * Revision 1.1.1.1  2001/12/11 21:53:04  alex
 * - Imported sources to CVS.
 */


#define PORTAB_CHECK_TYPES		/* Prueffunktion einbinden, s.u. */


#include <portab.h>
#include "global.h"

#include <imp.h>

#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "channel.h"
#include "client.h"
#include "conf.h"
#include "conn.h"
#include "irc.h"
#include "log.h"
#include "parse.h"

#include <exp.h>
#include "ngircd.h"


LOCAL VOID Initialize_Signal_Handler( VOID );
LOCAL VOID Signal_Handler( INT Signal );

LOCAL VOID Initialize_Listen_Ports( VOID );


GLOBAL INT main( INT argc, CONST CHAR *argv[] )
{
	/* Datentypen der portab-Library ueberpruefen */
	portab_check_types( );

	while( ! NGIRCd_Quit )
	{
		/* Globale Variablen initialisieren */
		NGIRCd_Start = time( NULL );
		strftime( NGIRCd_StartStr, 64, "%a %b %d %Y at %H:%M:%S (%Z)", localtime( &NGIRCd_Start ));
		NGIRCd_Restart = FALSE;
		NGIRCd_Quit = FALSE;

		/* Module initialisieren */
		Log_Init( );
		Conf_Init( );
		Parse_Init( );
		IRC_Init( );
		Channel_Init( );
		Client_Init( );
		Conn_Init( );

		/* Signal-Handler initialisieren */
		Initialize_Signal_Handler( );

		/* Listen-Ports initialisieren */
		Initialize_Listen_Ports( );

		/* Hauptschleife */
		while( TRUE )
		{
			if( NGIRCd_Quit || NGIRCd_Restart ) break;
			Conn_Handler( 5 );
		}

		/* Alles abmelden */
		Conn_Exit( );
		Client_Exit( );
		Channel_Exit( );
		IRC_Exit( );
		Parse_Exit( );
		Conf_Exit( );
		Log_Exit( );
	}
	return 0;
} /* main */


LOCAL VOID Initialize_Signal_Handler( VOID )
{
	/* Signal-Handler initialisieren: einige Signale
	 * werden ignoriert, andere speziell behandelt. */

	struct sigaction saction;

	/* Signal-Struktur initialisieren */
	memset( &saction, 0, sizeof( saction ));

	/* Signal-Handler einhaengen */
	saction.sa_handler = Signal_Handler;
	sigaction( SIGINT, &saction, NULL );
	sigaction( SIGQUIT, &saction, NULL );
	sigaction( SIGTERM, &saction, NULL);
	sigaction( SIGCHLD, &saction, NULL);

	/* einige Signale ignorieren */
	saction.sa_handler = SIG_IGN;
	sigaction( SIGPIPE, &saction, NULL );
} /* Initialize_Signal_Handler */


LOCAL VOID Signal_Handler( INT Signal )
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
			Log( LOG_WARNING, "Got signal %d, terminating now ...", Signal );
			NGIRCd_Quit = TRUE;
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


LOCAL VOID Initialize_Listen_Ports( VOID )
{
	/* Ports, auf denen der Server Verbindungen entgegennehmen
	 * soll, initialisieren */
	
	INT created, i;

	created = 0;
	for( i = 0; i < Conf_ListenPorts_Count; i++ )
	{
		if( Conn_NewListener( Conf_ListenPorts[i] )) created++;
		else Log( LOG_ERR, "Can't listen on port %d!", Conf_ListenPorts[i] );
	}

	if( created < 1 )
	{
		Log( LOG_ALERT, "Server isn't listening on a single port!" );
		Log( LOG_ALERT, PACKAGE" exiting due to fatal errors!" );
		exit( 1 );
	}
} /* Initialize_Listen_Ports */

/* -eof- */
