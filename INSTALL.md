# [ngIRCd](https://ngircd.barton.de) - Installation

This document describes how to install ngIRCd, the lightweight Internet Relay
Chat (IRC) server.

The first section lists noteworthy changes to earlier releases; you definitely
should read this when upgrading your setup! But you can skip over this section
when you are working on a fresh installation.

The subsequent sections describe the steps required to build and install ngIRCd
_from sources_. The information given here is not relevant when you are using
packages provided by your operating system vendor or third-party repositories!

Please see the file `doc/QuickStart.md` in the `doc/` directory or on
[GitHub](https://github.com/ngircd/ngircd/blob/master/doc/QuickStart.md) for
information about _setting up_ and _running_ ngIRCd, including some real-world
configuration examples.

## Upgrade Information

This section lists important updates and breaking changes that you should be
aware of *before* starting the upgrade:

Differences to version 26

- **Attention**:
  Starting with release 27, ngIRCd validates SSL/TLS certificates on outgoing
  server-server links by default and drops(!) connections when the remote
  certificate is invalid (for example self-signed, expired, not matching the
  host name, ...). Therefore you have to make sure that all relevant
  *certificates are valid* (or to disable certificate validation on this
  connection using the new `SSLVerify = false` setting in the affected
  `[Server]` block, where the remote certificate is not valid and you can not
  fix this issue).

Differences to version 25

- **Attention**:
  All already deprecated legacy options (besides the newly deprecated *Key* and
  *MaxUsers* settings, see below) were removed in ngIRCd 26, so make sure to
  update your configuration before upgrading, if you haven't done so already
  (you got a warning on daemon startup when using deprecated options): you can
  check your configuration using `ngircd --configtest` -- which is a good idea
  anyway ;-)

- Setting modes for predefined channels in *[Channel]* sections has been
  enhanced: now you can set *all* modes, like in IRC "MODE" commands, and have
  this setting multiple times per *[Channel]* block. Modifying lists (ban list,
  invite list, exception list) is supported, too.

  Both the *Key* and *MaxUsers* settings are now deprecated and should be
  replaced by `Modes = +l <limit>` and `Modes = +k <key>` respectively.

Differences to version 22.x

- The *NoticeAuth* `ngircd.conf` configuration variable has been renamed to
  *NoticeBeforeRegistration*. The old *NoticeAuth* variable still works but
  is deprecated now.

- The default value of the SSL *CipherList* variable has been changed to
  "HIGH:!aNULL:@STRENGTH:!SSLv3" (OpenSSL) and "SECURE128:-VERS-SSL3.0"
  (GnuTLS) to disable the old SSLv3 protocol by default.

  To enable connections of clients still requiring the weak SSLv3 protocol,
  the *CipherList* must be set to its old value (not recommended!), which
  was "HIGH:!aNULL:@STRENGTH" (OpenSSL) and "SECURE128" (GnuTLS), see below.

Differences to version 20.x

- Starting with ngIRCd 21, the ciphers used by SSL are configurable and
  default to "HIGH:!aNULL:@STRENGTH" (OpenSSL) or "SECURE128" (GnuTLS).
  Previous version were using the OpenSSL or GnuTLS defaults, "DEFAULT"
  and "NORMAL" respectively.

- When adding GLINE's or KLINE's to ngIRCd 21 (or newer), all clients matching
  the new mask will be KILL'ed. This was not the case with earlier versions
  that only added the mask but didn't kill already connected users.

- The *PredefChannelsOnly* configuration variable has been superseded by the
  new *AllowedChannelTypes* variable. It is still supported and translated to
  the appropriate *AllowedChannelTypes* setting but is deprecated now.

Differences to version 19.x

- Starting with ngIRCd 20, users can "cloak" their hostname only when the
  configuration variable *CloakHostModeX* (introduced in 19.2) is set.
  Otherwise, only IRC operators, other servers, and services are allowed to
  set mode +x. This prevents regular users from changing their hostmask to
  the name of the IRC server itself, which confused quite a few people ;-)

Differences to version 17.x

- Support for ZeroConf/Bonjour/Rendezvous service registration has been
  removed. The configuration option *NoZeroconf* is no longer available.

