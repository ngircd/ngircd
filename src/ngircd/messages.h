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
 * $Id: messages.h,v 1.15 2002/01/02 02:42:58 alex Exp $
 *
 * irc.h: IRC-Befehle (Header)
 *
 * $Log: messages.h,v $
 * Revision 1.15  2002/01/02 02:42:58  alex
 * - Copyright-Texte aktualisiert.
 *
 * Revision 1.14  2001/12/31 16:00:57  alex
 * - "o" zu den unterstuetzten Modes hinzugefuegt.
 *
 * Revision 1.13  2001/12/31 15:33:13  alex
 * - neuer Befehl NAMES, kleinere Bugfixes.
 * - Bug bei PING behoben: war zu restriktiv implementiert :-)
 *
 * Revision 1.12  2001/12/31 02:18:51  alex
 * - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
 * - neuen Header "defines.h" mit (fast) allen Konstanten.
 * - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
 *
 * Revision 1.11  2001/12/30 19:25:39  alex
 * - RPL_MYINFO_MSG um unterstuetzte User-Modes ergaengz.
 *
 * Revision 1.10  2001/12/30 11:42:00  alex
 * - der Server meldet nun eine ordentliche "Start-Zeit".
 *
 * Revision 1.9  2001/12/29 03:06:56  alex
 * - Texte ergaenzt, einige Bugs behoben (Leerzeichen falsch gesetzt, z.B.)
 *
 * Revision 1.8  2001/12/27 19:17:26  alex
 * - neue Befehle PRIVMSG, NOTICE, PING.
 *
 * Revision 1.7  2001/12/27 16:56:06  alex
 * - RPL_WELCOME an Client_GetID() angepasst.
 *
 * Revision 1.6  2001/12/26 22:48:53  alex
 * - MOTD-Datei ist nun konfigurierbar und wird gelesen.
 *
 * Revision 1.5  2001/12/26 03:51:13  alex
 * - in ERR_NOTREGISTERED_MSG fehlte ein "%s" - jetzt steht auch hier der Nick.
 *
 * Revision 1.4  2001/12/26 03:22:40  alex
 * - Format der Meldungen ueberarbeitet: fast immer ist nun der Nick enthalten.
 *
 * Revision 1.3  2001/12/25 19:20:11  alex
 * - neue Message: ERR_NICKNAMEINUSE[_MSG].
 *
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
#define RPL_WELCOME_MSG			RPL_WELCOME" %s :Welcome to the Internet Relay Network %s"

#define RPL_YOURHOST			"002"
#define RPL_YOURHOST_MSG		RPL_YOURHOST" %s :Your host is %s, running ngircd "VERSION"-"P_OSNAME"/"P_ARCHNAME

#define RPL_CREATED			"003"
#define RPL_CREATED_MSG			RPL_CREATED" %s :This server was started %s"

#define RPL_MYINFO			"004"
#define RPL_MYINFO_MSG			RPL_MYINFO" %s %s ngircd-"VERSION" ior +"

#define RPL_MOTDSTART			"375"
#define RPL_MOTDSTART_MSG		RPL_MOTDSTART" %s :- %s message of the day"

#define RPL_MOTD			"372"
#define RPL_MOTD_MSG			RPL_MOTD" %s :- %s"

#define RPL_ENDOFMOTD			"376"
#define RPL_ENDOFMOTD_MSG		RPL_ENDOFMOTD" %s :End of MOTD command"

#define RPL_UMODEIS			"211"
#define RPL_UMODEIS_MSG			RPL_UMODEIS" %s +%s"

#define RPL_USERHOST			"302"
#define RPL_USERHOST_MSG		RPL_USERHOST" %s :"

#define RPL_ISON			"303"
#define RPL_ISON_MSG			RPL_ISON" %s :"

#define RPL_WHOISUSER			"311"
#define RPL_WHOISUSER_MSG		RPL_WHOISUSER" %s %s %s %s * :%s"

#define RPL_WHOISSERVER			"312"
#define RPL_WHOISSERVER_MSG		RPL_WHOISSERVER" %s %s %s :%s"

