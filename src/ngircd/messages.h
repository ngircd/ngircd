/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2023 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#ifndef __messages_h__
#define __messages_h__

/**
 * @file
 * IRC numerics (Header)
 */

#define RPL_WELCOME_MSG			"001 %s :Welcome to the Internet Relay Network %s"
#define RPL_YOURHOST_MSG		"002 %s :Your host is %s, running version ngircd-%s (%s/%s/%s)"
#define RPL_CREATED_MSG			"003 %s :This server has been started %s"
#define RPL_MYINFO_MSG			"004 %s %s ngircd-%s %s %s"
#define RPL_ISUPPORTNET_MSG		"005 %s NETWORK=%s :is my network name"
#define RPL_ISUPPORT1_MSG		"005 %s RFC2812 IRCD=ngIRCd CHARSET=UTF-8 CASEMAPPING=ascii PREFIX=(qaohv)~&@%%+ CHANTYPES=%s CHANMODES=beI,k,l,imMnOPQRstVz CHANLIMIT=%s:%d :are supported on this server"
#define RPL_ISUPPORT2_MSG		"005 %s CHANNELLEN=%d NICKLEN=%d TOPICLEN=%d AWAYLEN=%d KICKLEN=%d MODES=%d MAXLIST=beI:%d EXCEPTS=e INVEX=I PENALTY FNC :are supported on this server"

#define RPL_TRACELINK_MSG		"200 %s Link %s-%s %s %s V%s %ld %d %d"
#define RPL_TRACEOPERATOR_MSG		"204 %s Oper 2 :%s"
#define RPL_TRACESERVER_MSG		"206 %s Serv 1 0S 0C %s[%s@%s] *!*@%s :V%s"
#define RPL_STATSLINKINFO_MSG		"211 %s %s %d %ld %ld %ld %ld :%ld"
#define RPL_STATSCOMMANDS_MSG		"212 %s %s %ld %ld %ld"
#define RPL_STATSXLINE_MSG		"216 %s %c %s %ld :%s"
#define RPL_ENDOFSTATS_MSG		"219 %s %c :End of STATS report"
#define RPL_UMODEIS_MSG			"221 %s +%s"
#define RPL_SERVLIST_MSG		"234 %s %s %s %s %d %d :%s"
#define RPL_SERVLISTEND_MSG		"235 %s %s %s :End of service listing"
#define RPL_STATSUPTIME			"242 %s :Server Up %u days %u:%02u:%02u"
#define RPL_LUSERCLIENT_MSG		"251 %s :There are %ld users and %ld services on %ld servers"
#define RPL_LUSEROP_MSG			"252 %s %lu :operator(s) online"
#define RPL_LUSERUNKNOWN_MSG		"253 %s %lu :unknown connection(s)"
#define RPL_LUSERCHANNELS_MSG		"254 %s %lu :channels formed"
#define RPL_LUSERME_MSG			"255 %s :I have %lu users, %lu services and %lu servers"
#define RPL_ADMINME_MSG			"256 %s %s :Administrative info"
#define RPL_ADMINLOC1_MSG		"257 %s :%s"
#define RPL_ADMINLOC2_MSG		"258 %s :%s"
#define RPL_ADMINEMAIL_MSG		"259 %s :%s"
#define RPL_TRACEEND_MSG		"262 %s %s %s-%s.%s :End of TRACE"
#define RPL_LOCALUSERS_MSG		"265 %s %lu %lu :Current local users: %lu, Max: %lu"
#define RPL_NETUSERS_MSG		"266 %s %lu %lu :Current global users: %lu, Max: %lu"
#define RPL_STATSCONN_MSG		"250 %s :Highest connection count: %lu (%lu connections received)"
#define RPL_WHOISSSL_MSG		"275 %s %s :is connected via SSL (secure link)"
#define RPL_WHOISCERTFP_MSG		"276 %s %s :has client certificate fingerprint %s"

