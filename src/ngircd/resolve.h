/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * $Id: resolve.h,v 1.4 2002/12/28 15:01:45 alex Exp $
 *
 * Asynchronous resolver (header)
 */


#ifndef __resolve_h__
#define __resolve_h__


#include <sys/types.h>
#include <netinet/in.h>


typedef struct _Res_Stat
{
	INT pid;			/* PID des Child-Prozess */
	INT pipe[2];			/* Pipe fuer IPC */
} RES_STAT;


GLOBAL fd_set Resolver_FDs;


GLOBAL VOID Resolve_Init PARAMS(( VOID ));

GLOBAL RES_STAT *Resolve_Addr PARAMS(( struct sockaddr_in *Addr ));
GLOBAL RES_STAT *Resolve_Name PARAMS(( CHAR *Host ));


#endif


/* -eof- */
