/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c)2001-2024 Alexander Barton (alex@barton.de) and Contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Please read the file COPYING, README and AUTHORS for more information.
 */

#define RESOLVER_TIMEOUT (Conf_PongTimeout*3)/4

#include "portab.h"

/**
 * @file
 * Asynchronous resolver
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef IDENTAUTH
#ifdef HAVE_IDENT_H
#include <ident.h>
#endif
#endif

#include "conn.h"
#include "conf.h"
#include "log.h"
#include "ng_ipaddr.h"

#include "resolve.h"

static void Do_ResolveAddr_Ident PARAMS(( const ng_ipaddr_t *Addr, int Sock, int w_fd ));
static void Do_ResolveName PARAMS(( const char *Host, int w_fd ));

#ifdef WANT_IPV6
extern bool Conf_ConnectIPv4;
extern bool Conf_ConnectIPv6;
#endif


/**
 * Resolve IP address and do IDENT lookup asynchronously.
 */
GLOBAL bool
Resolve_Addr_Ident(PROC_STAT * s, const ng_ipaddr_t *Addr, int identsock,
		   void (*cbfunc) (int, short))
{
	int pipefd[2];
	pid_t pid;

	assert(s != NULL);

	pid = Proc_Fork(s, pipefd, cbfunc, RESOLVER_TIMEOUT);
	if (pid > 0) {
		LogDebug("Resolver for %s created (PID %d).", ng_ipaddr_tostr(Addr), pid);
		return true;
	} else if( pid == 0 ) {
		/* Sub process */
		Log_Init_Subprocess("Resolver");
		Conn_CloseAllSockets(identsock);
		Do_ResolveAddr_Ident(Addr, identsock, pipefd[1]);
		Log_Exit_Subprocess("Resolver");
		exit(0);
	}
	return false;
} /* Resolve_Addr */


/**
 * Resolve hostname (asynchronous!).
 */
GLOBAL bool
Resolve_Name( PROC_STAT *s, const char *Host, void (*cbfunc)(int, short))
{
	int pipefd[2];
	pid_t pid;

	assert(s != NULL);

	pid = Proc_Fork(s, pipefd, cbfunc, RESOLVER_TIMEOUT);
	if (pid > 0) {
		/* Main process */
		Log( LOG_DEBUG, "Resolver for \"%s\" created (PID %d).", Host, pid );
		return true;
	} else if( pid == 0 ) {
		/* Sub process */
		Log_Init_Subprocess("Resolver");
		Conn_CloseAllSockets(NONE);
		Do_ResolveName(Host, pipefd[1]);
		Log_Exit_Subprocess("Resolver");
		exit(0);
	}
	return false;
} /* Resolve_Name */

#if !defined(HAVE_WORKING_GETADDRINFO) || !defined(HAVE_GETNAMEINFO)
#ifdef h_errno
static char *
Get_Error( int H_Error )
{
	/* Get error message for H_Error */
	switch( H_Error ) {
	case HOST_NOT_FOUND:
		return "host not found";
	case NO_DATA:
		return "name valid but no IP address defined";
	case NO_RECOVERY:
		return "name server error";
	case TRY_AGAIN:
		return "name server temporary not available";
	}
	return "unknown error";
}
#endif
#endif


/* Do "IDENT" (aka "AUTH") lookup and append result to resolved_addr array */
static void
Do_IdentQuery(int identsock, array *resolved_addr)
{
#ifdef IDENTAUTH
	char *res;

	if (identsock < 0)
		return;

	Log_Subprocess(LOG_DEBUG, "Doing IDENT lookup on socket %d ...",
		       identsock);
	res = ident_id( identsock, 10 );
	Log_Subprocess(LOG_DEBUG, "Ok, IDENT lookup on socket %d done: \"%s\"",
		       identsock, res ? res : "(NULL)");
	if (!res) /* no result */
		return;
	if (!array_cats(resolved_addr, res))
		Log_Subprocess(LOG_WARNING,
			       "Resolver: Cannot copy IDENT result: %s!",
			       strerror(errno));

	free(res);
#else
	(void) identsock;
	(void) resolved_addr;
#endif
}


/**
 * perform reverse DNS lookup and put result string into resbuf.
 * If no hostname could be obtained, this function stores the string representation of
 * the IP address in resbuf and returns false.
 * @param IpAddr ip address to resolve
 * @param resbuf result buffer to store DNS name/string representation of ip address
 * @param reslen size of result buffer (must be >= NGT_INET_ADDRSTRLEN)
 * @return true if reverse lookup successful, false otherwise
 */
