/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2013 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Static configuration file for Mac OS X Xcode project
 */

#define PACKAGE_NAME "ngIRCd"
#define PACKAGE "ngircd"
#ifndef VERSION
#define VERSION "??("__DATE__")"
#endif
#define SYSCONFDIR "/etc/ngircd"
#define DOCDIR "/usr/share/doc/ngircd"

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

/* Define if zlib compression should be enabled */
#define ZLIB 1

/* Define if IPV6 protocol should be enabled */
#define WANT_IPV6 1

/* Define if PAM should be used */
#define PAM 1

/* Define if libiconv can be used, e.g. for CHARCONV */
#define ICONV 1

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
/* Define to 1 if you have the <netinet/ip.h> header file. */
#define HAVE_NETINET_IP_H 1

/* Define to 1 if you have the `gai_strerror' function. */
#define HAVE_GAI_STRERROR 1
/* Define to 1 if you have the `iconv_open' function. */
#define HAVE_ICONV_OPEN 1
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
/* Define to 1 if you have the `inet_aton' function. */
#define HAVE_INET_ATON 1
/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1
/* Define to 1 if you have the `getnameinfo' function. */
#define HAVE_GETNAMEINFO 1
/* Define to 1 if you have the `sigaction' function. */
#define HAVE_SIGACTION 1
/* Define to 1 if you have the `setsid' function. */
#define HAVE_SETSID 1

/* Define if socklen_t exists */
#define HAVE_socklen_t 1

#ifdef PAM
/* Define to 1 if you have the `pam_authenticate' function. */
#define HAVE_PAM_AUTHENTICATE 1
#if (__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 1060)
/* Define to 1 if you have the <pam/pam_appl.h> header file. */
#define HAVE_PAM_PAM_APPL_H 1
/* Mac OS X <10.6 doesn't have pam_fail_delay() */
#define NO_PAM_FAIL_DELAY 1
#else
/* Define to 1 if you have the <security/pam_appl.h> header file. */
#define HAVE_SECURITY_PAM_APPL_H 1
#endif
#endif

/* -eof- */
