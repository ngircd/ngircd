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
 * $Id: irc-write.h,v 1.1 2002/02/27 23:26:21 alex Exp $
 *
 * irc-write.h: IRC-Texte und Befehle ueber Netzwerk versenden (Header)
 *
 * $Log: irc-write.h,v $
 * Revision 1.1  2002/02/27 23:26:21  alex
 * - Modul aus irc.c bzw. irc.h ausgegliedert.
 *
 */


#ifndef __irc_write_h__
#define __irc_write_h__

#include "channel.h"


GLOBAL BOOLEAN IRC_WriteStrClient( CLIENT *Client, CHAR *Format, ... );
GLOBAL BOOLEAN IRC_WriteStrClientPrefix( CLIENT *Client, CLIENT *Prefix, CHAR *Format, ... );

GLOBAL BOOLEAN IRC_WriteStrChannel( CLIENT *Client, CHANNEL *Chan, BOOLEAN Remote, CHAR *Format, ... );
GLOBAL BOOLEAN IRC_WriteStrChannelPrefix( CLIENT *Client, CHANNEL *Chan, CLIENT *Prefix, BOOLEAN Remote, CHAR *Format, ... );

GLOBAL VOID IRC_WriteStrServers( CLIENT *ExceptOf, CHAR *Format, ... );
GLOBAL VOID IRC_WriteStrServersPrefix( CLIENT *ExceptOf, CLIENT *Prefix, CHAR *Format, ... );

GLOBAL BOOLEAN IRC_WriteStrRelatedPrefix( CLIENT *Client, CLIENT *Prefix, BOOLEAN Remote, CHAR *Format, ... );


#endif


/* -eof- */
