#!/bin/sh
# ngIRCd Test Suite
# $Id: tests.sh,v 1.4.2.1 2004/09/04 20:49:36 alex Exp $

# detect source directory
[ -z "$srcdir" ] && srcdir=`dirname $0`

name=`basename $0`
test=`echo ${name} | cut -d '.' -f 1`
mkdir -p logs

if [ ! -r "$test" ]; then
  echo "      ${name}: test \"$test\" not found!";  exit 77
  exit 1
fi

type expect > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "      ${name}: \"expect\" not found.";  exit 77
fi
type telnet > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "      ${name}: \"telnet\" not found.";  exit 77
fi

echo -n "      running ${test} ..."
expect ${srcdir}/${test}.e > logs/${test}.log 2>&1; r=$?
[ $r -eq 0 ] && echo " ok." || echo " failure!"

exit $r

# -eof-
