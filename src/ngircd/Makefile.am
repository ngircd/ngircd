# $Id: Makefile.am,v 1.5 2001/12/21 22:25:30 alex Exp $

# $Log: Makefile.am,v $
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

CFLAGS = -Wall -g

sbin_PROGRAMS = ngircd

ngircd_SOURCES = ngircd.c channel.c client.c conf.c conn.c irc.c log.c parse.c tool.c

noinst_HEADERS = ngircd.h channel.c client.c conf.h conn.h irc.h log.h parse.h tool.h global.h

maintainer-clean-local:
	rm -f Makefile Makefile.in

# -eof-
