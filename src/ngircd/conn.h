/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001 by Alexander Barton (alex@barton.de)
 *
 * Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
 * der GNU General Public License (GPL), wie von der Free Software Foundation
 * herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
 * der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
 * Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
 * der an comBase beteiligten Autoren finden Sie in der Datei AUTHORS.
 *
 * $Id: conn.h,v 1.5 2001/12/23 21:57:48 alex Exp $
 *
 * conn.h: Verwaltung aller Netz-Verbindungen ("connections") (Header)
 *
 * $Log: conn.h,v $
 * Revision 1.5  2001/12/23 21:57:48  alex
 * - Conn_WriteStr() unterstuetzt nun variable Parameter.
 *
 * Revision 1.4  2001/12/15 00:08:27  alex
 * - neue globale Funktionen: Conn_Write() und Conn_WriteStr().
 *
 * Revision 1.3  2001/12/14 08:15:45  alex
 * - CONN_ID wird definiert.
 *
 * Revision 1.2  2001/12/13 01:33:32  alex
 * - Conn_Handler() unterstuetzt nun einen Timeout (in Sekunden).
 *
 * Revision 1.1  2001/12/12 17:18:38  alex
 * - Modul zur Verwaltung aller Netzwerk-Verbindungen begonnen.
 */


#ifndef __conn_h__
#define __conn_h__


typedef INT CONN_ID;


GLOBAL VOID Conn_Init( VOID );
GLOBAL VOID Conn_Exit( VOID );

GLOBAL BOOLEAN Conn_New_Listener( CONST INT Port );

GLOBAL VOID Conn_Handler( INT Timeout );

GLOBAL BOOLEAN Conn_Write( CONN_ID Idx, CHAR *Data, INT Len );
GLOBAL BOOLEAN Conn_WriteStr( CONN_ID Idx, CHAR *Format, ... );


#endif


/* -eof- */
