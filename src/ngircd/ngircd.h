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
 * $Id: ngircd.h,v 1.22 2005/06/24 19:20:56 fw Exp $
 *
 * Prototypes of the "main module".
 */


#ifndef __ngircd_h__
#define __ngircd_h__

#include <time.h>

#include "defines.h"

#define C_ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))


GLOBAL time_t NGIRCd_Start;		/* Startzeitpunkt des Daemon */
GLOBAL char NGIRCd_StartStr[64];
GLOBAL char NGIRCd_Version[126];
GLOBAL char NGIRCd_VersionAddition[126];

#ifdef DEBUG
GLOBAL bool NGIRCd_Debug;		/* Debug-Modus aktivieren */
#endif

#ifdef SNIFFER
GLOBAL bool NGIRCd_Sniffer;		/* Sniffer aktivieren */
#endif

GLOBAL bool NGIRCd_Passive;		/* nicht zu anderen Servern connecten */

GLOBAL bool NGIRCd_SignalQuit;	/* true: quit server*/
GLOBAL bool NGIRCd_SignalRestart;	/* true: restart server */

GLOBAL char NGIRCd_DebugLevel[2];	/* Debug-Level fuer IRC_VERSION() */

GLOBAL char NGIRCd_ConfFile[FNAME_LEN];	/* Konfigurationsdatei */

GLOBAL char NGIRCd_ProtoID[COMMAND_LEN];/* Protokoll- und Server-Identifikation */


#endif


/* -eof- */
