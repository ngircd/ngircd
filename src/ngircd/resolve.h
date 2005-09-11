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
 * $Id: resolve.h,v 1.10 2005/09/11 11:42:48 fw Exp $
 *
 * Asynchronous resolver (header)
 */


#ifndef __resolve_h__
#define __resolve_h__

#include "array.h"

#ifdef HAVE_SYS_SELECT_H
#	include <sys/select.h>
#endif
#include <sys/types.h>
#include <netinet/in.h>


typedef struct _Res_Stat
{
	int pid;			/* PID of resolver process */
	int pipe[2];			/* pipe for lookup result */
	int stage;			/* Hostname/IP(0) or IDENT(1)? */
	array buffer;			/* resolved hostname / ident result */
} RES_STAT;


#ifdef IDENTAUTH
GLOBAL RES_STAT *Resolve_Addr PARAMS(( struct sockaddr_in *Addr, int Sock ));
#else
GLOBAL RES_STAT *Resolve_Addr PARAMS(( struct sockaddr_in *Addr ));
#endif

GLOBAL RES_STAT *Resolve_Name PARAMS(( char *Host ));


#endif
/* -eof- */
