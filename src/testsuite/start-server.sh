#!/bin/sh
# ngIRCd Test Suite
# $Id: start-server.sh,v 1.10 2002/11/10 14:28:06 alex Exp $

[ -z "$srcdir" ] && srcdir=`dirname $0`

echo "      starting server ..."

# alte Logfiles loeschen
rm -rf logs *.log

# pruefen, ob getpid.sh gueltige PID's liefert. Wenn dem nicht so ist,
# wird kein ngIRCd gestartet, da dieser ansonsten nicht mehr am Ende
# des Testlaufs beendet werden koennte!
./getpid.sh sh > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "      error: getpid.sh FAILED!"
  exit 1
fi

# MOTD fuer Test-Server erzeugen
echo "This is an ngIRCd Test Server" > ngircd-test.motd

# Test-Server starten ...
./T-ngircd -np -f ${srcdir}/ngircd-test.conf > ngircd-test.log 2>&1 &
sleep 1

# validieren, dass Server laeuft
pid=`./getpid.sh T-ngircd`
[ -n "$pid" ] && kill -0 $pid > /dev/null 2>&1 || exit 1

# -eof-
