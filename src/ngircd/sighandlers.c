/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2015 Alexander Barton (alex@barton.de) and Contributors.
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
 * Signal Handlers: Actions to be performed when the program
 * receives a signal.
 */

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "conn.h"
#include "channel.h"
#include "conf.h"
#include "io.h"
#include "log.h"
#include "ngircd.h"

#include "sighandlers.h"

static int signalpipe[2];

static const int signals_catch[] = {
       SIGINT, SIGQUIT, SIGTERM, SIGHUP, SIGCHLD, SIGUSR1, SIGUSR2
};

#ifdef DEBUG

static void
Dump_State(void)
{
	Log(LOG_DEBUG, "--- Internal server state: %s ---",
	    Client_ID(Client_ThisServer()));
#ifdef HAVE_LONG_LONG
	Log(LOG_DEBUG, "time()=%llu", (unsigned long long)time(NULL));
#else
	Log(LOG_DEBUG, "time()=%lu", (unsigned long)time(NULL));
#endif
	Conf_DebugDump();
	Conn_DebugDump();
	Client_DebugDump();
	Log(LOG_DEBUG, "--- End of state dump ---");
} /* Dump_State */

#endif

static void
Signal_Block(int sig)
{
#ifdef HAVE_SIGPROCMASK
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, sig);

	sigprocmask(SIG_BLOCK, &set, NULL);
#else
	sigblock(sig);
#endif
}

static void
Signal_Unblock(int sig)
{
#ifdef HAVE_SIGPROCMASK
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, sig);

	sigprocmask(SIG_UNBLOCK, &set, NULL);
#else
	int old = sigblock(0) & ~sig;
	sigsetmask(old);
#endif
}

/**
 * Reload the server configuration file.
 */
static void
Rehash(void)
{
	char old_name[CLIENT_ID_LEN];
	unsigned old_nicklen;

	Log( LOG_NOTICE|LOG_snotice, "Re-reading configuration NOW!" );

	/* Remember old server name and nickname length */
	strlcpy( old_name, Conf_ServerName, sizeof old_name );
	old_nicklen = Conf_MaxNickLength;

	/* Re-read configuration ... */
	if (!Conf_Rehash( ))
		return;

	/* Close down all listening sockets */
	Conn_ExitListeners( );

	/* Recover old server name and nickname length: these values can't
	 * be changed during run-time */
	if (strcmp(old_name, Conf_ServerName) != 0 ) {
		strlcpy(Conf_ServerName, old_name, sizeof Conf_ServerName);
		Log(LOG_ERR,
		    "Can't change server name (\"Name\") on runtime! Ignored new name.");
	}
	if (old_nicklen != Conf_MaxNickLength) {
		Conf_MaxNickLength = old_nicklen;
		Log(LOG_ERR,
		    "Can't change \"MaxNickLength\" on runtime! Ignored new value.");
	}

	/* Create new pre-defined channels */
	Channel_InitPredefined( );

	if (!ConnSSL_InitLibrary())
		Log(LOG_WARNING,
		    "Re-Initializing of SSL failed, using old keys!");

	/* Start listening on sockets */
	Conn_InitListeners( );

	/* Sync configuration with established connections */
	Conn_SyncServerStruct( );

	Log( LOG_NOTICE|LOG_snotice, "Re-reading of configuration done." );
} /* Rehash */

/**
 * Signal handler of ngIRCd.
 * This function is called whenever ngIRCd catches a signal sent by the
 * user and/or the system to it. For example SIGTERM and SIGHUP.
 *
 * It blocks the signal and queues it for later execution by Signal_Handler_BH.
 * @param Signal Number of the signal to handle.
 */
static void
Signal_Handler(int Signal)
{
	switch (Signal) {
	case SIGTERM:
	case SIGINT:
	case SIGQUIT:
		/* shut down sever */
		NGIRCd_SignalQuit = true;
		return;
	case SIGCHLD:
		/* child-process exited, avoid zombies */
		while (waitpid( -1, NULL, WNOHANG) > 0)
			;
		return;
#ifdef DEBUG
	case SIGUSR1:
		if (! NGIRCd_Debug) {
			Log(LOG_INFO|LOG_snotice,
			    "Got SIGUSR1, debug mode activated.");
#ifdef SNIFFER
			strcpy(NGIRCd_DebugLevel, "2");
			NGIRCd_Debug = true;
			NGIRCd_Sniffer = true;
#else
			strcpy(NGIRCd_DebugLevel, "1");
			NGIRCd_Debug = true;
#endif /* SNIFFER */
		} else {
			Log(LOG_INFO|LOG_snotice,
			    "Got SIGUSR1, debug mode deactivated.");
			strcpy(NGIRCd_DebugLevel, "");
			NGIRCd_Debug = false;
#ifdef SNIFFER
			NGIRCd_Sniffer = false;
#endif /* SNIFFER */
		}
		return;
#endif
	}

	/*
	 * other signal: queue for later execution.
	 * This has the advantage that we are not restricted
	 * to functions that can be called safely from signal handlers.
	 */
	if (write(signalpipe[1], &Signal, sizeof(Signal)) != -1)
		Signal_Block(Signal);
} /* Signal_Handler */

