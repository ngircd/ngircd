#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2012 Alexander Barton (alex@barton.de) and Contributors
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#

TEMPLATE_MANS = ngircd.conf.5.tmpl ngircd.8.tmpl

SUFFIXES = .tmpl .

.tmpl:
	$(AM_V_GEN)sed \
	    -e "s@:SBINDIR:@${sbindir}@" \
	    -e "s@:BINDIR:@${bindir}@" \
	    -e "s@:ETCDIR:@${sysconfdir}@" \
	    <$< >$@

man_MANS = ngircd.conf.5 ngircd.8

CLEANFILES = $(man_MANS)

EXTRA_DIST = $(TEMPLATE_MANS)

maintainer-clean-local:
	rm -f Makefile Makefile.in

# -eof-
