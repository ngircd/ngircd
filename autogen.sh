#!/bin/sh
#
# $Id: autogen.sh,v 1.4 2003/01/11 15:35:47 alex Exp $
#

aclocal && \
 autoheader && \
 automake --add-missing && \
 autoconf && \
 echo "Okay, autogen.sh war erfolgreich."

# -eof-
