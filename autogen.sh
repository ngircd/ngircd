#!/bin/sh
#
# $Id: autogen.sh,v 1.6.2.1 2003/04/22 10:18:41 alex Exp $
#

WANT_AUTOMAKE=1.6
export WANT_AUTOMAKE

aclocal && \
 autoheader && \
 automake --add-missing && \
 autoconf && \
 echo "Okay, autogen.sh done."

# -eof-
