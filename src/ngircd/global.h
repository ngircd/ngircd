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
 * $Id: global.h,v 1.7 2002/02/25 11:42:15 alex Exp $
 *
 * global.h: Globaler Header, wir in jedes(!) Modul eingebunden.
 *
 * $Log: global.h,v $
 * Revision 1.7  2002/02/25 11:42:15  alex
 * - unter A/UX wird _POSIX_SOURCE definiert: fuer Systemheader notwendig.
 *
 * Revision 1.6  2002/01/05 15:55:11  alex
 * - Wrapper fuer inet_aton(): liefert immer Fehler.
 *
 * Revision 1.5  2002/01/02 02:42:58  alex
 * - Copyright-Texte aktualisiert.
 *
 * Revision 1.4  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.3  2001/12/14 08:14:34  alex
 * - NONE als -1 definiert. Macht den Source lesbarer ;-)
 *
 * Revision 1.2  2001/12/12 01:58:53  alex
 * - Test auf socklen_t verbessert.
 *
 * Revision 1.1.1.1  2001/12/11 21:53:04  alex
 * Imported sources to CVS.
 */


#ifndef __global_h__
#define __global_h__


#include "config.h"
#include "defines.h"

#if OS_UNIX_AUX
#define _POSIX_SOURCE			/* muss unter A/UX definiert sein */
#endif

#ifndef HAVE_socklen_t
#define socklen_t int			/* u.a. fuer Mac OS X */
#endif

#ifndef HAVE_INET_ATON
#define inet_aton( opt, bind ) 0	/* Dummy fuer inet_aton() */
#endif


#endif


/* -eof- */
