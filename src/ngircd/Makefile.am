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
# $Id: Makefile.am,v 1.12 2002/02/21 18:47:49 alex Exp $
#
# $Log: Makefile.am,v $
# Revision 1.12  2002/02/21 18:47:49  alex
# - die Rechte der check-Scripts wurde nicht gesetzt.
#
# Revision 1.11  2002/02/21 17:25:16  alex
# - "check"-Target für "make check" und "make distcheck" begonnen ...
#
# Revision 1.10  2002/01/02 02:43:50  alex
# - Copyright-Text ergaenzt bzw. aktualisiert.
#
# Revision 1.9  2001/12/31 02:18:51  alex
# - viele neue Befehle (WHOIS, ISON, OPER, DIE, RESTART),
# - neuen Header "defines.h" mit (fast) allen Konstanten.
# - Code Cleanups und viele "kleine" Aenderungen & Bugfixes.
#
# Revision 1.8  2001/12/30 19:25:03  alex
# - in "noinst_HEADERS" waren einige Dateien als .c anstelle .h aufgefuehrt.
#
# Revision 1.7  2001/12/27 00:37:07  alex
# - Erkennung der "portab header" geaendert, CFLAGS werden nun anders gesetzt.
#
# Revision 1.6  2001/12/23 21:53:54  alex
# - Header messages.h eingefuegt.
#
# Revision 1.5  2001/12/21 22:25:30  alex
# - neues Modul "parse" eingebunden.
#
# Revision 1.4  2001/12/14 08:14:03  alex
# - neue Module (irc, client, channel) aufgenommen.
#
# Revision 1.3  2001/12/13 02:03:33  alex
# - beim Compilieren werden nun Informationen fuer den Debugger erzeugt.
#
# Revision 1.2  2001/12/12 17:20:02  alex
# - neue Sourcefiles und Header ergaenzt.
#
# Revision 1.1.1.1  2001/12/11 21:53:04  alex
# - Imported sources to CVS.
#

sbin_PROGRAMS = ngircd

ngircd_SOURCES = ngircd.c channel.c client.c conf.c conn.c irc.c log.c \
	parse.c tool.c

noinst_HEADERS = ngircd.h channel.h client.h conf.h conn.h irc.h log.h \
	parse.h tool.h global.h messages.h defines.h

clean-local:
	rm -f check-version check-help

maintainer-clean-local:
	rm -f Makefile Makefile.in

check-version: Makefile
	echo "#!/bin/sh" > check-version
	echo "./ngircd --version | grep version > /dev/null 2>&1" >> check-version
	chmod 755 check-version

check-help: Makefile
	echo "#!/bin/sh" > check-help
	echo "./ngircd --help | grep help > /dev/null 2>&1" >> check-help
	chmod 755 check-help

TESTS = check-version check-help

# -eof-
