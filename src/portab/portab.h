/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2005 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: portab.h,v 1.22 2005/07/31 20:13:11 alex Exp $
 *
 * Portability functions and declarations (header for libngbportab).
 */


#ifndef __PORTAB__
#define __PORTAB__


#include "config.h"

#ifndef DEBUG
# define NDEBUG
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
# define NGIRC_GOT_INTTYPES
#else
# ifdef HAVE_STDINT_H
#  include <stdint.h>
#  define NGIRC_GOT_INTTYPES
# endif
#endif

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#endif

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#endif

/* compiler features */

#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7))
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


/* datatypes */

#ifndef PROTOTYPES
# ifndef signed
#  define signed
# endif
#endif

typedef void POINTER;

#ifdef NGIRC_GOT_INTTYPES
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
#else
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
#endif

#ifndef HAVE_STDBOOL_H
typedef unsigned char bool;
#define true (bool)1
#define false (bool)0
#endif

#ifndef NULL
#ifdef PROTOTYPES
# define NULL (void *)0
#else
# define NULL 0L
#endif
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
typedef int socklen_t;			/* for Mac OS X, amongst others */
#endif

#ifndef HAVE_SNPRINTF
extern int snprintf PARAMS(( char *str, size_t count, const char *fmt, ... ));
#endif

#ifndef HAVE_STRLCAT
extern size_t strlcat PARAMS(( char *dst, const char *src, size_t size ));
#endif

#ifndef HAVE_STRLCPY
extern size_t strlcpy PARAMS(( char *dst, const char *src, size_t size ));
#endif

#ifndef HAVE_STRDUP
extern char * strdup PARAMS(( const char *s ));
#endif

#ifndef HAVE_VSNPRINTF
#include <stdarg.h>
extern int vsnprintf PARAMS(( char *str, size_t count, const char *fmt, va_list args ));
#endif

#ifndef PACKAGE_NAME
#define PACKAGE_NAME PACKAGE
#endif

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION VERSION
#endif


#endif


/* -eof- */
