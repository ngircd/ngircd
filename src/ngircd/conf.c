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
 * $Id: conf.c,v 1.4 2001/12/26 22:48:53 alex Exp $
 *
 * conf.h: Konfiguration des ngircd
 *
 * $Log: conf.c,v $
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

#include <exp.h>
#include "conf.h"


LOCAL VOID Read_Config( VOID );


GLOBAL VOID Conf_Init( VOID )
{
	/* Konfigurationsvariablen initialisieren: zunaechst Default-
	 * Werte setzen, dann Konfigurationsdtaei einlesen. */
	
	strcpy( Conf_File, "/usr/local/etc/ngircd.conf" );
	
	Conf_PingTimeout = 120;
	Conf_PongTimeout = 10;

	strcpy( Conf_MotdFile, "/usr/local/etc/ngircd.motd" );

	/* Konfigurationsdatei einlesen */
	Read_Config( );
} /* Config_Init */


GLOBAL VOID Conf_Exit( VOID )
{
	/* ... */
} /* Config_Exit */


LOCAL VOID Read_Config( VOID )
{
	/* Konfigurationsdatei einlesen. */
	
	/* ... */
} /* Read_Config */


/* -eof- */
