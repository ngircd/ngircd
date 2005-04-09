#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#
# $Id: Makefile.am,v 1.6 2005/04/09 12:22:41 alex Exp $
#

SUBDIRS = portab tool ngircd testsuite

maintainer-clean-local:
	rm -f Makefile Makefile.in config.h config.h.in stamp-h.in

srcdoc:
	@doxygen --version >/dev/null 2>&1 \
	 || ( echo; echo "Error: \"doxygen\" not found!"; echo; exit 1 )
	doxygen

# -eof-
