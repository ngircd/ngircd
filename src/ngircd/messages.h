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
 * $Id: messages.h,v 1.37.2.1 2002/04/29 14:11:23 alex Exp $
 *
 * irc.h: IRC-Befehle (Header)
 */


#ifndef __messages_h__
#define __messages_h__


#define RPL_WELCOME			"001"
#define RPL_WELCOME_MSG			RPL_WELCOME" %s :Welcome to the Internet Relay Network %s"

#define RPL_YOURHOST			"002"
#define RPL_YOURHOST_MSG		RPL_YOURHOST" %s :Your host is %s, running ngircd "VERSION"-"TARGET_CPU"/"TARGET_VENDOR"/"TARGET_OS

#define RPL_CREATED			"003"
#define RPL_CREATED_MSG			RPL_CREATED" %s :This server was started %s"

#define RPL_MYINFO			"004"
#define RPL_MYINFO_MSG			RPL_MYINFO" %s :%s ngircd-"VERSION" "USERMODES" "CHANMODES

#define RPL_UMODEIS			"211"
#define RPL_UMODEIS_MSG			RPL_UMODEIS" %s +%s"

#define RPL_LUSERCLIENT			"251"
#define RPL_LUSERCLIENT_MSG		RPL_LUSERCLIENT" %s :There are %d users and %d services on %d servers"

#define RPL_LUSEROP			"252"
#define RPL_LUSEROP_MSG			RPL_LUSEROP" %s %d :operator(s) online"

#define RPL_LUSERUNKNOWN		"253"
#define	RPL_LUSERUNKNOWN_MSG		RPL_LUSERUNKNOWN" %s %d :unknown connection(s)"

#define RPL_LUSERCHANNELS		"254"
#define RPL_LUSERCHANNELS_MSG		RPL_LUSERCHANNELS" %s %d :channels formed"

#define RPL_LUSERME			"255"
#define	RPL_LUSERME_MSG			RPL_LUSERME" %s :I have %d users, %d services and %d servers"


#define RPL_UNAWAY			"305"
#define RPL_UNAWAY_MSG			RPL_UNAWAY" %s :You are no longer marked as being away"

#define RPL_NOWAWAY			"306"
#define RPL_NOWAWAY_MSG			RPL_NOWAWAY" %s :You have been marked as being away"

#define RPL_MOTD			"372"
#define RPL_MOTD_MSG			RPL_MOTD" %s :- %s"

#define RPL_MOTDSTART			"375"
#define RPL_MOTDSTART_MSG		RPL_MOTDSTART" %s :- %s message of the day"

#define RPL_ENDOFMOTD			"376"
#define RPL_ENDOFMOTD_MSG		RPL_ENDOFMOTD" %s :End of MOTD command"

#define	RPL_AWAY			"301"
#define RPL_AWAY_MSG			RPL_AWAY" %s %s :%s"

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

#define RPL_ENDOFWHO			"315"
#define RPL_ENDOFWHO_MSG		RPL_ENDOFWHO" %s %s :End of WHO list"

#define RPL_WHOISIDLE			"317"
#define RPL_WHOISIDLE_MSG		RPL_WHOISIDLE" %s %s %ld :seconds idle"

#define RPL_ENDOFWHOIS			"318"
#define RPL_ENDOFWHOIS_MSG		RPL_ENDOFWHOIS" %s %s :End of WHOIS list"

#define RPL_WHOISCHANNELS		"319"
#define RPL_WHOISCHANNELS_MSG		RPL_WHOISCHANNELS" %s %s :"

#define RPL_LIST			"322"
#define RPL_LIST_MSG			RPL_LIST" %s %s %d :%s"

#define RPL_LISTEND			"323"
#define RPL_LISTEND_MSG			RPL_LISTEND" %s :End of LIST"

#define RPL_CHANNELMODEIS		"324"
#define RPL_CHANNELMODEIS_MSG		RPL_CHANNELMODEIS" %s %s +%s"

#define RPL_NOTOPIC			"331"
#define RPL_NOTOPIC_MSG			RPL_NOTOPIC" %s %s :No topic is set"

#define RPL_TOPIC			"332"
#define RPL_TOPIC_MSG			RPL_TOPIC" %s %s :%s"

