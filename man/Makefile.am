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
# $Id: Makefile.am,v 1.2 2002/03/31 20:55:07 alex Exp $
#

man_MANS = ngircd.conf.5 ngircd.8

EXTRA_DIST = ngircd.conf.5.nroff ngircd.8.nroff

ngircd.conf.5: ngircd.conf.5.nroff
	$(NROFF) -man -Tascii $(srcdir)/ngircd.conf.5.nroff > $(srcdir)/ngircd.conf.5

ngircd.8: ngircd.8.nroff
	$(NROFF) -man -Tascii $(srcdir)/ngircd.8.nroff > $(srcdir)/ngircd.8

clean-local:
	rm -f *.[0-9]

maintainer-clean-local:
	rm -f Makefile Makefile.in

# -eof-
