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
 * $Id: resolve.h,v 1.7 2004/05/11 00:01:11 alex Exp $
 *
 * Asynchronous resolver (header)
 */


#ifndef __resolve_h__
#define __resolve_h__


#ifdef HAVE_SYS_SELECT_H
#	include <sys/select.h>
#endif
#include <sys/types.h>
#include <netinet/in.h>


typedef struct _Res_Stat
{
	INT pid;			/* PID des Child-Prozess */
	INT pipe[2];			/* Pipe fuer IPC */
	INT stage;			/* Hostname/IP(0) or IDENT(1)? */
	INT bufpos;			/* Position in buffer */
	CHAR buffer[HOST_LEN];		/* Buffer */
} RES_STAT;


GLOBAL fd_set Resolver_FDs;


GLOBAL VOID Resolve_Init PARAMS(( VOID ));

#ifdef IDENTAUTH
GLOBAL RES_STAT *Resolve_Addr PARAMS(( struct sockaddr_in *Addr, int Sock ));
#else
GLOBAL RES_STAT *Resolve_Addr PARAMS(( struct sockaddr_in *Addr ));
#endif

GLOBAL RES_STAT *Resolve_Name PARAMS(( CHAR *Host ));


#endif


/* -eof- */
