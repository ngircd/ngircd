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
# $Id: Makefile.am,v 1.6 2006/12/25 16:13:26 alex Exp $
#

TEMPLATE_MANS = ngircd.conf.5.tmpl ngircd.8.tmpl

SUFFIXES = .tmpl .

.tmpl:
	$(AM_V_GEN)sed \
	    -e s@:SBINDIR:@${sbindir}@ \
	    -e s@:BINDIR:@${bindir}@ \
	    -e s@:ETCDIR:@${sysconfdir}@ \
	    <$< >$@

man_MANS = ngircd.conf.5 ngircd.8

CLEANFILES = $(man_MANS)

EXTRA_DIST = $(TEMPLATE_MANS)

maintainer-clean-local:
	rm -f Makefile Makefile.in

# -eof-
