#!/bin/sh
# ngIRCd Test Suite
# $Id: stop-server.sh,v 1.10.2.1 2004/09/04 20:49:36 alex Exp $

[ -z "$srcdir" ] && srcdir=`dirname $0`

echo -n "      stopping server ..."

# stop test-server ...
pid=`./getpid.sh T-ngircd`
if [ -z "$pid" ]; then
  echo " failure: no running server found!?"
  exit 1
fi
kill $pid > /dev/null 2>&1 || exit 1

# waiting ...
for i in 1 2 3 4 5; do
  kill -0 $pid > /dev/null 2>&1; r=$?
  if [ $r -eq 0 ]; then
    echo " ok".
    exit 0
  fi
  sleep 1
done
echo " failure: server still running!?"
exit 1

# -eof-
