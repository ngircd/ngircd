#!/bin/sh
# ngIRCd Test Suite
# $Id: stop-server.sh,v 1.4.2.3 2002/09/20 15:18:56 alex Exp $

echo "      stopping server ..."

# Test-Server stoppen ...
pid=`./getpid.sh ngircd-TEST`
[ -n "$pid" ] && kill $pid > /dev/null 2>&1 || exit 1
sleep 1

# jetzt duerfte der Prozess nicht mehr laufen
kill -0 $pid > /dev/null 2>&1 && exit 1 || exit 0

# -eof-
