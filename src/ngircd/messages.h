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
 * $Id: messages.h,v 1.2 2001/12/24 01:30:46 alex Exp $
 *
 * irc.h: IRC-Befehle (Header)
 *
 * $Log: messages.h,v $
 * Revision 1.2  2001/12/24 01:30:46  alex
 * - einige Messages korrigiert, andere ergaenzt (u.a. fuer MOTD).
 *
 * Revision 1.1  2001/12/23 21:53:32  alex
 * - Ich habe diesen Header begonnen.
 *
 * Revision 1.1  2001/12/14 08:13:43  alex
 * - neues Modul begonnen :-)
 */


#ifndef __messages_h__
#define __messages_h__


#define RPL_WELCOME			"001"
#define RPL_WELCOME_MSG			RPL_WELCOME" %s :Welcome to the Internet Relay Network %s!%s@%s"

#define RPL_YOURHOST			"002"
#define RPL_YOURHOST_MSG		RPL_YOURHOST" %s :Your host is %s, running ngircd "VERSION

#define RPL_CREATED			"003"
#define RPL_CREATED_MSG			RPL_CREATED" %s :This server was created once upon a time ... ;-)"

#define RPL_MYINFO			"004"
#define RPL_MYINFO_MSG			RPL_MYINFO" %s %s ngircd-"VERSION" + +"

#define RPL_MOTDSTART			"375"
#define RPL_MOTDSTART_MSG		RPL_MOTDSTART" %s :- %s Message of the day - "

#define RPL_MOTD			"372"
#define RPL_MOTD_MSG			RPL_MOTD" %s :- %s"

#define RPL_ENDOFMOTD			"376"
#define RPL_ENDOFMOTD_MSG		RPL_ENDOFMOTD" %s :End of MOTD command."


#define ERR_UNKNOWNCOMMAND		"421"
#define ERR_UNKNOWNCOMMAND_MSG		ERR_UNKNOWNCOMMAND" %s :Unknown command"

#define ERR_ERRONEUSNICKNAME		"432"
#define ERR_ERRONEUSNICKNAME_MSG	ERR_ERRONEUSNICKNAME" %s :Erroneous nickname"

#define ERR_NEEDMOREPARAMS		"461"
#define ERR_NEEDMOREPARAMS_MSG		ERR_NEEDMOREPARAMS" :Syntax error"

#define ERR_ALREADYREGISTRED		"462"
#define ERR_ALREADYREGISTRED_MSG	ERR_ALREADYREGISTRED" :Connection already registered"

#define ERR_NOTREGISTERED		"451"
#define ERR_NOTREGISTERED_MSG		ERR_NOTREGISTERED" :Connection not registered"


#endif


/* -eof- */
