/*
 * ngIRCd -- The Next Generation IRC Daemon
 * Copyright (c) 2005-2008 Florian Westphal <fw@strlen.de>
 */

#include "portab.h"

/**
 * @file
 * SSL wrapper functions
 */

#include "imp.h"
#include "conf-ssl.h"

#ifdef SSL_SUPPORT

#include "io.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define CONN_MODULE
#include "conn.h"
#include "conf.h"
#include "conn-func.h"
#include "conn-ssl.h"
#include "log.h"

#include "exp.h"
#include "defines.h"

extern struct SSLOptions Conf_SSLOptions;

#ifdef HAVE_LIBSSL
#include <openssl/err.h>
#include <openssl/rand.h>

static SSL_CTX * ssl_ctx;
static DH *dh_params;

static bool ConnSSL_LoadServerKey_openssl PARAMS(( SSL_CTX *c ));
#endif

#ifdef HAVE_LIBGNUTLS
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <gnutls/x509.h>

#define DH_BITS 2048
#define DH_BITS_MIN 1024

static gnutls_certificate_credentials_t x509_cred;
static gnutls_dh_params_t dh_params;
static bool ConnSSL_LoadServerKey_gnutls PARAMS(( void ));
#endif

static bool ConnSSL_Init_SSL PARAMS(( CONNECTION *c ));
static int ConnectAccept PARAMS(( CONNECTION *c, bool connect ));
static int ConnSSL_HandleError PARAMS(( CONNECTION *c, const int code, const char *fname ));

#ifdef HAVE_LIBGNUTLS
static char * openreadclose(const char *name, size_t *len)
{
	char *buf = NULL;
	struct stat s;
	ssize_t br;
	int fd = open(name, O_RDONLY);
	if (fd < 0) {
		Log(LOG_ERR, "Could not open %s: %s", name, strerror(errno));
		return NULL;
	}
	if (fstat(fd, &s)) {
		Log(LOG_ERR, "Could not fstat %s: %s", name, strerror(errno));
		goto out;
	}
	if (!S_ISREG(s.st_mode)) {
		Log(LOG_ERR, "%s: Not a regular file", name);
		goto out;
	}
	if (s.st_size <= 0) {
		Log(LOG_ERR, "%s: invalid file length (size %ld <= 0)", name, (long) s.st_size);
		goto out;
	}
	buf = malloc(s.st_size);
	if (!buf) {
		Log(LOG_ERR, "Could not malloc %lu bytes for file %s: %s", s.st_size, name, strerror(errno));
		goto out;
	}
	br = read(fd, buf, s.st_size);
	if (br != (ssize_t)s.st_size) {
		Log(LOG_ERR, "Could not read file %s: read returned %ld, expected %ld: %s",
			name, (long) br, (long) s.st_size, br == -1 ? strerror(errno):"short read?!");
		memset(buf, 0, s.st_size);
		free(buf);
		buf = NULL;
	} else {
		*len = br;
	}
out:
	close(fd);
	return buf;
}
#endif


#ifdef HAVE_LIBSSL
static void
LogOpenSSLError( const char *msg, const char *msg2 )
{
	unsigned long err = ERR_get_error();
	char * errmsg = err ? ERR_error_string(err, NULL) : "Unable to determine error";

	if (!msg) msg = "SSL Error";
	if (msg2)
		Log( LOG_ERR, "%s: %s: %s", msg, msg2, errmsg);
	else
		Log( LOG_ERR, "%s: %s", msg, errmsg);
}


static int
pem_passwd_cb(char *buf, int size, int rwflag, void *password)
{
	array *pass = password;
	int passlen;

	(void)rwflag;		/* rwflag is unused if DEBUG is not set. */
	assert(rwflag == 0);	/* 0 -> callback used for decryption.
				 * See SSL_CTX_set_default_passwd_cb(3) */

	passlen = (int) array_bytes(pass);

	LogDebug("pem_passwd_cb buf size %d, array size %d", size, passlen);
	assert(passlen >= 0);
	if (passlen <= 0) {
		Log(LOG_ERR, "pem_passwd_cb: password required, but not set");
		return 0;
	}
	size = passlen > size ? size : passlen;
	memcpy(buf, (char *)(array_start(pass)), size);
	return size;
}
#endif


