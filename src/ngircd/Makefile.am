# $Id: Makefile.am,v 1.9 2001/12/31 02:18:51 alex Exp $

# $Log: Makefile.am,v $
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

sbin_PROGRAMS = ngircd

ngircd_SOURCES = ngircd.c channel.c client.c conf.c conn.c irc.c log.c \
	parse.c tool.c

noinst_HEADERS = ngircd.h channel.h client.h conf.h conn.h irc.h log.h \
	parse.h tool.h global.h messages.h defines.h

maintainer-clean-local:
	rm -f Makefile Makefile.in

# -eof-
