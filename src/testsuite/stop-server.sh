#!/bin/sh
# ngIRCd Test Suite
# $Id: stop-server.sh,v 1.9.4.1 2003/11/07 20:51:11 alex Exp $

[ -z "$srcdir" ] && srcdir=`dirname $0`

echo "      stopping server ..."

# stop test-server ...
pid=`./getpid.sh T-ngircd`
if [ -z "$pid" ]; then
  echo "      no running server found!?"
  exit 1
fi
kill $pid > /dev/null 2>&1 || exit 1

# waiting ...
for i in 1 2 3 4 5; do
  kill -0 $pid > /dev/null 2>&1 || exit 0
  sleep 1
done
echo "      server still running!?"
exit 1

# -eof-
