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
 * $Id: ngircd.c,v 1.10 2001/12/24 01:34:38 alex Exp $
 *
 * ngircd.c: Hier beginnt alles ;-)
 *
 * $Log: ngircd.c,v $
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


GLOBAL INT main( INT argc, CONST CHAR *argv[] )
{
	/* Datentypen der portab-Library ueberpruefen */
	portab_check_types( );

	/* Globale Variablen initialisieren */
	NGIRCd_Quit = FALSE;

	/* Module initialisieren */
	Log_Init( );
	Conf_Init( );
	Parse_Init( );
	IRC_Init( );
	Channel_Init( );
	Client_Init( );
	Conn_Init( );

	Initialize_Signal_Handler( );
	
	if( ! Conn_New_Listener( 6668 )) exit( 1 );
	if( ! Conn_New_Listener( 6669 )) Log( LOG_WARNING, "Can't create second listening socket!" );
	
	/* Hauptschleife */
	while( ! NGIRCd_Quit )
	{
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
	
	return 0;
} /* main */


LOCAL VOID Initialize_Signal_Handler( VOID )
{
	/* Signal-Handler initialisieren: Strukturen anlegen und einhaengen :-) */

	struct sigaction saction;

	/* Signal-Struktur initialisieren */
	memset( &saction, 0, sizeof( saction ));

	/* Signal-Handler einhaengen */
	saction.sa_handler = Signal_Handler;
	sigaction( SIGINT, &saction, NULL );
	sigaction( SIGQUIT, &saction, NULL );
	sigaction( SIGTERM, &saction, NULL);

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
			Log( LOG_NOTICE, "Got signal %d, terminating now ...", Signal );
			NGIRCd_Quit = TRUE;
			break;
		default:
			/* unbekanntes bzw. unbehandeltes Signal */
			Log( LOG_NOTICE, "Got signal %d! Ignored.", Signal );
	}
} /* Signal_Handler */


/* -eof- */
