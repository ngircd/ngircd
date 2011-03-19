#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001-2011 Alexander Barton (alex@barton.de) and Contributors.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# Please read the file COPYING, README and AUTHORS for more information.
#

EXTRA_DIST = Doxyfile footer.inc.html

maintainer-clean-local:
	rm -f Makefile Makefile.in

distclean-local:
	rm -rf html

srcdoc:
	@doxygen --version >/dev/null 2>&1 \
	  || ( echo; echo "Error: \"doxygen\" not found!"; echo; exit 1 )
	doxygen

# -eof-
