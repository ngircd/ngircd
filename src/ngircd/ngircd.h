/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: ngircd.h,v 1.18 2002/12/19 04:30:00 alex Exp $
 *
 * Prototypes of the "main module".
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

GLOBAL BOOLEAN NGIRCd_SignalQuit;	/* TRUE: quit server*/
GLOBAL BOOLEAN NGIRCd_SignalRestart;	/* TRUE: restart server */
GLOBAL BOOLEAN NGIRCd_SignalRehash;	/* TRUE: reload configuration */

GLOBAL CHAR NGIRCd_DebugLevel[2];	/* Debug-Level fuer IRC_VERSION() */

GLOBAL CHAR NGIRCd_ConfFile[FNAME_LEN];	/* Konfigurationsdatei */

GLOBAL CHAR NGIRCd_ProtoID[1024];	/* Protokoll- und Server-Identifikation */


GLOBAL CHAR *NGIRCd_Version PARAMS((VOID ));
GLOBAL CHAR *NGIRCd_VersionAddition PARAMS((VOID ));

GLOBAL VOID NGIRCd_Rehash PARAMS(( VOID ));


#endif


/* -eof- */
