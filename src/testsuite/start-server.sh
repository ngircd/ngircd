#!/bin/sh
# ngIRCd Test Suite
# $Id: start-server.sh,v 1.2 2002/09/09 22:56:07 alex Exp $

echo "      starting server ..."

echo "This is an ngIRCd Test Server" > ngircd-test.motd

../ngircd/ngircd -np -f ngircd-test.conf > ngircd-test.log 2>&1 &
sleep 1

pid=`ps a | grep ngircd-test | head -n 1 | cut -d ' ' -f 1`
kill -0 $pid > /dev/null 2>&1

rm -rf logs

# -eof-