#define RPL_WHOISOPERATOR		"313"
#define RPL_WHOISOPERATOR_MSG		RPL_WHOISOPERATOR" %s %s :is an IRC operator"

#define RPL_WHOISIDLE			"317"
#define RPL_WHOISIDLE_MSG		RPL_WHOISIDLE" %s %s %ld :seconds idle"

#define RPL_ENDOFWHOIS			"318"
#define RPL_ENDOFWHOIS_MSG		RPL_ENDOFWHOIS" %s %s :End of WHOIS list"

#define RPL_WHOISCHANNELS		"319"
#define RPL_WHOISCHANNELS_MSG		RPL_WHOISCHANNELS" %s :"

#define RPL_NAMREPLY			"353"
#define RPL_NAMREPLY_MSG		RPL_NAMREPLY" %s %s %s :%s"

#define RPL_ENDOFNAMES			"366"
#define RPL_ENDOFNAMES_MSG		RPL_ENDOFNAMES" %s %s :End of NAMES list"

#define RPL_YOUREOPER			"381"
#define RPL_YOUREOPER_MSG		RPL_YOUREOPER" %s :You are now an IRC Operator"


#define ERR_NOSUCHNICK			"401"
#define ERR_NOSUCHNICK_MSG		ERR_NOSUCHNICK" %s %s :No such nick or channel name"

#define ERR_NOORIGIN			"409"
#define ERR_NOORIGIN_MSG		ERR_NOORIGIN" %s :No origin specified"

#define ERR_NORECIPIENT			"411"
#define ERR_NORECIPIENT_MSG		ERR_NORECIPIENT" %s :No receipient given (%s)"

#define ERR_NOTEXTTOSEND		"412"
#define ERR_NOTEXTTOSEND_MSG		ERR_NOTEXTTOSEND" %s :No text to send"

#define ERR_UNKNOWNCOMMAND		"421"
#define ERR_UNKNOWNCOMMAND_MSG		ERR_UNKNOWNCOMMAND" %s %s :Unknown command"

#define ERR_NOMOTD			"422"
#define ERR_NOMOTD_MSG			ERR_NOMOTD" %s :MOTD file is missing"

#define ERR_ERRONEUSNICKNAME		"432"
#define ERR_ERRONEUSNICKNAME_MSG	ERR_ERRONEUSNICKNAME" %s %s :Erroneous nickname"

#define ERR_NICKNAMEINUSE		"433"
#define ERR_NICKNAMEINUSE_MSG		ERR_NICKNAMEINUSE" %s %s :Nickname already in use"

#define ERR_NEEDMOREPARAMS		"461"
#define ERR_NEEDMOREPARAMS_MSG		ERR_NEEDMOREPARAMS" %s %s :Syntax error"

#define ERR_ALREADYREGISTRED		"462"
#define ERR_ALREADYREGISTRED_MSG	ERR_ALREADYREGISTRED" %s :Connection already registered"

#define ERR_PASSWDMISMATCH		"464"
#define ERR_PASSWDMISMATCH_MSG		ERR_PASSWDMISMATCH" %s: Invalid password"

#define ERR_NOTREGISTERED		"451"
#define ERR_NOTREGISTERED_MSG		ERR_NOTREGISTERED" %s :Connection not registered"

#define ERR_NOPRIVILEGES		"481"
#define ERR_NOPRIVILEGES_MSG		ERR_NOPRIVILEGES" %s :Permission denied"

#define ERR_RESTRICTED			"484"
#define ERR_RESTRICTED_MSG		ERR_RESTRICTED" %s :Your connection is restricted"

#define ERR_NOOPERHOST			"491"
#define ERR_NOOPERHOST_MSG		ERR_NOOPERHOST" %s :Not configured for your host"

#define ERR_UMODEUNKNOWNFLAG		"501"
#define ERR_UMODEUNKNOWNFLAG_MSG	ERR_UMODEUNKNOWNFLAG" %s :Unknown mode flag"

#define ERR_USERSDONTMATCH		"502"
#define ERR_USERSDONTMATCH_MSG		ERR_USERSDONTMATCH" %s :Can't set/get mode for other users"


#endif


/* -eof- */