static bool
ReverseLookup(const ng_ipaddr_t *IpAddr, char *resbuf, size_t reslen)
{
	char tmp_ip_str[NG_INET_ADDRSTRLEN];
	const char *errmsg;
#ifdef HAVE_GETNAMEINFO
	static const char funcname[]="getnameinfo";
	int res;

	*resbuf = 0;

	res = getnameinfo((struct sockaddr *) IpAddr, ng_ipaddr_salen(IpAddr),
			  resbuf, (socklen_t)reslen, NULL, 0, NI_NAMEREQD);
	if (res == 0)
		return true;

	if (res == EAI_SYSTEM)
		errmsg = strerror(errno);
	else
		errmsg = gai_strerror(res);
#else
	const struct sockaddr_in *Addr = (const struct sockaddr_in *) IpAddr;
	struct hostent *h;
	static const char funcname[]="gethostbyaddr";

	h = gethostbyaddr((char *)&Addr->sin_addr, sizeof(Addr->sin_addr), AF_INET);
	if (h) {
		if (strlcpy(resbuf, h->h_name, reslen) < reslen)
			return true;
		errmsg = "hostname too long";
	} else {
# ifdef h_errno
		errmsg = Get_Error(h_errno);
# else
		errmsg = "unknown error";
# endif /* h_errno */
	}
#endif	/* HAVE_GETNAMEINFO */

	assert(errmsg);
	assert(reslen >= NG_INET_ADDRSTRLEN);
	ng_ipaddr_tostr_r(IpAddr, tmp_ip_str);

	Log_Subprocess(LOG_WARNING, "Can't resolve address \"%s\": %s [%s].",
		       tmp_ip_str, errmsg, funcname);
	strlcpy(resbuf, tmp_ip_str, reslen);
	return false;
}


/**
 * perform DNS lookup of given host name and fill IpAddr with a list of
 * ip addresses associated with that name.
 * ip addresses found are stored in the "array *IpAddr" argument (type ng_ipaddr_t)
 * @param hostname The domain name to look up.
 * @param IpAddr pointer to empty and initialized array to store results
 * @return true if lookup successful, false if domain name not found
 */
static bool
#ifdef HAVE_WORKING_GETADDRINFO
ForwardLookup(const char *hostname, array *IpAddr, int af)
#else
ForwardLookup(const char *hostname, array *IpAddr, UNUSED int af)
#endif
{
	ng_ipaddr_t addr;

#ifdef HAVE_WORKING_GETADDRINFO
	int res;
	struct addrinfo *a, *ai_results;
	static struct addrinfo hints;

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_family = af;

	memset(&addr, 0, sizeof(addr));

	res = getaddrinfo(hostname, NULL, &hints, &ai_results);
	switch (res) {
	case 0:	break;
	case EAI_SYSTEM:
		Log_Subprocess(LOG_WARNING, "Can't resolve \"%s\": %s", hostname, strerror(errno));
		return false;
	default:
		Log_Subprocess(LOG_WARNING, "Can't resolve \"%s\": %s", hostname, gai_strerror(res));
		return false;
	}

	for (a = ai_results; a != NULL; a = a->ai_next) {
		assert((size_t)a->ai_addrlen <= sizeof(addr));

		if ((size_t)a->ai_addrlen > sizeof(addr))
			continue;

		memcpy(&addr, a->ai_addr, a->ai_addrlen);

		if (!array_catb(IpAddr, (char *)&addr, sizeof(addr)))
			break;
	}

	freeaddrinfo(ai_results);
	return a == NULL;
#else
	struct hostent *h = gethostbyname(hostname);

	if (!h) {
#ifdef h_errno
		Log_Subprocess(LOG_WARNING, "Can't resolve \"%s\": %s",
			       hostname, Get_Error(h_errno));
#else
		Log_Subprocess(LOG_WARNING, "Can't resolve \"%s\"", hostname);
#endif
		return false;
	}
	memset(&addr, 0, sizeof(addr));

	addr.sin4.sin_family = AF_INET;
	memcpy(&addr.sin4.sin_addr, h->h_addr, sizeof(struct in_addr));

	return array_copyb(IpAddr, (char *)&addr, sizeof(addr));
#endif /* HAVE_GETADDRINFO */
}


static bool
Addr_in_list(const array *resolved_addr, const ng_ipaddr_t *Addr)
{
	char tmp_ip_str[NG_INET_ADDRSTRLEN];
	const ng_ipaddr_t *tmpAddrs = array_start(resolved_addr);
	size_t len = array_length(resolved_addr, sizeof(*tmpAddrs));

	assert(len > 0);
	assert(tmpAddrs);

	while (len > 0) {
		if (ng_ipaddr_ipequal(Addr, tmpAddrs))
			return true;
		tmpAddrs++;
		len--;
	}
	/* failed; print list of addresses */
	ng_ipaddr_tostr_r(Addr, tmp_ip_str);
	len = array_length(resolved_addr, sizeof(*tmpAddrs));
	tmpAddrs = array_start(resolved_addr);

	while (len > 0) {
		Log_Subprocess(LOG_WARNING, "Address mismatch: %s != %s",
			tmp_ip_str, ng_ipaddr_tostr(tmpAddrs));
		tmpAddrs++;
		len--;
	}

	return false;
}


