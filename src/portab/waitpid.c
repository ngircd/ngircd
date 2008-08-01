/*
 * ngIRCd -- The Next Generation IRC Daemon
 *
 * waitpid() implementation. Public domain.
 * Written by Steven D. Blackford for the NeXT system.
 *
 */

#include "portab.h"

#include "imp.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "exp.h"

#ifndef HAVE_WAITPID

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
