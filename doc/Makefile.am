#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2007 by Alexander Barton (alex@barton.de)
#
# Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
# der GNU General Public License (GPL), wie von der Free Software Foundation
# herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
# der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
# Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
# der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
#
# $Id: Makefile.am,v 1.22 2007/11/20 21:39:35 alex Exp $
#

SUBDIRS = src

EXTRA_DIST = CVS.txt FAQ.txt Protocol.txt Platforms.txt README-AUX.txt \
	README-BeOS.txt RFC.txt SSL.txt Zeroconf.txt sample-ngircd.conf

maintainer-clean-local:
	rm -f Makefile Makefile.in

docdir = $(datadir)/doc/$(PACKAGE)

documents = $(EXTRA_DIST) ../AUTHORS ../COPYING ../ChangeLog ../INSTALL \
	../NEWS ../README

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)
	if [ ! -f $(DESTDIR)$(sysconfdir)/ngircd.conf ]; then \
	  $(INSTALL) -m 600 -c $(srcdir)/sample-ngircd.conf $(DESTDIR)$(sysconfdir)/ngircd.conf; \
	 fi
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	for f in $(documents); do \
	  $(INSTALL) -m 644 -c $(srcdir)/$$f $(DESTDIR)$(docdir)/; \
	 done

uninstall-hook:
	rm -rf $(DESTDIR)$(docdir)

srcdoc:
	make -C src srcdoc

# -eof-
