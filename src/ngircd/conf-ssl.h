/*
 * ngIRCd -- The Next Generation IRC Daemon
 * SSL defines.
 */

#ifndef conf_ssl_h
#define conf_ssl_h

#ifdef HAVE_LIBSSL
#define SSL_SUPPORT
#include <openssl/ssl.h>
#endif
#ifdef HAVE_LIBGNUTLS
#define SSL_SUPPORT
#include <gnutls/gnutls.h>
#endif

#ifdef SSL_SUPPORT
struct ConnSSL_State {
#ifdef HAVE_LIBSSL
	SSL *ssl;
#endif
#ifdef HAVE_LIBGNUTLS
	gnutls_session_t gnutls_session;
	void *cookie;	/* pointer to server configuration structure (for outgoing connections), or NULL. */
#endif
};


bool
ConnSSL_InitLibrary(void);
#else
static inline bool
ConnSSL_InitLibrary(void) { return true; }
#endif /* SSL_SUPPORT */
#endif /* conf_ssl_h */
/* -eof- */