static bool
Load_DH_params(void)
{
#ifdef HAVE_LIBSSL
	FILE *fp;
	bool ret = true;

	if (!Conf_SSLOptions.DHFile) {
		Log(LOG_NOTICE, "Configuration option \"SSLDHFile\" not set!");
		return false;
	}
	fp = fopen(Conf_SSLOptions.DHFile, "r");
	if (!fp) {
		Log(LOG_ERR, "%s: %s", Conf_SSLOptions.DHFile, strerror(errno));
		return false;
	}
	dh_params = PEM_read_DHparams(fp, NULL, NULL, NULL);
	if (!dh_params) {
		Log(LOG_ERR, "%s: PEM_read_DHparams failed!",
		    Conf_SSLOptions.DHFile);
		ret = false;
	}
	fclose(fp);
	return ret;
#endif
#ifdef HAVE_LIBGNUTLS
	bool need_dhgenerate = true;
	int err;
	gnutls_dh_params_t tmp_dh_params;

	err = gnutls_dh_params_init(&tmp_dh_params);
	if (err < 0) {
		Log(LOG_ERR, "gnutls_dh_params_init: %s", gnutls_strerror(err));
		return false;
	}
	if (Conf_SSLOptions.DHFile) {
		gnutls_datum_t dhparms;
		size_t size;
		dhparms.data = (unsigned char *) openreadclose(Conf_SSLOptions.DHFile, &size);
		if (dhparms.data) {
			dhparms.size = size;
			err = gnutls_dh_params_import_pkcs3(tmp_dh_params, &dhparms, GNUTLS_X509_FMT_PEM);
			if (err == 0)
				need_dhgenerate = false;
			else
				Log(LOG_ERR, "gnutls_dh_params_init: %s", gnutls_strerror(err));

			memset(dhparms.data, 0, size);
			free(dhparms.data);
		}
	}
	if (need_dhgenerate) {
		Log(LOG_WARNING,
		    "SSLDHFile not set, generating %u bit DH parameters. This may take a while ...",
		    DH_BITS);
		err = gnutls_dh_params_generate2(tmp_dh_params, DH_BITS);
		if (err < 0) {
			Log(LOG_ERR, "gnutls_dh_params_generate2: %s", gnutls_strerror(err));
			return false;
		}
        }
	dh_params = tmp_dh_params;
	return true;
#endif
}


void ConnSSL_Free(CONNECTION *c)
{
#ifdef HAVE_LIBSSL
	SSL *ssl = c->ssl_state.ssl;
	if (ssl) {
		SSL_shutdown(ssl);
		SSL_free(ssl);
		c->ssl_state.ssl = NULL;
	}
#endif
#ifdef HAVE_LIBGNUTLS
	gnutls_session_t sess = c->ssl_state.gnutls_session;
	if (Conn_OPTION_ISSET(c, CONN_SSL)) {
		gnutls_bye(sess, GNUTLS_SHUT_RDWR);
		gnutls_deinit(sess);
	}
#endif
	assert(Conn_OPTION_ISSET(c, CONN_SSL));
	/* can't just set bitmask to 0 -- there are other, non-ssl related flags, e.g. CONN_ZIP. */
	Conn_OPTION_DEL(c, CONN_SSL_FLAGS_ALL);
}


