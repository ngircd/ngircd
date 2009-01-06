/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2009 Alexander Barton (alex@barton.de).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Static configuration file for Mac OS X Xcode project
 */

#define PACKAGE_NAME "ngircd"
#define VERSION "??"
#define SYSCONFDIR "/etc/ngircd"

#ifndef TARGET_VENDOR
#define TARGET_VENDOR "apple"
#define TARGET_OS "darwin"
#endif

/* -- Build options -- */

/* Define if debug-mode should be enabled */
#define DEBUG 1

/* Define if the server should do IDENT requests */
/*#define IDENTAUTH 1*/

/* Define if IRC+ protocol should be used */
#define IRCPLUS 1

/* Define if IRC sniffer should be enabled */
/*#define SNIFFER 1*/

/* Define if syslog should be used for logging */
#define SYSLOG 1

/* Define if TCP wrappers should be used */
/*#define TCPWRAP 1*/

/* Define if support for Zeroconf should be included */
/*#define ZEROCONF 1*/

/* Define if zlib compression should be enabled */
#define ZLIB 1

/* -- Supported features -- */

/* Define if SSP C support is enabled. */
#define ENABLE_SSP_CC 1

/* Define to 1 if the C compiler supports function prototypes. */
#define PROTOTYPES 1
/* Define like PROTOTYPES; this can be used by system headers. */
#define __PROTOTYPES 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1
/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1
/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1
/* Define to 1 if you have the <stdbool.h> header file. */
#define HAVE_STDBOOL_H 1
/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `kqueue' function. */
#define HAVE_KQUEUE 1
/* Define to 1 if you have the `inet_ntoa' function. */
#define HAVE_INET_NTOA 1
/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1
/* Define to 1 if you have the `strlcat' function. */
#define HAVE_STRLCAT 1
/* Define to 1 if you have the `strlcpy' function. */
#define HAVE_STRLCPY 1
/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1
/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define if socklen_t exists */
#define HAVE_socklen_t 1

#ifdef ZEROCONF
/* Define to 1 if you have the <DNSServiceDiscovery/DNSServiceDiscovery.h> header file. */
#define HAVE_DNSSERVICEDISCOVERY_DNSSERVICEDISCOVERY_H 1
/* Define to 1 if you have the `DNSServiceRegistrationCreate' function. */
#define HAVE_DNSSERVICEREGISTRATIONCREATE 1
#endif

/* -eof- */
