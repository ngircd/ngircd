#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2015 Alexander Barton (alex@barton.de) and Contributors
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#

SUBDIRS = doc src man contrib

EXTRA_DIST = README.md INSTALL.md autogen.sh configure.ng .clang_complete .mailmap

clean-local: osxpkg-clean
	rm -f build-stamp*

maintainer-clean-local:
	rm -rf autom4te.cache
	rm -f Makefile.in Makefile aclocal.m4 configure configure.ac
	rm -f ar-lib mkinstalldirs missing depcomp install-sh
	rm -f config.log debian

testsuite:
	cd src/testsuite && make check

lint:
	cd src/ngircd && make lint

srcdoc:
	cd doc && make srcdoc

have-xcodebuild:
	@xcodebuild -project contrib/MacOSX/ngIRCd.xcodeproj -list \
	 >/dev/null 2>&1 \
	 || ( echo; echo "Error: \"xcodebuild\" not found!"; echo; exit 1 )

have-packagemaker:
	@packagemaker >/dev/null 2>&1; [ $$? -le 1 ] \
	 || ( echo; echo "Error: \"packagemaker\" not found!"; echo; exit 2)

xcode: have-xcodebuild
	rel=`git describe|sed -e 's/rel-//g'|sed -e 's/-/~/'`; \
	 def="GCC_PREPROCESSOR_DEFINITIONS=\"VERSION=\\\"$$rel\\\"\""; \
	 xcodebuild -project contrib/MacOSX/ngIRCd.xcodeproj -alltargets \
	 -configuration Default $$def build

xcode-clean: have-xcodebuild
	xcodebuild -project contrib/MacOSX/ngIRCd.xcodeproj -alltargets \
	 -configuration Default clean
	rm -fr contrib/MacOSX/build

rpm: distcheck
	rpm -ta ngircd-*.tar.gz

deb:
	[ -f debian/rules ] || ln -s contrib/Debian debian
	dpkg-buildpackage -rfakeroot -i

osxpkg: have-packagemaker osxpkg-dest
	cd contrib/MacOSX && packagemaker --no-recommend \
	 --doc ngIRCd.pmdoc \
	 --out ../../$(distdir).mpkg
	rm -f $(distdir).mpkg.zip
	zip -ro9 $(distdir).mpkg.zip $(distdir).mpkg
	make osxpkg-clean

osxpkg-clean:
	[ ! -r ngircd.dest ] || sudo -n rm -rf ngircd.dest
	rm -rf ngircd.dest $(distdir).mpkg

osxpkg-dest: have-xcodebuild osxpkg-clean clean
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
	sudo chown -R root:wheel ngircd.dest

.PHONY: deb have-packagemaker have-xcodebuild lint osxpkg osxpkg-clean \
	osxpkg-dest rpm srcdoc testsuite xcode xcode-clean

# -eof-
