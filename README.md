# [ngIRCd](https://ngircd.barton.de) - Internet Relay Chat Server

## Introduction

*ngIRCd* is a free, portable and lightweight *Internet Relay Chat* ([IRC])
server for small or private networks, developed under the terms of the GNU
General Public License ([GPL]); please see the file `COPYING` for licensing
information.

The server is quite easy to configure, can handle dynamic IP addresses, and
optionally supports IDENT, IPv6 connections, SSL-protected links, and PAM for
user authentication as well as character set conversion for legacy clients. The
server has been written from scratch and is not based on the "forefather", the
daemon of the IRCNet.

The name ngIRCd means *next-generation IRC daemon*, which is a little bit
exaggerated: *lightweight Internet Relay Chat server* most probably would have
been a better name :-)

Please see the `INSTALL.md` document for installation and upgrade information,
online available here: <https://ngircd.barton.de/doc/INSTALL.md>!

## Status

The development of ngIRCd started back in 2001 and in the meantime it should be
quite feature-complete and stable to be used as a daemon in real-world IRC
networks.

It is not the goal of ngIRCd to implement all the nasty behaviors of the
original ircd, but to implement most of the useful commands and semantics
specified by the RFCs that are used by existing clients.

## Features (or: why use ngIRCd?)

- Well arranged (lean) configuration file.
- Simple to build, install, configure, and maintain.
- Supports IPv6 and SSL.
- Can use PAM for user authentication.
- Lots of popular user and channel modes are implemented.
- Supports "cloaking" of users.
- No problems with servers that have dynamic IP addresses.
- Freely available, modern, portable and tidy C source.
- Wide field of supported platforms, including AIX, A/UX, FreeBSD, HP-UX,
  IRIX, Linux, macOS, NetBSD, OpenBSD, Solaris, and Windows with Cygwin.

## Documentation

The **homepage** of the ngIRCd project is <https://ngircd.barton.de>.

Installation on ngIRCd is described in the file `INSTALL.md` in the source
directory; please see the file `doc/QuickStart.md` in the `doc/` directory for
some configuration examples.

More documentation can be found in the `doc/` directory and
[online](https://ngircd.barton.de/documentation).

## Downloads & Source Code

You can find the latest information about the ngIRCd and the most recent
stable release on the [news](https://ngircd.barton.de/news) and
[downloads](https://ngircd.barton.de/download) pages of the homepage.

Visit our source code repository at [GitHub](https://github.com) if you are
interested in the latest development code: <https://github.com/ngircd/ngircd>.

## Problems, Bugs, Patches

Please don't hesitate to contact us if you encounter problems:

- On IRC: <irc://irc.barton.de/ngircd>
- Via the mailing list: <ngircd-ml@ngircd.barton.de>

See <http://ngircd.barton.de/support> for details.

If you find any bugs in ngIRCd (which most probably will be there ...), please
report them to our issue tracker at GitHub:

- Bug tracker: <https://github.com/ngircd/ngircd/issues>
- Patches, "pull requests": <https://github.com/ngircd/ngircd/pulls>

[IRC]: https://wikipedia.org/wiki/Internet_Relay_Chat
[GPL]: https://wikipedia.org/wiki/GNU_General_Public_License
