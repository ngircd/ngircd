#!/bin/sh
#
# $Id: autogen.sh,v 1.6 2003/04/13 22:34:17 alex Exp $
#

export WANT_AUTOMAKE=1.6

aclocal && \
 autoheader && \
 automake --add-missing && \
 autoconf && \
 echo "Okay, autogen.sh done."

# -eof-