static void
Log_Forgery_NoIP(const char *ip, const char *host)
{
	Log_Subprocess(LOG_WARNING,
		"Possible forgery: %s resolved to \"%s\", which has no IP address!",
		ip, host);
}

static void
Log_Forgery_WrongIP(const char *ip, const char *host)
{
	Log_Subprocess(LOG_WARNING,
		"Possible forgery: %s resolved to \"%s\", which points to a different address!",
		ip, host);
}


static void
ArrayWrite(int fd, const array *a)
{
	size_t len = array_bytes(a);
	const char *data = array_start(a);

	assert(data);

	if( (size_t)write(fd, data, len) != len )
		Log_Subprocess( LOG_CRIT, "Resolver: Can't write to parent: %s!",
							strerror(errno));
}


static void
Do_ResolveAddr_Ident(const ng_ipaddr_t *Addr, int identsock, int w_fd)
{
	/* Resolver sub-process: resolve IP address and write result into
	 * pipe to parent. */
	char hostname[CLIENT_HOST_LEN];
	char tmp_ip_str[NG_INET_ADDRSTRLEN];
	size_t len;
	array resolved_addr;

	hostname[0] = '\0';
	array_init(&resolved_addr);
	ng_ipaddr_tostr_r(Addr, tmp_ip_str);

	/* Skip DNS lookup when DNS is disabled; just return an empty ("") host
	 * name but still issue an IDENT query, if supported and enabled. */
	if (!Conf_DNS)
		goto dns_done;

	Log_Subprocess(LOG_DEBUG, "Now resolving %s ...", tmp_ip_str);
	if (!ReverseLookup(Addr, hostname, sizeof(hostname)))
		goto dns_done;

	if (ForwardLookup(hostname, &resolved_addr, ng_ipaddr_af(Addr))) {
		if (!Addr_in_list(&resolved_addr, Addr)) {
			Log_Forgery_WrongIP(tmp_ip_str, hostname);
			strlcpy(hostname, tmp_ip_str, sizeof(hostname));
		}
	} else {
		Log_Forgery_NoIP(tmp_ip_str, hostname);
		strlcpy(hostname, tmp_ip_str, sizeof(hostname));
	}
	Log_Subprocess(LOG_DEBUG, "Ok, translated %s to \"%s\".", tmp_ip_str, hostname);
 dns_done:
	len = strlen(hostname);
	hostname[len] = '\n';
	if (!array_copyb(&resolved_addr, hostname, ++len)) {
		Log_Subprocess(LOG_CRIT,
			       "Resolver: Can't copy resolved name: %s!",
			       strerror(errno));
		array_free(&resolved_addr);
		return;
	}

	Do_IdentQuery(identsock, &resolved_addr);

	ArrayWrite(w_fd, &resolved_addr);

	array_free(&resolved_addr);
} /* Do_ResolveAddr_Ident */


static void
Do_ResolveName( const char *Host, int w_fd )
{
	/* Resolver sub-process: resolve name and write result into pipe
	 * to parent. */
	array IpAddrs;
	int af;
	ng_ipaddr_t *addr;
	size_t len;
	Log_Subprocess(LOG_DEBUG, "Now resolving \"%s\" ...", Host);

	array_init(&IpAddrs);

#ifdef WANT_IPV6
	af = AF_UNSPEC;
	assert(Conf_ConnectIPv6 || Conf_ConnectIPv4);

	if (!Conf_ConnectIPv6)
		af = AF_INET;
	if (!Conf_ConnectIPv4)
		af = AF_INET6;
#else
	af = AF_INET;
#endif
	if (!ForwardLookup(Host, &IpAddrs, af)) {
		close(w_fd);
		return;
	}
	len = array_length(&IpAddrs, sizeof(*addr));
	assert(len > 0);
	addr = array_start(&IpAddrs);
	assert(addr);
	for (; len > 0; --len,addr++) {
		Log_Subprocess(LOG_DEBUG, "translated \"%s\" to %s.",
					Host, ng_ipaddr_tostr(addr));
	}
	/* Write result into pipe to parent */
	ArrayWrite(w_fd, &IpAddrs);

	array_free(&IpAddrs);
} /* Do_ResolveName */


/* -eof- */
