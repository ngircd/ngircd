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
 * $Id: acconfig.h,v 1.6 2001/12/31 02:21:00 alex Exp $
 *
 * $Log: acconfig.h,v $
 * Revision 1.6  2001/12/31 02:21:00  alex
 * - "doc"-Unterverzeichnis aufgenommen.
 *
 * Revision 1.5  2001/12/29 03:04:35  alex
 * - neue configure-Option "--enable-strict-rfc", #define STRICT_RFC.
 *
 * Revision 1.4  2001/12/27 01:44:49  alex
 * - die Verwendung von syslog kann nun abgeschaltet werden.
 *
 * Revision 1.3  2001/12/25 22:01:47  alex
 * - neue configure-Option "--enable-sniffer".
 *
 * Revision 1.2  2001/12/21 23:54:15  alex
 * - zusaetzliche Debug-Ausgaben koennen eingeschaltet werden.
 *
 * Revision 1.1  2001/12/12 01:58:52  alex
 * - Test auf socklen_t verbessert.
 */


#undef HAVE_socklen_t

#undef DEBUG

#undef SNIFFER

#undef USE_SYSLOG

#undef STRICT_RFC


/* -eof- */
