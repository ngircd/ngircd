#
# ngIRCd -- The Next Generation IRC Daemon
# Copyright (c)2001,2002 by Alexander Barton (alex@barton.de)
#
# Dieses Programm ist freie Software. Sie koennen es unter den Bedingungen
# der GNU General Public License (GPL), wie von der Free Software Foundation
# herausgegeben, weitergeben und/oder modifizieren, entweder unter Version 2
# der Lizenz oder (wenn Sie es wuenschen) jeder spaeteren Version.
# Naehere Informationen entnehmen Sie bitter der Datei COPYING. Eine Liste
# der an ngIRCd beteiligten Autoren finden Sie in der Datei AUTHORS.
#
# $Id: Makefile.am,v 1.5 2002/01/02 02:52:09 alex Exp $
#
# $Log: Makefile.am,v $
# Revision 1.5  2002/01/02 02:52:09  alex
# - Copyright-Texte angepasst ;-)
#
# Revision 1.4  2001/12/31 02:20:20  alex
# - Unterverzeichnis "doc" aufgenommen.
#
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

SUBDIRS = doc MacOSX src

noinst_HEADERS = acconfig.h

maintainer-clean-local:
	rm -f Makefile.in Makefile aclocal.m4 configure
	rm -f mkinstalldirs missing depcomp install-sh
	rm -f config.log

# -eof-