#define RPL_AWAY_MSG			"301 %s %s :%s"
#define RPL_USERHOST_MSG		"302 %s :"
#define RPL_ISON_MSG			"303 %s :"
#define RPL_UNAWAY_MSG			"305 %s :You are no longer marked as being away"
#define RPL_NOWAWAY_MSG			"306 %s :You have been marked as being away"
#define RPL_WHOISREGNICK_MSG		"307 %s %s :is a registered nick"
#define RPL_WHOISSERVICE_MSG		"310 %s %s :is an IRC service"
#define RPL_WHOISUSER_MSG		"311 %s %s %s %s * :%s"
#define RPL_WHOISSERVER_MSG		"312 %s %s %s :%s"
#define RPL_WHOISOPERATOR_MSG		"313 %s %s :is an IRC operator"
#define RPL_WHOWASUSER_MSG		"314 %s %s %s %s * :%s"
#define RPL_ENDOFWHO_MSG		"315 %s %s :End of WHO list"
#define RPL_WHOISIDLE_MSG		"317 %s %s %lu %lu :seconds idle, signon time"
#define RPL_ENDOFWHOIS_MSG		"318 %s %s :End of WHOIS list"
#define RPL_WHOISCHANNELS_MSG		"319 %s %s :"
#define RPL_LISTSTART_MSG		"321 %s Channel :Users  Name"
#define RPL_LIST_MSG			"322 %s %s %ld :%s"
#define RPL_LISTEND_MSG			"323 %s :End of LIST"
#define RPL_CHANNELMODEIS_MSG		"324 %s %s +%s"
#define RPL_CREATIONTIME_MSG		"329 %s %s %ld"
#define RPL_WHOISLOGGEDIN_MSG		"330 %s %s %s :is logged in as"
#define RPL_NOTOPIC_MSG			"331 %s %s :No topic is set"
#define RPL_TOPIC_MSG			"332 %s %s :%s"
#define RPL_TOPICSETBY_MSG		"333 %s %s %s %u"
#define RPL_WHOISBOT_MSG		"335 %s %s :is an IRC Bot"
#define RPL_INVITING_MSG		"341 %s %s %s%s"
#define RPL_INVITELIST_MSG		"346 %s %s %s %s %d"
#define RPL_ENDOFINVITELIST_MSG		"347 %s %s :End of channel invite list"
#define RPL_EXCEPTLIST_MSG		"348 %s %s %s %s %d"
#define RPL_ENDOFEXCEPTLIST_MSG		"349 %s %s :End of channel exception list"
#define RPL_VERSION_MSG			"351 %s %s-%s.%s %s :%s"
#define RPL_WHOREPLY_MSG		"352 %s %s %s %s %s %s %s :%d %s"
#define RPL_NAMREPLY_MSG		"353 %s %c %s :"
#define RPL_LINKS_MSG			"364 %s %s %s :%d %s"
#define RPL_ENDOFLINKS_MSG		"365 %s %s :End of LINKS list"
#define RPL_ENDOFNAMES_MSG		"366 %s %s :End of NAMES list"
#define RPL_BANLIST_MSG			"367 %s %s %s %s %d"
#define RPL_ENDOFBANLIST_MSG		"368 %s %s :End of channel ban list"
#define RPL_ENDOFWHOWAS_MSG		"369 %s %s :End of WHOWAS list"
#define RPL_INFO_MSG    		"371 %s :%s"
#define RPL_ENDOFINFO_MSG    		"374 %s :End of INFO list"
#define RPL_MOTD_MSG			"372 %s :- %s"
#define RPL_MOTDSTART_MSG		"375 %s :- %s message of the day"
#define RPL_ENDOFMOTD_MSG		"376 %s :End of MOTD command"
#define RPL_WHOISHOST_MSG		"378 %s %s :is connecting from *@%s %s"
#define RPL_WHOISMODES_MSG		"379 %s %s :is using modes +%s"
#define RPL_YOUREOPER_MSG		"381 %s :You are now an IRC Operator"
#define RPL_REHASHING_MSG		"382 %s :Rehashing"
#define RPL_YOURESERVICE_MSG		"383 %s :You are service %s"
#define RPL_TIME_MSG			"391 %s %s :%s"
#define RPL_HOSTHIDDEN_MSG		"396 %s %s :is your displayed hostname now"

