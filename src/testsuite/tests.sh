#!/bin/sh
# ngIRCd Test Suite
# $Id: tests.sh,v 1.5 2004/09/04 14:22:38 alex Exp $

# detect source directory
[ -z "$srcdir" ] && srcdir=`dirname $0`

name=`basename $0`
test=`echo ${name} | cut -d '.' -f 1`
mkdir -p logs

type expect > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "      ${name}: \"expect\" not found.";  exit 77
fi
type telnet > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "      ${name}: \"telnet\" not found.";  exit 77
fi

echo "      doing ${test} ..."
expect ${srcdir}/${test}.e > logs/${test}.log

# -eof-
