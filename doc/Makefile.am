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
# $Id: Makefile.am,v 1.5 2002/03/10 17:05:23 alex Exp $
#
# $Log: Makefile.am,v $
# Revision 1.5  2002/03/10 17:05:23  alex
# - die Beispiel-Konfigurationsdatei wird als ngircd.conf installiert,
#   wenn noch keine "echte" Konfigurationsdatei vorhanden ist.
#
# Revision 1.4  2002/02/25 14:10:38  alex
# - die neuen Texte werden nun auch in Archive ("make dist") aufgenommen ;-)
#
# Revision 1.3  2002/01/02 02:40:31  alex
# - Copyright-Text ergaenzt.
#
# Revision 1.2  2001/12/31 02:45:24  alex
# - nun werden alle generierten Dateien bei "maintainer-clean" geloescht.
#
# Revision 1.1  2001/12/31 02:22:29  alex
# - Makefile.am fuer das "doc"-Verzeichnis begonnen.
#

EXTRA_DIST = CC-Speed.txt CVS.txt FAQ.txt README-AUX.txt README-BeOS.txt \
	RFC.txt sample-ngircd.conf

maintainer-clean-local:
	rm -f Makefile Makefile.in

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)
	if [ ! -e $(DESTDIR)$(sysconfdir)/ngircd.conf ]; then \
	  $(INSTALL) -m 600 -c sample-ngircd.conf $(DESTDIR)$(sysconfdir)/ngircd.conf; \
	  fi

# -eof-
