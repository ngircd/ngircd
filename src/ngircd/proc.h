/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2011 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __proc_h__
#define __proc_h__

/**
 * @file
 * Process management (header)
 */

/** Process status. This struct must not be accessed directly! */
typedef struct _Proc_Stat {
	pid_t pid;	/**< PID of the child process or 0 if none */
	int pipe_fd;	/**< Pipe file descriptor or -1 if none */
} PROC_STAT;

/** Return true if sub-process is still running */
#define Proc_InProgress(x)	((x)->pid != 0)

/** Return file descriptor of pipe to sub-process (or -1 if none open) */
#define Proc_GetPipeFd(x)	((x)->pipe_fd)

GLOBAL void Proc_InitStruct PARAMS((PROC_STAT *proc));

GLOBAL pid_t Proc_Fork PARAMS((PROC_STAT *proc, int *pipefds,
			       void (*cbfunc)(int, short), int timeout));

GLOBAL void Proc_GenericSignalHandler PARAMS((int Signal));

GLOBAL size_t Proc_Read PARAMS((PROC_STAT *proc, void *buffer, size_t buflen));

GLOBAL void Proc_Close PARAMS((PROC_STAT *proc));


#endif

/* -eof- */