/**
 * Signal processing handler of ngIRCd.
 * This function is called from the main conn event loop in (io_dispatch)
 * whenever ngIRCd has queued a signal.
 *
 * This function runs in normal context, not from the real signal handler,
 * thus its not necessary to only use functions that are signal safe.
 * @param Signal Number of the signal that was queued.
 */
static void
Signal_Handler_BH(int Signal)
{
	switch (Signal) {
	case SIGHUP:
		/* re-read configuration */
		Rehash();
		break;
#ifdef DEBUG
	case SIGUSR2:
		if (NGIRCd_Debug) {
			Log(LOG_INFO|LOG_snotice,
			    "Got SIGUSR2, dumping internal state ...");
			Dump_State();
		}
		break;
	default:
		Log(LOG_DEBUG, "Got signal %d! Ignored.", Signal);
#endif
	}
	Signal_Unblock(Signal);
}

static void
Signal_Callback(int fd, short UNUSED what)
{
	int sig, ret;
	(void) what;

	do {
		ret = (int)read(fd, &sig, sizeof(sig));
		if (ret == sizeof(int))
			Signal_Handler_BH(sig);
	} while (ret == sizeof(int));

	if (ret == -1) {
		if (errno == EAGAIN || errno == EINTR)
			return;

		Log(LOG_EMERG, "Read from signal pipe: %s - Exiting!",
		    strerror(errno));
		exit(1);
	}

	Log(LOG_EMERG, "EOF on signal pipe!? - Exiting!");
	exit(1);
}

/**
 * Initialize the signal handlers, catch
 * those signals we are interested in and sets SIGPIPE to be ignored.
 * @return true if initialization was successful.
 */
bool
Signals_Init(void)
{
	size_t i;
#ifdef HAVE_SIGACTION
	struct sigaction saction;
#endif
	if (signalpipe[0] > 0 || signalpipe[1] > 0)
		return true;

	if (pipe(signalpipe))
		return false;

	if (!io_setnonblock(signalpipe[0]) ||
	    !io_setnonblock(signalpipe[1]))
		return false;
	if (!io_setcloexec(signalpipe[0]) ||
	    !io_setcloexec(signalpipe[1]))
		return false;
#ifdef HAVE_SIGACTION
	memset( &saction, 0, sizeof( saction ));
	saction.sa_handler = Signal_Handler;
#ifdef SA_RESTART
	saction.sa_flags |= SA_RESTART;
#endif
#ifdef SA_NOCLDWAIT
	saction.sa_flags |= SA_NOCLDWAIT;
#endif

	for (i=0; i < C_ARRAY_SIZE(signals_catch) ; i++)
		sigaction(signals_catch[i], &saction, NULL);

	/* we handle write errors properly; ignore SIGPIPE */
	saction.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &saction, NULL);
#else
	for (i=0; i < C_ARRAY_SIZE(signals_catch) ; i++)
		signal(signals_catch[i], Signal_Handler);

	signal(SIGPIPE, SIG_IGN);
#endif
	return io_event_create(signalpipe[0], IO_WANTREAD, Signal_Callback);
} /* Signals_Init */

/**
 * Restores signals to their default behavior.
 *
 * This should be called after a fork() in the new
 * child prodcess, especially when we are about to call
 * 3rd party code (e.g. PAM).
 */
void
Signals_Exit(void)
{
	size_t i;
#ifdef HAVE_SIGACTION
	struct sigaction saction;

	memset(&saction, 0, sizeof(saction));
	saction.sa_handler = SIG_DFL;

	for (i=0; i < C_ARRAY_SIZE(signals_catch) ; i++)
		sigaction(signals_catch[i], &saction, NULL);
	sigaction(SIGPIPE, &saction, NULL);
#else
	for (i=0; i < C_ARRAY_SIZE(signals_catch) ; i++)
		signal(signals_catch[i], SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
#endif
	close(signalpipe[1]);
	close(signalpipe[0]);
	signalpipe[0] = signalpipe[1] = 0;
}

/* -eof- */
