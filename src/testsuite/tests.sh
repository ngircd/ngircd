#!/bin/sh
# ngIRCd Test Suite
# $Id: tests.sh,v 1.2 2002/09/09 22:56:07 alex Exp $

name=`basename $0`
test=`echo ${name} | cut -d '.' -f 1`
mkdir -p logs

type expect > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "SKIP: ${name} -- \"expect\" not found.";  exit 77
fi
type telnet > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "SKIP: ${name} -- \"telnet\" not found.";  exit 77
fi

echo "      doing ${test} ..."
expect ${test}.e > logs/${test}.log

# -eof-
