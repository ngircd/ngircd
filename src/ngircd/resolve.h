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
 * $Id: resolve.h,v 1.1 2002/05/27 11:23:27 alex Exp $
 *
 * resolve.h: asyncroner Resolver (Header)
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
