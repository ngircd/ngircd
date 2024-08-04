# [ngIRCd](https://ngircd.barton.de) - SSL/TLS Encrypted Connections

ngIRCd supports SSL/TLS encrypted connections using the *OpenSSL* or *GnuTLS*
libraries. Both encrypted server-server links as well as client-server links
are supported.

SSL is a compile-time option which is disabled by default. Use one of these
options of the ./configure script to enable it:

- `--with-openssl`: enable SSL support using OpenSSL.
- `--with-gnutls`: enable SSL support using GnuTLS.

You can check the output of `ngircd --version` to validate if your executable
includes support for SSL or not: "+SSL" must be listed in the feature flags.

You also need a SSL key and certificate, for example using Let's Encrypt, which
is out of the scope of this document.

From a feature point of view, ngIRCds support for both libraries is
comparable. The only major difference (at this time) is that ngIRCd with GnuTLS
does not support password protected private keys.

## Configuration

SSL-encrypted connections and plain-text connects can't run on the same network
port (which is a limitation of the IRC protocol); therefore you have to define
separate port(s) in your `[SSL]` block in the configuration file.

A minimal configuration for *accepting* SSL-encrypted client
connections looks like this:

``` ini
[SSL]
CertFile = /etc/ssl/certs/my-fullchain.pem
KeyFile = /etc/ssl/certs/my-privkey.pem
Ports = 6697, 6698
```

In this case, the server only deals with unauthenticated incoming
connections and never has to validate SSL certificates itself, and therefore
no "Certificate Authorities" are needed.

If you want to use *outgoing* SSL-connections to other servers or accept
incoming *server* connections, you need to add:

``` ini
[SSL]
...
CAFile = /etc/ssl/certs/ca-certificates.crt
DHFile = /etc/ngircd/dhparams.pem

[SERVER]
...
SSLConnect = yes
```

The `CAFile` option configures a file listing all the certificates of the
trusted Certificate Authorities.

The Diffie-Hellman parameters file `dhparams.pem` can be created like this:

- OpenSSL: `openssl dhparam -2 -out /etc/ngircd/dhparams.pem 4096`
- GnuTLS: `certtool --generate-dh-params --bits 4096 --outfile /etc/ngircd/dhparams.pem`

Note that enabling `SSLConnect` not only enforces SSL-encrypted links for
*outgoing* connections to other servers, but for *incoming* connections as well:
If a server configured with `SSLConnect = yes` tries to connect on a plain-text
connection, it won't be accepted to prevent data leakage! Therefore you should
set this for *all* servers you expect to use SSL-encrypted connections!

## Accepting untrusted Remote Certificates

If you are using self-signed certificates or otherwise invalid certificates,
which ngIRCd would reject by default, you can force ngIRCd to skip certificate
validation on a per-server basis and continue establishing outgoing connections
to the respective peer by setting `SSLVerify = no` in the `[SERVER]` block of
this remote server in your configuration.

But please think twice before doing so: the established connection is still
encrypted but the remote site is *not verified at all* and man-in-the-middle
attacks are possible!
