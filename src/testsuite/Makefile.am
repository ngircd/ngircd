#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2011 Alexander Barton (alex@barton.de) and Contributors.
#
# Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
# der GNU General Public License (GPL), wie von der Free Software Foundation
# herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
# der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
# Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
# der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
#

AUTOMAKE_OPTIONS = ../portab/ansi2knr

INCLUDES = -I$(srcdir)/../portab

EXTRA_DIST = \
	README functions.inc getpid.sh \
	start-server.sh stop-server.sh tests.sh stress-server.sh \
	test-loop.sh wait-tests.sh \
	channel-test.e connect-test.e check-idle.e invite-test.e \
	join-test.e kick-test.e message-test.e misc-test.e mode-test.e \
	opless-channel-test.e server-link-test.e who-test.e whois-test.e \
	stress-A.e stress-B.e \
	start-server1 stop-server1 ngircd-test1.conf \
	start-server2 stop-server2 ngircd-test2.conf

all:

clean-local:
	rm -rf logs tests *-test ngircd-test*.log procs.tmp \
	 T-ngircd1 ngircd-test1.motd T-ngircd2 ngircd-test2.motd

maintainer-clean-local:
	rm -f Makefile Makefile.in

check_SCRIPTS = ngircd-TEST-Binary tests.sh

ngircd-TEST-Binary:
	cp ../ngircd/ngircd T-ngircd1
	cp ../ngircd/ngircd T-ngircd2
	[ -f getpid.sh ] || ln -s $(srcdir)/getpid.sh .

connect-test: tests.sh
	rm -f connect-test
	ln -s $(srcdir)/tests.sh connect-test

channel-test: tests.sh
	rm -f channel-test
	ln -s $(srcdir)/tests.sh channel-test

invite-test: tests.sh
	rm -f invite-test
	ln -s $(srcdir)/tests.sh invite-test

join-test: tests.sh
	rm -f join-test
	ln -s $(srcdir)/tests.sh join-test

kick-test: tests.sh
	rm -f kick-test
	ln -s $(srcdir)/tests.sh kick-test

message-test: tests.sh
	rm -f message-test
	ln -s $(srcdir)/tests.sh message-test

misc-test: tests.sh
	rm -f misc-test
	ln -s $(srcdir)/tests.sh misc-test

mode-test: tests.sh
	rm -f mode-test
	ln -s $(srcdir)/tests.sh mode-test

opless-channel-test: tests.sh
	rm -f opless-channel-test
	ln -s $(srcdir)/tests.sh opless-channel-test

server-link-test: tests.sh
	rm -f server-link-test
	ln -s $(srcdir)/tests.sh server-link-test

who-test: tests.sh
	rm -f who-test
	ln -s $(srcdir)/tests.sh who-test

whois-test: tests.sh
	rm -f whois-test
	ln -s $(srcdir)/tests.sh whois-test

TESTS = start-server1 \
	connect-test \
	start-server2 \
	channel-test \
	invite-test \
	join-test \
	kick-test \
	message-test \
	misc-test \
	mode-test \
	opless-channel-test \
	who-test \
	whois-test \
	server-link-test \
	stop-server2 \
	stress-server.sh \
	stop-server1

# -eof-