#define ERR_NOSUCHNICK_MSG		"401 %s %s :No such nick or channel name"
#define ERR_NOSUCHSERVER_MSG		"402 %s %s :No such server"
#define ERR_NOSUCHCHANNEL_MSG		"403 %s %s :No such channel"
#define ERR_CANNOTSENDTOCHAN_MSG	"404 %s %s :Cannot send to channel"
#define ERR_TOOMANYCHANNELS_MSG		"405 %s %s :You have joined too many channels"
#define ERR_WASNOSUCHNICK_MSG		"406 %s %s :There was no such nickname"
#define ERR_TOOMANYTARGETS_MSG		"407 %s :Too many recipients"
#define ERR_NOORIGIN_MSG		"409 %s :No origin specified"
#define ERR_INVALIDCAP_MSG		"410 %s %s :Invalid CAP subcommand"
#define ERR_NORECIPIENT_MSG		"411 %s :No recipient given (%s)"
#define ERR_NOTEXTTOSEND_MSG		"412 %s :No text to send"
#define ERR_WILDTOPLEVEL_MSG		"414 %s :Wildcard in toplevel domain"
#define ERR_UNKNOWNCOMMAND_MSG		"421 %s %s :Unknown command"
#define ERR_NOMOTD_MSG			"422 %s :MOTD file is missing"
#define ERR_NONICKNAMEGIVEN_MSG		"431 %s :No nickname given"
#define ERR_ERRONEUSNICKNAME_MSG	"432 %s %s :Erroneous nickname"
#define ERR_NICKNAMETOOLONG_MSG		"432 %s %s :Nickname too long, max. %u characters"
#define ERR_FORBIDDENNICKNAME_MSG	"432 %s %s :Nickname is forbidden/blocked"
#define ERR_NICKNAMEINUSE_MSG		"433 %s %s :Nickname already in use"
#define ERR_USERNOTINCHANNEL_MSG	"441 %s %s %s :They aren't on that channel"
#define ERR_NOTONCHANNEL_MSG		"442 %s %s :You are not on that channel"
#define ERR_USERONCHANNEL_MSG		"443 %s %s %s :is already on channel"
#define ERR_SUMMONDISABLED_MSG		"445 %s :SUMMON has been disabled"
#define ERR_USERSDISABLED_MSG		"446 %s :USERS has been disabled"
#define ERR_NONICKCHANGE_MSG		"447 %s :Cannot change nickname while on %s(+N)"
#define ERR_NOTREGISTERED_MSG		"451 %s :Connection not registered"
#define ERR_NOTREGISTEREDSERVER_MSG	"451 %s :Connection not registered as server link"
#define ERR_NEEDMOREPARAMS_MSG		"461 %s %s :Syntax error"
#define ERR_ALREADYREGISTRED_MSG	"462 %s :Connection already registered"
#define ERR_PASSWDMISMATCH_MSG		"464 %s :Invalid password"
#define ERR_CHANNELISFULL_MSG		"471 %s %s :Cannot join channel (+l) -- Channel is full, try later"
#define ERR_SECURECHANNEL_MSG		"471 %s %s :Cannot join channel (+z) -- SSL connections only"
#define ERR_OPONLYCHANNEL_MSG		"471 %s %s :Cannot join channel (+O) -- IRC opers only"
#define ERR_REGONLYCHANNEL_MSG		"471 %s %s :Cannot join channel (+R) -- Registered users only"
#define ERR_UNKNOWNMODE_MSG		"472 %s %c :is unknown mode char for %s"
#define ERR_INVITEONLYCHAN_MSG		"473 %s %s :Cannot join channel (+i) -- Invited users only"
#define ERR_BANNEDFROMCHAN_MSG		"474 %s %s :Cannot join channel (+b) -- You are banned"
#define ERR_BADCHANNELKEY_MSG		"475 %s %s :Cannot join channel (+k) -- Wrong channel key"
#define ERR_NOCHANMODES_MSG		"477 %s %s :Channel doesn't support modes"
#define ERR_NEEDREGGEDNICK_MSG		"477 %s %s :Cannot send to channel (+M) -- You need to be identified to a registered account to message this channel"
#define ERR_LISTFULL_MSG		"478 %s %s %s :Channel list is full (%d)"
#define ERR_NOPRIVILEGES_MSG		"481 %s :Permission denied"
#define ERR_CHANOPRIVSNEEDED_MSG	"482 %s %s :You are not channel operator"
#define ERR_CHANOPPRIVTOOLOW_MSG	"482 %s %s :Your privileges are too low"
#define ERR_KICKDENY_MSG		"482 %s %s :Cannot kick, %s is protected"
#define ERR_CANTKILLSERVER_MSG		"483 %s :You can't kill a server!"
#define ERR_RESTRICTED_MSG		"484 %s :Your connection is restricted"
#define ERR_NICKREGISTER_MSG		"484 %s :Cannot modify user mode (+R) -- Use IRC services"
#define ERR_NONONREG_MSG		"486 %s :Cannot send to user (+b) -- You must identify to a registered nick to private message %s"
#define ERR_NOOPERHOST_MSG		"491 %s :Not configured for your host"
#define ERR_NOTONSAMECHANNEL_MSG	"493 %s :You must share a common channel with %s"

#define ERR_UMODEUNKNOWNFLAG_MSG	"501 %s :Unknown mode"
#define ERR_UMODEUNKNOWNFLAG2_MSG	"501 %s :Unknown mode \"%c%c\""
#define ERR_USERSDONTMATCH_MSG		"502 %s :Can't set/get mode for other users"
#define ERR_USERNOTONSERV_MSG		"504 %s %s :User is not on this server"
#define ERR_NOINVITE_MSG		"518 %s :Cannot invite to %s (+V)"

#define ERR_INVALIDMODEPARAM_MSG	"696 %s %s %c * :Invalid mode parameter"

#ifdef ZLIB
# define RPL_STATSLINKINFOZIP_MSG	"211 %s %s %d %ld %ld/%ld %ld %ld/%ld :%ld"
#endif

#ifdef IRCPLUS

# define RPL_IP_CHARCONV_MSG		"801 %s %s :Client encoding set"

# define ERR_IP_CHARCONV_MSG		"851 %s :Can't initialize client encoding"

#endif /* IRCPLUS */

#endif

/* -eof- */
