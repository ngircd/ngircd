#!/bin/sh
# ngIRCd Test Suite
# $Id: start-server.sh,v 1.4 2002/09/13 06:04:49 alex Exp $

echo "      starting server ..."

rm -rf logs

echo "This is an ngIRCd Test Server" > ngircd-test.motd

./ngircd-TEST -np -f ${srcdir}/ngircd-test.conf > ngircd-test.log 2>&1 &
sleep 1

ps ax > procs.tmp
pid=`cat procs.tmp | grep ngircd-TEST | awk "{ print \\\$1 }"`
kill -0 $pid > /dev/null 2>&1

# -eof-
