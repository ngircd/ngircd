/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: tool.h,v 1.4 2001/12/31 02:18:51 alex Exp $
 *
 * log.h: Hilfsfunktionen (Header)
 *
 * $Log: tool.h,v $
 * Revision 1.4  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.3  2001/12/26 14:45:37  alex
 * - "Code Cleanups".
 *
 * Revision 1.2  2001/12/12 17:20:33  alex
 * - Tool-Funktionen haben nun das Praefix "ngt_".
 *
 * Revision 1.1.1.1  2001/12/11 21:53:04  alex
 * - Imported sources to CVS.
 */


#ifndef __tool_h__
#define __tool_h__


GLOBAL VOID ngt_TrimStr( CHAR *String );


#endif


/* -eof- */
