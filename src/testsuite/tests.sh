#!/bin/sh
# ngIRCd Test Suite
# $Id: tests.sh,v 1.1 2002/09/09 10:16:24 alex Exp $

name=`basename $0`
test=`echo ${name} | cut -d '.' -f 1`

type expect > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "SKIP: ${name} -- \"expect\" not found.";  exit 77
fi

echo "      doing ${test} ..."
expect ${test}.e > ${test}.log

# -eof-
