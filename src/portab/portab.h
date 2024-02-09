/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2014 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __PORTAB__
#define __PORTAB__

/**
 * @file
 * Portability functions and declarations (header)
 */

#include "config.h"

/* remove assert() macro at compile time if DEBUG is not set. */

#ifndef DEBUG
# define NDEBUG
#endif

/* compiler features */

#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 7))
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

#include <sys/types.h>

#ifdef HAVE_STDDEF_H
# include <stddef.h>
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

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else
typedef unsigned char bool;
# define true (bool)1
# define false (bool)0
#endif

#ifndef NULL
# ifdef PROTOTYPES
#  define NULL (void *)0
# else
#  define NULL 0L
# endif
#endif

#ifdef NeXT
# define S_IRUSR 0000400		/* read permission, owner */
# define S_IWUSR 0000200		/* write permission, owner */
# define S_IRGRP 0000040		/* read permission, group */
# define S_IROTH 0000004		/* read permission, other */
# define ssize_t int
#endif

#undef GLOBAL
#ifdef GLOBAL_INIT
#define GLOBAL
#else
#define GLOBAL extern
#endif

/* target constants  */

#ifndef HOST_OS
# define HOST_OS "unknown"
#endif

#ifndef HOST_CPU
# define HOST_CPU "unknown"
#endif

#ifndef HOST_VENDOR
# define HOST_VENDOR "unknown"
#endif

#ifdef __HAIKU__
# define SINGLE_USER_OS
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

#ifndef HAVE_STRNDUP
extern char * strndup PARAMS((const char *s, size_t maxlen));
#endif

#ifndef HAVE_STRTOK_R
extern char * strtok_r PARAMS((char *str, const char *delim, char **saveptr));
#endif

#ifndef HAVE_VSNPRINTF
#include <stdarg.h>
extern int vsnprintf PARAMS(( char *str, size_t count, const char *fmt, va_list args ));
#endif

#ifndef HAVE_GAI_STRERROR
# define gai_strerror(r) "unknown error"
#endif

#ifndef PACKAGE_NAME
# define PACKAGE_NAME PACKAGE
#endif

#ifndef PACKAGE_VERSION
# define PACKAGE_VERSION VERSION
#endif

#ifndef SYSCONFDIR
# define SYSCONFDIR "/etc"
#endif

#endif

/* -eof- */