bool
ConnSSL_InitLibrary( void )
{
#ifdef HAVE_LIBSSL
	SSL_CTX *newctx;

	if (!ssl_ctx) {
		SSL_library_init();
		SSL_load_error_strings();
	}

	if (!RAND_status()) {
		Log(LOG_ERR, "OpenSSL PRNG not seeded: /dev/urandom missing?");
		/*
		 * it is probably best to fail and let the user install EGD or a similar program if no kernel random device is available.
		 * According to OpenSSL RAND_egd(3): "The automatic query of /var/run/egd-pool et al was added in OpenSSL 0.9.7";
		 * so it makes little sense to deal with PRNGD seeding ourselves.
		 */
		return false;
	}

	newctx = SSL_CTX_new(SSLv23_method());
	if (!newctx) {
		LogOpenSSLError("SSL_CTX_new()", NULL);
		return false;
	}

	if (!ConnSSL_LoadServerKey_openssl(newctx))
		goto out;

	SSL_CTX_set_options(newctx, SSL_OP_SINGLE_DH_USE|SSL_OP_NO_SSLv2);
	SSL_CTX_set_mode(newctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
	SSL_CTX_free(ssl_ctx);
	ssl_ctx = newctx;
	Log(LOG_INFO, "%s initialized.", SSLeay_version(SSLEAY_VERSION));
	return true;
out:
	SSL_CTX_free(newctx);
	return false;
#endif
#ifdef HAVE_LIBGNUTLS
	int err;
	static bool initialized;
	if (initialized) /* TODO: cannot reload gnutls keys: can't simply free x509 context -- it may still be in use */
		return false;

	err = gnutls_global_init();
	if (err) {
		Log(LOG_ERR, "gnutls_global_init(): %s", gnutls_strerror(err));
		return false;
	}
	if (!ConnSSL_LoadServerKey_gnutls())
		return false;
	Log(LOG_INFO, "gnutls %s initialized.", gnutls_check_version(NULL));
	initialized = true;
	return true;
#endif
}


#ifdef HAVE_LIBGNUTLS
static bool
ConnSSL_LoadServerKey_gnutls(void)
{
	int err;
	const char *cert_file;

	err = gnutls_certificate_allocate_credentials(&x509_cred);
	if (err < 0) {
		Log(LOG_ERR, "gnutls_certificate_allocate_credentials: %s", gnutls_strerror(err));
		return false;
	}

	cert_file = Conf_SSLOptions.CertFile ? Conf_SSLOptions.CertFile:Conf_SSLOptions.KeyFile;
	if (!cert_file) {
		Log(LOG_NOTICE, "No SSL server key configured, SSL disabled.");
		return false;
	}

	if (array_bytes(&Conf_SSLOptions.KeyFilePassword))
		Log(LOG_WARNING,
		    "Ignoring KeyFilePassword: Not supported by GNUTLS.");

	if (!Load_DH_params())
		return false;

	gnutls_certificate_set_dh_params(x509_cred, dh_params);
	err = gnutls_certificate_set_x509_key_file(x509_cred, cert_file, Conf_SSLOptions.KeyFile, GNUTLS_X509_FMT_PEM);
	if (err < 0) {
		Log(LOG_ERR, "gnutls_certificate_set_x509_key_file (cert %s, key %s): %s",
				cert_file, Conf_SSLOptions.KeyFile ? Conf_SSLOptions.KeyFile : "(NULL)", gnutls_strerror(err));
		return false;
	}
	return true;
}
#endif


#ifdef HAVE_LIBSSL
static bool
ConnSSL_LoadServerKey_openssl(SSL_CTX *ctx)
{
	char *cert_key;

	assert(ctx);
	if (!Conf_SSLOptions.KeyFile) {
		Log(LOG_NOTICE, "No SSL server key configured, SSL disabled.");
		return false;
	}

	SSL_CTX_set_default_passwd_cb(ctx, pem_passwd_cb);
	SSL_CTX_set_default_passwd_cb_userdata(ctx, &Conf_SSLOptions.KeyFilePassword);

	if (SSL_CTX_use_PrivateKey_file(ctx, Conf_SSLOptions.KeyFile, SSL_FILETYPE_PEM) != 1) {
		array_free_wipe(&Conf_SSLOptions.KeyFilePassword);
		LogOpenSSLError("SSL_CTX_use_PrivateKey_file",  Conf_SSLOptions.KeyFile);
		return false;
	}

	cert_key = Conf_SSLOptions.CertFile ? Conf_SSLOptions.CertFile:Conf_SSLOptions.KeyFile;
	if (SSL_CTX_use_certificate_chain_file(ctx, cert_key) != 1) {
		array_free_wipe(&Conf_SSLOptions.KeyFilePassword);
		LogOpenSSLError("SSL_CTX_use_certificate_file", cert_key);
		return false;
	}

	array_free_wipe(&Conf_SSLOptions.KeyFilePassword);

	if (!SSL_CTX_check_private_key(ctx)) {
		LogOpenSSLError("Server Private Key does not match certificate", NULL);
		return false;
	}
	if (Load_DH_params()) {
		if (SSL_CTX_set_tmp_dh(ctx, dh_params) != 1)
			LogOpenSSLError("Error setting DH Parameters", Conf_SSLOptions.DHFile);
		/* don't return false here: the non-DH modes will still work */
		DH_free(dh_params);
		dh_params = NULL;
	}
	return true;
}


#endif
static bool
ConnSSL_Init_SSL(CONNECTION *c)
{
	int ret;
	assert(c != NULL);
#ifdef HAVE_LIBSSL
	if (!ssl_ctx) {
		Log(LOG_ERR, "Cannot init ssl_ctx: OpenSSL initialization failed at startup");
		return false;
	}
	assert(c->ssl_state.ssl == NULL);

	c->ssl_state.ssl = SSL_new(ssl_ctx);
	if (!c->ssl_state.ssl) {
		LogOpenSSLError("SSL_new()", NULL);
		return false;
	}

	ret = SSL_set_fd(c->ssl_state.ssl, c->sock);
	if (ret != 1) {
		LogOpenSSLError("SSL_set_fd()", NULL);
		ConnSSL_Free(c);
		return false;
	}
#endif
#ifdef HAVE_LIBGNUTLS
	ret = gnutls_set_default_priority(c->ssl_state.gnutls_session);
	if (ret < 0) {
		Log(LOG_ERR, "gnutls_set_default_priority: %s", gnutls_strerror(ret));
		ConnSSL_Free(c);
		return false;
	}
	/*
	 * The intermediate (long) cast is here to avoid a warning like:
	 * "cast to pointer from integer of different size" on 64-bit platforms.
	 * There doesn't seem to be an alternate GNUTLS API we could use instead, see e.g.
	 * http://www.mail-archive.com/help-gnutls@gnu.org/msg00286.html
	 */
	gnutls_transport_set_ptr(c->ssl_state.gnutls_session, (gnutls_transport_ptr_t) (long) c->sock);
	ret = gnutls_credentials_set(c->ssl_state.gnutls_session, GNUTLS_CRD_CERTIFICATE, x509_cred);
	if (ret < 0) {
		Log(LOG_ERR, "gnutls_credentials_set: %s", gnutls_strerror(ret));
		ConnSSL_Free(c);
		return false;
	}
	gnutls_dh_set_prime_bits(c->ssl_state.gnutls_session, DH_BITS_MIN);
#endif
	Conn_OPTION_ADD(c, CONN_SSL);
	return true;
}


bool
ConnSSL_PrepareConnect(CONNECTION *c, UNUSED CONF_SERVER *s)
{
	bool ret;
#ifdef HAVE_LIBGNUTLS
	int err;

	err = gnutls_init(&c->ssl_state.gnutls_session, GNUTLS_CLIENT);
	if (err) {
		Log(LOG_ERR, "gnutls_init: %s", gnutls_strerror(err));
		return false;
        }
#endif
	ret = ConnSSL_Init_SSL(c);
	if (!ret)
		return false;
	Conn_OPTION_ADD(c, CONN_SSL_CONNECT);
#ifdef HAVE_LIBSSL
	assert(c->ssl_state.ssl);
	SSL_set_verify(c->ssl_state.ssl, SSL_VERIFY_NONE, NULL);
#endif
	return true;
}


/*
  Check an Handle Error return code after failed calls to ssl/tls functions.
  OpenSSL:
  SSL_connect(), SSL_accept(), SSL_do_handshake(), SSL_read(), SSL_peek(), or SSL_write() on ssl.
  GNUTLS:
  gnutlsssl_read(), gnutls_write() or gnutls_handshake().
  Return: -1 on fatal error, 0 if we can try again later.
 */
static int
ConnSSL_HandleError( CONNECTION *c, const int code, const char *fname )
{
#ifdef HAVE_LIBSSL
	int ret = SSL_ERROR_SYSCALL;
	unsigned long sslerr;
	int real_errno = errno;

	ret = SSL_get_error(c->ssl_state.ssl, code);
	switch (ret) {
	case SSL_ERROR_WANT_READ:
		io_event_del(c->sock, IO_WANTWRITE);
		Conn_OPTION_ADD(c, CONN_SSL_WANT_READ);
		return 0;	/* try again later */
	case SSL_ERROR_WANT_WRITE:
		io_event_del(c->sock, IO_WANTREAD);
		Conn_OPTION_ADD(c, CONN_SSL_WANT_WRITE); /* fall through */
	case SSL_ERROR_NONE:
		return 0;	/* try again later */
	case SSL_ERROR_ZERO_RETURN:
		LogDebug("TLS/SSL connection shut down normally");
		break;
	/*
	SSL_ERROR_WANT_CONNECT, SSL_ERROR_WANT_ACCEPT, SSL_ERROR_WANT_X509_LOOKUP
	*/
	case SSL_ERROR_SYSCALL:
		sslerr = ERR_get_error();
		if (sslerr) {
			Log( LOG_ERR, "%s: %s", fname, ERR_error_string(sslerr, NULL ));
		} else {

			switch (code) {	/* EOF that violated protocol */
			case 0:
				Log(LOG_ERR, "%s: Client Disconnected", fname );
				break;
			case -1: /* low level socket I/O error, check errno */
				Log(LOG_ERR, "%s: %s", fname, strerror(real_errno));
			}
		}
		break;
	case SSL_ERROR_SSL:
		LogOpenSSLError("TLS/SSL Protocol Error", fname);
		break;
	default:
		Log( LOG_ERR, "%s: Unknown error %d!", fname, ret);
	}
	ConnSSL_Free(c);
	return -1;
#endif
#ifdef HAVE_LIBGNUTLS
	switch (code) {
	case GNUTLS_E_AGAIN:
	case GNUTLS_E_INTERRUPTED:
		if (gnutls_record_get_direction(c->ssl_state.gnutls_session)) {
			Conn_OPTION_ADD(c, CONN_SSL_WANT_WRITE);
			io_event_del(c->sock, IO_WANTREAD);
		} else {
			Conn_OPTION_ADD(c, CONN_SSL_WANT_READ);
			io_event_del(c->sock, IO_WANTWRITE);
		}
		break;
	default:
		assert(code < 0);
		if (gnutls_error_is_fatal(code)) {
			Log(LOG_ERR, "%s: %s", fname, gnutls_strerror(code));
			ConnSSL_Free(c);
			return -1;
		}
	}
	return 0;
#endif
}


static void
ConnSSL_LogCertInfo( CONNECTION *c )
{
#ifdef HAVE_LIBSSL
	SSL *ssl = c->ssl_state.ssl;

	assert(ssl);

	Log(LOG_INFO, "Connection %d: initialized %s using cipher %s.",
		c->sock, SSL_get_version(ssl), SSL_get_cipher(ssl));
#endif
#ifdef HAVE_LIBGNUTLS
	gnutls_session_t sess = c->ssl_state.gnutls_session;
	gnutls_cipher_algorithm_t cipher = gnutls_cipher_get(sess);

	Log(LOG_INFO, "Connection %d: initialized %s using cipher %s-%s.",
	    c->sock,
	    gnutls_protocol_get_name(gnutls_protocol_get_version(sess)),
	    gnutls_cipher_get_name(cipher),
	    gnutls_mac_get_name(gnutls_mac_get(sess)));
#endif
}


/*
 Accept incoming SSL connection.
 Return Values:
	 1: SSL Connection established
	 0: try again
	-1: SSL Connection not established due to fatal error.
*/
int
ConnSSL_Accept( CONNECTION *c )
{
	assert(c != NULL);
	if (!Conn_OPTION_ISSET(c, CONN_SSL)) {
#ifdef HAVE_LIBGNUTLS
		int err = gnutls_init(&c->ssl_state.gnutls_session, GNUTLS_SERVER);
		if (err) {
			Log(LOG_ERR, "gnutls_init: %s", gnutls_strerror(err));
			return false;
		}
#endif
		LogDebug("ConnSSL_Accept: Initializing SSL data");
		if (!ConnSSL_Init_SSL(c))
			return -1;
	}
	return ConnectAccept(c, false );
}


int
ConnSSL_Connect( CONNECTION *c )
{
	assert(c != NULL);
#ifdef HAVE_LIBSSL
	assert(c->ssl_state.ssl);
#endif
	assert(Conn_OPTION_ISSET(c, CONN_SSL));
	return ConnectAccept(c, true);
}


/* accept/connect wrapper. if connect is true, connect to peer, otherwise wait for incoming connection */
static int
ConnectAccept( CONNECTION *c, bool connect)
{
	int ret;
#ifdef HAVE_LIBSSL
	SSL *ssl = c->ssl_state.ssl;
	assert(ssl != NULL);

	ret = connect ? SSL_connect(ssl) : SSL_accept(ssl);
	if (1 != ret)
		return ConnSSL_HandleError(c, ret, connect ? "SSL_connect": "SSL_accept");
#endif
#ifdef HAVE_LIBGNUTLS
	(void) connect;
	ret = gnutls_handshake(c->ssl_state.gnutls_session);
	if (ret)
		return ConnSSL_HandleError(c, ret, "gnutls_handshake");
#endif /* _GNUTLS */
	Conn_OPTION_DEL(c, (CONN_SSL_WANT_WRITE|CONN_SSL_WANT_READ|CONN_SSL_CONNECT));
	ConnSSL_LogCertInfo(c);

	Conn_StartLogin(CONNECTION2ID(c));
	return 1;
}


ssize_t
ConnSSL_Write(CONNECTION *c, const void *buf, size_t count)
{
	ssize_t bw;

	Conn_OPTION_DEL(c, CONN_SSL_WANT_WRITE|CONN_SSL_WANT_READ);

	assert(count > 0);
#ifdef HAVE_LIBSSL
	bw = (ssize_t) SSL_write(c->ssl_state.ssl, buf, count);
#endif
#ifdef HAVE_LIBGNUTLS
	bw = gnutls_write(c->ssl_state.gnutls_session, buf, count);
#endif
	if (bw > 0)
		return bw;
	if (ConnSSL_HandleError( c, bw, "ConnSSL_Write") == 0)
		errno = EAGAIN; /* try again */
	return -1;
}


ssize_t
ConnSSL_Read(CONNECTION *c, void * buf, size_t count)
{
	ssize_t br;

	Conn_OPTION_DEL(c, CONN_SSL_WANT_WRITE|CONN_SSL_WANT_READ);
#ifdef HAVE_LIBSSL
        br = (ssize_t) SSL_read(c->ssl_state.ssl, buf, count);
	if (br > 0)	/* on EOF we have to call ConnSSL_HandleError(), see SSL_read(3) */
		return br;
#endif
#ifdef HAVE_LIBGNUTLS
	br = gnutls_read(c->ssl_state.gnutls_session, buf, count);
	if (br >= 0)	/* on EOF we must _not_ call ConnSSL_HandleError, see gnutls_record_recv(3) */
		return br;
#endif
	/* error on read: switch ConnSSL_HandleError() return values -> 0 is "try again", so return -1 and set EAGAIN */
	if (ConnSSL_HandleError(c, br, "ConnSSL_Read") == 0) {
		errno = EAGAIN;
		return -1;
	}
	return 0;
}


bool
ConnSSL_GetCipherInfo(CONNECTION *c, char *buf, size_t len)
{
#ifdef HAVE_LIBSSL
	char *nl;
	SSL *ssl = c->ssl_state.ssl;

	if (!ssl)
		return false;
	*buf = 0;
	SSL_CIPHER_description(SSL_get_current_cipher(ssl), buf, len);
	nl = strchr(buf, '\n');
	if (nl)
		*nl = 0;
	return true;
#endif
#ifdef HAVE_LIBGNUTLS
	if (Conn_OPTION_ISSET(c, CONN_SSL)) {
		const char *name_cipher, *name_mac, *name_proto, *name_keyexchange;
		unsigned keysize;

		gnutls_session_t sess = c->ssl_state.gnutls_session;
		gnutls_cipher_algorithm_t cipher = gnutls_cipher_get(sess);
		name_cipher = gnutls_cipher_get_name(cipher);
		name_mac = gnutls_mac_get_name(gnutls_mac_get(sess));
		keysize = gnutls_cipher_get_key_size(cipher) * 8;
		name_proto = gnutls_protocol_get_name(gnutls_protocol_get_version(sess));
		name_keyexchange = gnutls_kx_get_name(gnutls_kx_get(sess));

		return snprintf(buf, len, "%s-%s%15s Kx=%s      Enc=%s(%u) Mac=%s",
			name_cipher, name_mac, name_proto, name_keyexchange, name_cipher, keysize, name_mac) > 0;
	}
	return false;
#endif
}


#endif /* SSL_SUPPORT */
/* -eof- */


