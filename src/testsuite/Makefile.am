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
# $Id: Makefile.am,v 1.1 2002/09/09 10:16:24 alex Exp $
#

AUTOMAKE_OPTIONS = ../portab/ansi2knr

INCLUDES = -I$(srcdir)/../portab

EXTRA_DIST = ngircd-test.conf connect-test.e channel-test.e mode-test.e

clean-local:
	rm -f *.log *-test

maintainer-clean-local:
	rm -f Makefile Makefile.in

check_SCRIPTS = tests.sh

connect-test: tests.sh
	ln -s tests.sh connect-test

channel-test: tests.sh
	ln -s tests.sh channel-test

mode-test: tests.sh
	ln -s tests.sh mode-test

TESTS = start-server.sh \
	connect-test \
	channel-test \
	mode-test \
	stop-server.sh

# -eof-
