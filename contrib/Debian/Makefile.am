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
# $Id: Makefile.am,v 1.3 2004/01/13 01:10:11 alex Exp $
#

EXTRA_DIST = rules changelog compat control copyright \
	ngircd.init ngircd.postinst

maintainer-clean-local:
	rm -f Makefile Makefile.in
	rm -f ngircd.postinst.debhelper ngircd.postrm.debhelper \
	 ngircd.prerm.debhelper ngircd.substvars
	rm -f ngircd-full.postinst.debhelper ngircd-full.postrm.debhelper \
	 ngircd-full.prerm.debhelper ngircd-full.substvars

	rm -rf ngircd ngircd-full
	rm -f files

# -eof-
