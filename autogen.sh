#!/bin/sh
#
# $Id: autogen.sh,v 1.7 2003/04/22 10:15:46 alex Exp $
#

WANT_AUTOMAKE=1.6
export WANT_AUTOMAKE

aclocal && \
 autoheader && \
 automake --add-missing && \
 autoconf && \
 echo "Okay, autogen.sh done."

# -eof-
