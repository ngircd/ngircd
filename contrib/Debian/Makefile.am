#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2024 Alexander Barton (alex@barton.de) and Contributors
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#

EXTRA_DIST = \
	changelog \
	control \
	copyright \
	ngircd.default \
	ngircd.pam \
	rules \
	watch \
	source/format

maintainer-clean-local:
	rm -f Makefile Makefile.in

clean-local:
	rm -f *.log *.debhelper *.substvars
	rm -f debhelper-build-stamp files ngircd.service
	rm -rf .debhelper/ ngircd/

# -eof-
