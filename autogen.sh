#!/bin/sh
#
# $Id: autogen.sh,v 1.1 2001/12/11 21:53:04 alex Exp $
#
# $Log: autogen.sh,v $
# Revision 1.1  2001/12/11 21:53:04  alex
# Initial revision
#

if [ -f configure ]; then
	echo "autogen.sh: configure-Skript existiert bereits!"
fi

aclocal && \
 autoheader && \
 automake --add-missing --copy && \
 autoconf && \
 echo "Okay, autogen.sh war erfolgreich."

# -eof-
