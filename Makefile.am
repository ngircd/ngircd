# $Id: Makefile.am,v 1.3 2001/12/30 19:24:28 alex Exp $

# $Log: Makefile.am,v $
# Revision 1.3  2001/12/30 19:24:28  alex
# - acconfig.h in die "noinst_HEADERS" aufgenommen.
#
# Revision 1.2  2001/12/12 17:21:58  alex
# - Projektdatei fuer den Mac OS X Project Builder erstellt.
#
# Revision 1.1.1.1  2001/12/11 21:53:04  alex
# Imported sources to CVS.
#

AUTOMAKE_OPTIONS = gnu

SUBDIRS = MacOSX src

noinst_HEADERS = acconfig.h

maintainer-clean-local:
	rm -f Makefile.in Makefile aclocal.m4 configure
	rm -f mkinstalldirs missing depcomp install-sh
	rm -f config.log

# -eof-
