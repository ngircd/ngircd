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
 * $Id: messages.h,v 1.40 2002/05/30 16:52:21 alex Exp $
 *
 * irc.h: IRC-Befehle (Header)
 */


#ifndef __messages_h__
#define __messages_h__


#define RPL_WELCOME			"001"
#define RPL_WELCOME_MSG			"001 %s :Welcome to the Internet Relay Network %s"

#define RPL_YOURHOST			"002"
#define RPL_YOURHOST_MSG		"002 %s :Your host is %s, running ngircd %s-%s/%s/%s"

#define RPL_CREATED			"003"
#define RPL_CREATED_MSG			"003 %s :This server was started %s"

#define RPL_MYINFO			"004"
#define RPL_MYINFO_MSG			"004 %s :%s ngircd-%s %s %s"

#define RPL_UMODEIS			"211"
#define RPL_UMODEIS_MSG			"211 %s +%s"

#define RPL_LUSERCLIENT			"251"
#define RPL_LUSERCLIENT_MSG		"251 %s :There are %d users and %d services on %d servers"

#define RPL_LUSEROP			"252"
#define RPL_LUSEROP_MSG			"252 %s %d :operator(s) online"

#define RPL_LUSERUNKNOWN		"253"
#define	RPL_LUSERUNKNOWN_MSG		"253 %s %d :unknown connection(s)"

#define RPL_LUSERCHANNELS		"254"
#define RPL_LUSERCHANNELS_MSG		"254 %s %d :channels formed"

#define RPL_LUSERME			"255"
#define	RPL_LUSERME_MSG			"255 %s :I have %d users, %d services and %d servers"


#define RPL_UNAWAY			"305"
#define RPL_UNAWAY_MSG			"305 %s :You are no longer marked as being away"

#define RPL_NOWAWAY			"306"
#define RPL_NOWAWAY_MSG			"306 %s :You have been marked as being away"

#define RPL_MOTD			"372"
#define RPL_MOTD_MSG			"372 %s :- %s"

#define RPL_MOTDSTART			"375"
#define RPL_MOTDSTART_MSG		"375 %s :- %s message of the day"

#define RPL_ENDOFMOTD			"376"
#define RPL_ENDOFMOTD_MSG		"376 %s :End of MOTD command"

#define	RPL_AWAY			"301"
#define RPL_AWAY_MSG			"301 %s %s :%s"

#define RPL_USERHOST			"302"
#define RPL_USERHOST_MSG		"302 %s :"

#define RPL_ISON			"303"
#define RPL_ISON_MSG			"303 %s :"

#define RPL_WHOISUSER			"311"
#define RPL_WHOISUSER_MSG		"311 %s %s %s %s * :%s"

#define RPL_WHOISSERVER			"312"
#define RPL_WHOISSERVER_MSG		"312 %s %s %s :%s"

#define RPL_WHOISOPERATOR		"313"
#define RPL_WHOISOPERATOR_MSG		"313 %s %s :is an IRC operator"

#define RPL_ENDOFWHO			"315"
#define RPL_ENDOFWHO_MSG		"315 %s %s :End of WHO list"

#define RPL_WHOISIDLE			"317"
#define RPL_WHOISIDLE_MSG		"317 %s %s %ld :seconds idle"

#define RPL_ENDOFWHOIS			"318"
#define RPL_ENDOFWHOIS_MSG		"318 %s %s :End of WHOIS list"

#define RPL_WHOISCHANNELS		"319"
#define RPL_WHOISCHANNELS_MSG		"319 %s %s :"

#define RPL_LIST			"322"
#define RPL_LIST_MSG			"322 %s %s %d :%s"

#define RPL_LISTEND			"323"
#define RPL_LISTEND_MSG			"323 %s :End of LIST"

#define RPL_CHANNELMODEIS		"324"
#define RPL_CHANNELMODEIS_MSG		"324 %s %s +%s"

#define RPL_NOTOPIC			"331"
#define RPL_NOTOPIC_MSG			"331 %s %s :No topic is set"

#define RPL_TOPIC			"332"
#define RPL_TOPIC_MSG			"332 %s %s :%s"

#define RPL_VERSION			"351"
#define RPL_VERSION_MSG			"351 %s %s-%s.%s %s :%s"

