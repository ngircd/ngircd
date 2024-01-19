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

clean-local:
	rm -f build-stamp*

maintainer-clean-local:
	rm -rf autom4te.cache
	rm -f Makefile.in Makefile aclocal.m4 configure configure.ac
	rm -f ar-lib mkinstalldirs missing depcomp install-sh
	rm -f config.log debian

testsuite:
	cd src/testsuite && ${MAKE} check

lint:
	cd src/ngircd && ${MAKE} lint

srcdoc:
	cd doc && ${MAKE} srcdoc

rpm: distcheck
	rpm -ta ngircd-*.tar.gz

deb:
	[ -f debian/rules ] || ln -s contrib/Debian debian
	dpkg-buildpackage -rfakeroot -i

.PHONY: deb lint rpm srcdoc testsuite

# -eof-
