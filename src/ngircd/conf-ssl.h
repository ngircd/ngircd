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
#ifndef LIBGNUTLS_VERSION_MAJOR
#define gnutls_certificate_credentials_t gnutls_certificate_credentials
#define gnutls_cipher_algorithm_t gnutls_cipher_algorithm
#define gnutls_datum_t gnutls_datum
#define gnutls_dh_params_t gnutls_dh_params
#define gnutls_session_t gnutls_session
#define gnutls_transport_ptr_t gnutls_transport_ptr
#endif
#endif

#ifdef SSL_SUPPORT
struct ConnSSL_State {
#ifdef HAVE_LIBSSL
	SSL *ssl;
#endif
#ifdef HAVE_LIBGNUTLS
	gnutls_session gnutls_session;
	void *cookie;		/* pointer to server configuration structure
				   (for outgoing connections), or NULL. */
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
