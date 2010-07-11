/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2010 Alexander Barton (alex@barton.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 *
 * Process management
 */

#include "portab.h"

#include "imp.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
#include "io.h"

#include "exp.h"
#include "proc.h"

/**
 * Initialize process structure.
 */
GLOBAL void
Proc_InitStruct (PROC_STAT *proc)
{
	assert(proc != NULL);
	proc->pid = 0;
	proc->pipe_fd = -1;
}

/**
 * Fork a child process.
 */
GLOBAL pid_t
Proc_Fork(PROC_STAT *proc, int *pipefds, void (*cbfunc)(int, short))
{
	pid_t pid;

	assert(proc != NULL);
	assert(pipefds != NULL);
	assert(cbfunc != NULL);

	if (pipe(pipefds) != 0) {
		Log(LOG_ALERT, "Can't create output pipe for child process: %s!",
		    strerror(errno));
		return -1;
	}

	pid = fork();
	switch (pid) {
	case -1:
		/* Error on fork: */
		Log(LOG_CRIT, "Can't fork child process: %s!", strerror(errno));
		close(pipefds[0]);
		close(pipefds[1]);
		return -1;
	case 0:
		/* New child process: */
		close(pipefds[0]);
		return 0;
	}

	/* Old parent process: */
	close(pipefds[1]);

	if (!io_setnonblock(pipefds[0])
	 || !io_event_create(pipefds[0], IO_WANTREAD, cbfunc)) {
		Log(LOG_CRIT, "Can't register callback for child process: %s!",
		    strerror(errno));
		close(pipefds[0]);
		return -1;
	}

	proc->pid = pid;
	proc->pipe_fd = pipefds[0];
	return pid;
}

/**
 * Kill forked child process.
 */
GLOBAL void
Proc_Kill(PROC_STAT *proc)
{
	assert(proc != NULL);

	if (proc->pipe_fd > 0)
		io_close(proc->pipe_fd);
	if (proc->pid > 0)
		kill(proc->pid, SIGTERM);
	Proc_InitStruct(proc);
}

/**
 * Generic signal handler for forked child processes.
 */
GLOBAL void
Proc_GenericSignalHandler(int Signal)
{
	switch(Signal) {
	case SIGTERM:
#ifdef DEBUG
		Log_Subprocess(LOG_DEBUG, "Child got TERM signal, exiting.");
#endif
		exit(1);
	}
}

/* -eof- */
