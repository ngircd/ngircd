#!/bin/sh
#
# $Id: autogen.sh,v 1.3 2002/03/12 14:37:51 alex Exp $
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
