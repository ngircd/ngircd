#!/bin/sh
# ngIRCd Test Suite
# $Id: stop-server.sh,v 1.9 2002/11/10 14:28:06 alex Exp $

[ -z "$srcdir" ] && srcdir=`dirname $0`

echo "      stopping server ..."

# Test-Server stoppen ...
pid=`./getpid.sh T-ngircd`
[ -n "$pid" ] && kill $pid > /dev/null 2>&1 || exit 1
sleep 1

# jetzt duerfte der Prozess nicht mehr laufen
kill -0 $pid > /dev/null 2>&1 && exit 1 || exit 0

# -eof-
