#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
#
# Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
# der GNU General Public License (GPL), wie von der Free Software Foundation
# herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
# der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
# Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
# der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
#
# $Id: Makefile.am,v 1.7 2002/03/30 15:33:14 alex Exp $
#

EXTRA_DIST = CC-Speed.txt CVS.txt FAQ.txt README-AUX.txt README-BeOS.txt \
	RFC.txt sample-ngircd.conf

maintainer-clean-local:
	rm -f Makefile Makefile.in

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)
	if [ ! -f $(DESTDIR)$(sysconfdir)/ngircd.conf ]; then \
	  $(INSTALL) -m 600 -c $(srcdir)/sample-ngircd.conf $(DESTDIR)$(sysconfdir)/ngircd.conf; \
	  fi

# -eof-
