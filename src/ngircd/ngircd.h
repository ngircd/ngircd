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
 * $Id: ngircd.h,v 1.3 2001/12/30 11:42:00 alex Exp $
 *
 * ngircd.h: Prototypen aus dem "Haupt-Modul"
 *
 * $Log: ngircd.h,v $
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


GLOBAL time_t NGIRCd_Start;		/* Startzeitpunkt des Daemon */
GLOBAL CHAR NGIRCd_StartStr[64];

GLOBAL BOOLEAN NGIRCd_Quit;		/* TRUE: Hauptschleife beenden */


#endif


/* -eof- */