#define RPL_WHOREPLY			"352"
#define RPL_WHOREPLY_MSG		"352 %s %s %s %s %s %s %s :%d %s"

#define RPL_NAMREPLY			"353"
#define RPL_NAMREPLY_MSG		"353 %s %s %s :"

#define RPL_LINKS			"364"
#define RPL_LINKS_MSG			"364 %s %s %s :%d %s"

#define RPL_ENDOFLINKS			"365"
#define RPL_ENDOFLINKS_MSG		"365 %s %s :End of LINKS list"

#define RPL_ENDOFNAMES			"366"
#define RPL_ENDOFNAMES_MSG		"366 %s %s :End of NAMES list"

#define RPL_YOUREOPER			"381"
#define RPL_YOUREOPER_MSG		"381 %s :You are now an IRC Operator"


#define ERR_NOSUCHNICK			"401"
#define ERR_NOSUCHNICK_MSG		"401 %s %s :No such nick or channel name"

#define ERR_NOSUCHSERVER		"402"
#define ERR_NOSUCHSERVER_MSG		"402 %s %s :No such server"

#define ERR_NOSUCHCHANNEL		"403"
#define ERR_NOSUCHCHANNEL_MSG		"403 %s %s :No such channel"

#define ERR_CANNOTSENDTOCHAN		"404"
#define ERR_CANNOTSENDTOCHAN_MSG	"404 %s %s :Cannot send to channel"

#define ERR_NOORIGIN			"409"
#define ERR_NOORIGIN_MSG		"409 %s :No origin specified"

#define ERR_NORECIPIENT			"411"
#define ERR_NORECIPIENT_MSG		"411 %s :No receipient given (%s)"

#define ERR_NOTEXTTOSEND		"412"
#define ERR_NOTEXTTOSEND_MSG		"412 %s :No text to send"

#define ERR_UNKNOWNCOMMAND		"421"
#define ERR_UNKNOWNCOMMAND_MSG		"421 %s %s :Unknown command"

#define ERR_NOMOTD			"422"
#define ERR_NOMOTD_MSG			"422 %s :MOTD file is missing"

#define ERR_ERRONEUSNICKNAME		"432"
#define ERR_ERRONEUSNICKNAME_MSG	"432 %s %s :Erroneous nickname"

#define ERR_NICKNAMEINUSE		"433"
#define ERR_NICKNAMEINUSE_MSG		"433 %s %s :Nickname already in use"

#define ERR_NOTONCHANNEL		"442"
#define ERR_NOTONCHANNEL_MSG		"442 %s %s :You are not on that channel"

#define ERR_NOTREGISTERED		"451"
#define ERR_NOTREGISTERED_MSG		"451 %s :Connection not registered"
#define ERR_NOTREGISTEREDSERVER_MSG	"451 %s :Connection not registered as server link"

#define ERR_NEEDMOREPARAMS		"461"
#define ERR_NEEDMOREPARAMS_MSG		"461 %s %s :Syntax error"

#define ERR_ALREADYREGISTRED		"462"
#define ERR_ALREADYREGISTRED_MSG	"462 %s :Connection already registered"

#define ERR_PASSWDMISMATCH		"464"
#define ERR_PASSWDMISMATCH_MSG		"464 %s: Invalid password"

#define ERR_NOPRIVILEGES		"481"
#define ERR_NOPRIVILEGES_MSG		"481 %s :Permission denied"

#define ERR_CHANOPRIVSNEEDED		"482"
#define ERR_CHANOPRIVSNEEDED_MSG	"482 %s %s :You are not channel operator"

#define ERR_RESTRICTED			"484"
#define ERR_RESTRICTED_MSG		"484 %s :Your connection is restricted"

#define ERR_NOOPERHOST			"491"
#define ERR_NOOPERHOST_MSG		"491 %s :Not configured for your host"


#define ERR_UMODEUNKNOWNFLAG		"501"
#define ERR_UMODEUNKNOWNFLAG_MSG	"501 %s :Unknown mode"
#define ERR_UMODEUNKNOWNFLAG2_MSG	"501 %s :Unknown mode \"%c%c\""

#define ERR_USERSDONTMATCH		"502"
#define ERR_USERSDONTMATCH_MSG		"502 %s :Can't set/get mode for other users"


#endif


/* -eof- */
