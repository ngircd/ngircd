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
# $Id: Makefile.am,v 1.3 2002/05/19 01:17:40 alex Exp $
#

noinst_LIBRARIES = libngportab.a

libngportab_a_SOURCES = vsnprintf.c

check_PROGRAMS = portabtest

portabtest_SOURCES = portabtest.c

noinst_HEADERS = imp.h exp.h portab.h

maintainer-clean-local:
	rm -f Makefile Makefile.in

TESTS = portabtest

# -eof-
