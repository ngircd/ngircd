#!/bin/sh
# ngIRCd Test Suite
# $Id: stop-server.sh,v 1.4.2.4 2002/10/03 16:13:38 alex Exp $

[ -z "$srcdir" ] && srcdir=`dirname $0`

echo "      stopping server ..."

# Test-Server stoppen ...
pid=`./getpid.sh ngircd-TEST`
[ -n "$pid" ] && kill $pid > /dev/null 2>&1 || exit 1
sleep 1

# jetzt duerfte der Prozess nicht mehr laufen
kill -0 $pid > /dev/null 2>&1 && exit 1 || exit 0

# -eof-
