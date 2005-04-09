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
# $Id: Makefile.am,v 1.16 2005/04/09 12:27:40 alex Exp $
#

AUTOMAKE_OPTIONS = gnu

SUBDIRS = doc src man contrib

clean-local:
	rm -f build-stamp*

maintainer-clean-local:
	rm -rf autom4te.cache
	rm -f Makefile.in Makefile aclocal.m4 configure
	rm -f mkinstalldirs missing depcomp install-sh
	rm -f config.log debian

lint:
	make -C src/ngircd lint

srcdoc:
	make -C src srcdoc

xcode:
	@xcodebuild -project contrib/MacOSX/ngIRCd.xcode -list >/dev/null 2>&1 \
	 || ( echo; echo "Error: \"xcodebuild\" not found!"; echo; exit 1 )
	xcodebuild -project contrib/MacOSX/ngIRCd.xcode -alltargets \
	 -buildstyle Development

rpm: distcheck
	rpm -ta ngircd-*.tar.gz

deb:
	[ -f debian/rules ] || ln -s contrib/Debian debian
	dpkg-buildpackage -rfakeroot

# -eof-
