#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2008 Alexander Barton (alex@barton.de)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
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

testsuite:
	make -C src/testsuite check

lint:
	make -C src/ngircd lint

srcdoc:
	make -C doc srcdoc

xcode:
	@xcodebuild -project contrib/MacOSX/ngIRCd.xcodeproj -list \
	 >/dev/null 2>&1 \
	 || ( echo; echo "Error: \"xcodebuild\" not found!"; echo; exit 1 )
	xcodebuild -project contrib/MacOSX/ngIRCd.xcodeproj -alltargets \
	 -buildstyle Development

rpm: distcheck
	rpm -ta ngircd-*.tar.gz

deb:
	[ -f debian/rules ] || ln -s contrib/Debian debian
	dpkg-buildpackage -rfakeroot

# -eof-
