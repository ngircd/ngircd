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
 * $Id: portab.h,v 1.9 2002/12/12 11:26:08 alex Exp $
 *
 * portab.h: "Portabilitaets-Definitionen"
 */


#ifndef __PORTAB__
#define __PORTAB__


#include "config.h"


/* Compiler Features */

#ifdef __GNUC__
# define PUNUSED(x) __attribute__ ((unused)) x
# define UNUSED     __attribute__ ((unused))
#else
# define PUNUSED(x) x
# define UNUSED
#endif

#ifndef PARAMS
# if PROTOTYPES
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif
#endif


/* Keywords */

#define EXTERN extern
#define STATIC static
#define LOCAL static
#define CONST const
#define REGISTER register


/* Datatentypen */

#ifndef PROTOTYPES
# ifndef signed
#  define signed
# endif
#endif

typedef void VOID;
typedef void POINTER;

typedef signed int INT;
typedef unsigned int UINT;
typedef signed long LONG;
typedef unsigned long ULONG;

typedef signed char INT8;
typedef unsigned char UINT8;
typedef signed short INT16;
typedef unsigned short UINT16;
typedef signed long INT32;
typedef unsigned long UINT32;

typedef double DOUBLE;
typedef float FLOAT;

typedef char CHAR;

typedef UINT8 BOOLEAN;

#undef TRUE
#define TRUE (BOOLEAN)1

#undef FALSE
#define FALSE (BOOLEAN)0

#undef NULL
#ifdef PROTOTYPES
# define NULL (VOID *)0
#else
# define NULL 0L
#endif

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
