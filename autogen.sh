#!/bin/sh
#
# $Id: autogen.sh,v 1.5 2003/04/04 10:05:34 alex Exp $
#

export WANT_AUTOMAKE=1.6

aclocal && \
 autoheader && \
 automake --add-missing && \
 autoconf && \
 echo "Okay, autogen.sh war erfolgreich."

# -eof-
