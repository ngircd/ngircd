#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2003 by Alexander Barton (alex@barton.de)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#
# $Id: Makefile.am,v 1.10.4.2 2003/07/09 21:14:08 alex Exp $
#

AUTOMAKE_OPTIONS = gnu

SUBDIRS = doc MacOSX src man contrib debian

maintainer-clean-local:
	rm -rf autom4te.cache
	rm -f Makefile.in Makefile aclocal.m4 configure
	rm -f mkinstalldirs missing depcomp install-sh
	rm -f config.log

lint:
	make -C src/ngircd lint

rpm: distcheck
	rpm -ta ngircd-*.tar.gz

deb:
	dpkg-buildpackage -rfakeroot

# -eof-
