#!/bin/sh
# ngIRCd Test Suite
# $Id: tests.sh,v 1.4 2003/08/22 11:31:18 alex Exp $

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
