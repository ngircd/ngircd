#!/bin/sh
#
# $Id: autogen.sh,v 1.2 2001/12/12 01:58:17 alex Exp $
#
# $Log: autogen.sh,v $
# Revision 1.2  2001/12/12 01:58:17  alex
# - fuer fehlende Dateien werden nun "nur noch" symbolische Links erzeugt.
#
# Revision 1.1.1.1  2001/12/11 21:53:04  alex
# Imported sources to CVS.
#

if [ -f configure ]; then
	echo "autogen.sh: configure-Skript existiert bereits ..."
fi

aclocal && \
 autoheader && \
 automake --add-missing && \
 autoconf && \
 echo "Okay, autogen.sh war erfolgreich."

# -eof-
