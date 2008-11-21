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
	rm -rf ngircd.dest

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
	dpkg-buildpackage -rfakeroot -i

osxpkg:
	@packagemaker >/dev/null 2>&1; [ $$? -ge 1 ] \
	 || ( echo; echo "Error: \"packagemaker\" not found!"; echo; exit 2)
	make clean
	./configure --prefix=/opt/ngircd
	make xcode
	make -C contrib/MacOSX de.barton.ngircd.plist
	mkdir -p ngircd.dest/opt/ngircd/sbin
	DESTDIR="$$PWD/ngircd.dest" make -C doc install
	DESTDIR="$$PWD/ngircd.dest" make -C contrib install
	DESTDIR="$$PWD/ngircd.dest" make -C man install
	cp contrib/MacOSX/build/Default/ngIRCd \
	 ngircd.dest/opt/ngircd/sbin/ngircd
	rm ngircd.dest/opt/ngircd/etc/ngircd.conf
	echo "Have a nice day IRCing!" >ngircd.dest/opt/ngircd/etc/ngircd.motd
	chmod -R a-s,og-w,a+rX ngircd.dest
	cd contrib/MacOSX && packagemaker \
	 --doc ngIRCd.pmdoc \
	 --out ../../$(distdir).mpkg
	rm -f $(distdir).mpkg.zip
	zip -ro9 $(distdir).mpkg.zip $(distdir).mpkg
	rm -rf ngircd.dest $(distdir).mpkg

# -eof-
