#!/bin/sh
# ngIRCd Test Suite
# $Id: start-server.sh,v 1.5.2.2 2002/09/20 13:55:51 alex Exp $

echo "      starting server ..."

rm -rf logs

echo "This is an ngIRCd Test Server" > ngircd-test.motd

./ngircd-TEST -np -f ${srcdir}/ngircd-test.conf > ngircd-test.log 2>&1 &
sleep 1

PS_FLAGS=-f; PS_PIDCOL=2
ps $PS_FLAGS > /dev/null 2>&1
if [ $? -ne 0 ]; then PS_FLAGS=a; PS_PIDCOL=1; fi

ps $PS_FLAGS > procs.tmp
pid=`cat procs.tmp | grep ngircd-TEST | awk "{ print \\\$$PS_PIDCOL }"`
[ -n "$pid" ] && kill -0 $pid > /dev/null 2>&1 || exit 1

# -eof-