- The structure of `ngircd.conf` has been cleaned up and three new configuration
  sections have been introduced: *[Limits]*, *[Options]*, and *[SSL]*.

  Lots of configuration variables stored in the *[Global]* section are now
  deprecated there and should be stored in one of these new sections (but
  still work in *[Global]*):

  - *AllowRemoteOper*    -> [Options]
  - *ChrootDir*          -> [Options]
  - *ConnectIPv4*        -> [Options]
  - *ConnectIPv6*        -> [Options]
  - *ConnectRetry*       -> [Limits]
  - *MaxConnections*     -> [Limits]
  - *MaxConnectionsIP*   -> [Limits]
  - *MaxJoins*           -> [Limits]
  - *MaxNickLength*      -> [Limits]
  - *NoDNS*              -> [Options], and renamed to *DNS*
  - *NoIdent*            -> [Options], and renamed to *Ident*
  - *NoPAM*              -> [Options], and renamed to *PAM*
  - *OperCanUseMode*     -> [Options]
  - *OperServerMode*     -> [Options]
  - *PingTimeout*        -> [Limits]
  - *PongTimeout*        -> [Limits]
  - *PredefChannelsOnly* -> [Options]
  - *SSLCertFile*        -> [SSL], and renamed to *CertFile*
  - *SSLDHFile*          -> [SSL], and renamed to *DHFile*
  - *SSLKeyFile*         -> [SSL], and renamed to *KeyFile*
  - *SSLKeyFilePassword* -> [SSL], and renamed to *KeyFilePassword*
  - *SSLPorts*           -> [SSL], and renamed to *Ports*
  - *SyslogFacility*     -> [Options]
  - *WebircPassword*     -> [Options]

  You should adjust your `ngircd.conf` and run `ngircd --configtest` to make
  sure that your settings are correct and up to date!

Differences to version 16.x

- Changes to the *MotdFile* specified in `ngircd.conf` now require a ngIRCd
  configuration reload to take effect (HUP signal, *REHASH* command).

Differences to version 0.9.x

- The option of the configure script to enable support for Zeroconf/Bonjour/
  Rendezvous/WhateverItIsNamedToday has been renamed:

  - `--with-rendezvous`  ->  `--with-zeroconf`

Differences to version 0.8.x

- The maximum length of passwords has been raised to 20 characters (instead
  of 8 characters). If your passwords are longer than 8 characters then they
  are cut at an other position now.

Differences to version 0.6.x

- Some options of the configure script have been renamed:

  - `--disable-syslog`  ->  `--without-syslog`
  - `--disable-zlib`    ->  `--without-zlib`

  Please call `./configure --help` to review the full list of options!

Differences to version 0.5.x

- Starting with version 0.6.0, other servers are identified using asynchronous
  passwords: therefore the variable *Password* in *[Server]*-sections has been
  replaced by *MyPassword* and *PeerPassword*.

- New configuration variables, section *[Global]*: *MaxConnections*, *MaxJoins*
  (see example configuration file `doc/sample-ngircd.conf`!).

## Standard Installation

*Note*: This sections describes installing ngIRCd *from sources*. If you use
packages available for your operating system distribution you should skip over
and continue with the *Configuration* section, see below.

ngIRCd is developed for UNIX-based systems, which means that the installation
on modern UNIX-like systems that are supported by GNU autoconf and GNU
automake ("`configure` script") should be no problem.

The normal installation procedure after getting (and expanding) the source
files (using a distribution archive or Git) is as following:

1) Satisfy prerequisites
2) `./autogen.sh` [only necessary when using "raw" sources with Git]
3) `./configure`
4) `make`
5) `make install`

(Please see details below!)

Now the newly compiled executable "ngircd" is installed in its standard
location, `/usr/local/sbin/`.

