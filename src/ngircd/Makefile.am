#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2003 by Alexander Barton (alex@barton.de)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#
# $Id: Makefile.am,v 1.38 2003/03/31 15:54:21 alex Exp $
#

AUTOMAKE_OPTIONS = ../portab/ansi2knr

INCLUDES = -I$(srcdir)/../portab -I$(srcdir)/../tool

LINTARGS = -weak -warnunixlib +unixlib -booltype BOOLEAN

sbin_PROGRAMS = ngircd

ngircd_SOURCES = ngircd.c channel.c client.c conf.c conn.c conn-zip.c conn-func.c \
	hash.c irc.c irc-channel.c irc-info.c irc-login.c irc-mode.c irc-op.c \
	irc-oper.c irc-server.c irc-write.c lists.c log.c match.c parse.c \
	rendezvous.c resolve.c

ngircd_LDFLAGS = -L../portab -L../tool

ngircd_LDADD = -lngportab -lngtool

noinst_HEADERS = ngircd.h channel.h client.h conf.h conn.h conn-zip.h conn-func.h \
	hash.h irc.h irc-channel.h irc-info.h irc-login.h irc-mode.h irc-op.h \
	irc-oper.h irc-server.h irc-write.h lists.h log.h match.h parse.h \
	rendezvous.h resolve.h \
	messages.h defines.h

clean-local:
	rm -f check-version check-help lint.out cvs-version.*

maintainer-clean-local:
	rm -f Makefile Makefile.in

check-version: Makefile
	echo "#!/bin/sh" > check-version
	echo "./ngircd --version | grep ngircd > /dev/null 2>&1" >> check-version
	chmod 755 check-version

check-help: Makefile
	echo "#!/bin/sh" > check-help
	echo "./ngircd --help | grep help > /dev/null 2>&1" >> check-help
	chmod 755 check-help

lint:
	rm -f lint.out
	for f in *.c; do \
	 echo "checking $$f ..."; \
	 splint $$f $(LINTARGS) -I./.. -I./../portab $(AM_CFLAGS) > lint.out 2>&1; \
	 grep "no warnings" lint.out > /dev/null 2>&1; \
	 if [ $$? -ne 0 ]; then \
	  echo; cat lint.out; echo; \
	 fi; \
	done;

ngircd.c: cvs-date cvs-version.h

cvs-date:
	grep PACKAGE_VERSION ../config.h | grep "CVS" \
	 && echo "#define CVSDATE \"$$( grep "\$$Id" $(srcdir)/*.c \
	    | $(AWK) "{ print \$$9 }" | sort | tail -1 \
	    | sed -e "s/\//-/g" )\"" > cvs-version.new \
	 || echo "" > cvs-version.new
	diff cvs-version.h cvs-version.new || cp cvs-version.new cvs-version.h

TESTS = check-version check-help

# -eof-
