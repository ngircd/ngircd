# $Id: Makefile.am,v 1.3 2001/12/13 02:03:33 alex Exp $

# $Log: Makefile.am,v $
# Revision 1.3  2001/12/13 02:03:33  alex
# - beim Compilieren werden nun Informationen fuer den Debugger erzeugt.
#
# Revision 1.2  2001/12/12 17:20:02  alex
# - neue Sourcefiles und Header ergaenzt.
#
# Revision 1.1.1.1  2001/12/11 21:53:04  alex
# Imported sources to CVS.
#

CFLAGS = -Wall -g

sbin_PROGRAMS = ngircd

ngircd_SOURCES = ngircd.c conf.c conn.c log.c tool.c

noinst_HEADERS = ngircd.h conf.h conn.h log.h tool.h global.h

maintainer-clean-local:
	rm -f Makefile Makefile.in

# -eof-
