/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2003 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: portab.h,v 1.13 2003/03/17 00:53:52 alex Exp $
 *
 * Portability functions and declarations (header for libngbportab).
 */


#ifndef __PORTAB__
#define __PORTAB__


#include "config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


/* compiler features */

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


/* keywords */

#define EXTERN extern
#define STATIC static
#define LOCAL static
#define CONST const
#define REGISTER register


/* datatypes */

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


/* target constants  */

#ifndef TARGET_OS
#define TARGET_OS "unknown"
#endif

#ifndef TARGET_CPU
#define TARGET_CPU "unknown"
#endif

#ifndef TARGET_VENDOR
#define TARGET_VENDOR "unknown"
#endif


/* configure options */

#ifndef HAVE_socklen_t
#define socklen_t int			/* u.a. fuer Mac OS X */
#endif

#ifndef HAVE_SNPRINTF
EXTERN INT snprintf PARAMS(( CHAR *str, size_t count, CONST CHAR *fmt, ... ));
#endif

#ifndef HAVE_STRLCAT
EXTERN size_t strlcat PARAMS(( CHAR *dst, CONST CHAR *src, size_t size ));
#endif

#ifndef HAVE_STRLCPY
EXTERN size_t strlcpy PARAMS(( CHAR *dst, CONST CHAR *src, size_t size ));
#endif

#ifndef HAVE_VSNPRINTF
EXTERN INT vsnprintf PARAMS(( CHAR *str, size_t count, CONST CHAR *fmt, va_list args ));
#endif


#endif


/* -eof- */
