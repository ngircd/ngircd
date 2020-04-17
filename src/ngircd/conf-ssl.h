/*
 * ngIRCd -- The Next Generation IRC Daemon
 */

#ifndef conf_ssl_h
#define conf_ssl_h

/**
 * @file
 * SSL related definitions
 */

#ifdef HAVE_LIBSSL
#define SSL_SUPPORT
#include <openssl/ssl.h>
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define OpenSSL_version SSLeay_version
#define OPENSSL_VERSION SSLEAY_VERSION
#endif
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
	gnutls_session_t gnutls_session;
	void *cookie;		/* pointer to server configuration structure
				   (for outgoing connections), or NULL. */
	size_t x509_cred_idx;	/* index of active x509 credential record */
#endif
	char *fingerprint;
};

#endif

GLOBAL bool ConnSSL_InitLibrary PARAMS((void));

#endif /* conf_ssl_h */

/* -eof- */
