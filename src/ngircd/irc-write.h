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
 * $Id: irc-write.h,v 1.4 2002/09/03 23:56:06 alex Exp $
 *
 * irc-write.h: IRC-Texte und Befehle ueber Netzwerk versenden (Header)
 */


#ifndef __irc_write_h__
#define __irc_write_h__


GLOBAL BOOLEAN IRC_WriteStrClient PARAMS((CLIENT *Client, CHAR *Format, ... ));
GLOBAL BOOLEAN IRC_WriteStrClientPrefix PARAMS((CLIENT *Client, CLIENT *Prefix, CHAR *Format, ... ));

GLOBAL BOOLEAN IRC_WriteStrChannel PARAMS((CLIENT *Client, CHANNEL *Chan, BOOLEAN Remote, CHAR *Format, ... ));
GLOBAL BOOLEAN IRC_WriteStrChannelPrefix PARAMS((CLIENT *Client, CHANNEL *Chan, CLIENT *Prefix, BOOLEAN Remote, CHAR *Format, ... ));

GLOBAL VOID IRC_WriteStrServers PARAMS((CLIENT *ExceptOf, CHAR *Format, ... ));
GLOBAL VOID IRC_WriteStrServersPrefix PARAMS((CLIENT *ExceptOf, CLIENT *Prefix, CHAR *Format, ... ));
GLOBAL VOID IRC_WriteStrServersPrefixFlag PARAMS((CLIENT *ExceptOf, CLIENT *Prefix, CHAR Flag, CHAR *Format, ... ));

GLOBAL BOOLEAN IRC_WriteStrRelatedPrefix PARAMS((CLIENT *Client, CLIENT *Prefix, BOOLEAN Remote, CHAR *Format, ... ));


#endif


/* -eof- */
