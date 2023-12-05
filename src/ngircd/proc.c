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

#include "portab.h"

/**
 * @file
 * Process management
 */

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "log.h"
#include "io.h"
#include "sighandlers.h"

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
Proc_Fork(PROC_STAT *proc, int *pipefds, void (*cbfunc)(int, short), int timeout)
{
	pid_t pid;
#ifndef HAVE_ARC4RANDOM
	unsigned int seed;
#endif

	assert(proc != NULL);
	assert(pipefds != NULL);
	assert(cbfunc != NULL);

	if (pipe(pipefds) != 0) {
		Log(LOG_ALERT, "Can't create output pipe for child process: %s!",
		    strerror(errno));
		return -1;
	}

#ifndef HAVE_ARC4RANDOM
	seed = (unsigned int)rand();
#endif
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
#ifdef HAVE_ARC4RANDOM_STIR
		arc4random_stir();
#endif
#ifndef HAVE_ARC4RANDOM
		srand(seed ^ (unsigned int)time(NULL) ^ getpid());
#endif
		Signals_Exit();
		signal(SIGTERM, Proc_GenericSignalHandler);
		signal(SIGALRM, Proc_GenericSignalHandler);
		close(pipefds[0]);
		alarm(timeout);
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
 * Generic signal handler for forked child processes.
 */
GLOBAL void
Proc_GenericSignalHandler(int Signal)
{
	switch(Signal) {
	case SIGTERM:
		Log_Subprocess(LOG_DEBUG, "Child got TERM signal, exiting.");
		exit(1);
	case SIGALRM:
		Log_Subprocess(LOG_DEBUG, "Child got ALARM signal, exiting.");
		exit(1);
	}
}

/**
 * Read bytes from a pipe of a forked child process.
 * In addition, this function makes sure that the child process is ignored
 * after all data has been read or a fatal error occurred.
 */
GLOBAL size_t
Proc_Read(PROC_STAT *proc, void *buffer, size_t buflen)
{
	ssize_t bytes_read = 0;

	assert(buffer != NULL);
	assert(buflen > 0);

	bytes_read = read(proc->pipe_fd, buffer, buflen);
	if (bytes_read < 0) {
		if (errno == EAGAIN)
			return 0;
		Log(LOG_CRIT, "Can't read from child process %ld: %s",
		    proc->pid, strerror(errno));
		Proc_Close(proc);
		bytes_read = 0;
	} else if (bytes_read == 0) {
		/* EOF: clean up */
		LogDebug("Child process %ld: EOF reached, closing pipe.",
		         proc->pid);
		Proc_Close(proc);
	}
	return (size_t)bytes_read;
}

/**
 * Close pipe to a forked child process.
 */
GLOBAL void
Proc_Close(PROC_STAT *proc)
{
	/* Close socket, if it exists */
	if (proc->pipe_fd >= 0)
		io_close(proc->pipe_fd);

	Proc_InitStruct(proc);
}

/* -eof- */