#define RPL_VERSION			"351"
#define RPL_VERSION_MSG			RPL_VERSION" %s "PACKAGE"-"VERSION".%s %s :%s"

#define RPL_WHOREPLY			"352"
#define RPL_WHOREPLY_MSG		RPL_WHOREPLY" %s %s %s %s %s %s %s :%d %s"

#define RPL_NAMREPLY			"353"
#define RPL_NAMREPLY_MSG		RPL_NAMREPLY" %s %s %s :"

#define RPL_LINKS			"364"
#define RPL_LINKS_MSG			RPL_LINKS" %s %s %s :%d %s"

#define RPL_ENDOFLINKS			"365"
#define RPL_ENDOFLINKS_MSG		RPL_ENDOFLINKS" %s %s :End of LINKS list"

#define RPL_ENDOFNAMES			"366"
#define RPL_ENDOFNAMES_MSG		RPL_ENDOFNAMES" %s %s :End of NAMES list"

#define RPL_YOUREOPER			"381"
#define RPL_YOUREOPER_MSG		RPL_YOUREOPER" %s :You are now an IRC Operator"


#define ERR_NOSUCHNICK			"401"
#define ERR_NOSUCHNICK_MSG		ERR_NOSUCHNICK" %s %s :No such nick or channel name"

#define ERR_NOSUCHSERVER		"402"
#define ERR_NOSUCHSERVER_MSG		ERR_NOSUCHSERVER" %s %s :No such server"

#define ERR_NOSUCHCHANNEL		"403"
#define ERR_NOSUCHCHANNEL_MSG		ERR_NOSUCHCHANNEL" %s %s :No such channel"

#define ERR_CANNOTSENDTOCHAN		"404"
#define ERR_CANNOTSENDTOCHAN_MSG	ERR_CANNOTSENDTOCHAN" %s %s :Cannot send to channel"

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

#define ERR_NOTONCHANNEL		"442"
#define ERR_NOTONCHANNEL_MSG		ERR_NOTONCHANNEL" %s %s :You are not on that channel"

#define ERR_NOTREGISTERED		"451"
#define ERR_NOTREGISTERED_MSG		ERR_NOTREGISTERED" %s :Connection not registered"
#define ERR_NOTREGISTEREDSERVER_MSG	ERR_NOTREGISTERED" %s :Connection not registered as server link"

#define ERR_NEEDMOREPARAMS		"461"
#define ERR_NEEDMOREPARAMS_MSG		ERR_NEEDMOREPARAMS" %s %s :Syntax error"

#define ERR_ALREADYREGISTRED		"462"
#define ERR_ALREADYREGISTRED_MSG	ERR_ALREADYREGISTRED" %s :Connection already registered"

#define ERR_PASSWDMISMATCH		"464"
#define ERR_PASSWDMISMATCH_MSG		ERR_PASSWDMISMATCH" %s: Invalid password"

#define ERR_NOPRIVILEGES		"481"
#define ERR_NOPRIVILEGES_MSG		ERR_NOPRIVILEGES" %s :Permission denied"

#define ERR_CHANOPRIVSNEEDED		"482"
#define ERR_CHANOPRIVSNEEDED_MSG	ERR_CHANOPRIVSNEEDED" %s %s :You are not channel operator"

#define ERR_RESTRICTED			"484"
#define ERR_RESTRICTED_MSG		ERR_RESTRICTED" %s :Your connection is restricted"

#define ERR_NOOPERHOST			"491"
#define ERR_NOOPERHOST_MSG		ERR_NOOPERHOST" %s :Not configured for your host"


#define ERR_UMODEUNKNOWNFLAG		"501"
#define ERR_UMODEUNKNOWNFLAG_MSG	ERR_UMODEUNKNOWNFLAG" %s :Unknown mode"
#define ERR_UMODEUNKNOWNFLAG2_MSG	ERR_UMODEUNKNOWNFLAG" %s :Unknown mode \"%c%c\""

#define ERR_USERSDONTMATCH		"502"
#define ERR_USERSDONTMATCH_MSG		ERR_USERSDONTMATCH" %s :Can't set/get mode for other users"


#endif


/* -eof- */
