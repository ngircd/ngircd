#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2008 Alexander Barton <alex@barton.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#
# $Id: Makefile.am,v 1.3 2008/02/17 15:31:15 alex Exp $
#

SUBDIRS = ngIRCd.xcodeproj

EXTRA_DIST = de.barton.ngircd.plist.tmpl config.h cvs-version.h

SUFFIXES = .tmpl .

.tmpl:
	sed \
	    -e s@:SBINDIR:@${sbindir}@ \
	    <$< >$@

install-data-local:
	[ `uname -s` != "Darwin" ] || make install-sys-darwin

install-sys-darwin:
	@if [ `id -u` -eq 0 ]; then \
	  make install-sys-darwin-root; \
	else \
	  echo; \
	  echo " ** NOTE: Not installing with root privileges, so the LaunchDaemon script"; \
	  echo " ** \"/Library/LaunchDaemons/de.barton.ngircd.plist\" can't be installed/updated!"; \
	  echo; \
	fi

install-sys-darwin-root: de.barton.ngircd.plist
	install -d -m 755 -o root -g wheel $(DESTDIR)/Library/LaunchDaemons
	install -c -m 644 -b -o root -g wheel de.barton.ngircd.plist \
	 $(DESTDIR)/Library/LaunchDaemons/de.barton.ngircd.plist
	@echo
	@echo " ** \"/Library/LaunchDaemons/de.barton.ngircd.plist\" has been installed,"
	@echo " ** but is disabled. Use launchctl(8) to enable/run ngIRCd on Darwin/Mac OS X."
	@echo

clean-local:
	rm -rf build
	rm -f de.barton.ngircd.plist

maintainer-clean-local:
	rm -f Makefile Makefile.in

# -eof-
