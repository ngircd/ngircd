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
 * $Id: portab.h,v 1.3 2002/03/25 19:13:19 alex Exp $
 *
 * portab.h: "Portabilitaets-Definitionen"
 */


#ifndef __PORTAB__
#define __PORTAB__


#include "config.h"


/* Keywords */

#define EXTERN extern
#define STATIC static
#define LOCAL static
#define CONST const


/* Datatentypen */

typedef void VOID;
typedef void POINTER;

typedef signed int INT;
typedef unsigned int UINT;
typedef signed char INT8;
typedef unsigned char UINT8;
typedef signed short INT16;
typedef unsigned short UINT16;
typedef signed long INT32;
typedef unsigned long UINT32;

typedef float FLOAT;

typedef char CHAR;

typedef UINT8 BOOLEAN;

#undef TRUE
#define TRUE (BOOLEAN)1

#undef FALSE
#define FALSE (BOOLEAN)0

#undef NULL
#define NULL (VOID *)0

#undef GLOBAL
#define GLOBAL


/* SPLint */


#ifdef S_SPLINT_S
#include "splint.h"
#endif


/* configure-Optionen */

#ifndef HAVE_socklen_t
#define socklen_t int			/* u.a. fuer Mac OS X */
#endif

#ifndef HAVE_INET_ATON
#define inet_aton( opt, bind ) 0	/* Dummy fuer inet_aton() */
#endif

#if OS_UNIX_AUX
#define _POSIX_SOURCE			/* muss unter A/UX definiert sein */
#endif


/* Konstanten */

#ifndef TARGET_OS
#define TARGET_OS "unknown"
#endif

#ifndef TARGET_CPU
#define TARGET_CPU "unknown"
#endif

#ifndef TARGET_VENDOR
#define TARGET_VENDOR "unknown"
#endif


#endif


/* -eof- */
