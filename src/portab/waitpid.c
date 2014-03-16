/*
 * ngIRCd -- The Next Generation IRC Daemon
 */

#include "portab.h"

/**
 * @file
 * waitpid() implementation. Public domain.
 * Written by Steven D. Blackford for the NeXT system.
 */

#ifndef HAVE_WAITPID

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

GLOBAL int
waitpid(pid, stat_loc, options)
int pid, *stat_loc, options;
{
	for (;;) {
		int wpid = wait(stat_loc);
		if (wpid == pid || wpid == -1)
			return wpid;
	}
}

#endif
