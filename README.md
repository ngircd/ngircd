# [ngIRCd](https://ngircd.barton.de) - Internet Relay Chat Server

## Introduction

*ngIRCd* is a free, portable and lightweight *Internet Relay Chat* ([IRC])
server for small or private networks, developed under the terms of the GNU
General Public License ([GPL]); please see the file `COPYING` for licensing
information.

The server is quite easy to configure and runs as a single-node server or can
be part of a network of ngIRCd servers in a LAN or across the internet. It
optionally supports the IPv6 protocol, SSL/TLS-protected client-server and
server-server links, the Pluggable Authentication Modules (PAM) system for user
authentication, IDENT requests, and character set conversion for legacy
clients.

The name ngIRCd stands for *next-generation IRC daemon*, which is a little bit
exaggerated: *lightweight Internet Relay Chat server* most probably would have
been a better name :-)

## Status

Development of *ngIRCd* started back in 2001: The server has been written from
scratch in C, tries to follow all relevant standards, and is not based on the
forefather, the daemon of the IRCNet.

It is not the goal of ngIRCd to implement all the nasty behaviors of the
original `ircd` or corner-cases in the RFCs, but to implement most of the useful
commands and semantics that are used by existing clients.

*ngIRCd* is used as the daemon in real-world in-house and public IRC networks
and included in the package repositories of various operating systems.

## Advantages and strengths

- Well arranged (lean) configuration file.
- Simple to build, install, configure, and maintain.
- Supports IPv6 and SSL.
- Can use PAM for user authentication.
- Lots of popular user and channel modes are implemented.
- Supports "cloaking" of users.
- No problems with servers that have dynamic IP addresses.
- Freely available, modern, portable and tidy C source.
- Wide field of supported platforms, including AIX, A/UX, FreeBSD, HP-UX,
  IRIX, Linux, macOS, NetBSD, OpenBSD, Solaris and Windows with WSL or Cygwin.

## Documentation

The **homepage** of the ngIRCd project is <https://ngircd.barton.de>.

The `INSTALL.md` document describes how to _install_ and _upgrade_ ngIRCd. It
is included in all distribution archives and available online on
[GitHub](https://github.com/ngircd/ngircd/blob/master/INSTALL.md).

Please see the file `doc/QuickStart.md` in the `doc/` directory or on
[GitHub](https://github.com/ngircd/ngircd/blob/master/doc/QuickStart.md) for
information about _setting up_ and _running_ ngIRCd, including some real-world
configuration examples.

More information can be found in a couple of files in the `doc/` directory
(online on [GitHub](https://github.com/ngircd/ngircd/tree/master/doc)) and in
the [documentation section](https://ngircd.barton.de/documentation) on the
[homepage of ngIRCd](https://ngircd.barton.de).

In addition, ngIRCd comes with two _manual pages_: `ngircd(8)` (for the daemon)
and `ngircd.conf(5)` (for its configuration file). They have even more details
and list all possible command line parameters and configuration options. You
can read them with the `man` command (when they are installed locally on your
system, e.g. `man 8 ngircd` and `man 5 ngircd.conf`) or online here:

- Daemon:
  [ngircd(8)](https://ngircd.barton.de/man/ngircd.8.html)
- Configuration file:
  [ngircd.conf(5)](https://ngircd.barton.de/man/ngircd.conf.5.html)

## Downloads & Source Code

You can find the latest information about the ngIRCd and the most recent
stable release on the [news](https://ngircd.barton.de/news) and
[downloads](https://ngircd.barton.de/download) pages of the homepage.

Visit our source code repository at [GitHub](https://github.com/ngircd/ngircd)
if you are interested in the latest development code.

## Problems, Bugs, Patches

Please don't hesitate to contact us if you encounter problems:

- On IRC: <irc://irc.barton.de/ngircd>
- Via the mailing list: <ngircd@lists.barton.de>

See <https://ngircd.barton.de/support> for details.

If you find any bugs in ngIRCd (which most probably will be there ...), please
report them to our issue tracker at GitHub:

- Bug tracker: <https://github.com/ngircd/ngircd/issues>
- Patches, "pull requests": <https://github.com/ngircd/ngircd/pulls>

[IRC]: https://wikipedia.org/wiki/Internet_Relay_Chat
[GPL]: https://wikipedia.org/wiki/GNU_General_Public_License
