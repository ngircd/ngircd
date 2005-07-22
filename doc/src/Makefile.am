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
# $Id: Makefile.am,v 1.1 2005/07/22 20:58:22 alex Exp $
#

distclean-local:
	rm -f html

srcdoc:
	@doxygen --version >/dev/null 2>&1 \
	  || ( echo; echo "Error: \"doxygen\" not found!"; echo; exit 1 )
	doxygen

# -eof-
