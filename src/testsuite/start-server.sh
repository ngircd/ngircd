#!/bin/sh
# ngIRCd Test Suite
# $Id: start-server.sh,v 1.5 2002/09/16 09:53:16 alex Exp $

echo "      starting server ..."

rm -rf logs

echo "This is an ngIRCd Test Server" > ngircd-test.motd

./ngircd-TEST -np -f ${srcdir}/ngircd-test.conf > ngircd-test.log 2>&1 &
sleep 1

PS_FLAGS=a; PS_PIDCOL=1
ps a > /dev/null 2>&1
if [ $? -ne 0 ]; then PS_FLAGS=-f; PS_PIDCOL=2; fi

ps $PS_FLAGS > procs.tmp
pid=`cat procs.tmp | grep ngircd-TEST | awk "{ print \\\$$PS_PIDCOL }"`
kill -0 $pid > /dev/null 2>&1

# -eof-
