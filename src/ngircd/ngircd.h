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
 * $Id: ngircd.h,v 1.8 2002/01/18 11:12:11 alex Exp $
 *
 * ngircd.h: Prototypen aus dem "Haupt-Modul"
 *
 * $Log: ngircd.h,v $
 * Revision 1.8  2002/01/18 11:12:11  alex
 * - der Sniffer wird nun nur noch aktiviert, wenn auf Kommandozeile angegeben.
 *
 * Revision 1.7  2002/01/11 14:45:18  alex
 * - Kommandozeilen-Parser implementiert: Debug- und No-Daemon-Modus, Hilfe.
 *
 * Revision 1.6  2002/01/02 02:44:37  alex
 * - neue Defines fuer max. Anzahl Server und Operatoren.
 *
 * Revision 1.5  2001/12/31 03:06:03  alex
 * - das #include fuer time.h hat noch gefehlt.
 *
 * Revision 1.4  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.3  2001/12/30 11:42:00  alex
 * - der Server meldet nun eine ordentliche "Start-Zeit".
 *
 * Revision 1.2  2001/12/12 23:30:01  alex
 * - NGIRCd_Quit ist nun das globale Flag zum Beenden des ngircd.
 *
 * Revision 1.1.1.1  2001/12/11 21:53:04  alex
 * - Imported sources to CVS.
 */


#ifndef __ngircd_h__
#define __ngircd_h__

#include <time.h>


GLOBAL time_t NGIRCd_Start;		/* Startzeitpunkt des Daemon */
GLOBAL CHAR NGIRCd_StartStr[64];

#ifdef DEBUG
GLOBAL BOOLEAN NGIRCd_Debug;		/* Debug-Modus aktivieren */
#endif

#ifdef SNIFFER
GLOBAL BOOLEAN NGIRCd_Sniffer;		/* Sniffer aktivieren */
#endif

GLOBAL BOOLEAN NGIRCd_NoDaemon;		/* nicht im Hintergrund laufen */

GLOBAL BOOLEAN NGIRCd_Quit;		/* TRUE: ngIRCd beenden */
GLOBAL BOOLEAN NGIRCd_Restart;		/* TRUE: neu starten */


GLOBAL CHAR *NGIRCd_Version( VOID );


#endif


/* -eof- */
