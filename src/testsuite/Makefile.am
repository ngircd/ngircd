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
# $Id: Makefile.am,v 1.18 2008/02/17 13:26:42 alex Exp $
#

AUTOMAKE_OPTIONS = ../portab/ansi2knr

INCLUDES = -I$(srcdir)/../portab

EXTRA_DIST = \
	README functions.inc getpid.sh \
	start-server.sh stop-server.sh tests.sh stress-server.sh \
	test-loop.sh wait-tests.sh \
	channel-test.e connect-test.e check-idle.e misc-test.e mode-test.e \
	kick-test.e \
	opless-channel-test.e \
	who-test.e stress-A.e stress-B.e \
	ngircd-test.conf

all:

clean-local:
	rm -rf logs tests *-test ngircd-test.log ngircd-test.motd \
	 T-ngircd procs.tmp 

maintainer-clean-local:
	rm -f Makefile Makefile.in

check_SCRIPTS = ngircd-TEST-Binary tests.sh

ngircd-TEST-Binary:
	cp ../ngircd/ngircd T-ngircd
	[ -f getpid.sh ] || ln -s $(srcdir)/getpid.sh .

connect-test: tests.sh
	rm -f connect-test
	ln -s $(srcdir)/tests.sh connect-test

channel-test: tests.sh
	rm -f channel-test
	ln -s $(srcdir)/tests.sh channel-test

kick-test: tests.sh
	rm -f kick-test
	ln -s $(srcdir)/tests.sh kick-test

opless-channel-test: tests.sh
	rm -f opless-channel-test
	ln -s $(srcdir)/tests.sh opless-channel-test

who-test: tests.sh
	rm -f who-test
	ln -s $(srcdir)/tests.sh who-test

misc-test: tests.sh
	rm -f misc-test
	ln -s $(srcdir)/tests.sh misc-test

mode-test: tests.sh
	rm -f mode-test
	ln -s $(srcdir)/tests.sh mode-test

TESTS = start-server.sh \
	connect-test \
	channel-test \
	kick-test \
	misc-test \
	mode-test \
	who-test \
	opless-channel-test \
	stress-server.sh \
	stop-server.sh

# -eof-
