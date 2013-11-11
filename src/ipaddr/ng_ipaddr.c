/*
 * (c) 2008 Florian Westphal <fw@strlen.de>, public domain.
 */

#include "portab.h"

/**
 * @file
 * Functions for AF_ agnostic ipv4/ipv6 handling.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_GETADDRINFO
#include <netdb.h>
#include <sys/types.h>
#endif

#include "ng_ipaddr.h"

GLOBAL bool
ng_ipaddr_init(ng_ipaddr_t *addr, const char *ip_str, UINT16 port)
{
#ifdef HAVE_WORKING_GETADDRINFO
	int ret;
	char portstr[64];
	struct addrinfo *res0;
	struct addrinfo hints;

	assert(ip_str);

	memset(&hints, 0, sizeof(hints));
#ifdef AI_NUMERICHOST
	hints.ai_flags = AI_NUMERICHOST;
#endif
#ifndef WANT_IPV6	/* do not convert ipv6 addresses */
	hints.ai_family = AF_INET;
#endif

	/* some getaddrinfo implementations require that ai_socktype is set. */
	hints.ai_socktype = SOCK_STREAM;

	/* silly, but ngircd stores UINT16 in server config, not string */
	snprintf(portstr, sizeof(portstr), "%u", (unsigned int) port);

	ret = getaddrinfo(ip_str, portstr, &hints, &res0);
	if (ret != 0)
		return false;

	assert(sizeof(*addr) >= (size_t)res0->ai_addrlen);
	if (sizeof(*addr) >= (size_t)res0->ai_addrlen)
		memcpy(addr, res0->ai_addr, res0->ai_addrlen);
	else
		ret = -1;
	freeaddrinfo(res0);
	return ret == 0;
#else /* HAVE_GETADDRINFO */
	assert(ip_str);
	memset(addr, 0, sizeof *addr);
#ifdef HAVE_sockaddr_in_len
	addr->sin4.sin_len = sizeof(addr->sin4);
#endif
	addr->sin4.sin_family = AF_INET;
# ifdef HAVE_INET_ATON
	if (inet_aton(ip_str, &addr->sin4.sin_addr) == 0)
		return false;
# else
	addr->sin4.sin_addr.s_addr = inet_addr(ip_str);
	if (addr->sin4.sin_addr.s_addr == (unsigned) -1)
		return false;
# endif
	ng_ipaddr_setport(addr, port);
	return true;
#endif /* HAVE_GETADDRINFO */
}


GLOBAL void
ng_ipaddr_setport(ng_ipaddr_t *a, UINT16 port)
{
#ifdef WANT_IPV6
	int af;

	assert(a != NULL);

	af = a->sa.sa_family;

	assert(af == AF_INET || af == AF_INET6);

	switch (af) {
	case AF_INET:
		a->sin4.sin_port = htons(port);
		break;
	case AF_INET6:
		a->sin6.sin6_port = htons(port);
		break;
	}
#else /* WANT_IPV6 */
	assert(a != NULL);
	assert(a->sin4.sin_family == AF_INET);
	a->sin4.sin_port = htons(port);
#endif /* WANT_IPV6 */
}



GLOBAL bool
ng_ipaddr_ipequal(const ng_ipaddr_t *a, const ng_ipaddr_t *b)
{
	assert(a != NULL);
	assert(b != NULL);
#ifdef WANT_IPV6
	if (a->sa.sa_family != b->sa.sa_family)
		return false;
	assert(ng_ipaddr_salen(a) == ng_ipaddr_salen(b));
	switch (a->sa.sa_family) {
	case AF_INET6:
		return IN6_ARE_ADDR_EQUAL(&a->sin6.sin6_addr, &b->sin6.sin6_addr);
	case AF_INET:
		return memcmp(&a->sin4.sin_addr, &b->sin4.sin_addr, sizeof(a->sin4.sin_addr)) == 0;
	}
	return false;
#else
	assert(a->sin4.sin_family == AF_INET);
	assert(b->sin4.sin_family == AF_INET);
	return memcmp(&a->sin4.sin_addr, &b->sin4.sin_addr, sizeof(a->sin4.sin_addr)) == 0;
#endif
}


#ifdef WANT_IPV6
GLOBAL const char *
ng_ipaddr_tostr(const ng_ipaddr_t *addr)
{
	static char strbuf[NG_INET_ADDRSTRLEN];

	strbuf[0] = 0;

	ng_ipaddr_tostr_r(addr, strbuf);
	return strbuf;
}


/* str must be at least NG_INET_ADDRSTRLEN bytes long */
GLOBAL bool
ng_ipaddr_tostr_r(const ng_ipaddr_t *addr, char *str)
{
#ifdef HAVE_GETNAMEINFO
	const struct sockaddr *sa = (const struct sockaddr *) addr;
	int ret;

	*str = 0;

	ret = getnameinfo(sa, ng_ipaddr_salen(addr),
			str, NG_INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
	/*
	 * avoid leading ':'.
	 * causes mis-interpretation of client host in e.g. /WHOIS
	 */
	if (*str == ':') {
		char tmp[NG_INET_ADDRSTRLEN] = "0";
		ret = getnameinfo(sa, ng_ipaddr_salen(addr),
				  tmp + 1, (socklen_t)sizeof(tmp) - 1,
				  NULL, 0, NI_NUMERICHOST);
		if (ret == 0)
			strlcpy(str, tmp, NG_INET_ADDRSTRLEN);
	}
	assert (ret == 0);
	return ret == 0;
#else
	abort(); /* WANT_IPV6 depends on HAVE_GETNAMEINFO */
#endif
}

#endif /* WANT_IPV6 */

/* -eof- */
