/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton (alex@barton.de).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __ngircd_h__
#define __ngircd_h__

/**
 * @file
 * Global variables of ngIRCd.
 */

#include <time.h>

#include "defines.h"

#define C_ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))

/** UNIX timestamp of ngIRCd start */
GLOBAL time_t NGIRCd_Start;

/** ngIRCd start time as string, used for RPL_CREATED_MSG (003) */
GLOBAL char NGIRCd_StartStr[64];

/** ngIRCd version number containing release number and compile-time options */
GLOBAL char NGIRCd_Version[126];

/** String specifying the compile-time options and target platform */
GLOBAL char NGIRCd_VersionAddition[126];

/** Flag indicating if debug mode is active (true) or not (false) */
GLOBAL bool NGIRCd_Debug;

#ifdef SNIFFER
/** Flag indication if sniffer is active (true) or not (false) */
GLOBAL bool NGIRCd_Sniffer;
#endif

/**
 * Flag indicating if NO outgoing connections should be established (true)
 * or not (false, the default)
 */
GLOBAL bool NGIRCd_Passive;

/** Flag indicating that ngIRCd has been requested to quit (true) */
GLOBAL bool NGIRCd_SignalQuit;

/** Flag indicating that ngIRCd has been requested to restart (true) */
GLOBAL bool NGIRCd_SignalRestart;

/**
 * Debug level for "VERSION" command, see description of numeric RPL_VERSION
 * (351) in RFC 2812. ngIRCd sets debuglevel to 1 when the debug mode is
 * active, and to 2 if the sniffer is running.
 */
GLOBAL char NGIRCd_DebugLevel[2];

/** Full path and file name of current configuration file */
GLOBAL char NGIRCd_ConfFile[FNAME_LEN];

/** Protocol and server identification string; see doc/Protocol.txt */
GLOBAL char NGIRCd_ProtoID[COMMAND_LEN];

#endif

/* -eof- */
