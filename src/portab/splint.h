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
 */

#ifndef __splint__
#define __splint__

/**
 * @file
 * Header file which is included for SPLint code checks
 *
 * This header is only included by portab.h if a SPLint code check is
 * running (when S_SPLINT_S is defined). It makes some definitions to
 * prevent SPLint from issuing false warnings.
 */

#define SYSCONFDIR "/"
#define LOCALSTATEDIR "/"

#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7

#define WNOHANG 0

#endif

/* -eof- */
