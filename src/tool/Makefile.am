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
# $Id: Makefile.am,v 1.1 2003/01/13 12:20:16 alex Exp $
#

AUTOMAKE_OPTIONS = ansi2knr

INCLUDES = -I$(srcdir)/../portab

noinst_LIBRARIES = libngtool.a

libngtool_a_SOURCES = tool.c

noinst_HEADERS = tool.h

maintainer-clean-local:
	rm -f Makefile Makefile.in

# -eof-
