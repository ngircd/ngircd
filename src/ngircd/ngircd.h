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
 * $Id: ngircd.h,v 1.15 2002/11/22 17:59:54 alex Exp $
 *
 * ngircd.h: Prototypen aus dem "Haupt-Modul"
 */


#ifndef __ngircd_h__
#define __ngircd_h__

#include <time.h>

#include "defines.h"


GLOBAL time_t NGIRCd_Start;		/* Startzeitpunkt des Daemon */
GLOBAL CHAR NGIRCd_StartStr[64];

#ifdef DEBUG
GLOBAL BOOLEAN NGIRCd_Debug;		/* Debug-Modus aktivieren */
#endif

#ifdef SNIFFER
GLOBAL BOOLEAN NGIRCd_Sniffer;		/* Sniffer aktivieren */
#endif

GLOBAL BOOLEAN NGIRCd_NoDaemon;		/* nicht im Hintergrund laufen */

GLOBAL BOOLEAN NGIRCd_Passive;		/* nicht zu anderen Servern connecten */

GLOBAL BOOLEAN NGIRCd_Quit;		/* TRUE: ngIRCd beenden */
GLOBAL BOOLEAN NGIRCd_Restart;		/* TRUE: neu starten */

GLOBAL CHAR NGIRCd_DebugLevel[2];	/* Debug-Level fuer IRC_VERSION() */

GLOBAL CHAR NGIRCd_ConfFile[FNAME_LEN];	/* Konfigurationsdatei */

GLOBAL CHAR NGIRCd_ProtoID[1024];	/* Protokoll- und Server-Identifikation */


GLOBAL CHAR *NGIRCd_Version PARAMS((VOID ));
GLOBAL CHAR *NGIRCd_VersionAddition PARAMS((VOID ));

GLOBAL VOID NGIRCd_Reload PARAMS(( VOID ));


#endif


/* -eof- */
