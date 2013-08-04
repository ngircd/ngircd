/*
 * ngIRCd -- The Next Generation IRC Daemon
 */

#ifndef conn_ssl_h
#define conn_ssl_h

/**
 * @file
 * SSL wrapper functions (header)
 */

#include "conf-ssl.h"
#include "conn.h"
#include "conf.h"

#ifdef SSL_SUPPORT
GLOBAL void ConnSSL_Free PARAMS(( CONNECTION *c ));

GLOBAL bool ConnSSL_PrepareConnect PARAMS(( CONNECTION *c, CONF_SERVER *s ));

GLOBAL int ConnSSL_Accept PARAMS(( CONNECTION *c ));
GLOBAL int ConnSSL_Connect PARAMS(( CONNECTION *c ));

GLOBAL ssize_t ConnSSL_Write PARAMS(( CONNECTION *c, const void *buf, size_t count));
GLOBAL ssize_t ConnSSL_Read PARAMS(( CONNECTION *c, void *buf, size_t count));

GLOBAL bool ConnSSL_GetCipherInfo PARAMS(( CONNECTION *c, char *buf, size_t len ));
GLOBAL char *ConnSSL_GetCertFp PARAMS(( CONNECTION *c ));
GLOBAL bool ConnSSL_SetCertFp PARAMS(( CONNECTION *c, const char *fingerprint ));

#endif /* SSL_SUPPORT */
#endif /* conn_ssl_h */

/* -eof- */