If no previous version of the configuration file exists (the standard name
is `/usr/local/etc/ngircd.conf)`, a sample configuration file containing all
possible options will be installed there. You'll find its template in the
`doc/` directory: `sample-ngircd.conf`.

The next step is to configure and afterwards start the daemon. See the section
*Configuration* below.

### Satisfy prerequisites

When building from source, you'll need some other software to build ngIRCd:
for example a working C compiler, make tool, and a few libraries depending on
the feature set you want to enable at compile time (like IDENT, SSL, and PAM).

And if you aren't using a distribution archive ("tar.gz" file), but cloned the
plain source archive, you need a few additional tools to generate the build
system itself: GNU automake and autoconf, as well as pkg-config.

If you are using one of the "big" operating systems or Linux distributions,
you can use the following commands to install all the required packages to
build the sources including all optional features and to run the test suite:

#### Red Hat / Fedora based distributions

``` shell
  yum install \
    autoconf automake expect gcc glibc-devel gnutls-devel \
    libident-devel make pam-devel pkg-config tcp_wrappers-devel \
    telnet zlib-devel
```

*Note:* More recent versions use the DNF package manager; so substitute "yum"
with "dnf" in the command above. And neither "libident-devel" (IDENT support)
nor "tcp_wrappers-devel" (TCP Wrappers) are provided any more!

So the resulting command looks like this:

``` shell
  dnf install \
    autoconf automake expect gcc glibc-devel gnutls-devel \
    make pam-devel pkg-config telnet zlib-devel
```

#### Debian / Ubuntu based distributions

``` shell
  apt-get install \
    autoconf automake build-essential expect libgnutls28-dev \
    libident-dev libpam-dev pkg-config libwrap0-dev libz-dev telnet
```

#### ArchLinux based distributions

``` shell
  pacman -S --needed \
    autoconf automake expect gcc gnutls inetutils libident libwrap \
    make pam pkg-config zlib
```

#### macOS with Homebrew

To build ngIRCd on Apple macOS, you need either Xcode or the command line
development tools. You can install the latter with the `xcode-select --install`
command.

Additional tools and libraries that are not part of macOS itself are best
installed with the [Homebrew](https://brew.sh) package manager:

``` shell
  brew install autoconf automake gnutls libident pkg-config
```

Note: To actually use the GnuTLS and IDENT libraries installed by Homebrew, you
need to pass the installation path to the `./configure` command (see below). For
example like this:

``` shell
  ./configure --with-gnutls=$(brew --prefix) --with-ident=$(brew --prefix) [...]
```

### `./autogen.sh`

The first step, to run `./autogen.sh`, is *only* necessary if the `configure`
script itself isn't already generated and available. This never happens in
official ("stable") releases in "tar.gz" archives, but when cloning the source
code repository using Git.

**This step is therefore only interesting for developers!**

The `autogen.sh` script produces the `Makefile.in`'s, which are necessary for
the configure script itself, and some more files for `make(1)`.

To run `autogen.sh` you'll need GNU autoconf, GNU automake and pkg-config: at
least autoconf 2.61 and automake 1.10 are required, newer is better. But don't
use automake 1.12 or newer for creating distribution archives: it will work
but lack "de-ANSI-fication" support in the generated Makefile's! Stick with
automake 1.11.x for this purpose ...

So *automake 1.11.x* and *autoconf 2.67+* is recommended.

Again: "end users" do not need this step and neither need GNU autoconf nor GNU
automake at all!

### `./configure`

The `configure` script is used to detect local system dependencies.

In the perfect case, `configure` should recognize all needed libraries, header
files and so on. If this shouldn't work, `./configure --help` shows all
possible options.

In addition, you can pass some command line options to `configure` to enable
and/or disable some features of ngIRCd. All these options are shown using
`./configure --help`, too.

Compiling a static binary will avoid you the hassle of feeding a chroot dir
(if you want use the chroot feature). Just do something like:

``` shell
  CFLAGS=-static ./configure [--your-options ...]
```

Then you can use a void directory as ChrootDir (like OpenSSH's `/var/empty`).

### `make`

The `make(1)` command uses the `Makefile`'s produced by `configure` and
compiles the ngIRCd daemon.

### `make install`

Use `make install` to install the server and a sample configuration file on
the local system. Normally, root privileges are necessary to complete this
step. If there is already an older configuration file present, it won't be
overwritten.

These files and folders will be installed by default:

- `/usr/local/sbin/ngircd`: executable server
- `/usr/local/etc/ngircd.conf`: sample configuration (if not already present)
- `/usr/local/share/doc/ngircd/`: documentation
- `/usr/local/share/man/`: manual pages

### Additional features

The following optional features can be compiled into the daemon by passing
options to the `configure` script. Most options can handle a `<path>` argument
which will be used to search for the required libraries and header files in
the given paths (`<path>/lib/...`, `<path>/include/...`) in addition to the
standard locations.

- Syslog Logging (autodetected by default):

  `--with-syslog[=<path>]` / `--without-syslog`

  Enable (disable) support for logging to "syslog", which should be
  available on most modern UNIX-like operating systems by default.

- ZLib Compression (autodetected by default):

  `--with-zlib[=<path>]` / `--without-zlib`

  Enable (disable) support for compressed server-server links.
  The Z compression library ("libz") is required for this option.

- IO Backend (autodetected by default):

  - `--with-select[=<path>]` / `--without-select`
  - `--with-poll[=<path>]` / `--without-poll`
  - `--with-devpoll[=<path>]` / `--without-devpoll`
  - `--with-epoll[=<path>]` / `--without-epoll`
  - `--with-kqueue[=<path>]` / `--without-kqueue`

  ngIRCd can use different IO "backends": the "old school" `select(2)` and
  `poll(2)` API which should be supported by most UNIX-like operating systems,
  or the more efficient and flexible `epoll(7)` (Linux >=2.6), `kqueue(2)`
  (BSD) and `/dev/poll` APIs.

  By default the IO backend is autodetected, but you can use `--without-xxx`
  to disable a more enhanced API.

  When using the `epoll(7)` API, support for `select(2)` is compiled in as
  well by default, to enable the binary to run on older Linux kernels (<2.6),
  too.

- IDENT-Support:

  `--with-ident[=<path>]`

  Include support for IDENT ("AUTH") lookups. The "ident" library is
  required for this option.

- TCP-Wrappers:

  `--with-tcp-wrappers[=<path>]`

  Include support for Wietse Venemas "TCP Wrappers" to limit client access
  to the daemon, for example by using `/etc/hosts.{allow|deny}`.
  The "libwrap" is required for this option.

- PAM:

  `--with-pam[=<path>]`

  Enable support for PAM, the Pluggable Authentication Modules library.
  See `doc/PAM.txt` for details.

- SSL:

  - `--with-openssl[=<path>]`
  - `--with-gnutls[=<path>]`

  Enable support for SSL/TLS using OpenSSL or GnuTLS libraries.
  See `doc/SSL.md` for details.

- IPv6 (autodetected by default):

  `--enable-ipv6` / `--disable-ipv6`

  Enable (disable) support for version 6 of the Internet Protocol, which should
  be available on most modern UNIX-like operating systems by default.
