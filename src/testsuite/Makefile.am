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
# $Id: Makefile.am,v 1.3.2.1 2002/09/20 15:18:56 alex Exp $
#

AUTOMAKE_OPTIONS = ../portab/ansi2knr

INCLUDES = -I$(srcdir)/../portab

EXTRA_DIST = \
	getpid.sh \
	start-server.sh stop-server.sh tests.sh stress-server.sh \
	connect-test.e channel-test.e mode-test.e \
	stress-A.e stress-B.e check-idle.e \
	ngircd-test.conf

clean-local:
	rm -rf logs tests *-test ngircd-test.log ngircd-test.motd \
	 ngircd-TEST* procs.tmp 

maintainer-clean-local:
	rm -f Makefile Makefile.in

check_SCRIPTS = ngircd-TEST-Binary tests.sh

ngircd-TEST-Binary:
	cp ../ngircd/ngircd ngircd-TEST

connect-test: tests.sh
	ln -s $(srcdir)/tests.sh connect-test

channel-test: tests.sh
	ln -s $(srcdir)/tests.sh channel-test

mode-test: tests.sh
	ln -s $(srcdir)/tests.sh mode-test

TESTS = start-server.sh \
	connect-test \
	channel-test \
	mode-test \
	stress-server.sh \
	stop-server.sh

# -eof-
